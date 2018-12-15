#include "array_object.h"
#include "native_object.h"
#include <sstream>

namespace mjs {

class array_object : public native_object {
public:
    friend gc_type_info_registration<array_object>;

    static gc_heap_ptr<array_object> make(const string& class_name, const object_ptr& prototype, uint32_t length) {
        return class_name.heap().make<array_object>(class_name, prototype, length);
    }

    value get(const std::wstring_view& name) const override {
        const uint32_t index = get_array_index(name);
        if (index != invalid_array_index) {
            auto& h = heap();
            return index_present(index) ? values_.dereference(h)[index].get_value(h) : value::undefined;
        } else {
            return native_object::get(name);
        }
    }

    void put(const string& name, const value& val, property_attribute attr) override {
        if (!can_put(name.view()) || do_put(name, val)) {
            return;
        }

        const uint32_t index = get_array_index(name.view());
        if (index != invalid_array_index) {
            if (index >= length_) {
                resize(index + 1);
            }
            unchecked_put(index, val);
        } else {
            object::put(name, val, attr);
        }
    }

    bool can_put(const std::wstring_view& name) const override {
        if (const uint32_t index = get_array_index(name); index != invalid_array_index) {
            return true;
        }
        return native_object::can_put(name);
    }

    bool has_property(const std::wstring_view& name) const override {
        if (const uint32_t index = get_array_index(name); index != invalid_array_index) {
            return index_present(index);
        }
        return native_object::has_property(name);
    }

    bool delete_property(const std::wstring_view& name) override {
        if (const uint32_t index = get_array_index(name); index != invalid_array_index) {
            if (index_present(index)) {
                present_mask_.dereference(heap())[index/64] &= ~(1ULL<<(index%64));
            }
            return true;
        }
        return native_object::delete_property(name);
    }

    bool check_own_property_attribute(const std::wstring_view& name, property_attribute mask, property_attribute expected) const override {
        if (const uint32_t index = get_array_index(name); index != invalid_array_index && index_present(index)) {
            return expected == property_attribute::none;
        }
        return native_object::check_own_property_attribute(name, mask, expected);
    }

    void unchecked_put(uint32_t index, const value& val) {
        assert(index < length_);
        values_.dereference(heap())[index] = value_representation{val};
        present_mask_.dereference(heap())[index/64] |= 1ULL<<(index%64);
    }

private:
    uint32_t length_;
    gc_heap_ptr_untracked<gc_vector<value_representation>> values_;
    gc_heap_ptr_untracked<gc_vector<uint64_t>> present_mask_;

    value get_length() const {
        return value{static_cast<double>(length_)};
    }

    void put_length(const value& v) {
        resize(to_uint32(v));
    }

    static constexpr uint32_t invalid_array_index = UINT32_MAX;

    bool index_present(uint32_t index) const {
        return index < length_ && (present_mask_.dereference(heap())[index/64]&(1ULL<<(index%64)));
    }

    uint32_t get_array_index(const std::wstring_view& name) const {
        // TODO: Optimize this
        const auto len = name.length();
        if (len == 0 || len > 10) {
            assert(len); // Shouldn't be passed the empty string
            return invalid_array_index;
        }
        uint32_t index = 0;
        for (uint32_t i = 0; i < len; ++i) {
            const auto ch = name[i];
            if (ch < L'0' || ch > L'9') {
                return invalid_array_index;
            }
            const auto last = index;
            index = index*10 + (ch - L'0');
            if (index < last) {
                // Overflow
                return invalid_array_index;
            }
        }
        return index;
    }

    void resize(uint32_t len) {
        if (len == 0) {
            values_ = nullptr;
            present_mask_ = nullptr;
        } else  {
            if (!values_) {
                values_ = gc_vector<value_representation>::make(heap(), len);
                present_mask_ = gc_vector<uint64_t>::make(heap(), (len+63) / 64);
            }
            values_.dereference(heap()).resize(len);
            present_mask_.dereference(heap()).resize((len + 63) / 64);
        }
        length_ = len;
    }

    void fixup() {
        values_.fixup(heap());
        present_mask_.fixup(heap());
        native_object::fixup();
    }

    void add_property_names(std::vector<string>& names) const override {
        native_object::add_property_names(names);
        if (length_) {
            auto& h = heap();
            const auto pm = present_mask_.dereference(h).data();
            for (uint32_t i = 0; i < length_; ++i) {
                if (pm[i/64] & (1ULL << (i%64))) {
                    names.push_back(string{h, index_string(i)});
                }
            }
        }
    }

