#ifndef MJS_GC_HEAP_H
#define MJS_GC_HEAP_H

// TODO: gc_type_info_registration: Make sure all known classes are (thread-safely) registered.
// TODO: Reduce uncecessary use of const_cast

#include <string>
#include <vector>
#include <set>
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
    bool fixup(gc_heap& new_heap, void* p) const {
        if (fixup_) {
            return fixup_ == no_fixup_needed || fixup_(new_heap, p);
        }
        return false;
    }

    // For debugging purposes only
    const char* name() const {
        return name_;
    }

    // Is the type convertible to object?
    bool is_convertible_to_object() const {
        return convertible_to_object_;
    }
protected:
    using destroy_function = void (*)(void*);
    using move_function = void (*)(void*, void*);
    using fixup_function = bool (*)(gc_heap&, void*);

    static const fixup_function no_fixup_needed; // special value (not callable!)

    explicit gc_type_info(destroy_function destroy, move_function move, fixup_function fixup, bool convertible_to_object, const char* name) : destroy_(destroy), move_(move), fixup_(fixup), convertible_to_object_(convertible_to_object), name_(name) {
        types_.push_back(this);
        assert(move_);
        assert(name_);
    }

private:
    destroy_function destroy_;
    move_function move_;
    fixup_function fixup_;
    bool convertible_to_object_;
    const char* name_;
    friend gc_heap;

    gc_type_info(gc_type_info&) = delete;
    gc_type_info& operator=(gc_type_info&) = delete;

    static std::vector<const gc_type_info*> types_;

    uint32_t get_index() const {
        auto it = std::find(types_.begin(), types_.end(), this);
        if (it == types_.end()) {
            std::abort();
        }
        return static_cast<uint32_t>(it - types_.begin());
    }
};

template<typename T>
class gc_type_info_registration : public gc_type_info {
public:
    static const gc_type_info_registration& get() {
        static const gc_type_info_registration reg;
        return reg;
    }

    bool is_convertible(const gc_type_info& t) const {
        return &t == this || (std::is_same_v<object, T> && t.is_convertible_to_object());
    }

private:
    explicit gc_type_info_registration() : gc_type_info(std::is_trivially_destructible_v<T>?nullptr:&destroy, &move, fixup_func(), std::is_convertible_v<T*, object*>, typeid(T).name()) {}

    friend gc_heap;

    // Helper so gc_*** classes don't have to friend both gc_heap and gc_type_info_registration
    template<typename... Args>
    static void construct(void* p, Args&&... args) {
        new (p) T(std::forward<Args>(args)...);
    }

    static void destroy(void* p) {
        static_cast<T*>(p)->~T();
    }

    // Detectors
    template<typename U, typename=void>
    struct has_fixup_t : std::false_type{};

    template<typename U>
    struct has_fixup_t<U, std::void_t<decltype(std::declval<U>().fixup(std::declval<gc_heap&>()))>> : std::true_type{};

    template<typename U, typename=void>
    struct has_trivial_fixup_t : std::false_type{};

    template<typename U>
    struct has_trivial_fixup_t<U, std::void_t<decltype(std::declval<U>().trivial_fixup())>> : std::true_type{};

    static void move(void* to, void* from) {
        new (to) T (std::move(*static_cast<T*>(from)));
    }

    static fixup_function fixup_func() {
        if constexpr (has_trivial_fixup_t<T>::value) {
            return no_fixup_needed;
        } else if constexpr(has_fixup_t<T>::value) {
            return [](gc_heap& new_heap, void* p) { return static_cast<T*>(p)->fixup(new_heap); };
        } else {
            return nullptr;
        }
    }
};

class value_representation {
public:
    value_representation() = default;
    explicit value_representation(const value& v) : repr_(to_representation(v)) {}
    value_representation& operator=(const value& v) {
        repr_ = to_representation(v);
        return *this;
    }
    value get_value(gc_heap& heap) const;
    void fixup_after_move(gc_heap& new_heap, gc_heap& old_heap);
private:
    uint64_t repr_;

    static uint64_t to_representation(const value& v);
};
static_assert(sizeof(value_representation) == sizeof(uint64_t));

class scoped_gc_heap;
class gc_heap {
public:
    friend gc_heap_ptr_untyped;
    friend scoped_gc_heap;
    friend value_representation;
    template<typename> friend class gc_heap_ptr_untracked;

    static constexpr uint32_t slot_size = sizeof(uint64_t);
    static constexpr uint32_t bytes_to_slots(size_t bytes) { return static_cast<uint32_t>((bytes + slot_size - 1) / slot_size); }

    explicit gc_heap(uint32_t capacity);
    ~gc_heap();

