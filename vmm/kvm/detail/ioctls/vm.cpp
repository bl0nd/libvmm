/*
 * vm.cpp - VM ioctls
 */

#include "vmm/kvm/detail/ioctls/vm.hpp"
#include "vmm/kvm/detail/ioctls/vcpu.hpp"
#include "vmm/kvm/detail/ioctls/device.hpp"

namespace vmm::kvm::detail {

/**
 * Adds a vcpu to a virtual machine.
 *
 * See the documentation for KVM_CREATE_VCPU.
 *
 * Examples
 * ========
 * ```
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * auto vm = kvm.vm();
 * auto vcpu = vm.vcpu(0);
 * ```
 */
auto vm::vcpu(unsigned vcpu_id) const -> vmm::kvm::detail::vcpu
{
    return vmm::kvm::detail::vcpu{m_fd.ioctl(KVM_CREATE_VCPU, vcpu_id)};
}

/**
 * Adds a device to a virtual machine.
 *
 * See the documentation for KVM_CREATE_DEVICE.
 *
 * Examples
 * ========
 * ```
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * auto vm = kvm.vm();
 * auto device = vm.device(KVM_DEV_TYPE_VFIO);
 * ```
 */
auto vm::device(uint32_t type, uint32_t flags) const -> vmm::kvm::detail::device
{
    auto dev = kvm_create_device{ .type = type, .flags = flags };
    return vmm::kvm::detail::device{dev};
}

/**
 * Returns a positive integer if a KVM extension is available; 0 otherwise.
 *
 * Based on their initialization, VMs may have different capabilities. Thus,
 * `kvm::vm::check_extension()` is preferred when querying for most
 * capabilities.
 *
 * Examples
 * ========
 * ```
 * #include <cassert>
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * auto vm = kvm.vm();
 * assert(vm.check_extension(KVM_CAP_ARM_VM_IPA_SIZE) >= 32);
 * ```
 */
auto vm::check_extension(unsigned cap) const -> unsigned
{
    return m_fd.ioctl(KVM_CHECK_EXTENSION, cap);
}

/**
 * Defines which vcpu is the Bootstrap Processor (BSP).
 *
 * The KVM_SET_BOOT_CPU_ID ioctl must be called before any vcpus are created
 * for a VM, otherwise the call will fail.
 *
 * See the documentation for KVM_SET_BOOT_CPU_ID.
 *
 * Examples
 * ========
 * ```
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * auto vm = kvm.vm();
 *
 * if (vm.check_extension(KVM_CAP_SET_BOOT_CPU_ID) > 0)
 *     throw;
 *
 * vm.set_bsp(0);
 * ```
 */
auto vm::set_bsp(unsigned vcpu_id) const -> void
{
    m_fd.ioctl(KVM_SET_BOOT_CPU_ID, vcpu_id);
}

/**
 * Creates, modifies, or deletes a guest physical memory slot.
 *
 * See the documentation for KVM_SET_USER_MEMORY_REGION.
 *
 * Examples
 * ========
 * ```
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * vmm::kvm::vm vm = kvm.vm();
 * auto mem_region = kvm_userspace_memory_region{
 *     .slot = 0,
 *     .flags = 0,
 *     .guest_phys_addr = 0x10000,
 *     .memory_size = 0x10000,
 *     .userspace_addr = 0,
 * };
 *
 * vm.memslot(mem_region);
 * ```
 */
auto vm::memslot(kvm_userspace_memory_region region) const -> void
{
    m_fd.ioctl(KVM_SET_USER_MEMORY_REGION, &region);
}

/**
 * Creates an interrupt controller model in the kernel
 *
 * See the documentation for `KVM_CREATE_IRQCHIP`.
 *
 * Examples
 * ========
 * ```
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * auto vm = kvm.vm();
 * vm.irqchip();
 * ```
 */
auto vm::irqchip() const -> void
{
    m_fd.ioctl(KVM_CREATE_IRQCHIP);
}

/**
 * Reads the state of a kernel interrupt controller into a buffer provided by
 * the caller.
 *
 * See the documentation for `KVM_GET_IRQCHIP`.
 *
 * Examples
 * ========
 * ```
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * auto vm = kvm.vm();
 * auto irqchip = kvm_irqchip{ .chip_id = KVM_IRQCHIP_PIC_MASTER };
 *
 * vm.irqchip();
 * vm.get_irqchip(irqchip);
 * ```
 */
auto vm::get_irqchip(kvm_irqchip &irqchip_p) const -> void
{
    m_fd.ioctl(KVM_GET_IRQCHIP, &irqchip_p);
}

/**
 * Sets the state of a kernel interrupt controller from a buffer provided by
 * the caller.
 *
 * See the documentation for `KVM_SET_IRQCHIP`.
 *
 *
 * Examples
 * ========
 * ```
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * auto vm = kvm.vm();
 * auto irqchip = kvm_irqchip{
 *     .chip_id = KVM_IRQCHIP_PIC_MASTER,
 *     .chip.pic.irq_base = 99
 * };
 *
 * vm.irqchip();
 * vm.set_irqchip(irqchip);
 * ```
 */
auto vm::set_irqchip(kvm_irqchip const &irqchip_p) const -> void
{
    m_fd.ioctl(KVM_SET_IRQCHIP, &irqchip_p);
}

auto vm::set_irq_line(const uint32_t irq, bool active) const -> void
{
    auto irq_level = kvm_irq_level {
        .irq = irq,
        .level = active ? uint32_t{1} : uint32_t{0}
    };

    m_fd.ioctl(KVM_IRQ_LINE, &irq_level);
}

/**
 * Gets the current timestamp of kvmclock as seen by the current guest.
 *
 * See the documentation for `KVM_GET_CLOCK`.
 *
 * Examples
 * ========
 * ```
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * auto vm = kvm.vm();
 * auto clock = vm.get_clock();
 * ```
 */
auto vm::get_clock() const -> kvm_clock_data
{
    auto clock = kvm_clock_data{};
    m_fd.ioctl(KVM_GET_CLOCK, &clock);
    return clock;
}

/**
 * Sets the current timestamp of kvmclock.
 *
 * See the documentation for `KVM_SET_CLOCK`.
 *
 * Examples
 * ========
 * ```
 * #include <vmm/kvm.hpp>
 *
 * auto kvm = vmm::kvm::system{};
 * auto vm = kvm.vm();
 * auto clock = kvm_clock_data{ .clock = 10 };
 *
 * vm.set_clock(&clock);
 * ```
 */
auto vm::set_clock(kvm_clock_data &clock) const -> void
{
    m_fd.ioctl(KVM_SET_CLOCK, &clock);
}

/**
 * Returns KVM_RUN's shared memory region size.
 */
auto vm::mmap_size() const -> std::size_t
{
    return m_mmap_size;
}

/**
 * Returns the recommended number for max_vcpus.
 */
auto vm::num_vcpus() const -> unsigned
{
    auto ret = check_extension(KVM_CAP_NR_VCPUS);
    return ret > 0 ? ret : 4;
}

/**
 * Returns the maximum possible value for max_vcpus.
 */
auto vm::max_vcpus() const -> unsigned
{
    auto ret = check_extension(KVM_CAP_MAX_VCPUS);
    return ret > 0 ? ret : num_vcpus();
}

/**
 * Returns the maximum number of allowed memory slots for a VM.
 */
auto vm::num_memslots() const -> unsigned
{
    auto ret = check_extension(KVM_CAP_NR_MEMSLOTS);
    return ret > 0 ? ret : 32;
}

}  // namespace vmm::kvm::detail
