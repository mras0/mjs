#ifndef MJS_GC_HEAP_H
#define MJS_GC_HEAP_H

#include <string>
#include <vector>
#include <algorithm>
#include <ostream>
#include <typeinfo>
#include <cstdlib>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace mjs {

class value;
class object;
class gc_heap;
class gc_heap_ptr_untyped;
template<typename T>
class gc_heap_ptr;
template<typename T>
class gc_heap_ptr_untracked;

class gc_type_info {
public:
    // Destroy the object at 'p'
    void destroy(void* p) const {
        if (destroy_) {
            destroy_(p);
        }
    }

    // Move the object from 'from' to 'to'
    void move(void* to, void* from) const {
        move_(to, from);
    }

    // Handle fixup of untacked pointers (happens after the object has been otherwise moved to avoid infite recursion)
    void fixup(void* p) const {
        if (fixup_) {
            fixup_(p);
        }
    }

    // For debugging purposes only
    const char* name() const {
        return name_;
    }

    // Is the type convertible to object?
    bool is_convertible_to_object() const {
        return convertible_to_object_;
    }

    // Return unique type index
    uint32_t get_index() const {
        return index_;
    }

    static uint32_t num_types() {
        return num_types_;
    }

    static const gc_type_info& from_index(uint32_t index) {
        assert(index < num_types_);
        return *types_[index];
    }

protected:
    using destroy_function = void (*)(void*);
    using move_function = void (*)(void*, void*);
    using fixup_function = void (*)(void*);

    explicit gc_type_info(destroy_function destroy, move_function move, fixup_function fixup, bool convertible_to_object, const char* name)
        : destroy_(destroy)
        , move_(move)
        , fixup_(fixup)
        , convertible_to_object_(convertible_to_object)
        , name_(name)
        , index_(num_types_++) {
        assert(index_ < max_types);
        types_[index_] = this;
        assert(move_);
        assert(name_);
    }

private:
    destroy_function destroy_;
    move_function move_;
    fixup_function fixup_;
    bool convertible_to_object_;
    const char* name_;
    const uint32_t index_;

    static constexpr uint32_t max_types = 32; // Arbitrary limit

    gc_type_info(gc_type_info&) = delete;
    gc_type_info& operator=(gc_type_info&) = delete;

    static uint32_t num_types_;
    static const gc_type_info* types_[max_types];
};

template<typename T>
class gc_type_info_registration : public gc_type_info {
    // Detectors
    template<typename U, typename=void>
    struct has_fixup_t : std::false_type{};

    template<typename U>
    struct has_fixup_t<U, std::void_t<decltype(std::declval<U>().fixup())>> : std::true_type{};

public:
    static constexpr bool needs_destroy = !std::is_trivially_destructible_v<T>;
    static constexpr bool needs_fixup   = has_fixup_t<T>::value;

    static_assert(!std::is_convertible_v<T*, object*> || (needs_destroy && needs_fixup), "Classes deriving from object MUST be properly destroyed and will need fixup");

    static const gc_type_info_registration& get() {
        return reg;
    }

    static uint32_t index() {
        return get().get_index();
    }

    bool is_convertible(const gc_type_info& t) const {
        return &t == this || (std::is_same_v<object, T> && t.is_convertible_to_object());
    }

    // Helper so gc_*** classes don't have to friend both gc_heap and gc_type_info_registration
    template<typename... Args>
    static void construct(void* p, Args&&... args) {
        new (p) T(std::forward<Args>(args)...);
    }

private:
    explicit gc_type_info_registration() : gc_type_info(needs_destroy?&destroy:nullptr, &move, needs_fixup?&fixup:nullptr, std::is_convertible_v<T*, object*>, typeid(T).name()) {
        static_assert(sizeof(gc_type_info_registration<T>) == sizeof(gc_type_info));
    }

    static const gc_type_info_registration reg;

    static void destroy(void* p) {
        static_cast<T*>(p)->~T();
    }

    static void move(void* to, void* from) {
        new (to) T (std::move(*static_cast<T*>(from)));
    }

    static void fixup([[maybe_unused]] void* p) {
        if constexpr (needs_fixup) {
            static_cast<T*>(p)->fixup();
        }
    }
};

template<typename T>
const gc_type_info_registration<T> gc_type_info_registration<T>::reg;

class value_representation {
public:
    value_representation() = default;
    explicit value_representation(const value& v);
    value get_value(gc_heap& heap) const;
    void fixup(gc_heap& old_heap);
private:
    uint64_t repr_;
};

