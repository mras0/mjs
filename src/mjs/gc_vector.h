#ifndef MJS_GC_VECTOR_H
#define MJS_GC_VECTOR_H

#include "gc_heap.h"

namespace mjs {

template<typename T>
class gc_vector {
public:
    static gc_heap_ptr<gc_vector> make(gc_heap& h, uint32_t initial_capacity) {
        return h.make<gc_vector>(h, initial_capacity);
    }

    gc_heap& heap() { return table_.heap(); }

    uint32_t length() const { return tab().length(); }
    uint32_t capacity() const { return tab().capacity(); }

    T* data() const {
        return tab().entries();
    }

    T& operator[](uint32_t index) {
        assert(index < tab().length());
        return tab().entries()[index];
    }

    T& front() {
        assert(length());
        return *data();
    }

    T& back() {
        return (*this)[length()-1];
    }

    void push_back(const T& e) {
        ensure_capacity();
        tab().push_back(e);
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        ensure_capacity();
        tab().emplace_back(std::forward<Args>(args)...);
    }

    void pop_back() {
        tab().pop_back();
    }

    void erase(uint32_t index) {
        tab().erase(index);
    }

    void erase(T* elem) {
        assert(elem >= begin() && elem < end());
        erase(static_cast<uint32_t>(elem - begin()));
    }

    T* begin() const { return data(); }
    T* end() const { return data() + length(); }

private:
    friend gc_type_info_registration<gc_vector>;

    void ensure_capacity() {
        if (tab().length() == tab().capacity()) {
            table_ = tab().copy_with_increased_capacity();
        }
    }

    // Could make this class generally available (again)
    class alignas(T) gc_table {
    public:
        gc_heap& heap() const { return const_cast<gc_heap&>(heap_); }
        uint32_t capacity() const { return capacity_; }
        uint32_t length() const { return length_; }

        template<typename... Args>
        static gc_heap_ptr<gc_table> make(gc_heap& h, uint32_t capacity, Args&&... args) {
            assert(capacity);
            return h.allocate_and_construct<gc_table>(sizeof(gc_table) + capacity * sizeof(T), h, capacity, std::forward<Args>(args)...);
        }

        [[nodiscard]] gc_heap_ptr<gc_table> copy_with_increased_capacity() const {
            auto nt = make(heap(), capacity() * 2);
            nt->length(length());
            // Since it's the same heap the representation can just be copied
            std::memcpy(nt->entries(), entries(), length() * sizeof(T));
            return nt;
        }

        void length(uint32_t new_length) {
            assert(new_length <= capacity_);
            length_ = new_length;
        }

        T* entries() const {
            return reinterpret_cast<T*>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) + sizeof(*this));
        }

        void push_back(const T& e) {
            assert(length() < capacity());
            new (&entries()[length_++]) T(e);
        }

        template<typename... Args>
        void emplace_back(Args&&... args) {
            assert(length() < capacity());
            new (&entries()[length_++]) T(std::forward<Args>(args)...);
        }

        void erase(uint32_t index) {
            assert(index < capacity());
            std::memmove(&entries()[index], &entries()[index+1], sizeof(T) * (length() - 1 - index));
            --length_;
        }

        void pop_back() {
            assert(length_);
            --length_;
        }
    private:
        friend gc_type_info_registration<gc_table>;

        explicit gc_table(gc_heap& heap, uint32_t capacity) : heap_(heap), capacity_(capacity) {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(std::is_trivially_destructible_v<T>);
            static_assert(gc_type_info_registration<gc_table>::needs_fixup == has_fixup_t<T>::value);
        }

        gc_table(gc_table&& other) noexcept : heap_(other.heap_), capacity_(other.capacity_), length_(other.length_) {
            std::memcpy(entries(), other.entries(), length_ * sizeof(T));
        }

        template<typename U, typename=void>
        struct has_fixup_t : std::false_type{};

        template<typename U>
        struct has_fixup_t<U, std::void_t<decltype(std::declval<U>().fixup(std::declval<gc_heap&>()))>> : std::true_type{};

        template<typename U = T>
        std::enable_if_t<has_fixup_t<U>::value> fixup() {
            auto e = entries();
            for (uint32_t i = 0; i < length_; ++i) {
                e[i].fixup(heap_);
            }
        }

        gc_heap& heap_;
        uint32_t capacity_;
        uint32_t length_ = 0;
    };
    gc_heap& heap_;
    gc_heap_ptr_untracked<gc_table> table_;
    static_assert(!gc_type_info_registration<gc_table>::needs_destroy);

    gc_table& tab() const { return table_.dereference(heap_); }

    void fixup() {
        table_.fixup(heap_);
    }

    explicit gc_vector(gc_heap& h, uint32_t initial_capacity) : heap_(h), table_(gc_table::make(h, initial_capacity)) {
        static_assert(gc_type_info_registration<gc_vector>::needs_fixup && !gc_type_info_registration<gc_vector>::needs_destroy);
        assert(initial_capacity);
    }
};

} // namespace mjs

#endif