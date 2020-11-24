#define CATCH_CONFIG_MAIN

#include <bitset>
#include <catch2/catch.hpp>

#include <string.h> // memcmp
#include <sys/mman.h> // mmap, PROT_READ, PROT_WRITE, MAP_ANONYMOUS, MAP_SHARED

#include "vmm/kvm/kvm.hpp"

TEST_CASE("vCPU creation") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();

    REQUIRE_NOTHROW(vm.vcpu(0));
}

TEST_CASE("Immediate exit") {
        auto kvm = vmm::kvm::system{};
        auto vm = kvm.vm();
        auto vcpu = vm.vcpu(0);

        REQUIRE(vcpu.immediate_exit() == 0);
        vcpu.set_immediate_exit(1);
        REQUIRE(vcpu.immediate_exit() == 1);
}

#if defined(__i386__) || defined(__x86_64__) || \
    defined(__arm__)  || defined(__aarch64__)
TEST_CASE("Multi-processing state") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);
    auto mp_state = vcpu.mp_state();

    REQUIRE_NOTHROW(vcpu.set_mp_state(mp_state));
    auto other = vcpu.mp_state();
    REQUIRE(mp_state.mp_state == other.mp_state);
}
#endif

#if defined(__i386__) || defined(__x86_64__) || \
    defined(__arm__)  || defined(__aarch64__)
TEST_CASE("vCPU events") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);

    REQUIRE(kvm.check_extension(KVM_CAP_VCPU_EVENTS));

    auto events = vcpu.vcpu_events();
    REQUIRE_NOTHROW(vcpu.set_vcpu_events(events));

    auto other = vcpu.vcpu_events();
    REQUIRE(memcmp(&events, &other, sizeof(kvm_vcpu_events)) == 0);
}
#endif

#if defined(__i386__) || defined(__x86_64__)
TEST_CASE("CPUID2") {
    auto kvm = vmm::kvm::system{};

    if (kvm.check_extension(KVM_CAP_EXT_CPUID)) {
        auto vm = kvm.vm();
        auto num_vcpus = vm.num_vcpus();
        auto supported_cpuids = kvm.supported_cpuids();

        REQUIRE(supported_cpuids.size() <= MAX_CPUID_ENTRIES);

        for (int id = 0; id < num_vcpus; id++) {
            auto vcpu = vm.vcpu(id);
            vcpu.set_cpuid2(supported_cpuids);
            auto cpuids = vcpu.cpuid2<MAX_CPUID_ENTRIES>();

            // Check the first few leafs since some (e.g. 13) are reserved.
            for (std::size_t i = 0; i < 3; i++) {
                REQUIRE(supported_cpuids[i].function == cpuids[i].function);
                REQUIRE(supported_cpuids[i].index == cpuids[i].index);
                REQUIRE(supported_cpuids[i].flags == cpuids[i].flags);
                REQUIRE(supported_cpuids[i].eax == cpuids[i].eax);
                REQUIRE(supported_cpuids[i].ebx == cpuids[i].ebx);
                REQUIRE(supported_cpuids[i].ecx == cpuids[i].ecx);
                REQUIRE(supported_cpuids[i].edx == cpuids[i].edx);
            }
        }
    }
}

TEST_CASE("Run") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);

    // Add 2 small numbers
    const auto code = std::array<uint8_t, 24>{
        0xba, 0xf8, 0x03,             // mov $0x3f8, %dx
        0x00, 0xd8,                   // add %bl, %al
        0x04, '0',                    // add $'0', %al
        0xee,                         // out %al, %dx
        0xec,                         // in %dx, %al
        0xc6, 0x06, 0x00, 0x80, 0x00, // movl $0, (0x8000); This generates a MMIO Write.
        0x8a, 0x16, 0x00, 0x80,       // movl (0x8000), %dl; This generates a MMIO Read.
        0xc6, 0x06, 0x00, 0x20, 0x00, // movl $0, (0x2000); Dirty one page in guest mem.
        0xf4,                         // hlt
    };

    // mmap code
    auto guest_addr = uint64_t{0x1000};
    auto mem_size = uint64_t{0x4000};
    auto mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    std::memcpy(mem, code.data(), sizeof(code));

    // Configure VM with the memory region storing the code
    auto mem_region = kvm_userspace_memory_region {
        0,
        KVM_MEM_LOG_DIRTY_PAGES,
        guest_addr,
        mem_size,
        reinterpret_cast<uintptr_t>(mem)
    };

    vm.memslot(mem_region);

    // Initialize CS to point at 0, via a read-modify-write of sregs.
    auto sregs = vcpu.sregs();

    REQUIRE(sregs.cs.base != 0);
    REQUIRE(sregs.cs.selector != 0);

    sregs.cs.base = 0;
    sregs.cs.selector = 0;
    vcpu.set_sregs(sregs);

    // Initialize registers: IP for our code, addends, and flags needed by x86.
    auto regs = kvm_regs{};
    regs.rip = guest_addr,
    regs.rax = 2,
    regs.rbx = 3,
    regs.rflags = 2,
    vcpu.set_regs(regs);

    // Run vCPU
    while (1) {
        auto exit = vcpu.run();

        switch (exit) {
            case vmm::kvm::VcpuExit::Io: {
                const auto io = vcpu.data()->io;

                if (io.direction == KVM_EXIT_IO_IN) {
                    REQUIRE(io.port == 0x3f8);
                    REQUIRE(io.count == 1);
                }
                else if (io.direction == KVM_EXIT_IO_OUT) {
                    REQUIRE(io.port == 0x3f8);
                    REQUIRE(io.count == 1);
                    REQUIRE(*(reinterpret_cast<char*>(vcpu.data()) + io.data_offset) == '5');
                }

                continue;
            }
            case vmm::kvm::VcpuExit::Mmio: {
                const auto mmio = vcpu.data()->mmio;

                REQUIRE(mmio.phys_addr == 0x8000);
                REQUIRE(mmio.len == 1);

                if (mmio.is_write)
                    REQUIRE(mmio.data[0] == 0);

                continue;
            }
            case vmm::kvm::VcpuExit::Hlt: {
                // The code snippet dirties 2 pages:
                //
                //  * When the code itself is loaded in memory.
                //  * From the `movl` that writes to address 0x8000.
                const auto dirty_pages = vm.dirty_log(0, mem_size);

                auto sum = uint64_t{};
                for (auto x : dirty_pages)
                    sum += std::bitset<64>{x}.count();

                REQUIRE(sum == 2);

                break;
            }
            default:
                VMM_THROW(/* TODO */);
        }

        break;
    }
}

