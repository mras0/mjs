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

class gc_heap;
class gc_heap_ptr_untyped;
template<typename T>
class gc_heap_ptr;
template<typename T, bool strong>
class gc_heap_ptr_untracked;

// Only here to be friended
class object;
class value_representation;

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

    static_assert(!std::is_convertible_v<T*, object*> || needs_fixup, "Classes deriving from object MUST handle fixup");

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
        // We're fine with mjs::object having a non-virtual destructor
        // Derived classes are properly destructed by their own type info classes
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif
        static_cast<T*>(p)->~T();
#ifdef __clang__
#pragma clang diagnostic pop
#endif
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

class gc_heap {
public:
    friend gc_heap_ptr_untyped;
    friend value_representation;
    template<typename, bool> friend class gc_heap_ptr_untracked;

    static constexpr uint32_t slot_size = sizeof(uint64_t);
    static constexpr uint32_t bytes_to_slots(size_t bytes) { return static_cast<uint32_t>((bytes + slot_size - 1) / slot_size); }

    explicit gc_heap(uint32_t capacity);
    explicit gc_heap(void* storage, uint32_t capacity);
    gc_heap(gc_heap&) = delete;
    gc_heap& operator=(gc_heap&) = delete;
    ~gc_heap();

    int use_percentage() const { return alloc_context_.use_percentage(); }

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

    class pointer_set {
        static constexpr uint32_t initial_capacity = 10;
    public:
        explicit pointer_set() : set_(alloc(initial_capacity)), capacity_(initial_capacity), size_(0) {
        }
        ~pointer_set() { std::free(set_); }
        pointer_set(pointer_set&) = delete;
        pointer_set& operator=(pointer_set&) = delete;

        bool empty() const { return !size_; }
        uint32_t size() const { return size_; }
        auto begin() { return set_; }
        auto end() { return set_ + size_; }
        auto begin() const { return set_; }
        auto end() const { return set_ + size_; }

        gc_heap_ptr_untyped** data() { return set_; }

        void insert(gc_heap_ptr_untyped& p) {
            // Note: garbage_collect() assumes nodes are added to the back
            // assert(std::find(begin(), end(), &p) == end()); // Other asserts should catch this
            if (size_ == capacity_) {
                const auto new_cap = capacity_ * 2;
                auto n = alloc(new_cap);
                std::memcpy(n, set_, size_*sizeof(gc_heap_ptr_untyped*));
                std::free(set_);
                set_ = n;
                capacity_ = new_cap;
            }
            set_[size_++] = &p;
        }

        void erase(const gc_heap_ptr_untyped& p) {
            // Search from the back since objects tend to be short lived
            for (uint32_t i = size_; i--;) {
                if (set_[i] == &p) {
                    std::swap(set_[i], set_[--size_]);
                    return;
                }
            }
            assert(!"Pointer not found in set!");
        }
    private:
        gc_heap_ptr_untyped** set_;
        uint32_t capacity_;
        uint32_t size_;

        static gc_heap_ptr_untyped** alloc(uint32_t cap) {
            auto p = std::malloc(sizeof(gc_heap_ptr_untyped*) * cap);
            if (!p) throw std::bad_alloc{};
            return static_cast<gc_heap_ptr_untyped**>(p);
        }
    };

    struct allocation_result {
        uint32_t pos;
        slot* obj;

        slot_allocation_header& hdr() { return obj[-1].allocation; }
    };

    class allocation_context {
    public:    
        explicit allocation_context(void* s, uint32_t capacity) : allocation_context(static_cast<slot*>(s), capacity/2, 0) {
        }

        slot* storage() { return storage_; }
        uint32_t next_free() const { return next_free_; }

        // Allocate at least 'num_bytes' of storage, returns the offset (in slots) of the allocation (header) inside 'storage_'
        // The object must be constructed one slot beyond the allocation header and the type field of the allocation header updated
        allocation_result allocate(size_t num_bytes);

        void run_destructors();

        allocation_context other_half();

#ifndef NDEBUG
        bool pos_inside(uint32_t pos) const {
            return pos >= start_ && pos < next_free_;
        }
#endif

        int use_percentage() const {
            return static_cast<int>((next_free_ - start_) * 100ULL / (capacity_ - start_));
        }

        slot* get_at(uint32_t pos) const {
            assert(pos_inside(pos));
            return const_cast<slot*>(&storage_[pos]);
        }

        bool is_internal(const void* p) const {
            return reinterpret_cast<uintptr_t>(p) >= reinterpret_cast<uintptr_t>(storage_) && reinterpret_cast<uintptr_t>(p) < reinterpret_cast<uintptr_t>(storage_ + capacity_);
        }

