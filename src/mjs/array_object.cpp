#include "array_object.h"
#include "native_object.h"
#include "function_object.h"
#include "error_object.h"
#include <sstream>
#include <cmath>

namespace mjs {

namespace {

uint32_t check_array_length(const global_object& g, const double n) {
    const auto l = to_uint32(n);
    if (g.language_version() >= version::es3 && n != l) {
        throw native_error_exception(native_error_type::range, g.stack_trace(), L"Invalid array length");
    }
    return l;
}

} // unnamed namespace

class array_object : public native_object {
public:
    friend gc_type_info_registration<array_object>;

    static gc_heap_ptr<array_object> make(const gc_heap_ptr<global_object>& global, const string& class_name, const object_ptr& prototype, uint32_t length) {
        return class_name.heap().make<array_object>(global, class_name, prototype, length);
    }

    static gc_heap_ptr<array_object> make(const gc_heap_ptr<global_object>& global, uint32_t length) {
        auto ap = global->array_prototype();
        return global.heap().make<array_object>(global, ap->class_name(), ap, length);
    }

    value get(const std::wstring_view& name) const override {
        const uint32_t index = index_value_from_string(name);
        if (index != invalid_index_value) {
            return get_at(index);
        } else {
            return native_object::get(name);
        }
    }

    void put(const string& name, const value& val, property_attribute attr) override {
        if (!can_put(name.view()) || do_native_put(name, val)) {
            return;
        }

        const uint32_t index = index_value_from_string(name.view());
        if (index != invalid_index_value) {
            if (!is_extensible() && !index_present(index)) {
                return;
            }
            if (index >= length_) {
                resize(index + 1);
            }
            unchecked_put(index, val);
        } else {
            object::put(name, val, attr);
        }
    }

    bool delete_property(const std::wstring_view& name) override {
        if (const uint32_t index = index_value_from_string(name); index != invalid_index_value) {
            if (index_present(index)) {
                present_mask_.dereference(heap())[index/64] &= ~(1ULL<<(index%64));
            }
            return true;
        }
        return native_object::delete_property(name);
    }

    void unchecked_put(uint32_t index, const value& val) {
        assert(index < length_);
        values_.dereference(heap())[index] = value_representation{val};
        present_mask_.dereference(heap())[index/64] |= 1ULL<<(index%64);
    }

private:
    gc_heap_ptr_untracked<global_object> global_;
    uint32_t length_;
    gc_heap_ptr_untracked<gc_vector<value_representation>> values_;
    gc_heap_ptr_untracked<gc_vector<property_attribute>> attributes_;
    gc_heap_ptr_untracked<gc_vector<uint64_t>> present_mask_;

    value get_length() const {
        return value{static_cast<double>(length_)};
    }

    void put_length(const value& v) {
        // ES3, 15.4.5.1
        resize(check_array_length(global_.dereference(heap()), to_number(v)));
    }

    bool index_present(uint32_t index) const {
        return index < length_ && (present_mask_.dereference(heap())[index/64]&(1ULL<<(index%64)));
    }

    value get_at(uint32_t index) const {
        auto& h = heap();
        return index_present(index) ? values_.dereference(h)[index].get_value(h) : value::undefined;
    }

    void resize(uint32_t len) {
        if (len == 0) {
            values_ = nullptr;
            attributes_ = nullptr;
            present_mask_ = nullptr;
        } else  {
            auto& h = heap();
            if (!values_) {
                values_ = gc_vector<value_representation>::make(h, len);
                attributes_ = gc_vector<property_attribute>::make(h, len);
                present_mask_ = gc_vector<uint64_t>::make(h, (len+63) / 64);
            }
            values_.dereference(h).resize(len);
            attributes_.dereference(h).resize(len);
            present_mask_.dereference(h).resize((len + 63) / 64);
        }
        length_ = len;
    }

    void fixup() {
        auto& h = heap();
        global_.fixup(h);
        values_.fixup(h);
        attributes_.fixup(h);
        present_mask_.fixup(h);
        native_object::fixup();
    }