TEST_CASE("FPU") {
    // From https://github.com/torvalds/linux/blob/master/arch/x86/include/asm/fpu/internal.h
    auto KVM_FPU_CWD = uint16_t{0x37f};
    auto KVM_FPU_MXCSR = uint32_t{0x1f80};
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);
    auto fpu = kvm_fpu{};
    fpu.fcw = KVM_FPU_CWD;
    fpu.mxcsr = KVM_FPU_MXCSR;

    REQUIRE_NOTHROW(vcpu.set_fpu(fpu));
    REQUIRE(vcpu.fpu().fcw == KVM_FPU_CWD);
}

TEST_CASE("Xsave") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);
    auto xsave = vcpu.xsave();

    REQUIRE_NOTHROW(vcpu.set_xsave(xsave));
    auto other = vcpu.xsave();

    for (int i = 0; i < 1024; i++)
        REQUIRE(xsave.region[i] == other.region[i]);
}

TEST_CASE("Xcrs") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);
    auto xcrs = vcpu.xcrs();

    REQUIRE_NOTHROW(vcpu.set_xcrs(xcrs));
    auto other = vcpu.xcrs();

    REQUIRE(xcrs.nr_xcrs == other.nr_xcrs);
    REQUIRE(xcrs.flags == other.flags);

    for (int i = 0; i < KVM_MAX_XCRS; i++) {
        REQUIRE(xcrs.xcrs[i].xcr == other.xcrs[i].xcr);
        REQUIRE(xcrs.xcrs[i].value == other.xcrs[i].value);
    }
}

TEST_CASE("Debug registers") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);
    auto regs = vcpu.debug_regs();

    REQUIRE_NOTHROW(vcpu.set_debug_regs(regs));
    auto other = vcpu.debug_regs();

    for (int i = 0; i < 4; i++)
        REQUIRE(regs.db[i] == other.db[i]);

    REQUIRE(regs.dr6 == other.dr6);
    REQUIRE(regs.dr7 == other.dr7);
    REQUIRE(regs.flags == other.flags);
}

TEST_CASE("MSRs") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);
    auto entries = std::array<kvm_msr_entry, 2>{{
        {0x0000'0174},
        {0x0000'0175, 0, 1}
    }};

    auto msrs_to_set = vmm::kvm::Msrs<2>{entries.begin(), entries.end()};
    REQUIRE_NOTHROW(vcpu.set_msrs(msrs_to_set));

    auto msrs_to_read = vmm::kvm::Msrs<2>{{
        kvm_msr_entry{0x0000'0174},
        kvm_msr_entry{0x0000'0175}
    }};
    auto nmsrs = vcpu.get_msrs(msrs_to_read);

    REQUIRE(nmsrs == msrs_to_set.size());
    REQUIRE(nmsrs == msrs_to_read.size());

    for (std::size_t i = 0; i < msrs_to_read.size(); i++) {
        REQUIRE(msrs_to_read[i].index == entries[i].index);
        REQUIRE(msrs_to_read[i].data == entries[i].data);
    }
}
#endif

#if defined(__arm__) || defined(__aarch64__)
TEST_CASE("Preferred target initialization") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);
    auto kvi = kvm_vcpu_init{};

    REQUIRE_THROWS(vcpu.init(kvi));
    kvi = vm.preferred_target();
    REQUIRE_NOTHROW(vcpu.init(kvi));
}

TEST_CASE("Register") {
    auto kvm = vmm::kvm::system{};
    auto vm = kvm.vm();
    auto vcpu = vm.vcpu(0);
    auto kvi = vm.preferred_target();

    vcpu.init(kvi);

    SECTION("Setting") {
        auto data = uint64_t{};
        auto id = uint64_t{};

        REQUIRE_THROWS(vcpu.set_reg(id, data));

        // Exercising KVM_SET_ONE_REG by trying to alter the data inside the
        // PSTATE register (which is a specific aarch64 register).
        const auto PSTATE_REG_ID = uint64_t{0x6030'0000'0010'0042};
        REQUIRE_NOTHROW(vcpu.set_reg(PSTATE_REG_ID, data));
    }

    SECTION("Getting") {
        // PSR (Processor State Register) bits. Taken from
        // arch/arm64/include/uapi/asm/ptrace.h.
        const auto PSTATE_REG_ID = uint64_t{0x6030'0000'0010'0042};
        const auto PSTATE_FAULT_BITS_64 = uint64_t{PSR_MODE_EL1h | PSR_A_BIT | PSR_F_BIT | PSR_I_BIT | PSR_D_BIT};
        auto data = PSTATE_FAULT_BITS_64;

        REQUIRE_NOTHROW(vcpu.set_reg(PSTATE_REG_ID, data));
        REQUIRE(vcpu.reg(PSTATE_REG_ID) == PSTATE_FAULT_BITS_64);
    }
}
#endif