class gc_heap {
public:
    friend gc_heap_ptr_untyped;
    friend value_representation;
    template<typename> friend class gc_heap_ptr_untracked;

    static constexpr uint32_t slot_size = sizeof(uint64_t);
    static constexpr uint32_t bytes_to_slots(size_t bytes) { return static_cast<uint32_t>((bytes + slot_size - 1) / slot_size); }

    explicit gc_heap(uint32_t capacity);
    ~gc_heap();

    void debug_print(std::wostream& os) const;
    uint32_t calc_used() const;

    void garbage_collect();

    template<typename T, typename... Args>
    gc_heap_ptr<T> allocate_and_construct(size_t num_bytes, Args&&... args);

    template<typename T, typename... Args>
    gc_heap_ptr<T> make(Args&&... args) {
        return allocate_and_construct<T>(sizeof(T), std::forward<Args>(args)...);
    }

private:
    static constexpr uint32_t uninitialized_type_index = UINT32_MAX;
    static constexpr uint32_t gc_moved_type_index      = uninitialized_type_index-1;

    struct slot_allocation_header {
        uint32_t size; // size in slots including the allocation header
        uint32_t type; // index into gc_type_info::types_ OR one of the special xxxx_type_index values

        constexpr bool active() const {
            return type != uninitialized_type_index && type != gc_moved_type_index;
        }

        const gc_type_info& type_info() const {
            return gc_type_info::from_index(type);
        }
    };
    static_assert(sizeof(slot_allocation_header) == slot_size);

    union slot {
        uint64_t               representation;
        uint32_t               new_position;   // Only ever valid during garbage collection (and then only in the first slot after the allocation header)
        slot_allocation_header allocation;     // Only valid for allocation header slots
    };
    static_assert(sizeof(slot) == slot_size);

    struct gc_state;
    class pointer_set {
        std::vector<gc_heap_ptr_untyped*> set_;
    public:
        bool empty() const { return set_.empty(); }
        uint32_t size() const { return static_cast<uint32_t>(set_.size()); }
        auto begin() { return set_.begin(); }
        auto end() { return set_.end(); }
        auto begin() const { return set_.cbegin(); }
        auto end() const { return set_.cend(); }

        gc_heap_ptr_untyped** data() { return set_.data(); }

        void insert(gc_heap_ptr_untyped& p) {
            // Note: garbage_collect() assumes nodes are added to the back
            // assert(std::find(begin(), end(), &p) == end()); // Other asserts should catch this
            set_.push_back(&p);
        }

        void erase(const gc_heap_ptr_untyped& p) {
            // Search from the back since objects tend to be short lived
            for (size_t i = set_.size(); i--;) {
                if (set_[i] == &p) {
                    set_.erase(set_.begin() + i);
                    return;
                }
            }
            assert(!"Pointer not found in set!");
        }
    };

    pointer_set pointers_;
    slot*       storage_;
    uint32_t    capacity_;
    uint32_t    next_free_ = 0;

    // Only valid during GC
    struct gc_state {
#ifndef NDEBUG
        bool initial_state() const { return level == 0 && new_heap == nullptr && pending_fixups.empty(); }
#endif

        uint32_t level = 0;                     // recursion depth
        gc_heap* new_heap = nullptr;            // the "new_heap" is only kept for allocation purposes, no references to it should be kept
        std::vector<uint32_t*> pending_fixups;  // pending fixup addresses
    } gc_state_;

    void run_destructors();

    void attach(gc_heap_ptr_untyped& p);
    void detach(gc_heap_ptr_untyped& p);

    bool is_internal(const void* p) const {
        return reinterpret_cast<uintptr_t>(p) >= reinterpret_cast<uintptr_t>(storage_) && reinterpret_cast<uintptr_t>(p) < reinterpret_cast<uintptr_t>(storage_ + capacity_);
    }

    // Allocate at least 'num_bytes' of storage, returns the offset (in slots) of the allocation (header) inside 'storage_'
    // The object must be constructed one slot beyond the allocation header and the type field of the allocation header updated
    uint32_t allocate(size_t num_bytes);

    uint32_t gc_move(uint32_t pos);

    void register_fixup(uint32_t& pos);

    template<typename T>
    gc_heap_ptr<T> unsafe_create_from_position(uint32_t pos);
};

class gc_heap_ptr_untyped {
public:
    friend gc_heap;
    friend value_representation;
    template<typename> friend class gc_heap_ptr_untracked;

