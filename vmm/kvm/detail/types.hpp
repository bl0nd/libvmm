/*
 * types.hpp - KVM types
 */

#pragma once

#include "vmm/utility/utility.hpp"

#include <algorithm>
#include <memory>
#include <linux/kvm.h>

namespace vmm::kvm::detail {

/**
 * Size of reserved range for KVM-specific MSRs (0x4b564d00 to 0x4b564dff)
 */
#define MAX_IO_MSRS 256

/**
 * Size of `msr_based_features_all` array in linux/arch/x86/kvm/x86.c
 */
#define MAX_IO_MSRS_FEATURES 22

/**
 * Defined in linux/arch/x86/include/asm/kvm_host.h
 */
#define MAX_CPUID_ENTRIES 80

/**
 * Basic wrapper around C FAM structs.
 */
template<typename Struct, typename Buffer>
class FamStruct {
    protected:
        std::unique_ptr<Struct, void(*)(Struct*)> ptr_;

        FamStruct(const size_t n)
            : ptr_{reinterpret_cast<Struct*>(new Buffer[n]()),
                   [](Struct *p){ delete[] reinterpret_cast<Buffer*>(p); }} {}
    public:
        auto get() -> Struct* { return ptr_.get(); }
};

class MsrIndexList : public FamStruct<kvm_msr_list, uint32_t> {
    protected:
        MsrIndexList(const size_t n);
    public:
        MsrIndexList();

        auto nmsrs() const -> uint32_t;
        auto begin() -> uint32_t*;
        auto end() -> uint32_t*;
        auto begin() const -> uint32_t const*;
        auto end() const -> uint32_t const*;
        auto cbegin() const -> uint32_t const*;
        auto cend() const -> uint32_t const*;
};

class MsrFeatureList : public MsrIndexList {
    public:
        MsrFeatureList();
};

class MsrList : public FamStruct<kvm_msrs, uint64_t> {
    private:
        MsrList(const size_t n);
    public:
        MsrList(kvm_msr_entry entry);

        /**
         * Range constructor.
         *
         * Examples
         * ========
         * ```
         * #include <vmm/kvm.hpp>
         *
         * auto kvm = vmm::kvm::system{};
         * auto msr_list = kvm.msr_feature_list();
         * auto entries = std::vector<kvm_msr_entry>{};
         *
         * for (auto msr : msr_list) {
         *     auto entry = kvm_msr_entry{msr};
         *     entries.push_back(entry);
         * }
         *
         * auto msrs = kvm::MsrList{entries.begin(), entries.end()};
         * ```
         */
        template <typename Iterator>
        MsrList(Iterator first, Iterator last) : MsrList(std::distance(first, last)) {
            std::copy_if(first, last, ptr_->entries, [](kvm_msr_entry) { return true; });
        }

        /**
         * Container constructor.
         *
         * Examples
         * ========
         * ```
         * #include <vmm/kvm.hpp>
         *
         * auto kvm = vmm::kvm::system{};
         * auto msr_list = kvm.msr_feature_list();
         * auto entries = std::vector<kvm_msr_entry>{};
         *
         * for (auto msr : msr_list) {
         *     auto entry = kvm_msr_entry{msr};
         *     entries.push_back(entry);
         * }
         *
         * auto msrs = kvm::MsrList{entries};
         * ```
         */
        template <class Container>
        MsrList(Container& c) : MsrList(c.begin(), c.end()) { }


        MsrList(const MsrList& other);
        MsrList(MsrList&& other) = default;
        auto operator=(MsrList other) -> MsrList&;

        auto nmsrs() const -> uint32_t;
        auto begin() -> kvm_msr_entry*;
        auto end() -> kvm_msr_entry*;
        auto begin() const -> kvm_msr_entry const*;
        auto end() const -> kvm_msr_entry const*;
        auto cbegin() const -> kvm_msr_entry const*;
        auto cend() const -> kvm_msr_entry const*;
};

class CpuidList : public FamStruct<kvm_cpuid2, uint32_t> {
    private:
        CpuidList(const uint32_t n);
    public:
        CpuidList();
        CpuidList(kvm_cpuid_entry2 entry);

        /**
         * Range constructor.
         *
         * Examples
         * ========
         * ```
         * #include <vmm/kvm.hpp>
         *
         * auto kvm = vmm::kvm::system{};
         * TODO
         * ```
         */
        template <class Iterator>
        CpuidList(Iterator first, Iterator last) : CpuidList(std::distance(first, last)) {
            std::copy_if(first, last, ptr_->entries, [](kvm_cpuid_entry2) { return true; });
        }

        /**
         * Container constructor.
         *
         * Examples
         * ========
         * ```
         * #include <vmm/kvm.hpp>
         *
         * auto kvm = vmm::kvm::system{};
         * TODO
         * ```
         */
        template <class Container>
        CpuidList(Container& c) : CpuidList(c.begin(), c.end()) { }

        CpuidList(const CpuidList& other);
        CpuidList(CpuidList&& other) = default;
        auto operator=(CpuidList other) -> CpuidList&;

        auto nent() const -> uint32_t;
        auto begin() -> kvm_cpuid_entry2*;
        auto end() -> kvm_cpuid_entry2*;
        auto begin() const -> kvm_cpuid_entry2 const*;
        auto end() const -> kvm_cpuid_entry2 const*;
        auto cbegin() const -> kvm_cpuid_entry2 const*;
        auto cend() const -> kvm_cpuid_entry2 const*;
};

}  // namespace vmm::kvm::detail
