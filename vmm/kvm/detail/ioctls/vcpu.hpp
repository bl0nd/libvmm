//
// vcpu.hpp - vcpu ioctls
//

#pragma once

#include <cstddef> // size_t
#include <cstdint> // uint32_t, uint64_t
#include <linux/kvm.h> // kvm_*, KVM_*

#include "vmm/kvm/detail/ioctls/vm.hpp"
#include "vmm/kvm/detail/types/file_descriptor.hpp"

namespace vmm::kvm::detail {

class vcpu
{
    friend vcpu vm::vcpu(unsigned vcpu_id) const;

    private:
        KvmFd m_fd;

        explicit vcpu(int fd) noexcept : m_fd{fd} {}

#if defined(__i386__) || defined(__x86_64__)  || \
    defined(__arm__)  || defined(__aarch64__) || \
    defined(__s390__)
        auto mp_state() const -> kvm_mp_state;
        auto set_mp_state(const kvm_mp_state&) const -> void;
#endif

#if defined(__i386__) || defined(__x86_64__) || \
    defined(__ppc__)  || defined(__ppc64__)
        auto sregs() const -> kvm_sregs;
        auto set_sregs(const kvm_sregs&) const -> void;
#endif

#if defined(__i386__) || defined(__x86_64__)  || \
    defined(__arm__)  || defined(__aarch64__)
        auto vcpu_events() const -> kvm_vcpu_events;
        auto set_vcpu_events(const kvm_vcpu_events&) const -> void;
#endif

#if defined(__i386__) || defined(__x86_64__)
        auto fpu() const -> kvm_fpu;
        auto set_fpu(const kvm_fpu&) const -> void;
        auto lapic() const -> kvm_lapic_state;
        auto set_lapic(const kvm_lapic_state&) const -> void;
        auto xsave() const -> kvm_xsave;
        auto set_xsave(const kvm_xsave&) const -> void;
        auto xcrs() const -> kvm_xcrs;
        auto set_xcrs(const kvm_xcrs&) const -> void;
        auto debug_regs() const -> kvm_debugregs;
        auto set_debug_regs(const kvm_debugregs&) const -> void;
#endif

#if defined(__arm__) || defined(__aarch64__)
        auto init(const kvm_vcpu_init&) const -> void;
#endif

#if !defined(__arm__) && !defined(__aarch64__)
        auto regs() const -> kvm_regs;
        auto set_regs(const kvm_regs&) const -> void;
#endif
};

}  // namespace vmm::kvm::detail
