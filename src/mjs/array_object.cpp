#include "array_object.h"
#include "native_object.h"
#include "function_object.h"
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
            return get_at(index);
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

    static string to_locale_string(const gc_heap_ptr<global_object>& global, const gc_heap_ptr<array_object>& arr);

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

    value get_at(uint32_t index) const {
        auto& h = heap();
        return index_present(index) ? values_.dereference(h)[index].get_value(h) : value::undefined;
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
        DEFINE_NATIVE_PROPERTY(array_object, length);
        resize(length);
    }
};

string array_object::to_locale_string(const gc_heap_ptr<global_object>& global, const gc_heap_ptr<array_object>& arr) {
    auto& h = arr.heap();
    const uint32_t len = arr->length_;
    std::wstring s;
    for (uint32_t i = 0; i < len; ++i) {
        if (i) s += L",";
        auto v = arr->get_at(i);
        if (v.type() != value_type::undefined && v.type() != value_type::null) {
            auto o = global->to_object(v);
            s += to_string(h, call_function(o->get(L"toLocaleString"), value{o}, {})).view();
        }
    }
    return string{h, s};
}

namespace {

string array_join(const object_ptr& o, const std::wstring_view& sep) {
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

value array_pop(const gc_heap_ptr<global_object>& global, const object_ptr& o) {
    const uint32_t l = to_uint32(o->get(L"length"));
    if (l == 0) {
        o->put(global->common_string("length"), value{0.});
        return value::undefined;
    }
    const auto idx_str = index_string(l - 1);
    auto res = o->get(idx_str);
    o->delete_property(idx_str);
    o->put(global->common_string("length"), value{static_cast<double>(l-1)});
    return res;
}

value array_push(const gc_heap_ptr<global_object>& global, const object_ptr& o, const std::vector<value>& args) {
    // FIXME: Overflow of n is possible
    auto& h = global.heap();
    uint32_t n = to_uint32(o->get(L"length"));
    for (const auto& a: args) {
        o->put(string{h, index_string(n++)}, a);
    }
    value l{static_cast<double>(n)};
    o->put(global->common_string("length"), l);
    return l;
}

value array_shift(const gc_heap_ptr<global_object>& global, const object_ptr& o) {
    auto& h = global.heap();
    const uint32_t l = to_uint32(o->get(L"length"));
    if (l == 0) {
        o->put(global->common_string("length"), value{0.});
        return value::undefined;
    }
    auto last_idx = string{h, index_string(0)};
    auto res = o->get(last_idx.view());
    for (uint32_t k = 1; k < l; ++k) {
        auto this_idx = string{h, index_string(k)};
        if (o->has_property(this_idx.view())) { // should this be hasOwnProperty?
            o->put(last_idx, o->get(this_idx.view()));
        } else {
            o->delete_property(last_idx.view());
        }
        last_idx = this_idx;
    }
    o->put(global->common_string("length"), value{static_cast<double>(l-1)});
    return res;
}

value array_unshift(const gc_heap_ptr<global_object>& global, const object_ptr& o, const std::vector<value>& args) {
    // ES3, 15.4.4.13
    auto& h = global.heap();
    const uint32_t l = to_uint32(o->get(L"length"));
    uint32_t k = l;
    const uint32_t num_args = static_cast<uint32_t>(args.size());
    for (; k; k--) {
        const auto last_idx = index_string(k-1);
        const auto this_idx = index_string(k+num_args-1);
        if (o->has_property(last_idx)) {
            o->put(string{h, this_idx}, o->get(last_idx));
        } else {
            o->delete_property(this_idx);
        }
    }
    k = 0;
    for (const auto& a: args) {
        o->put(string{h, index_string(k++)}, a);
    }
    value new_l{static_cast<double>(l + num_args)};
    o->put(global->common_string("length"), new_l);
    return new_l;
}

value array_slice(const gc_heap_ptr<global_object>& global, const object_ptr& o, const std::vector<value>& args) {
    // ES3, 15.4.4.10
    const uint32_t l = to_uint32(o->get(L"length"));
    uint32_t start = 0, end = l;
    if (args.size() > 0) {
        auto s = to_integer(args[0]);
        if (s < 0) {
            s = std::max(0., l + s);
        } else {
            s = std::min(s, static_cast<double>(l));
        }
        start = static_cast<uint32_t>(s);
    }
    if (args.size() > 1) {
        auto e = to_integer(args[1]);
        if (e < 0) {
            e = std::max(0., l + e);
        } else {
            e = std::min(e, static_cast<double>(l));
        }
        end = static_cast<uint32_t>(e);
    }

    // TODO: Could use unchecked_put since we know the length
    auto res = make_array(global->array_prototype(), {});
    auto& h = global.heap();
    for (uint32_t n = 0; start + n < end; ++n) {
        res->put(string{h, index_string(n)}, o->get(index_string(start+n)));
    }

    return value{res};
}

} // unnamed namespace

create_result make_array_object(global_object& global) {
    auto Array_str_ = global.common_string("Array");
    auto prototype = array_object::make(Array_str_, global.object_prototype(), 0);

    auto c = make_function(global, [prototype](const value&, const std::vector<value>& args) {
        return value{make_array(prototype, args)};
    }, Array_str_.unsafe_raw_get(), 1);
    c->default_construct_function();

    const auto version = global.language_version();

    if (version < version::es3) {
        put_native_function(global, prototype, "toString", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
            global->validate_object(this_);
            return value{array_join(this_.object_value(), L",")};
        }, 0);
    } else {
        put_native_function(global, prototype, "toString", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
            global->validate_type(this_, global->array_prototype(), "array");
            return value{array_join(this_.object_value(), L",")};
        }, 0);
        put_native_function(global, prototype, "toLocaleString", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
            global->validate_type(this_, global->array_prototype(), "array");
            return value{array_object::to_locale_string(global, static_cast<gc_heap_ptr<array_object>>(this_.object_value()))};
        }, 0);
        put_native_function(global, prototype, "pop", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
            global->validate_object(this_);
            return array_pop(global, this_.object_value());
        }, 0);
        put_native_function(global, prototype, "push", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            return array_push(global, this_.object_value(), args);
        }, 1);
        put_native_function(global, prototype, "shift", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
            global->validate_object(this_);
            return array_shift(global, this_.object_value());
        }, 0);
        put_native_function(global, prototype, "unshift", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            return array_unshift(global, this_.object_value(), args);
        }, 1);
        put_native_function(global, prototype, "slice", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            return array_slice(global, this_.object_value(), args);
        }, 2);
    }
    put_native_function(global, prototype, "join", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
        global->validate_object(this_);
        auto& h = global.heap();
        return value{array_join(this_.object_value(), !args.empty() ? to_string(h, args.front()).view() : std::wstring_view{L","})};
    }, 1);
    put_native_function(global, prototype, "reverse", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
        global->validate_object(this_);
        const auto& o = this_.object_value();
        const uint32_t length = to_uint32(o->get(L"length"));
        auto& h = global.heap();
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
    put_native_function(global, prototype, "sort", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
        global->validate_object(this_);
        const auto& o = *this_.object_value();
        auto& h = o.heap(); // Capture heap reference (which continues to be valid even after GC) since `this` can move when calling a user-defined compare function (since this can cause GC)
        const uint32_t length = to_uint32(o.get(L"length"));

        value comparefn = !args.empty() ? args.front() : value::undefined;

        auto sort_compare = [comparefn, &h](const value& x, const value& y) {
            if (x.type() == value_type::undefined && y.type() == value_type::undefined) {
                return 0;
            } else if (x.type() == value_type::undefined) {
                return 1;
            } else if (y.type() == value_type::undefined) {
                return -1;
            }
            if (comparefn.type() != value_type::undefined) {
                const auto r = to_number(call_function(comparefn, value::null, {x,y}));
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