    bool do_redefine_own_property(const string& name, const value& val, property_attribute attr) override {
        if (const uint32_t index = index_value_from_string(name.view()); index != invalid_index_value) {
            if (index_present(index)) {
                auto& h = heap();
                values_.dereference(h)[index] = value_representation{val};
                attributes_.dereference(h)[index] = attr;
            } else {
                if (!is_extensible()) {
                    return false;
                }
                put(name, val, attr);
            }
            return true;
        }
        return native_object::do_redefine_own_property(name, val, attr);
    }

    property_attribute do_own_property_attributes(const std::wstring_view& name) const override {
        if (const uint32_t index = index_value_from_string(name); index != invalid_index_value) {
            return index_present(index) ? attributes_.dereference(heap())[index] : property_attribute::invalid;
        }
        return native_object::do_own_property_attributes(name);
    }

    void add_own_property_names(std::vector<string>& names, bool check_enumerable) const override {
        if (length_) {
            auto& h = heap();
            const auto pm = present_mask_.dereference(h).data();
            const auto attrs = attributes_.dereference(h).data();
            for (uint32_t i = 0; i < length_; ++i) {
                if (pm[i/64] & (1ULL << (i%64))) {
                    if (!check_enumerable || !has_attributes(attrs[i], property_attribute::dont_enum)) {
                        names.push_back(string{h, index_string(i)});
                    }
                }
            }
        }
        native_object::add_own_property_names(names, check_enumerable);
    }

    void do_debug_print_extra(std::wostream& os, int indent_incr, int max_nest, int indent) const override {
        native_object::do_debug_print_extra(os, indent_incr, max_nest, indent);

        const auto indent_string = std::wstring(indent, ' ');
        for (uint32_t i = 0; i < length_; ++i) {
            if (index_present(i)) {
                os << indent_string << i << ": ";
                mjs::debug_print(os, get_at(i), indent_incr, 1, indent + indent_incr);
                os << "\n";
            }
        }
    }

