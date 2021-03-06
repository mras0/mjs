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

    static constexpr uint32_t max_normal_size = 1<<16;  // Maximum size before to grow to, indices beyond are handled as normal object properties

    static gc_heap_ptr<array_object> make(const gc_heap_ptr<global_object>& global, const string& class_name, const object_ptr& prototype, uint32_t length) {
        return class_name.heap().make<array_object>(global, class_name, prototype, length);
    }

    static gc_heap_ptr<array_object> make(const gc_heap_ptr<global_object>& global, uint32_t length) {
        auto ap = global->array_prototype();
        return global.heap().make<array_object>(global, ap->class_name(), ap, length);
    }

    void put(const string& name, const value& val, property_attribute attr) override {
        if (!can_put(name.view()) || do_native_put(name, val)) {
            return;
        }

        const uint32_t index = index_value_from_string(name.view());
        if (index != invalid_index_value) {
            if (!is_extensible() && is_valid(own_property_attributes(name.view()))) {
                return;
            }
            if (index >= length_) {
                length_ = index + 1;
            }
        }

        object::put(name, val, attr);
    }

private:
    gc_heap_ptr_untracked<global_object> global_;
    uint32_t length_;

    value get_length() const {
        return value{static_cast<double>(length_)};
    }

    void put_length(const value& v) {
        // ES3, 15.4.5.1
        const auto old_length = length_;
        length_ = check_array_length(global_.dereference(heap()), to_number(v));
        for (auto l = length_; l < old_length; ++l) {
            delete_property(index_string(l));
        }
    }

    void fixup() {
        auto& h = heap();
        global_.fixup(h);
        native_object::fixup();
    }

    explicit array_object(const gc_heap_ptr<global_object>& global, const string& class_name, const object_ptr& prototype, uint32_t length) : native_object{class_name, prototype}, global_(global), length_(length) {
        DEFINE_NATIVE_PROPERTY(array_object, length);
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

value array_concat(gc_heap_ptr<global_object> global, const value& this_, const std::vector<value>& args) {
    auto& h = global.heap();
    auto a = make_array(global, 0);
    uint32_t n = 0;

    auto add_value = [&](const value& e) {
        a->put(string{h, index_string(n++)}, e);
    };

    auto add_object = [&](const object_ptr& e) {
        if (is_array(e)) {
            const uint32_t l = to_uint32(e->get(L"length"));
            for (uint32_t k = 0; k < l; ++k) {
                const auto is = index_string(k);
                if (e->has_property(is)) {
                    add_value(e->get(is));
                }
            }
        } else {
            add_value(value{e});
        }
    };

    add_object(global->to_object(this_));

    for (const auto& item: args) {
        if (item.type() == value_type::object) {
            add_object(item.object_value());
        } else {
            add_value(item);
        }
    }

    return value{a};
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

    // TODO: Could use put_at since we know the length
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
    if (search_element.type() == value_type::number && std::isnan(search_element.number_value())) {
        // NaN never matches
        return -1.0;
    }
    const auto n = args.size() > 1 ? to_integer(args[1]) : 0.;
    if (n >= len) {
        return -1.0;
    }
    auto k = n >= 0 ? static_cast<uint32_t>(n) : len - std::fabs(n) < 0 ? 0 : static_cast<uint32_t>(len - std::fabs(n));

    if (len < array_object::max_normal_size) {
        for (; k < len; ++k) {
            const auto is = index_string(k);
            if (o->has_property(is) && o->get(is) == search_element) {
                return static_cast<double>(k);
            }
        }
    } else {
        // For large/sparse arrays only look at the actual properties
        auto pn = o->enumerable_property_names();
        for (const auto& name : pn) {
            const auto kk = index_value_from_string(name.view());
            if (kk != invalid_index_value && kk >= k && o->get(name.view()) == search_element) {
                return static_cast<double>(kk);
            }
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
    if (search_element.type() == value_type::number && std::isnan(search_element.number_value())) {
        // NaN never matches
        return -1.0;
    }
    const auto n = args.size() > 1 ? to_integer(args[1]) : len-1;
    auto k = n >= 0 ? std::min(n, len-1.) : len - std::fabs(n);
    if (len < array_object::max_normal_size) {
        for (; k >= 0; --k) {
            const auto is = index_string(static_cast<uint32_t>(k));
            if (o->has_property(is) && o->get(is) == search_element) {
                return k;
            }
        }
    } else {
        // For large/sparse arrays only look at the actual properties
        auto pn = o->enumerable_property_names();
        std::reverse(pn.begin(), pn.end());
        for (const auto& name : pn) {
            const auto kk = index_value_from_string(name.view());
            if (kk <= k && o->get(name.view()) == search_element) {
                return static_cast<double>(kk);
            }
        }
    }
    return -1.0;
}

template<typename Init, typename Iter>
void for_each_helper(gc_heap_ptr<global_object> global, const value& this_, const std::vector<value>& args, const Init& init, const Iter& iter) {
    auto o = global->to_object(this_);
    const auto len = to_uint32(o->get(L"length"));
    const auto callback = !args.empty() ? args[0] : value::undefined;
    global->validate_type(callback, global->function_prototype(), "function");
    const auto this_arg = args.size() > 1 ? args[1] : value::undefined;

    init(len);
    if (len < array_object::max_normal_size) {
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
    } else {
        // For large/sparse array, don't run the loop a billion times...
        // This is a large win for the es5 conformance suite, but might not
        // be appropriate otherwise.
        auto pn = o->enumerable_property_names();
        for (const auto& n : pn) {
            const auto k = index_value_from_string(n.view());
            if (k == invalid_index_value) {
                continue;
            }
            auto kval = o->get(n.view());
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
    }, [&a](uint32_t k, const value& v) {
        a->put(string{a.heap(), index_string(k)}, v);
        return true;
    });
    return a;
}

object_ptr array_filter(const gc_heap_ptr<global_object>& global, const value& this_, const std::vector<value>& args) {
    object_ptr a = make_array(global, 0);
    uint32_t len = 0;
    for_each_helper(global, this_, args, [](uint32_t) {}, [&this_, &a, &len](uint32_t k, const value& v) {
        if (to_boolean(v)) {
            a->put(string{a.heap(), index_string(len++)}, this_.object_value()->get(index_string(k)));
        }
        return true;
    });
    return a;
}

value array_reduce(gc_heap_ptr<global_object> global, const value& this_, const std::vector<value>& args, bool reduce_right) {
    auto o = global->to_object(this_);
    const auto len = to_uint32(o->get(L"length"));
    const auto callback = !args.empty() ? args[0] : value::undefined;
    global->validate_type(callback, global->function_prototype(), "function");

    value accumulator = value::undefined;
    // i always counts from 0 to len-1 (inclusive)
    uint32_t i = 0;
    // k() is the index to use (same as i if left reducing)
    auto k = [&i, len, reduce_right]() { return reduce_right ? len - 1 - i : i; };

    if (args.size() > 1) {
        // 7. initialValue is present
        accumulator = args[1];
    } else {
        // 6. len is 0 and initialValue is not present
        // 8. initialValue is not present
        for (;;) {
            if (i == len) {
                throw native_error_exception{native_error_type::type, global->stack_trace(), "cannot reduce empty array"};
            }
            const auto is = index_string(k());
            ++i; // increase i after having called k() for this loop but before breaking (since the value was consumed)
            if (o->has_property(is)) {
                accumulator = o->get(is);
                break;
            }
        }
    }

    for (; i < len; ++i) {
        const auto is = index_string(k());
        if (o->has_property(is)) {
            accumulator = call_function(callback, value::undefined, { accumulator, o->get(is), value{static_cast<double>(k())}, value{o} });
        }
    }

    return accumulator;
}

} // unnamed namespace

global_object_create_result make_array_object(const gc_heap_ptr<global_object>& global) {
    auto Array_str_ = global->common_string("Array");
    auto prototype = array_object::make(global, Array_str_, global->object_prototype(), 0);

    auto c = make_function(global, [global](const value&, const std::vector<value>& args) {
        return value{make_array(global, args)};
    }, Array_str_.unsafe_raw_get(), 1);
    c->default_construct_function();

    prototype->put(global->common_string("constructor"), value{c}, global_object::default_attributes);

    const auto version = global->language_version();

    if (version < version::es3) {
        put_native_function(global, prototype, "toString", [global = global](const value& this_, const std::vector<value>&) {
            global->validate_object(this_);
            return value{array_join(this_.object_value(), L",")};
        }, 0);
    } else {
        put_native_function(global, prototype, "toString", [version, global = global](const value& this_, const std::vector<value>&) {
            if (version < version::es5) global->validate_type(this_, global->array_prototype(), "array");
            return value{array_join(this_.object_value(), L",")};
        }, 0);
        put_native_function(global, prototype, "toLocaleString", [version, global = global](const value& this_, const std::vector<value>&) {
            if (version < version::es5) global->validate_type(this_, global->array_prototype(), "array");
            return value{array_to_locale_string(global, this_.object_value())};
        }, 0);
        put_native_function(global, prototype, "concat", [global](const value& this_, const std::vector<value>& args) {
            return array_concat(global, this_, args);
        }, 1);
        put_native_function(global, prototype, "pop", [global](const value& this_, const std::vector<value>&) {
            global->validate_object(this_);
            return array_pop(global, this_.object_value());
        }, 0);
        put_native_function(global, prototype, "push", [global](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            return array_push(global, this_.object_value(), args);
        }, 1);
        put_native_function(global, prototype, "shift", [global](const value& this_, const std::vector<value>&) {
            global->validate_object(this_);
            return array_shift(global, this_.object_value());
        }, 0);
        put_native_function(global, prototype, "unshift", [global](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            return array_unshift(global, this_.object_value(), args);
        }, 1);
        put_native_function(global, prototype, "slice", [global](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            return array_slice(global, this_.object_value(), args);
        }, 2);
        put_native_function(global, prototype, "splice", [global](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            return array_splice(global, this_.object_value(), args);
        }, 2);
    }
    put_native_function(global, prototype, "join", [global](const value& this_, const std::vector<value>& args) {
        global->validate_object(this_);
        auto& h = global.heap();
        return value{array_join(this_.object_value(), !args.empty() ? to_string(h, args.front()).view() : std::wstring_view{L","})};
    }, 1);
    put_native_function(global, prototype, "reverse", [global](const value& this_, const std::vector<value>&) {
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
    put_native_function(global, prototype, "sort", [global](const value& this_, const std::vector<value>& args) {
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
        put_native_function(global, prototype, "indexOf", [global](const value& this_, const std::vector<value>& args) {
            return value{array_index_of(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "lastIndexOf", [global](const value& this_, const std::vector<value>& args) {
            return value{array_last_index_of(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "every", [global](const value& this_, const std::vector<value>& args) {
            return value{array_every(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "some", [global](const value& this_, const std::vector<value>& args) {
            return value{array_some(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "forEach", [global](const value& this_, const std::vector<value>& args) {
            array_for_each(global, this_, args);
            return value::undefined;
        }, 1);

        put_native_function(global, prototype, "map", [global](const value& this_, const std::vector<value>& args) {
            return value{array_map(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "filter", [global](const value& this_, const std::vector<value>& args) {
            return value{array_filter(global, this_, args)};
        }, 1);

        put_native_function(global, prototype, "reduce", [global](const value& this_, const std::vector<value>& args) {
            return value{array_reduce(global, this_, args, false)};
        }, 1);

        put_native_function(global, prototype, "reduceRight", [global](const value& this_, const std::vector<value>& args) {
            return value{array_reduce(global, this_, args, true)};
        }, 1);

        put_native_function(global, c, "isArray", [global](const value&, const std::vector<value>& args) {
            return value{!args.empty() && args.front().type() == value_type::object && is_array(args.front().object_value())};
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
        arr->put(string{global.heap(), index_string(i)}, args[i], property_attribute::none);
    }
    return arr;
}

bool is_array(const object_ptr& o) {
    return o.has_type<array_object>();
}

} // namespace mjs