    private:
        slot*    storage_;
        uint32_t capacity_;
        uint32_t start_;
        uint32_t next_free_;

        explicit allocation_context(slot* storage, uint32_t capacity, uint32_t start) : storage_(storage), capacity_(capacity), start_(start), next_free_(start) {
            assert(start < capacity);
        }
    };

    pointer_set         pointers_;
    allocation_context  alloc_context_;
    bool                owns_storage_;

    slot* get_at(uint32_t pos) const {
        return alloc_context_.get_at(pos);
    }

#ifndef NDEBUG
    template<typename T>
    bool type_check(uint32_t pos) const {
        return gc_type_info_registration<T>::get().is_convertible(get_at(pos-1)->allocation.type_info());
    }
#endif

    // Only valid during GC
    struct gc_state {
#ifndef NDEBUG
        bool initial_state() const { return level == 0 && new_context == nullptr && pending_fixups.empty() && weak_fixups.empty(); }
#endif

        uint32_t level = 0;                         // recursion depth
        allocation_context* new_context = nullptr;  // new allocation context (references to it should not be kept)
        std::vector<uint32_t*> pending_fixups;      // pending fixup addresses
        std::vector<uint32_t*> weak_fixups;         // pending weak fixup addresses
    } gc_state_;
    

    void attach(gc_heap_ptr_untyped& p);
    void detach(gc_heap_ptr_untyped& p);

    uint32_t gc_move(uint32_t pos);

    void register_fixup(uint32_t& pos);
    void register_weak_fixup(uint32_t& pos);

    template<typename T>
    gc_heap_ptr<T> unsafe_create_from_position(uint32_t pos);
};

class gc_heap_ptr_untyped {
public:
    friend gc_heap;
    friend value_representation;
    template<typename, bool> friend class gc_heap_ptr_untracked;

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
        return heap().get_at(pos_);
    }

    template<typename T>
    bool has_type() const {
        return pos_ && heap_->get_at(pos_-1)->allocation.type == gc_type_info_registration<T>::index();
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

template<typename T, bool strong = true>
class gc_heap_ptr_untracked {
    // TODO: Add debug mode where e.g. the MSB of pos_ is set when the pointer is copied
    //       Then check that 1) it is set in fixup 2) NOT set in the destructor
    //       Should also do something similar for value_representation
    //       NOTE: this will probably make gc_table have a non-trivial destructor, so will need global flag/define
public:
    gc_heap_ptr_untracked() : pos_(0) {}
    gc_heap_ptr_untracked(std::nullptr_t) : pos_(0) {}
    gc_heap_ptr_untracked(const gc_heap_ptr<T>& p) : pos_(p.pos_) {}
    gc_heap_ptr_untracked(const gc_heap_ptr_untracked&) = default;
    gc_heap_ptr_untracked& operator=(const gc_heap_ptr_untracked&) = default;

    explicit operator bool() const { return pos_; }

    T& dereference(gc_heap& h) const {
        assert(h.type_check<T>(pos_));
        return *reinterpret_cast<T*>(h.get_at(pos_));
    }

    gc_heap_ptr<T> track(gc_heap& h) const {
        return h.unsafe_create_from_position<T>(pos_);
    }

    void fixup(gc_heap& old_heap) {
        if (pos_) {
            if constexpr (strong) {
                old_heap.register_fixup(pos_);
            } else {
                old_heap.register_weak_fixup(pos_);
            }
        }
    }

private:
    uint32_t pos_;

    explicit gc_heap_ptr_untracked(uint32_t pos) : pos_(pos) {}
};

template<typename T>
using gc_heap_weak_ptr_untracked = gc_heap_ptr_untracked<T, false>;

template<typename T, typename... Args>
gc_heap_ptr<T> gc_heap::allocate_and_construct(size_t num_bytes, Args&&... args) {
    auto a = alloc_context_.allocate(num_bytes);
    assert(a.hdr().type == uninitialized_type_index);
    gc_type_info_registration<T>::construct(a.obj, std::forward<Args>(args)...);
    a.hdr().type = gc_type_info_registration<T>::index();
    return gc_heap_ptr<T>{*this, a.pos};
}

template<typename T>
gc_heap_ptr<T> gc_heap::unsafe_create_from_position(uint32_t pos) {
    assert(type_check<T>(pos));
    return gc_heap_ptr<T>{*this, pos};
}

} // namespace mjs

#endif
