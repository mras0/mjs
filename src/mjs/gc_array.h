#ifndef MJS_GC_ARRAY_H
#define MJS_GC_ARRAY_H

#include "gc_heap.h"

namespace mjs {

//
// Helper for implementing dynamic arrays on the GC heap
//

template<typename Derived, typename Entry>
class alignas(uint64_t) gc_array_base {
public:
    gc_heap& heap() const { return const_cast<gc_heap&>(heap_); }
    uint32_t capacity() const { return capacity_; }
    uint32_t length() const { return length_; }

protected:
    template<typename... Args>
    static gc_heap_ptr<Derived> make(gc_heap& h, uint32_t capacity, Args&&... args) {
        assert(capacity);
        static_assert(std::is_base_of_v<gc_array_base, Derived>);
        return h.allocate_and_construct<Derived>(sizeof(Derived) + capacity * sizeof(Entry), h, capacity, std::forward<Args>(args)...);
    }

    // TODO: Provide better options than this
    void length(uint32_t new_length) {
        assert(new_length <= capacity_);
        length_ = new_length;
    }

    Entry* entries() const {
        return reinterpret_cast<Entry*>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) + sizeof(Derived));
    }


    explicit gc_array_base(gc_heap& heap, uint32_t capacity) : heap_(heap), capacity_(capacity) {
    }

    gc_array_base(gc_array_base&& other) noexcept : heap_(other.heap_), capacity_(other.capacity_), length_(other.length_) {
        static_assert(std::is_trivially_copyable_v<Entry>);
        std::memcpy(entries(), other.entries(), length_ * sizeof(Entry));
    }


private:
    friend gc_type_info_registration<Derived>;

    gc_heap& heap_;
    uint32_t capacity_;
    uint32_t length_ = 0;
};

} // namespace mjs

#endif // !MJS_GC_ARRAY_H