    explicit array_object(const string& class_name, const object_ptr& prototype, uint32_t length) : native_object{class_name, prototype}, length_(0) {
        add_native_property<array_object, &array_object::get_length, &array_object::put_length>("length", property_attribute::dont_enum | property_attribute::dont_delete);
        resize(length);
    }
};

string join(const object_ptr& o, const std::wstring_view& sep) {
    auto& h = o.heap();
    const uint32_t l = to_uint32(o->get(L"length"));
    std::wstring s;
    for (uint32_t i = 0; i < l; ++i) {
        if (i) s += sep;
        const auto& oi = o->get(index_string(i));
        if (oi.type() != value_type::undefined && oi.type() != value_type::null) {
            s += to_string(h, oi).view();
        }
    }
    return string{h, s};
}

create_result make_array_object(global_object& global) {
    auto& h = global.heap();
    auto Array_str_ = global.common_string("Array");
    auto prototype = array_object::make(Array_str_, global.object_prototype(), 0);

    auto c = global.make_function([prototype](const value&, const std::vector<value>& args) {
        return value{make_array(prototype, args)};
    }, global.native_function_body(Array_str_), 1);
    global.make_constructable(c);

    global.put_native_function(prototype, "toString", [](const value& this_, const std::vector<value>&) {
        assert(this_.type() == value_type::object);
        return value{join(this_.object_value(), L",")};
    }, 0);
    global.put_native_function(prototype, "join", [&h](const value& this_, const std::vector<value>& args) {
        assert(this_.type() == value_type::object);
        string sep{h, L","};
        if (!args.empty()) {
            sep = to_string(h, args.front());
        }
        return value{join(this_.object_value(), sep.view())};
    }, 1);
    global.put_native_function(prototype, "reverse", [&h](const value& this_, const std::vector<value>&) {
        assert(this_.type() == value_type::object);
        const auto& o = this_.object_value();
        const uint32_t length = to_uint32(o->get(L"length"));
        for (uint32_t k = 0; k != length / 2; ++k) {
            const auto i1 = index_string(k);
            const auto i2 = index_string(length - k - 1);
            auto v1 = o->get(i1);
            auto v2 = o->get(i2);
            o->put(string{h, i1}, v2);
            o->put(string{h, i2}, v1);
        }
        return this_;
    }, 0);
    global.put_native_function(prototype, "sort", [](const value& this_, const std::vector<value>& args) {
        assert(this_.type() == value_type::object);
        const auto& o = *this_.object_value();
        auto& h = o.heap(); // Capture heap reference (which continues to be valid even after GC) since `this` can move when calling a user-defined compare function (since this can cause GC)
        const uint32_t length = to_uint32(o.get(L"length"));

        native_function_type comparefn{};
        if (!args.empty()) {
            if (args.front().type() == value_type::object) {
                comparefn = args.front().object_value()->call_function();
            }
            if (!comparefn) {
                NOT_IMPLEMENTED("Invalid compare function given to sort");
            }
        }

        auto sort_compare = [comparefn, &h](const value& x, const value& y) {
            if (x.type() == value_type::undefined && y.type() == value_type::undefined) {
                return 0;
            } else if (x.type() == value_type::undefined) {
                return 1;
            } else if (y.type() == value_type::undefined) {
                return -1;
            }
            if (comparefn) {
                const auto r = to_number(comparefn->call(value::null, {x,y}));
                if (r < 0) return -1;
                if (r > 0) return 1;
                return 0;
            } else {
                const auto xs = to_string(h, x);
                const auto ys = to_string(h, y);
                if (xs.view() < ys.view()) return -1;
                if (xs.view() > ys.view()) return 1;
                return 0;
            }
        };

        std::vector<value> values(length);
        for (uint32_t i = 0; i < length; ++i) {
            values[i] = o.get(index_string(i));
        }
        std::stable_sort(values.begin(), values.end(), [&](const value& x, const value& y) {
            return sort_compare(x, y) < 0;
        });
        // Note: `this` has possibly become invalid here (!)
        auto& o_after = *this_.object_value(); // Recapture object reference (since it could also have moved)
        for (uint32_t i = 0; i < length; ++i) {
            o_after.put(string{h, index_string(i)}, values[i]);
        }
        return this_;
    }, 1);
    return { c, prototype };
}

object_ptr make_array(const object_ptr& array_prototype, const std::vector<value>& args) {
    if (args.size() == 1 && args[0].type() == value_type::number) {
        return array_object::make(array_prototype->class_name(), array_prototype, to_uint32(args[0].number_value()));
    }
    auto arr = array_object::make(array_prototype->class_name(), array_prototype, static_cast<uint32_t>(args.size()));
    for (uint32_t i = 0; i < args.size(); ++i) {
        arr->unchecked_put(i, args[i]);
    }
    return arr;
}

} // namespace mjs