    gc_heap_ptr_untyped() : heap_(nullptr), pos_(0) {
    }
    gc_heap_ptr_untyped(const gc_heap_ptr_untyped& p) : heap_(p.heap_), pos_(p.pos_) {
        if (heap_) {
            heap_->attach(*this);
        }
    }
    ~gc_heap_ptr_untyped() {
        if (heap_) {
            heap_->detach(*this);
        }
    }
    gc_heap_ptr_untyped& operator=(const gc_heap_ptr_untyped& p) {
        if (this != &p) {
            if (heap_) {
                heap_->detach(*this);
            }
            heap_ = p.heap_;
            pos_ = p.pos_;
            if (heap_) {
                heap_->attach(*this);
            }
        }
        return *this;
    }

    gc_heap& heap() const {
        return const_cast<gc_heap&>(*heap_);
    }

    explicit operator bool() const { return heap_; }

    void* get() const {
        assert(heap_);
        return const_cast<void*>(static_cast<const void*>(&heap_->storage_[pos_]));
    }

protected:
    explicit gc_heap_ptr_untyped(gc_heap& heap, uint32_t pos) : heap_(&heap), pos_(pos) {
        heap_->attach(*this);
    }

private:
    gc_heap* heap_;
    uint32_t pos_;
};

template<typename T>
class gc_heap_ptr : public gc_heap_ptr_untyped {
public:
    friend gc_heap;

    gc_heap_ptr() = default;
    gc_heap_ptr(std::nullptr_t) : gc_heap_ptr_untyped() {}
    template<typename U, typename = typename std::enable_if<std::is_convertible_v<U*, T*>>::type>
    gc_heap_ptr(const gc_heap_ptr<U>& p) : gc_heap_ptr_untyped(p) {}

    T* get() const { return static_cast<T*>(gc_heap_ptr_untyped::get()); }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

private:
    explicit gc_heap_ptr(gc_heap& heap, uint32_t pos) : gc_heap_ptr_untyped(heap, pos) {}
    explicit gc_heap_ptr(const gc_heap_ptr_untyped& p) : gc_heap_ptr_untyped(p) {}
};

template<typename T>
class gc_heap_ptr_untracked {
    // TODO: Add debug mode where e.g. the MSB of pos_ is set when the pointer is copied
    //       Then check that 1) it is set in fixup 2) NOT set in the destructor
    //       Should also do something similar for value_representation
    //       NOTE: this will probably make gc_table have a non-trivial destructor, so will need global flag/define
public:
    gc_heap_ptr_untracked() : pos_(0) {}
    gc_heap_ptr_untracked(const gc_heap_ptr<T>& p) : pos_(p.pos_) {}
    gc_heap_ptr_untracked(const gc_heap_ptr_untracked&) = default;
    gc_heap_ptr_untracked& operator=(const gc_heap_ptr_untracked&) = default;

    explicit operator bool() const { return pos_; }

    T& dereference(gc_heap& h) const {
        assert(pos_ > 0 && pos_ < h.next_free_ && gc_type_info_registration<T>::get().is_convertible(h.storage_[pos_-1].allocation.type_info()));
        return *reinterpret_cast<T*>(&h.storage_[pos_]);
    }

    gc_heap_ptr<T> track(gc_heap& h) const {
        assert(pos_);
        return h.unsafe_create_from_position<T>(pos_);
    }

    void fixup(gc_heap& old_heap) {
        if (pos_) {
            old_heap.register_fixup(pos_);
        }
    }

private:
    uint32_t pos_;

    explicit gc_heap_ptr_untracked(uint32_t pos) : pos_(pos) {}
};

template<typename T, typename... Args>
gc_heap_ptr<T> gc_heap::allocate_and_construct(size_t num_bytes, Args&&... args) {
    const auto pos = allocate(num_bytes);
    auto& a = storage_[pos].allocation;
    assert(a.type == uninitialized_type_index);
    gc_type_info_registration<T>::construct(&storage_[pos+1], std::forward<Args>(args)...);
    a.type = gc_type_info_registration<T>::index();
    return gc_heap_ptr<T>{*this, pos+1};
}

template<typename T>
gc_heap_ptr<T> gc_heap::unsafe_create_from_position(uint32_t pos) {
    assert(pos > 0 && pos < next_free_ && gc_type_info_registration<T>::get().is_convertible(storage_[pos-1].allocation.type_info()));
    return gc_heap_ptr<T>{*this, pos};
}

} // namespace mjs

#endif
