/*
 * vcpu.hpp - vcpu ioctls
 */

#pragma once

#include "vmm/kvm/detail/base.hpp"
#include "vmm/kvm/detail/vm.hpp"

namespace vmm::kvm::detail {

class vcpu {
    private:
        KvmFd fd_;

        /**
         * Restricted constructor for kvm::vm objects.
         */
        vcpu(int fd) noexcept : fd_{fd} {}
        friend vcpu vm::vcpu(unsigned int vcpu_id);
};

}  // namespace vmm::kvm::detail
