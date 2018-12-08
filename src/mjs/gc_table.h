#ifndef MJS_GC_TABLE_H
#define MJS_GC_TABLE_H

#include "gc_heap.h"
#include "gc_array.h"
#include "value.h"
#include "value_representation.h"
#include "property_attribute.h"

namespace mjs {


struct gc_table_entry_representation {
    gc_heap_ptr_untracked<gc_string> key;
    property_attribute               attributes;
    value_representation             value;
};

class gc_table : public gc_array_base<gc_table, gc_table_entry_representation> {
public:
    using gc_array_base::make;
    using gc_array_base::copy_with_increased_capacity;

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
            return e().key.track(tab_->heap());
        }

        std::wstring_view key_view() const {
            return e().key.dereference(tab_->heap()).view();
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
            return e().value.get_value(tab_->heap());
        }

        bool has_attribute(property_attribute a) const {
            return (property_attributes() & a) == a;
        }

        void erase() {
            assert(tab_ && index_ < tab_->length());
            tab_->erase(index_);
        }

    private:
        gc_table_entry_representation& e() const {
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
        assert(&raw_key.heap() == &heap());
        assert(find(key.view()) == end());
        push_back(gc_table_entry_representation{
            raw_key,
            attr,
            value_representation{v}
        });
    }

    entry find(const std::wstring_view& key) {
        auto it = begin();
        for (auto e = end(); it != e; ++it) {
            if (it.key_view() == key) {
                break;
            }
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

    using gc_array_base::gc_array_base;

    void fixup();
};

} // namespace mjs
#endif
