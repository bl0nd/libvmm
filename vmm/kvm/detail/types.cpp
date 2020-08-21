/*
 * types.cpp - KVM types
 */

#include "vmm/kvm/detail/types.hpp"

#include <cassert>

namespace vmm::kvm::detail {

/**
 * Internal MsrIndexList/MsrFeatureList constructor.
 *
 * Relevant struct:
 *
 *     struct kvm_msr_list {
 *         __u32 nmsrs;
 *         __u32 indices[0];
 *     };
 *
 * Buffer size computation:
 *
 *     N + sizeof(__u32)
 */
MsrIndexList::MsrIndexList(const size_t n) : FamStruct(n + 1) { ptr_->nmsrs = n; }

// Element access
auto MsrIndexList::data() noexcept -> pointer { return ptr_->indices; }
auto MsrIndexList::data() const noexcept -> const_pointer { return ptr_->indices; }

// Capacity
auto MsrIndexList::empty() const noexcept -> bool { return size() == 0; }
auto MsrIndexList::size() const noexcept -> unsigned int { return ptr_->nmsrs; }

// Iterators
auto MsrIndexList::begin() noexcept -> iterator { return data(); }
auto MsrIndexList::end() noexcept -> iterator { return data() + size(); }
auto MsrIndexList::begin() const noexcept -> const_iterator { return data(); }
auto MsrIndexList::end() const noexcept -> const_iterator { return begin() + size(); }
auto MsrIndexList::cbegin() const noexcept -> const_iterator { return begin(); }
auto MsrIndexList::cend() const noexcept -> const_iterator { return end(); }


MsrFeatureList::MsrFeatureList(const size_t n) : MsrIndexList(n) {}


/**
 * Internal MsrList constructor.
 *
 * Relevant structs:
 *
 *     struct kvm_msrs {
 *         __u32 nmsrs;
 *         __u32 pad;
 *         struct kvm_msr_entry entries[0];
 *     };
 *
 *     struct kvm_msr_entry {
 *         __u32 index;
 *         __u32 reserved;
 *         __u64 data;
 *     };
 *
 * Buffer size computation:
 *
 *     N * (sizeof(kvm_msr_entry) / sizeof(__u64)) + sizeof(__u64)
 */
MsrList::MsrList(const size_t n) : FamStruct(n * 2 + 1) { ptr_->nmsrs = n; }
MsrList::MsrList(value_type entry) : MsrList(1) { ptr_->entries[0] = entry; }

/* Copy constructor */
MsrList::MsrList(const MsrList& other) : MsrList(other.begin(), other.end()) {}

/* Copy/move assignment operator */
auto MsrList::operator=(MsrList other) -> MsrList& {
    other.ptr_.swap(this->ptr_);
    return *this;
}

// Capacity
auto MsrList::size() const noexcept -> uint32_t { return ptr_->nmsrs; }

// Iterators
auto MsrList::begin() noexcept -> iterator { return ptr_->entries; }
auto MsrList::end() noexcept -> iterator { return ptr_->entries + ptr_->nmsrs; }
auto MsrList::begin() const noexcept -> const_iterator { return ptr_->entries; }
auto MsrList::end() const noexcept -> const_iterator { return ptr_->entries + ptr_->nmsrs; }
auto MsrList::cbegin() const noexcept -> const_iterator { return begin(); }
auto MsrList::cend() const noexcept -> const_iterator { return end(); }


/*
 * Internal CpuidList constructor.
 *
 * Relevant structs:
 *
 * struct kvm_cpuid2 {
 *     __u32 nent;
 *     __u32 padding;
 *     struct kvm_cpuid_entry2 entries[0];
 * };
 *
 * struct kvm_cpuid_entry2 {
 *     __u32 function;
 *     __u32 index;
 *     __u32 flags;
 *     __u32 eax;
 *     __u32 ebx;
 *     __u32 ecx;
 *     __u32 edx;
 *     __u32 padding[3];
 * };
 *
 * Buffer size computation:
 *
 *     N * sizeof(kvm_cpuid_entry2) + 2 * sizeof(__u32)
 */
CpuidList::CpuidList(const uint32_t n) : FamStruct(n * sizeof(value_type) + 2) { ptr_->nent = n; }
CpuidList::CpuidList() : CpuidList(MAX_CPUID_ENTRIES) {}
CpuidList::CpuidList(value_type entry) : CpuidList(1) { ptr_->entries[0] = entry; }

/* Copy constructor */
CpuidList::CpuidList(const CpuidList& other) : CpuidList(other.begin(), other.end()) {}
auto CpuidList::operator=(CpuidList other) -> CpuidList& {
    other.ptr_.swap(this->ptr_);
    return *this;
}

// Capacity
auto CpuidList::size() const noexcept -> uint32_t { return ptr_->nent; }

// Iterators
auto CpuidList::begin() noexcept -> iterator { return ptr_->entries; }
auto CpuidList::end() noexcept -> iterator { return ptr_->entries + ptr_->nent; }
auto CpuidList::begin() const noexcept -> const_iterator { return ptr_->entries; }
auto CpuidList::end() const noexcept -> const_iterator { return ptr_->entries + ptr_->nent; }
auto CpuidList::cbegin() const noexcept -> const_iterator { return begin(); }
auto CpuidList::cend() const noexcept -> const_iterator { return end(); }


/**
 * Internal IrqRoutingList constructor.
 *
 * Relevant structs:
 *
 * struct kvm_irq_routing {
 *       __u32 nr;
 *       __u32 flags;
 *       struct kvm_irq_routing_entry entries[0];
 * };
 *
 * struct kvm_irq_routing_entry {
 *       __u32 gsi;
 *       __u32 type;
 *       __u32 flags;
 *       __u32 pad;
 *       union {
 *               struct kvm_irq_routing_irqchip irqchip;
 *               struct kvm_irq_routing_msi msi;
 *               struct kvm_irq_routing_s390_adapter adapter;
 *               struct kvm_irq_routing_hv_sint hv_sint;
 *               __u32 pad[8];
 *       } u;
 * };
 */
IrqRoutingList::IrqRoutingList(const uint32_t n) : FamStruct(n * (sizeof(value_type) / 2) + 1) {
    ptr_->nr = n;
    ptr_->flags = 0;
}
IrqRoutingList::IrqRoutingList(value_type entry) : IrqRoutingList(1) { ptr_->entries[0] = entry; }

/* Copy constructor */
IrqRoutingList::IrqRoutingList(const IrqRoutingList& other) : IrqRoutingList(other.begin(), other.end()) {}

/* Copy/move assignment operator */
auto IrqRoutingList::operator=(IrqRoutingList other) -> IrqRoutingList& {
    other.ptr_.swap(this->ptr_);
    return *this;
}

// Capacity
auto IrqRoutingList::size() const noexcept -> uint32_t { return ptr_->nr; }

// Iterators
auto IrqRoutingList::begin() noexcept -> iterator { return ptr_->entries; }
auto IrqRoutingList::end() noexcept -> iterator { return ptr_->entries + ptr_->nr; }
auto IrqRoutingList::begin() const noexcept -> const_iterator { return ptr_->entries; }
auto IrqRoutingList::end() const noexcept -> const_iterator { return ptr_->entries + ptr_->nr; }
auto IrqRoutingList::cbegin() const noexcept -> const_iterator { return begin(); }
auto IrqRoutingList::cend() const noexcept -> const_iterator { return end(); }

} // namespace vmm::kvm::detail
