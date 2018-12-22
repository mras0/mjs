#include "gc_heap.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstdlib>

namespace mjs {

//
// gc_type_info
//

uint32_t gc_type_info::num_types_;
const gc_type_info* gc_type_info::types_[gc_type_info::max_types];

//
// gc_heap
//

gc_heap::gc_heap(uint32_t capacity) : alloc_context_(std::malloc(capacity * sizeof(slot)), capacity), owns_storage_(true) {
    if (!alloc_context_.storage()) {
        throw std::runtime_error("Could not allocate heap for " + std::to_string(capacity) + " slots");
    }
}

gc_heap::gc_heap(void* storage, uint32_t capacity) : alloc_context_(storage, capacity), owns_storage_(false) {
}

gc_heap::~gc_heap() {
    assert(gc_state_.initial_state());
    alloc_context_.run_destructors();
    if (owns_storage_) {
        std::free(alloc_context_.storage());
    }
    assert(pointers_.empty());
}

void gc_heap::garbage_collect() {
    assert(gc_state_.initial_state());

    // Determine roots and add their positions as pending fixups
    // TODO: Used to move the roots lower in the pointers_ array (since we know they won't be destroyed this time around). That still might be an optimization.
    for (auto p: pointers_) {
        if (!alloc_context_.is_internal(p)) {
            register_fixup(p->pos_);
        }
    }

    if (!gc_state_.pending_fixups.empty()) {
        allocation_context new_ac = alloc_context_.other_half();

        gc_state_.new_context = &new_ac;
        gc_state_.level = 0;

        // Keep going while there are still fixups to be processed (note: the array changes between loop iterations)
        while (!gc_state_.pending_fixups.empty()) {
            auto ppos = gc_state_.pending_fixups.back();
            gc_state_.pending_fixups.pop_back();
            *ppos = gc_move(*ppos);
        }

        // Handle weak pointers - if they moved update, otherwise invalidate
        for (auto& p: gc_state_.weak_fixups) {
            assert(alloc_context_.pos_inside(*p - 1));
            auto a = alloc_context_.get_at(*p - 1)->allocation;
            if (a.type == gc_moved_type_index) {
                *p = alloc_context_.get_at(*p)->new_position;
            } else {
                *p = 0;
            }
        }
        gc_state_.weak_fixups.clear();

        std::swap(alloc_context_, new_ac);
        new_ac.run_destructors();
        gc_state_.new_context = nullptr;
    } else {
        alloc_context_.run_destructors();
    }

    assert(gc_state_.initial_state());
}

uint32_t gc_heap::gc_move(const uint32_t pos) {
    struct auto_level {
        auto_level(uint32_t& l) : l(l) { ++l; assert(l < 4 && "Arbitrary recursion level reached"); }
        ~auto_level() { --l; }
        uint32_t& l;
    } al{gc_state_.level};

    assert(alloc_context_.pos_inside(pos-1));

    auto& a = alloc_context_.get_at(pos-1)->allocation;
    assert(a.type != uninitialized_type_index);
    assert(a.size > 1 && a.size <= alloc_context_.next_free() - (pos - 1));

    if (a.type == gc_moved_type_index) {
        return alloc_context_.get_at(pos)->new_position;
    }

    assert(a.type < gc_type_info::num_types());

    // Allocate memory block in new_heap of the same size
    auto new_obj = gc_state_.new_context->allocate((a.size-1)*slot_size);
    assert(new_obj.hdr().type == uninitialized_type_index && new_obj.hdr().size == a.size);

    // Record number of pointers that exist before constructing the new object
    const auto num_pointers_initially = pointers_.size();

    // Move the object to its new position
    const auto& type_info = a.type_info();
    void* const p = alloc_context_.get_at(pos);
    type_info.move(new_obj.obj, p);
    new_obj.hdr().type = a.type;

    // Register fixups for the position of all internal pointers that were created by the move (construction)
    // The new pointers will be at the end of the pointer set since they were just added
    // Note this obviously makes assumption about the pointer_set implementation!
    // TODO: Used to move the pointers lower in the pointers_ array (since we know they won't be destroyed this time around). That still might be an optimization.
    if (const auto num_internal_pointers  = pointers_.size() - num_pointers_initially) {
        auto ps = pointers_.data();
        for (uint32_t i = 0; i < num_internal_pointers; ++i) {
            assert(reinterpret_cast<uintptr_t>(ps[num_pointers_initially+i]) >= reinterpret_cast<uintptr_t>(new_obj.obj) && reinterpret_cast<uintptr_t>(ps[num_pointers_initially+i]) < reinterpret_cast<uintptr_t>(new_obj.obj + a.size - 1));
            register_fixup(ps[num_pointers_initially+i]->pos_);
        }
    }

    // And destroy it at the old position
    type_info.destroy(p);

    // There should now be the same amount of pointers (otherwise something went wrong with moving/destroying the object)
    assert(pointers_.size() == num_pointers_initially);

    // Record the object's new position at the old position
    a.type = gc_moved_type_index;
    alloc_context_.get_at(pos)->new_position = new_obj.pos;

    // After changing the allocation header infinite recursion can now be avoided when copying the internal pointers.

    // Let the object register its fixup
    type_info.fixup(new_obj.obj);

    return new_obj.pos;
}

void gc_heap::register_fixup(uint32_t& pos) {
    gc_state_.pending_fixups.push_back(&pos);
}

void gc_heap::register_weak_fixup(uint32_t& pos) {
    gc_state_.weak_fixups.push_back(&pos);
}

void gc_heap::attach(gc_heap_ptr_untyped& p) {
    assert(p.heap_ == this && alloc_context_.pos_inside(p.pos_));
    pointers_.insert(p);
}

void gc_heap::detach(gc_heap_ptr_untyped& p) {
    assert(p.heap_ == this);
    pointers_.erase(p);
}

gc_heap::allocation_result gc_heap::allocation_context::allocate(size_t num_bytes) {
    if (!num_bytes || num_bytes >= UINT32_MAX) {
        assert(!"Invalid allocation size");
        throw std::bad_alloc{};
    }

    const auto num_slots = 1 + bytes_to_slots(num_bytes);
    if (num_slots > capacity_ || next_free_ > capacity_ - num_slots) {
        assert(!"Not implemented: Ran out of heap");
        throw std::bad_alloc{};
    }
    const auto pos = next_free_;
    next_free_ += num_slots;
    storage_[pos].allocation.size = num_slots;
    storage_[pos].allocation.type = uninitialized_type_index;
    return { pos + 1, &storage_[pos + 1] };
}

void gc_heap::allocation_context::run_destructors() {
    for (uint32_t pos = start_; pos < next_free_;) {
        const auto a = storage_[pos].allocation;
        if (a.active()) {
            a.type_info().destroy(&storage_[pos+1]);
        }
        pos += a.size;
    }

    next_free_ = start_;
}

gc_heap::allocation_context gc_heap::allocation_context::other_half() {
    return allocation_context{ storage_, start_ ? capacity_ / 2 : capacity_ * 2, start_ ? 0 : capacity_ };
}

} // namespace mjs
