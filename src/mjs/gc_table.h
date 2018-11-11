#ifndef MJS_GC_TABLE_H
#define MJS_GC_TABLE_H

// TODO: gc_table: Allow real values to be saved
// TODO: gc_table: Attributes
#include "gc_heap.h"
#include "value.h"

namespace mjs {

class gc_table {
public:
    static gc_heap_ptr<gc_table> make(gc_heap& h, uint32_t capacity) {
        return gc_heap::construct<gc_table>(h.allocate(sizeof(gc_table) + capacity * sizeof(entry_representation)), h, capacity);
    }

    uint32_t capacity() const { return capacity_; }
    uint32_t length() const { return length_; }

    void push_back(const gc_heap_ptr<gc_string>& key, const gc_heap_ptr<gc_string>& value) {
        assert(length() < capacity());
        entries()[length_++] = entry_representation{
            key.unsafe_get_position(),
            value.unsafe_get_position()
        };
    }

    std::pair<gc_heap_ptr<gc_string>, gc_heap_ptr<gc_string>> get(uint32_t index) const {
        assert(index < length_);
        auto& e = entries()[index];
        return std::make_pair(heap_->unsafe_create_from_position<gc_string>(e.key_index),  heap_->unsafe_create_from_position<gc_string>(e.value_index));
    }

private:
    friend gc_type_info_registration<gc_table>;

    struct entry_representation {
        uint32_t key_index;
        uint32_t value_index;
    };
    gc_heap* heap_;     // TODO: Deduce somehow?
    uint32_t capacity_; // TODO: Get from allocation header
    uint32_t length_;

    entry_representation* entries() const {
        return reinterpret_cast<entry_representation*>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) + sizeof(*this));
    }

    explicit gc_table(gc_heap& h, uint32_t capacity) : heap_(&h), capacity_(capacity), length_(0) {
    }

    gc_heap_ptr_untyped move(gc_heap& new_heap) const {
        auto p = make(new_heap, capacity());
        p->heap_ = heap_;
        p->length_ = length();
        for (uint32_t i = 0; i < length(); ++i) {
            auto& oe = entries()[i];
            auto& ne = p->entries()[i];
            ne.key_index = heap_->unsafe_gc_move(new_heap, oe.key_index);
            ne.value_index = heap_->unsafe_gc_move(new_heap, oe.value_index);
        }
        return p;
    }
};

} // namespace mjs
#endif
