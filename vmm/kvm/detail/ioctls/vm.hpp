//
// vm.hpp - VM ioctls
//

#pragma once

#include <cstddef> // size_t
#include <cstdint> // uint32_t, uint64_t
#include <linux/kvm.h> // kvm_*, KVM_*

#include "vmm/kvm/detail/ioctls/system.hpp"
#include "vmm/kvm/detail/types/file_descriptor.hpp"
#include "vmm/kvm/detail/types/fam_struct.hpp"
#include "vmm/types/event.hpp"

namespace vmm::kvm::detail {

class vcpu;
class device;

class vm
{
    friend vm system::vm(unsigned machine_type) const;

    public:
        vm(const vm& other) = delete;
        vm(vm&& other) = default;
        auto operator=(const vm& other) -> vm& = delete;
        auto operator=(vm&& other) -> vm& = default;

        // Adds a vcpu to a virtual machine.
        //
        // See the documentation for KVM_CREATE_VCPU.
        [[nodiscard]] auto vcpu(unsigned vcpu_id) const -> vmm::kvm::detail::vcpu;

        // Adds a device to a virtual machine.
        //
        // See the documentation for KVM_CREATE_DEVICE.
        [[nodiscard]] auto device(uint32_t type, uint32_t flags=0) const -> vmm::kvm::detail::device;

        // Returns a positive integer if a KVM extension is available; 0
        // otherwise.
        //
        // Based on their initialization, VMs may have different capabilities.
        // Therefore, `kvm::vm::check_extension()` is preferred when querying
        // for most capabilities.
        [[nodiscard]] auto check_extension(unsigned cap) const -> unsigned;

        // Creates, modifies, or deletes a guest physical memory slot.
        //
        // See the documentation for KVM_SET_USER_MEMORY_REGION.
        auto memslot(kvm_userspace_memory_region) const -> void;

        // Attaches an ioeventfd to a legal pio/mmio address within the guest.
        //
        // See the documentation for KVM_IOEVENTFD.
        template<vmm::types::IoEventAddress T>
        auto attach_ioevent(vmm::types::EventFd eventfd, uint64_t addr,
                            uint64_t datamatch=0) const -> void
        {
            auto flags = uint32_t{};

            if (datamatch > 0) {
                flags |= KVM_IOEVENTFD_FLAG_DATAMATCH;
            }

            if constexpr (T == vmm::types::IoEventAddress::Pio) {
                flags |= KVM_IOEVENTFD_FLAG_PIO;
            }

            auto ioeventfd = kvm_ioeventfd {
                datamatch,
                addr,
                sizeof(uint64_t),
                eventfd.fd(),
                flags
            };

            m_fd.ioctl(KVM_IOEVENTFD, &ioeventfd);
        }

        // Detaches an ioeventfd from a legal pio/mmio address within the guest.
        template<vmm::types::IoEventAddress T>
        auto detach_ioevent(vmm::types::EventFd eventfd, uint64_t addr,
                            uint64_t datamatch=0) const -> void
        {
            auto flags = uint32_t{KVM_IOEVENTFD_FLAG_DEASSIGN};

            if (datamatch > 0) {
                flags |= KVM_IOEVENTFD_FLAG_DATAMATCH;
            }

            if constexpr (T == vmm::types::IoEventAddress::Pio) {
                flags |= KVM_IOEVENTFD_FLAG_PIO;
            }

            auto ioeventfd = kvm_ioeventfd {
                datamatch,
                addr,
                sizeof(uint64_t),
                eventfd.fd(),
                flags
            };

            m_fd.ioctl(KVM_IOEVENTFD, &ioeventfd);
        }

        // Returns KVM_RUN's shared memory region size.
        [[nodiscard]] auto mmap_size() const -> std::size_t;

        // Returns the recommended number for max_vcpus.
        [[nodiscard]] auto num_vcpus() const -> unsigned;

        // Returns the maximum possible value for max_vcpus.
        [[nodiscard]] auto max_vcpus() const -> unsigned;

        // Returns the maximum number of allowed memory slots for a VM.
        [[nodiscard]] auto num_memslots() const -> unsigned;

#if defined(__i386__) || defined(__x86_64__)
        // Defines which vcpu is the Bootstrap Processor (BSP).
        //
        // If a vCPU exists for a VM before this is called, the call will fail.
        //
        // See the documentation for KVM_SET_BOOT_CPU_ID.
        auto set_bsp(unsigned vcpu_id) const -> void;

        // Reads the state of a kernel interrupt controller into a buffer
        // provided by the caller.
        //
        // See the documentation for KVM_GET_IRQCHIP.
        auto get_irqchip(kvm_irqchip&) const -> void;

        // Sets the state of a kernel interrupt controller from a buffer
        // provided by the caller.
        //
        // See the documentation for KVM_SET_IRQCHIP.
        auto set_irqchip(const kvm_irqchip&) const -> void;

        // Gets the current timestamp of kvmclock as seen by the current guest.
        //
        // See the documentation for KVM_GET_CLOCK.
        [[nodiscard]] auto get_clock() const -> kvm_clock_data;

        // Sets the current timestamp of kvmclock.
        //
        // See the documentation for KVM_SET_CLOCK.
        auto set_clock(kvm_clock_data&) const -> void;

        // Sets the address of a three-page region in a VM's address space.
        //
        // See the documentation for KVM_SET_TSS_ADDR.
        auto set_tss_address(unsigned long address) const -> void;
#endif

#if defined(__arm__) || defined(__aarch64__)
        // Returns the preferred CPU target type which can be emulated by KVM
        // on underlying host.
        //
        // See documentation for KVM_ARM_PREFERRED_TARGET.
        [[nodiscard]] auto preferred_target() const -> kvm_vcpu_init;
#endif
    private:
        KvmFd m_fd;
        size_t m_mmap_size;

        vm(int fd, std::size_t mmap_size) noexcept
            : m_fd{fd}, m_mmap_size{mmap_size} {}
};

}  // namespace vmm::kvm::detail
