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
class gc_heap;
class gc_heap_ptr_untyped;
template<typename T>
class gc_heap_ptr;

class gc_type_info {
public:
    virtual const char* name() const = 0;
    virtual void destroy(void*) const = 0;
    virtual gc_heap_ptr_untyped move(gc_heap&, void*) const = 0;

    uint32_t get_index() {
        auto it = std::find(types_.begin(), types_.end(), this);
        if (it == types_.end()) {
            std::abort();
        }
        return static_cast<uint32_t>(it - types_.begin());
    }

protected:
    explicit gc_type_info() {
        types_.push_back(this);
    }

private:
    friend gc_heap;
    static std::vector<const gc_type_info*> types_;
};

template<typename T>
class gc_type_info_registration : public gc_type_info {
public:
    static gc_type_info_registration& get() {
        static gc_type_info_registration reg;
        return reg;
    }

    // Helper so gc_*** classes don't have to friend both gc_heap and gc_type_info_registration
    template<typename... Args>
    static void construct(void* p, Args&&... args) {
        new (p) T(std::forward<Args>(args)...);
    }

private:
    explicit gc_type_info_registration() {}

    const char* name() const override {
        return typeid(T).name();
    }

    void destroy(void* p) const override {
        static_cast<T*>(p)->~T();
    }

    gc_heap_ptr_untyped move(gc_heap& new_heap, void* p) const override;
};

class scoped_gc_heap;
class gc_heap {
public:
    friend gc_heap_ptr_untyped;
    friend scoped_gc_heap;

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

    template<typename T>
    gc_heap_ptr<T> unsafe_create_from_position(uint32_t pos);

    template<typename T>
    gc_heap_ptr<T> unsafe_create_from_pointer(T* ptr);

    uint32_t unsafe_gc_move(gc_heap& new_heap, uint32_t pos);

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
            assert(active());
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
    std::vector<slot> storage_;
    uint32_t next_free_ = 0;

    void attach(const gc_heap_ptr_untyped& p);
    void detach(const gc_heap_ptr_untyped& p);

    bool is_internal(const void* p) const {
        return reinterpret_cast<uintptr_t>(p) >= reinterpret_cast<uintptr_t>(storage_.data()) && reinterpret_cast<uintptr_t>(p) < reinterpret_cast<uintptr_t>(storage_.data() + storage_.size());
    }

    static slot_allocation_header& allocation_header(const gc_heap_ptr_untyped& p);

    uint32_t gc_move(gc_heap& new_heap, uint32_t pos);
};

class scoped_gc_heap : public gc_heap {
public:
    explicit scoped_gc_heap(uint32_t capacity) : gc_heap(capacity), old_heap_(gc_heap::local_heap_) {
        gc_heap::local_heap_ = this;
    }
    ~scoped_gc_heap() {
        assert(gc_heap::local_heap_ == this);
        gc_heap::local_heap_ = old_heap_;

#if 0 // TODO: This should be possible
#ifndef  NDEBUG
        garbage_collect();
        assert(calc_used() == 0);
#endif
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

    uint32_t unsafe_get_position() const {
        return pos_;
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
    gc_heap_ptr(std::nullptr_t) : gc_heap_ptr_untyped() {
    }
    template<typename U, typename = typename std::enable_if<std::is_convertible_v<U*, T*>>::type>
    gc_heap_ptr(const gc_heap_ptr<U>& p) : gc_heap_ptr_untyped(p) {
    }

    T* get() const { return static_cast<T*>(gc_heap_ptr_untyped::get()); }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

private:
    explicit gc_heap_ptr(const gc_heap_ptr_untyped& p) : gc_heap_ptr_untyped(p) {
    }
};

template<typename T>
gc_heap_ptr_untyped gc_type_info_registration<T>::move(gc_heap& new_heap, void* p) const {
    return static_cast<T*>(p)->move(new_heap);
}

template<typename T, typename... Args>
gc_heap_ptr<T> gc_heap::construct(const gc_heap_ptr_untyped& p, Args&&... args) {
    assert(allocation_header(p).type == unallocated_type_index);
    gc_type_info_registration<T>::construct(p.get(), std::forward<Args>(args)...);
    allocation_header(p).type = gc_type_info_registration<T>::get().get_index();
    return gc_heap_ptr<T>{p};
}

class object;

template<typename T>
gc_heap_ptr<T> gc_heap::unsafe_create_from_position(uint32_t pos) {
    assert(pos > 0 && pos < next_free_ && (storage_[pos-1].allocation.type == gc_type_info_registration<T>::get().get_index() || std::is_same_v<object, T>)); // Disable check for object (needed by object::default_value)
    return gc_heap_ptr<T>{gc_heap_ptr_untyped{*this, pos}};
}

template<typename T>
gc_heap_ptr<T> gc_heap::unsafe_create_from_pointer(T* ptr) {
    assert(is_internal(ptr));
    return unsafe_create_from_position<T>(static_cast<uint32_t>(reinterpret_cast<slot*>(ptr) - &storage_[0]));
}

} // namespace mjs

#endif