    explicit array_object(const gc_heap_ptr<global_object>& global, const string& class_name, const object_ptr& prototype, uint32_t length) : native_object{class_name, prototype}, global_(global), length_(0) {
        DEFINE_NATIVE_PROPERTY(array_object, length);
        resize(length);
    }
};

namespace {

string array_to_locale_string(const gc_heap_ptr<global_object>& global_, const object_ptr& arr) {
    auto& h = arr.heap();
    const uint32_t len = to_uint32(arr->get(L"length"));
    std::wstring s;
    gc_heap_ptr<global_object> global = global_; // Keep local copy since due to use of call_function below (XXX)
    for (uint32_t i = 0; i < len; ++i) {
        if (i) s += L",";
        auto v = arr->get(index_string(i));
        if (v.type() != value_type::undefined && v.type() != value_type::null) {
            auto o = global->to_object(v);
            s += to_string(h, call_function(o->get(L"toLocaleString"), value{o}, {})).view();
        }
    }
    return string{h, s};
}

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

uint32_t calc_start_index(const value& v, const uint32_t l) {
    auto s = to_integer(v);
    if (s < 0) {
        return static_cast<uint32_t>(std::max(0., l + s));
    } else {
        return static_cast<uint32_t>(std::min(s, static_cast<double>(l)));
    }
}

value array_slice(const gc_heap_ptr<global_object>& global, const object_ptr& o, const std::vector<value>& args) {
    // ES3, 15.4.4.10
    const uint32_t l = to_uint32(o->get(L"length"));
    const uint32_t start = args.size() > 0 ? calc_start_index(args[0], l) : 0;
    uint32_t end = l;
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
    auto res = make_array(global, 0);
    auto& h = global.heap();
    for (uint32_t n = 0; start + n < end; ++n) {
        res->put(string{h, index_string(n)}, o->get(index_string(start+n)));
    }

    return value{res};
}

value array_splice(const gc_heap_ptr<global_object>& global, const object_ptr& o, const std::vector<value>& args) {
    const uint32_t num_args = static_cast<uint32_t>(args.size());

    auto res = make_array(global, 0);

    // Fewer than 2 arguments isn't specified until ES2015
    // No arugments: same as splice(0,0)
    // One argument: same as splice(start, len-start)
    if (num_args == 0) {
        return value{res};
    }

    const uint32_t l = to_uint32(o->get(L"length")); // Result(3)
    const uint32_t start = calc_start_index(args[0], l); // Result(5)
    const uint32_t delete_count = static_cast<uint32_t>(std::min(num_args < 2 ? static_cast<double>(l) : std::max(to_integer(args[1]), 0.0), 0.0+l-start)); // Result(6)
    
    auto& h = global.heap();
    for (uint32_t k = 0; k < delete_count; ++k) {
        const auto prop_name = index_string(start + k);
        if (o->has_property(prop_name)) {
            res->put(string{h, index_string(k)}, o->get(prop_name));
        }
    }
    res->put(global->common_string("length"), value{static_cast<double>(delete_count)});

    // step 17
    const uint32_t item_count = num_args > 2 ? num_args - 2 : 0;
    if (item_count < delete_count) {
        // Step 20-30
        for (uint32_t k = start; k < l - delete_count; ++k) {
            const auto from_name = index_string(k + delete_count);
            const auto to_name = index_string(k + item_count);
            if (o->has_property(from_name)) {
                o->put(string{h, to_name}, o->get(from_name));
            } else {
                o->delete_property(to_name);
            }
        }
        for (uint32_t k = l; k-- > l - delete_count + item_count;) {
            o->delete_property(index_string(k));
        }
    } else if (item_count > delete_count) {
        // Step 31-47
        for (uint32_t k = l - delete_count; k-- > start; ) {
            const auto from_name = index_string(k + delete_count);
            const auto to_name = index_string(k + item_count);
            if (o->has_property(from_name)) {
                o->put(string{h, to_name}, o->get(from_name));
            } else {
                o->delete_property(to_name);
            }
        }
    }

    // step 48
    for (uint32_t i = 0; i < item_count; ++i) {
        o->put(string{h, index_string(start + i)}, args[2+i]);
    }
    o->put(global->common_string("length"), value{static_cast<double>(l - delete_count + item_count)});
    return value{res};
}

double array_index_of(const gc_heap_ptr<global_object>& global, const value& this_, const std::vector<value>& args) {
    auto o = global->to_object(this_);
    const auto len = to_uint32(o->get(L"length"));
    if (!len) {
        return -1.0;
    }
    const auto search_element = args.size() > 0 ? args[0] : value::undefined;
    const auto n = args.size() > 1 ? to_integer(args[1]) : 0.;
    if (n >= len) {
        return -1.0;
    }
    auto k = n >= 0 ? static_cast<uint32_t>(n) : len - std::fabs(n) < 0 ? 0 : static_cast<uint32_t>(len - std::fabs(n));
    for (; k < len; ++k) {
        const auto is = index_string(k);
        if (o->has_property(is) && o->get(is) == search_element) {
            return static_cast<double>(k);
        }
    }
    return -1.0;
}

double array_last_index_of(const gc_heap_ptr<global_object>& global, const value& this_, const std::vector<value>& args) {
    auto o = global->to_object(this_);
    const auto len = to_uint32(o->get(L"length"));
    if (!len) {
        return -1.0;
    }
    const auto search_element = args.size() > 0 ? args[0] : value::undefined;
    const auto n = args.size() > 1 ? to_integer(args[1]) : len-1;
    auto k = n >= 0 ? std::min(n, len-1.) : len - std::fabs(n);
    for (; k >= 0; --k) {
        const auto is = index_string(static_cast<uint32_t>(k));
        if (o->has_property(is) && o->get(is) == search_element) {
            return k;
        }
    }
    return -1.0;
}

template<typename Init, typename Iter>
void for_each_helper(const gc_heap_ptr<global_object>& global, const value& this_, const std::vector<value>& args, const Init& init, const Iter& iter) {
    auto o = global->to_object(this_);
    const auto len = to_uint32(o->get(L"length"));
    const auto callback = !args.empty() ? args[0] : value::undefined;
    global->validate_type(callback, global->function_prototype(), "function");
    const auto this_arg = args.size() > 1 ? args[1] : value::undefined;

    init(len);
    for (uint32_t k = 0; k < len; ++k) {
        const auto is = index_string(static_cast<uint32_t>(k));
        if (o->has_property(is)) {
            auto kval = o->get(is);
            auto res = call_function(callback, this_arg, { kval, value{static_cast<double>(k)}, value{o} });
            if (!iter(k, res)) {
                break;
            }
        }
    }
}

bool array_every(const gc_heap_ptr<global_object>& global, const value& this_, const std::vector<value>& args) {
    bool every = true;
    for_each_helper(global, this_, args, [](uint32_t){}, [&every](uint32_t, const value& v) {
        if (!to_boolean(v)) {
            every = false;
            return false;
        }
        return true;
    });
    return every;
}


bool array_some(const gc_heap_ptr<global_object>& global, const value& this_, const std::vector<value>& args) {
    bool some = false;
    for_each_helper(global, this_, args, [](uint32_t){}, [&some](uint32_t, const value& v) {
        if (to_boolean(v)) {
            some = true;
            return false;
        }
        return true;
    });
    return some;
}

void array_for_each(const gc_heap_ptr<global_object>& global, const value& this_, const std::vector<value>& args) {
    for_each_helper(global, this_, args, [](uint32_t){}, [](uint32_t, const value&) { return true; });
}

object_ptr array_map(const gc_heap_ptr<global_object>& global, const value& this_, const std::vector<value>& args) {
    object_ptr a;
    for_each_helper(global, this_, args, [&a, global](uint32_t length) {
        a = make_array(global, length);
    }, [&h=global.heap(), &a](uint32_t k, const value& v) {
        a->put(string{h, index_string(k)}, v);
        return true;
    });
    return a;
}

} // unnamed namespace

global_object_create_result make_array_object(global_object& global) {
    auto Array_str_ = global.common_string("Array");
    auto prototype = array_object::make(global.self_ptr(), Array_str_, global.object_prototype(), 0);

    auto c = make_function(global, [global = global.self_ptr()](const value&, const std::vector<value>& args) {
        return value{make_array(global, args)};
    }, Array_str_.unsafe_raw_get(), 1);
    c->default_construct_function();

    const auto version = global.language_version();

    if (version < version::es3) {
        put_native_function(global, prototype, "toString", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
            global->validate_object(this_);
            return value{array_join(this_.object_value(), L",")};
        }, 0);
    } else {
        put_native_function(global, prototype, "toString", [version, global = global.self_ptr()](const value& this_, const std::vector<value>&) {
            if (version < version::es5) global->validate_type(this_, global->array_prototype(), "array");
            return value{array_join(this_.object_value(), L",")};
        }, 0);
        put_native_function(global, prototype, "toLocaleString", [version, global = global.self_ptr()](const value& this_, const std::vector<value>&) {
            if (version < version::es5) global->validate_type(this_, global->array_prototype(), "array");
            return value{array_to_locale_string(global, this_.object_value())};
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
        put_native_function(global, prototype, "splice", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            return array_splice(global, this_.object_value(), args);
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


    if (version >= version::es5) {
        put_native_function(global, prototype, "indexOf", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            return value{array_index_of(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "lastIndexOf", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            return value{array_last_index_of(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "every", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            return value{array_every(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "some", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            return value{array_some(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "forEach", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            array_for_each(global, this_, args);
            return value::undefined;
        }, 1);

        put_native_function(global, prototype, "map", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            return value{array_map(global, this_, args)};
        }, 1);
    }

    return { c, prototype };
}

object_ptr make_array(const gc_heap_ptr<global_object>& global, uint32_t length) {
    return array_object::make(global, length);
}

object_ptr make_array(const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    auto array_prototype = global->array_prototype();
    if (args.size() == 1 && args[0].type() == value_type::number) {
        return array_object::make(global, check_array_length(*global, args[0].number_value()));
    }
    auto arr = array_object::make(global, static_cast<uint32_t>(args.size()));
    for (uint32_t i = 0; i < args.size(); ++i) {
        arr->unchecked_put(i, args[i]);
    }
    return arr;
}

} // namespace mjs