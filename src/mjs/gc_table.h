#ifndef MJS_GC_TABLE_H
#define MJS_GC_TABLE_H

#include "gc_heap.h"
#include "value.h"
#include "value_representation.h"
#include "property_attribute.h"

namespace mjs {

class alignas(uint64_t) gc_table {
private:
    struct entry_representation {
        gc_heap_ptr_untracked<gc_string> key;
        property_attribute               attributes;
        value_representation             value;
    };
public:
    static gc_heap_ptr<gc_table> make(gc_heap& h, uint32_t capacity) {
        assert(capacity > 0);
        return h.allocate_and_construct<gc_table>(sizeof(gc_table) + capacity * sizeof(entry_representation), h, capacity);
    }

    uint32_t capacity() const { return capacity_; }
    uint32_t length() const { return length_; }

    [[nodiscard]] gc_heap_ptr<gc_table> copy_with_increased_capacity() const {
        auto nt = make(heap_, capacity() * 2);
        nt->length_ = length();
        // Since it's the same heap the representation can just be copied
        std::memcpy(nt->entries(), entries(), length() * sizeof(entry_representation));
        return nt;
    }

    class entry {
    public:
        friend gc_table;

        bool operator==(const entry& e) const { assert(tab_ == e.tab_); return index_ == e.index_; }
        bool operator!=(const entry& e) const { return !(*this == e); }

        entry& operator++() {
            assert(tab_ && index_ < tab_->length());
            ++index_;
            return *this;
        }

        gc_heap_ptr<gc_string> key() const {
            assert(tab_);
            return e().key.track(tab_->heap_);
        }

        std::wstring_view key_view() const {
            return e().key.dereference(tab_->heap_).view();
        }

        property_attribute property_attributes() const {
            return e().attributes;
        }

        void value(const value& val) {
            assert(tab_);
            e().value = value_representation{val};
        }

        mjs::value value() const {
            assert(tab_);
            return e().value.get_value(tab_->heap_);
        }

        bool has_attribute(property_attribute a) const {
            return (property_attributes() & a) == a;
        }

        void erase() {
            assert(tab_ && index_ < tab_->length());
            std::memmove(&tab_->entries()[index_], &tab_->entries()[index_+1], sizeof(entry_representation) * (tab_->length() - 1 - index_));
            --tab_->length_;
        }

    private:
        entry_representation& e() const {
            assert(tab_ && index_ < tab_->length());
            return tab_->entries()[index_];
        }

        gc_table* tab_;
        uint32_t index_;
        explicit entry(gc_table& tab, uint32_t index) : tab_(&tab), index_(index) {
            assert(tab_ && index_ <= tab_->length());
        }
    };

    void insert(const string& key, const value& v, property_attribute attr) {
        auto& raw_key = key.unsafe_raw_get();
        assert(&raw_key.heap() == &heap_);
        assert(length() < capacity());
        assert(find(key.view()) == end());
        entries()[length_++] = entry_representation{
            raw_key,
            attr,
            value_representation{v}
        };
    }

    entry find(const std::wstring_view& key) {
        auto it = begin(); 
        while (it != end()) {
            if (it.key_view() == key) {
                break;
            }
            ++it;
        }
        return it;
    }

    entry find(const string& key) {
        return find(key.view());
    }

    entry begin() {
        return entry{*this, 0};
    }

    entry end() {
        return entry{*this, length()};
    }

private:
    friend gc_type_info_registration<gc_table>;

    gc_heap& heap_;     // TODO: Deduce somehow?
    uint32_t capacity_; // TODO: Get from allocation header
    uint32_t length_;

    entry_representation* entries() const {
        return reinterpret_cast<entry_representation*>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) + sizeof(*this));
    }

    explicit gc_table(gc_heap& h, uint32_t capacity) : heap_(h), capacity_(capacity), length_(0) {
    }

    gc_table(gc_table&& from);

    void fixup();
};

} // namespace mjs
#endif
