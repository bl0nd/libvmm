/*
 * kvm.cpp - KVM ioctls
 */

#include "../../include/vmm/kvm/system.hpp"
#include "../../include/vmm/kvm/vm.hpp"
#include "../../include/vmm/kvm/types.hpp"
#include "../../include/vmm/utils/utils.hpp"

#include <linux/kvm.h>
#include <sys/stat.h>

namespace vmm::kvm {
    /**
     * Creates a virtual machine and returns a file descriptor.
     *
     * This should only be used indirectly through system::vm().
     */
    auto system::create_vm() -> unsigned int {
        return utils::ioctl(fd_, KVM_CREATE_VM);
    }

    /**
     * Returns the KVM API version.
     *
     * # Examples
     *
     * ```
     * #include <vmm/kvm.hpp>
     *
     * kvm::system kvm;
     *
     * if (kvm.api_version() != KVM_API_VERSION)
     *     throw;
     * ```
     */
    auto system::api_version() -> unsigned int {
        return utils::ioctl(fd_, KVM_GET_API_VERSION);
    }

    /**
     * Returns the size of the shared memory region used to communicate with
     * userspace by the KVM_RUN ioctl.
     *
     * # Examples
     *
     * ```
     * #include <vmm/kvm.hpp>
     *
     * kvm::system kvm;
     * auto mmap_size {kvm.vcpu_mmap_size()};
     * ```
     */
    auto system::vcpu_mmap_size() -> unsigned int {
        return utils::ioctl(fd_, KVM_GET_VCPU_MMAP_SIZE);
    }

    /**
     * Returns a list of supported MSRs (host & KVM-specific).
     *
     * # Examples
     *
     * ```
     * #include <vmm/kvm.hpp>
     *
     * kvm::system kvm;
     * kvm::MsrIndexList msr_list {kvm.msr_index_list()};
     *
     * TODO: Use one of those MSRs.
     * ```
     */
    auto system::msr_index_list() -> MsrIndexList {
        MsrIndexList msr_list;
        utils::ioctl(fd_, KVM_GET_MSR_INDEX_LIST, msr_list.get());
        return msr_list;
    }

    /**
     * Returns a list of MSRs exposing MSR-based CPU features.
     *
     * This can be used, for instance, by a hypervisor to validate requested
     * CPU features.
     *
     * # Examples
     *
     * ```
     * #include <iostream>
     * #include <vmm/kvm.hpp>
     *
     * kvm::system kvm;
     * kvm::MsrFeatureList msr_list {kvm.msr_feature_list()};
     *
     * TODO: Validate CPU feature requested by VM
     * ```
     */
    auto system::msr_feature_list() -> MsrFeatureList {
        MsrFeatureList msr_list;
        utils::ioctl(fd_, KVM_GET_MSR_FEATURE_INDEX_LIST, msr_list.get());
        return msr_list;
    }

    /**
     * Reads the values of MSR-based features available for VMs. Returns the
     * number of successfully read values.
     *
     * # Examples
     *
     * ```
     * #include <vmm/kvm.hpp>
     *
     * kvm::system kvm;
     * kvm_msr_entry entry{0x174};
     * kvm::Msrs msrs{entry};
     * auto nmsrs {kvm.msrs(msrs)};
     * ```
     *
     * ```
     * #include <vector>
     * #include <vmm/kvm.hpp>
     *
     * kvm::system kvm;
     * kvm::MsrFeatureList msr_list {kvm.msr_feature_list()};
     * std::vector<kvm_msr_entry> entries;
     *
     * for (auto msr : msr_list) {
     *     kvm_msr_entry entry{msr};
     *     entries.push_back(entry);
     * }
     *
     * kvm::Msrs msrs{entries};
     * auto nmsrs {kvm.msrs(msrs)};
     * ```
     */
    auto system::msrs(Msrs& msrs) -> unsigned int {
        return utils::ioctl(fd_, KVM_GET_MSRS, msrs.get());
    }

    /**
     * Creates a virtual machine.
     *
     * This function will also initialize the size of the vCPU mmap area with
     * the KVM_GET_VCPU_MMAP_SIZE ioctl's result.
     *
     * # Examples
     *
     * ```
     * #include <vmm/kvm.hpp>
     *
     * kvm::system kvm;
     * kvm::vm {kvm.vm()};
     * ```
     */
    auto system::vm() -> vmm::kvm::vm {
        const auto mmap_size {vcpu_mmap_size()};
        const auto fd {create_vm()};
        return vmm::kvm::vm{fd, mmap_size};
    }

    system::~system() noexcept {
        if (!closed_) {
            try {
                utils::close(fd_);
            }
            catch (std::system_error& e) {
                /* TODO: log error */
            }
        }
    }

    /**
     * Closes the KVM subsystem handle.
     *
     * Use this if you'd like to handle possible failures of `utils::close()`.
     *
     * # Examples
     *
     * ```
     * #include <vmm/kvm.hpp>
     *
     * kvm::system kvm;
     *
     * try {
     *     kvm.close();
     * }
     * catch (std::system_error)
     *     throw;
     * ```
     */
    auto system::close() -> void {
        utils::close(fd_);
        closed_ = true;
    }
}