    void debug_print(std::wostream& os) const;
    uint32_t calc_used() const;

    void garbage_collect();

    gc_heap_ptr_untyped allocate(size_t num_bytes);

    template<typename T, typename... Args>
    static gc_heap_ptr<T> construct(const gc_heap_ptr_untyped& p, Args&&... args);

    template<typename T, typename... Args>
    gc_heap_ptr<T> make(Args&&... args) {
        return construct<T>(allocate(sizeof(T)), std::forward<Args>(args)...);
    }

    static gc_heap& local_heap() {
        assert(local_heap_);
        return *local_heap_;
    }

private:
    static constexpr uint32_t unallocated_type_index = UINT32_MAX;
    static constexpr uint32_t gc_moved_type_index    = unallocated_type_index-1;

    struct slot_allocation_header {
        uint32_t size;
        uint32_t type;

        constexpr bool active() const {
            return type != unallocated_type_index && type != gc_moved_type_index;
        }

        const gc_type_info& type_info() const {
            assert(active() && type < gc_type_info::types_.size());
            return *gc_type_info::types_[type];
        }
    };

    union slot {
        uint64_t               representation;
        uint32_t               new_position; // Only valid during garbage collection
        slot_allocation_header allocation;
    };
    static_assert(sizeof(slot) == slot_size);

    thread_local static gc_heap* local_heap_;
    std::set<const gc_heap_ptr_untyped*> pointers_;
    slot* storage_;
    uint32_t capacity_;
    uint32_t next_free_ = 0;

    void attach(const gc_heap_ptr_untyped& p);
    void detach(const gc_heap_ptr_untyped& p);

    bool is_internal(const void* p) const {
        return reinterpret_cast<uintptr_t>(p) >= reinterpret_cast<uintptr_t>(storage_) && reinterpret_cast<uintptr_t>(p) < reinterpret_cast<uintptr_t>(storage_ + capacity_);
    }

    uint32_t gc_move(gc_heap& new_heap, uint32_t pos);

    template<typename T>
    gc_heap_ptr<T> unsafe_create_from_position(uint32_t pos);

    uint32_t unsafe_gc_move(gc_heap& new_heap, uint32_t pos);
};

class scoped_gc_heap : public gc_heap {
public:
    explicit scoped_gc_heap(uint32_t capacity) : gc_heap(capacity), old_heap_(gc_heap::local_heap_) {
        gc_heap::local_heap_ = this;
    }
    ~scoped_gc_heap() {
        assert(gc_heap::local_heap_ == this);
        gc_heap::local_heap_ = old_heap_;

        garbage_collect();
#ifndef  NDEBUG
        assert(calc_used() == 0);
#endif
    }
    scoped_gc_heap(scoped_gc_heap&) = delete;
    scoped_gc_heap& operator=(scoped_gc_heap&) = delete;
private:
    gc_heap* old_heap_;
};

class gc_heap_ptr_untyped {
public:
    friend gc_heap;
    template<typename> friend class gc_heap_ptr_untracked;
    friend value_representation;

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

private:
    gc_heap* heap_;
    uint32_t pos_;

    explicit gc_heap_ptr_untyped(gc_heap& heap, uint32_t pos) : heap_(&heap), pos_(pos) {
        heap_->attach(*this);
    }
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
    explicit gc_heap_ptr(const gc_heap_ptr_untyped& p) : gc_heap_ptr_untyped(p) {
    }
};

template<typename T>
class gc_heap_ptr_untracked {
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

    void fixup_after_move(gc_heap& new_heap, gc_heap& old_heap) {
        if (pos_) {
            pos_ = old_heap.unsafe_gc_move(new_heap, pos_);
        }
    }

private:
    uint32_t pos_;
};

template<typename T, typename... Args>
gc_heap_ptr<T> gc_heap::construct(const gc_heap_ptr_untyped& p, Args&&... args) {
    assert(p.heap_ && p.pos_ > 0 && p.pos_ < p.heap_->next_free_);
    auto& a = p.heap().storage_[p.pos_ - 1].allocation;
    assert(a.type == unallocated_type_index);
    gc_type_info_registration<T>::construct(p.get(), std::forward<Args>(args)...);
    a.type = gc_type_info_registration<T>::get().get_index();
    return gc_heap_ptr<T>{p};
}

template<typename T>
gc_heap_ptr<T> gc_heap::unsafe_create_from_position(uint32_t pos) {
    assert(pos > 0 && pos < next_free_ && gc_type_info_registration<T>::get().is_convertible(storage_[pos-1].allocation.type_info()));
    return gc_heap_ptr<T>{gc_heap_ptr_untyped{*this, pos}};
}

} // namespace mjs

#endif
