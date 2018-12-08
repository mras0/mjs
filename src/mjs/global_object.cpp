#include "global_object.h"
#include "lexer.h" // get_hex_value2/4
#include "gc_array.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <memory>

// TODO: Use the well-known strings from global_object_impl in array_object
// TODO: Use string pool for index_string()'s? (or at least the commonly used ones...)

// TODO: Get rid of this stuff alltogether
#ifndef _WIN32
#define _mkgmtime timegm
#endif

namespace mjs {

namespace {

// FIXME: Is this sufficient to guard against clever users?
void validate_type(const value& v, const object_ptr& expected_prototype, const char* expected_type) {
    if (v.type() == value_type::object && v.object_value()->prototype().get() == expected_prototype.get()) {
        return;
    }
    std::wostringstream woss;
    mjs::debug_print(woss, v, 2, 1);
    woss << " is not a " << expected_type;
    THROW_RUNTIME_ERROR(woss.str());
}

value get_arg(const std::vector<value>& args, int index) {
    return index < static_cast<int>(args.size()) ? args[index] : value::undefined;
}

// TODO: Need better name
struct create_result {
    object_ptr obj;
    object_ptr prototype;
};

//
// Object
//

create_result make_object_object(global_object& global) {
    auto& h = global.heap();
    auto o = global.make_function([global = global.self_ptr()](const value&, const std::vector<value>& args) {
        if (args.empty() || args.front().type() == value_type::undefined || args.front().type() == value_type::null) {
            auto o = global->heap().make<object>(global->common_string("Object"), global->object_prototype());
            return value{o};
        }
        return value{global->to_object(args.front())};
    }, global.native_function_body(h, L"Object"), 1);
    global.make_constructable(o);

    // §15.2.4
    global.put_native_function(global.object_prototype(), "toString", [&h](const value& this_, const std::vector<value>&){
        return value{string{h, "[object "} + this_.object_value()->class_name() + string{h, "]"}};
    }, 0);
    global.put_native_function(global.object_prototype(), "valueOf", [](const value& this_, const std::vector<value>&){
        return this_;
    }, 0);
    return { o, global.object_prototype() };
}

//
// Function
//

create_result make_function_object(global_object& global, const object_ptr& prototype) {
    auto& h = global.heap();

    auto c = h.make<object>(prototype->class_name(), prototype); // Note: function constructor is added by interpreter

    // §15.3.4
    prototype->call_function(gc_function::make(h, [](const value&, const std::vector<value>&) {
        return value::undefined;
    }));
    global.put_native_function(prototype, "toString", [prototype](const value& this_, const std::vector<value>&) {
        validate_type(this_, prototype, "Function");
        assert(this_.object_value()->internal_value().type() == value_type::string);
        return this_.object_value()->internal_value();
    }, 0);
    return { c, prototype };
}

//
// Array
//

class array_object : public object {
public:
    friend gc_type_info_registration<array_object>;

    static constexpr const std::wstring_view length_str{L"length", 6};

    static gc_heap_ptr<array_object> make(const string& class_name, const object_ptr& prototype, uint32_t length) {
        return class_name.heap().make<array_object>(class_name, prototype, length);
    }

    void put(const string& name, const value& val, property_attribute attr) override {
        if (!can_put(name.view())) {
            return;
        }

        if (name.view() == length_str) {
            const uint32_t old_length = length();
            const uint32_t new_length = to_uint32(val);
            if (new_length < old_length) {
                for (uint32_t i = new_length; i < old_length; ++i) {
                    [[maybe_unused]] const bool res = object::delete_property(index_string(i));
                    assert(res);
                }
            }
            object::put(string{heap(), length_str}, value{static_cast<double>(new_length)});
        } else {
            object::put(name, val, attr);
            uint32_t index = to_uint32(value{name});
            if (name.view() == to_string(heap(), index).view() && index != UINT32_MAX && index >= length()) {
                object::put(string{heap(), length_str}, value{static_cast<double>(index+1)});
            }
        }
    }

    void unchecked_put(uint32_t index, const value& val) {
        const auto name = index_string(index);
        assert(index < to_uint32(get(length_str)) && can_put(name));
        object::put(string{heap(), name}, val);
    }

private:
    uint32_t length() {
        return to_uint32(object::get(length_str));
    }

    explicit array_object(const string& class_name, const object_ptr& prototype, uint32_t length) : object{class_name, prototype} {
        object::put(string{heap(), length_str}, value{static_cast<double>(length)}, property_attribute::dont_enum | property_attribute::dont_delete);
    }
};

string join(const object_ptr& o, const std::wstring_view& sep) {
    auto& h = o.heap();
    const uint32_t l = to_uint32(o->get(array_object::length_str));
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

    auto c = global.make_function([global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
        return global->array_constructor(this_, args);
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
        const uint32_t length = to_uint32(o->get(array_object::length_str));
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
        const uint32_t length = to_uint32(o.get(array_object::length_str));

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

//
// String
//

template<typename T>
class gc_vector {
public:
    explicit gc_vector(gc_heap& h, uint32_t initial_capacity) : table_(table::make(h, initial_capacity)) { assert(initial_capacity); }

    gc_heap& heap() { return table_.heap(); }

    uint32_t length() const { return table_->length(); }

    T* data() const {
        return table_->entries();
    }

    T& operator[](uint32_t index) {
        assert(index < table_->length());
        return table_->entries()[index];
    }

    void push_back(const T& e) {
        if (table_->length() == table_->capacity()) {
            table_ = table_->copy_with_increased_capacity();
        }
        table_->push_back(e);
    }

    void erase(uint32_t index) {
        table_->erase(index);
    }

    void erase(T* elem) {
        assert(elem >= begin() && elem < end());
        erase(static_cast<uint32_t>(elem - begin()));
    }

    T* begin() const { return data(); }
    T* end() const { return data() + length(); }

private:
    class table : public gc_array_base<table, T> {
    public:
        using gc_array_base<table, T>::make;
        using gc_array_base<table, T>::copy_with_increased_capacity;
        using gc_array_base<table, T>::push_back;
        using gc_array_base<table, T>::erase;
        using gc_array_base<table, T>::entries;
    private:
        friend gc_type_info_registration<table>;
        using gc_array_base<table, T>::gc_array_base;
    };
    gc_heap_ptr<table> table_;
};

class native_object;
using native_object_get_func = value (*)(const native_object&);
using native_object_put_func = void (*)(native_object&, const value&);

class native_object : public object {
public:
    virtual value get(const std::wstring_view& name) const override {
        if (auto it = find(name); it) {
            return it->get(*this);
        }
        return object::get(name);
    }

    virtual void put(const string& name, const value& val, property_attribute attr) override {
        if (auto it = find(name.view()); it) {
            if (it->has_attribute(property_attribute::read_only)) {
                return;
            }
            it->put(*this, val);
        } else {
            object::put(name, val, attr);
        }
    }

    virtual bool can_put(const std::wstring_view& name) const override {
        if (auto it = find(name); it) {
            return !it->has_attribute(property_attribute::read_only);
        }
        return object::can_put(name);
    }

    virtual bool has_property(const std::wstring_view& name) const override {
        return find(name) || object::has_property(name);
    }

    virtual bool delete_property(const std::wstring_view& name) override {
        if (auto it = find(name); it) {
            if (it->has_attribute(property_attribute::dont_delete)) {
                return false;
            }
            std::wcout << "TODO: Handle " << __func__ << " for native property " << name << "\n";
            std::abort();
        }
        return object::delete_property(name);
    }

private:
    struct native_object_property {
        char               name[32];
        property_attribute attributes;
        native_object_get_func get;
        native_object_put_func put;

        bool has_attribute(property_attribute a) const { return (attributes & a) == a; }

        native_object_property(const char* name, property_attribute attributes, native_object_get_func get, native_object_put_func put) : attributes(attributes), get(get), put(put) {
            assert(strlen(name) < sizeof(this->name));
            strcpy(this->name, name);
        }
    };
    gc_vector<native_object_property> native_properties_;

    native_object_property* find(const char* name) const {
        for (auto it = native_properties_.begin(), e = native_properties_.end(); it != e; ++it) {
            if (!strcmp(it->name, name)) {
                return it;
            }
        }
        return nullptr;
    }

    native_object_property* find(const std::wstring_view& v) const {
        const auto len = v.length();
        if (len >= sizeof(native_object_property::name)) {
            return nullptr;
        }
        char name[sizeof(native_object_property::name)];
        for (uint32_t i = 0; i < len; ++i) {
            if (v[i] < 32 || v[i] >= 127) {
                return nullptr;
            }
            name[i] = static_cast<char>(v[i]);
        }
        name[len] = '\0';
        return find(name);
    }

    void add_property_names(std::vector<string>& names) const override {
        for (const auto& p: native_properties_) {
            if (!p.has_attribute(property_attribute::dont_enum)) {
                names.emplace_back(heap(), p.name);
            }
        }
        object::add_property_names(names);
    }

protected:
    explicit native_object(const string& class_name, const object_ptr& prototype) : object(class_name, prototype), native_properties_(heap(), 16) {
    }

    void add_native_property(const char* name, property_attribute attributes, native_object_get_func get, native_object_put_func put) {
        assert(find(name) == nullptr);
        assert(((attributes & property_attribute::read_only) != property_attribute::none) == !put);
        native_properties_.push_back(native_object_property(name, attributes, get, put));
    }
};

class string_object : public native_object {
public:

private:
    friend gc_type_info_registration<string_object>;

    explicit string_object(const object_ptr& prototype, const string& val) : native_object(prototype->class_name(), prototype) {
        internal_value(value{val});
        add_native_property("length", global_object::prototype_attributes, [](const native_object& o) { return value{static_cast<double>(o.internal_value().string_value().view().length())}; }, nullptr);
    }
};

create_result make_string_object(global_object& global) {
    auto& h = global.heap();
    auto String_str_ = global.common_string("String");
    auto prototype = h.make<object>(String_str_, global.object_prototype());
    prototype->internal_value(value{string{h, ""}});

    auto c = global.make_function([&h](const value&, const std::vector<value>& args) {
        return value{args.empty() ? string{h, ""} : to_string(h, args.front())};
    }, global.native_function_body(String_str_), 1);
    global.make_constructable(c, gc_function::make(h, [prototype](const value&, const std::vector<value>& args) {
        return value{prototype.heap().make<string_object>(prototype, args.empty() ? string{prototype.heap(), ""} : to_string(prototype.heap(), args.front()))};
    }));

    global.put_native_function(c, string{h, "fromCharCode"}, [&h](const value&, const std::vector<value>& args){
        std::wstring s;
        for (const auto& a: args) {
            s.push_back(to_uint16(a));
        }
        return value{string{h, s}};
    }, 0);

    auto check_type = [prototype](const value& this_) {
        validate_type(this_, prototype, "String");
    };

    global.put_native_function(prototype, global.common_string("toString"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return this_.object_value()->internal_value();
    }, 0);
    global.put_native_function(prototype, global.common_string("valueOf"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return this_.object_value()->internal_value();
    }, 0);


    auto make_string_function = [&](const char* name, int num_args, auto f) {
        global.put_native_function(prototype, string{h, name}, [&h, f](const value& this_, const std::vector<value>& args){
            return value{f(to_string(h, this_).view(), args)};
        }, num_args);
    };

    make_string_function("charAt", 1, [&h](const std::wstring_view& s, const std::vector<value>& args){
        const int position = to_int32(get_arg(args, 0));
        if (position < 0 || position >= static_cast<int>(s.length())) {
            return string{h, ""};
        }
        return string{h, s.substr(position, 1)};
    });

    make_string_function("charCodeAt", 1, [](const std::wstring_view& s, const std::vector<value>& args){
        const int position = to_int32(get_arg(args, 0));
        if (position < 0 || position >= static_cast<int>(s.length())) {
            return static_cast<double>(NAN);
        }
        return static_cast<double>(s[position]);
    });

    make_string_function("indexOf", 2, [&h](const std::wstring_view& s, const std::vector<value>& args){
        const auto& search_string = to_string(h, get_arg(args, 0));
        const int position = to_int32(get_arg(args, 1));
        auto index = s.find(search_string.view(), position);
        return index == std::wstring_view::npos ? -1. : static_cast<double>(index);
    });

    make_string_function("lastIndexOf", 2, [&h](const std::wstring_view& s, const std::vector<value>& args){
        const auto& search_string = to_string(h, get_arg(args, 0));
        double position = to_number(get_arg(args, 1));
        const int ipos = std::isnan(position) ? INT_MAX : to_int32(position);
        auto index = s.rfind(search_string.view(), ipos);
        return index == std::wstring_view::npos ? -1. : static_cast<double>(index);
    });

    make_string_function("split", 1, [global = global.self_ptr()](const std::wstring_view& s, const std::vector<value>& args){
        auto& h = global->heap();
        auto a = global->array_constructor(value::null, {}).object_value();
        if (args.empty()) {
            a->put(string{h, index_string(0)}, value{string{h, s}});
        } else {
            const auto sep = to_string(h, args.front());
            if (sep.view().empty()) {
                for (uint32_t i = 0; i < s.length(); ++i) {
                    a->put(string{h, index_string(i)}, value{string{ h, s.substr(i,1) }});
                }
            } else {
                size_t pos = 0;
                uint32_t i = 0;
                for (; pos < s.length(); ++i) {
                    const auto next_pos = s.find(sep.view(), pos);
                    if (next_pos == std::wstring_view::npos) {
                        break;
                    }
                    a->put(string{h, index_string(i)}, value{string{ h, s.substr(pos, next_pos-pos) }});
                    pos = next_pos + 1;
                }
                if (pos < s.length()) {
                    a->put(string{h, index_string(i)}, value{string{ h, s.substr(pos) }});
                }
            }
        }
        return a;
    });

    make_string_function("substring", 1, [&h](const std::wstring_view& s, const std::vector<value>& args){
        int start = std::min(std::max(to_int32(get_arg(args, 0)), 0), static_cast<int>(s.length()));
        if (args.size() < 2) {
            return string{h, s.substr(start)};
        }
        int end = std::min(std::max(to_int32(get_arg(args, 1)), 0), static_cast<int>(s.length()));
        if (start > end) {
            std::swap(start, end);
        }
        return string{h, s.substr(start, end-start)};
    });

    make_string_function("toLowerCase", 0, [&h](const std::wstring_view& s, const std::vector<value>&){
        std::wstring res;
        for (auto c: s) {
            res.push_back(towlower(c));
        }
        return string{h, res};
    });

    make_string_function("toUpperCase", 0, [&h](const std::wstring_view& s, const std::vector<value>&){
        std::wstring res;
        for (auto c: s) {
            res.push_back(towupper(c));
        }
        return string{h, res};
    });

    return {c, prototype};
}

//
// Boolean
//

object_ptr new_boolean(const object_ptr& prototype, bool val) {
    auto o = prototype.heap().make<object>(prototype->class_name(), prototype);
    o->internal_value(value{val});
    return o;
}

create_result make_boolean_object(global_object& global) {
    auto& h = global.heap();
    auto bool_str = global.common_string("Boolean");
    auto prototype = h.make<object>(bool_str, global.object_prototype());
    prototype->internal_value(value{false});

    auto c = global.make_function([](const value&, const std::vector<value>& args) {
        return value{!args.empty() && to_boolean(args.front())};
    },  global.native_function_body(bool_str), 1);
    global.make_constructable(c, gc_function::make(h, [prototype](const value&, const std::vector<value>& args) {
        return value{new_boolean(prototype, !args.empty() && to_boolean(args.front()))};
    }));

    auto check_type = [prototype](const value& this_) {
        validate_type(this_, prototype, "Boolean");
    };

    global.put_native_function(prototype, global.common_string("toString"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return value{string{this_.object_value().heap(), this_.object_value()->internal_value().boolean_value() ? L"true" : L"false"}};
    }, 0);

    global.put_native_function(prototype, global.common_string("valueOf"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return this_.object_value()->internal_value();
    }, 0);

    return { c, prototype };
}

//
// Number
//

object_ptr new_number(const object_ptr& prototype,  double val) {
    auto o = prototype.heap().make<object>(prototype->class_name(), prototype);
    o->internal_value(value{val});
    return o;
}

create_result make_number_object(global_object& global) {
    auto& h = global.heap();
    auto Number_str_ = global.common_string("Number");
    auto prototype = h.make<object>(Number_str_, global.object_prototype());
    prototype->internal_value(value{0.});

    auto c = global.make_function([](const value&, const std::vector<value>& args) {
        return value{args.empty() ? 0.0 : to_number(args.front())};
    }, global.native_function_body(Number_str_), 1);
    global.make_constructable(c, gc_function::make(h, [prototype](const value&, const std::vector<value>& args) {
        return value{new_number(prototype, args.empty() ? 0.0 : to_number(args.front()))};
    }));

    c->put(string{h, "MAX_VALUE"}, value{1.7976931348623157e308}, global_object::default_attributes);
    c->put(string{h, "MIN_VALUE"}, value{5e-324}, global_object::default_attributes);
    c->put(string{h, "NaN"}, value{NAN}, global_object::default_attributes);
    c->put(string{h, "NEGATIVE_INFINITY"}, value{-INFINITY}, global_object::default_attributes);
    c->put(string{h, "POSITIVE_INFINITY"}, value{INFINITY}, global_object::default_attributes);

    auto check_type = [prototype](const value& this_) {
        validate_type(this_, prototype, "Number");
    };

    global.put_native_function(prototype, global.common_string("toString"), [check_type](const value& this_, const std::vector<value>& args){
        check_type(this_);
        const int radix = args.empty() ? 10 : to_int32(args.front());
        if (radix < 2 || radix > 36) {
            std::wostringstream woss;
            woss << "Invalid radix in Number.toString: " << to_string(this_.object_value().heap(), args.front());
            THROW_RUNTIME_ERROR(woss.str());
        }
        if (radix != 10) {
            NOT_IMPLEMENTED(radix);
        }
        auto o = this_.object_value();
        return value{to_string(o.heap(), o->internal_value())};
    }, 1);
    global.put_native_function(prototype, global.common_string("valueOf"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return this_.object_value()->internal_value();
    }, 0);

    return { c, prototype };
}

std::wstring_view ltrim(std::wstring_view s) {
    size_t start_pos = 0;
    while (start_pos < s.length() && isblank(s[start_pos]))
        ++start_pos;
    return s.substr(start_pos);
}

double parse_int(std::wstring_view s, int radix) {
    s = ltrim(s);
    int sign = 1;
    if (!s.empty() && (s[0] == '+' || s[0] == '-')) {
        if (s[0] == '-') sign = -1;
        s = s.substr(1);
    }
    // Result(5) = s
    if (radix != 0) {
        if (radix < 2 || radix > 36) {
            return NAN;
        }
        if (radix == 16 && s.length() >= 2 && s[0] == '0' && tolower(s[1]) == 'x') {
            s = s.substr(2);
        }
    }
    if (radix == 0) {
        if (s.empty() || s[0] != '0') {
            radix = 10;
        } else {
            if (s.size() >= 2 && tolower(s[1]) == 'x') {
                radix = 16;
                s = s.substr(2);
            } else {
                radix = 8;
            }
        }
    }

    double value = NAN;

    for (size_t i = 0; i < s.length(); ++i) {
        if (!isdigit(s[i]) && !isalpha(s[i])) {
            break;
        }
        const int val = s[i]>'9' ? 10+tolower(s[i])-'a' : s[i]-'0';
        if (val >= radix) {
            break;
        }
        if (std::isnan(value)) value = 0;
        value = value*radix + val;
    }

    return sign * value;
}

double parse_float(std::wstring_view s) {
    std::wistringstream wiss{std::wstring{ltrim(s)}};
    double val;
    return wiss >> val ? val : NAN;
}

std::wstring escape(std::wstring_view s) {
    std::wstring res;
    for (uint16_t ch: s) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '@' || ch == '*' || ch == '_' || ch == '+' || ch == '-' || ch == '.' || ch == '/') {
            res.push_back(ch);
        } else {
            const char* hexchars = "0123456789abcdef";
            if (ch > 255) {
                res.push_back('%');
                res.push_back('u');
                res.push_back(hexchars[(ch>>12)&0xf]);
                res.push_back(hexchars[(ch>>8)&0xf]);
                res.push_back(hexchars[(ch>>4)&0xf]);
                res.push_back(hexchars[ch&0xf]);
            } else {
                res.push_back('%');
                res.push_back(hexchars[(ch>>4)&0xf]);
                res.push_back(hexchars[ch&0xf]);
            }
        }
    }
    return res;
}

std::wstring unescape(std::wstring_view s) {
    std::wstring res;
    for (size_t i = 0; i < s.length(); ++i) {
        if (s[i] != '%') {
            res.push_back(s[i]);
            continue;
        }
        ++i;
        if (i + 2 > s.length() || (s[i] == 'u' && i + 5 > s.length())) {
            throw std::runtime_error("Invalid string in unescape");
        }
        if (s[i] == 'u') {
            res.push_back(static_cast<wchar_t>(get_hex_value4(&s[i+1])));
            i += 4;
        } else {
            res.push_back(static_cast<wchar_t>(get_hex_value2(&s[i])));
            i += 1;
        }
    }
    return res;
}

struct duration_printer {
    template<typename CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const duration_printer& dp) {
        const auto s = dp.d.count();
        constexpr auto ms = 1e3;
        constexpr auto us = 1e6;
        constexpr auto ns = 1e9;
        if (s < 1./us) return os << ns*s << "ns";
        if (s < 1./ms) return os << us*s << "us";
        if (s < 1) return os << ms*s << "ms";
        return os << s << "s";
    }

    std::chrono::duration<double> d;
};

template<typename Duration>
auto show_duration(Duration d) {
    return duration_printer{std::chrono::duration_cast<std::chrono::duration<double>>(d)};
}

// TODO: Optimize and use rules from 15.9.1 to elimiate c++ standard library calls
struct date_helper {
    static constexpr const int hours_per_day = 24;
    static constexpr const int minutes_per_hour = 60;
    static constexpr const int seconds_per_minute = 60;
    static constexpr const int ms_per_second = 1000;
    static constexpr const int ms_per_minute = ms_per_second * seconds_per_minute;
    static constexpr const int ms_per_hour = ms_per_minute * minutes_per_hour;
    static constexpr const int ms_per_day = ms_per_hour * hours_per_day;
    static_assert(ms_per_day == 86'400'000);

    static double current_time_utc() {
        return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    static int64_t day(double t) {
        return static_cast<int64_t>(t/ms_per_day);
    }

    static int64_t time_within_day(int64_t t) {
        return ((t % ms_per_day) + ms_per_day) % ms_per_day;
    }

    static int days_in_year(int64_t y) {
        if (y % 4)  return 365;
        if (y % 100) return 366;
        return y % 400 ? 365 : 366;
    }

    static int64_t day_from_year(int y) {
        return 365*(y-1970) + (y-1969)/4 - (y-1901)/100 + (y-1601)/400;
    }

    static int64_t time_from_year(int y) {
        return ms_per_day * day_from_year(y);
    }

    static bool in_leap_year(double t) {
        return days_in_year(year_from_time(t)) == 366;
    }

    static std::tm tm_from_time(double t) {
        const std::time_t tt = static_cast<std::time_t>(t / ms_per_second);
        return *gmtime(&tt);
    }

    static int64_t year_from_time(double t) {
        return tm_from_time(t).tm_year + 1900;
    }

    static int64_t month_from_time(double t) {
        return tm_from_time(t).tm_mon;
    }

    static int64_t date_from_time(double t) {
        return tm_from_time(t).tm_mday;
    }

    static int64_t week_day(double t) {
        return (day(t) + 4) % 7;
    }

    static int64_t hour_from_time(double t) {
        return static_cast<int64_t>(t / ms_per_hour) % hours_per_day;
    }

    static int64_t min_from_time(double t) {
        return static_cast<int64_t>(t / ms_per_minute) % minutes_per_hour;
    }

    static int64_t sec_from_time(double t) {
        return static_cast<int64_t>(t / ms_per_second) % seconds_per_minute;
    }

    static int64_t ms_from_time(double t) {
        return static_cast<int64_t>(t) % ms_per_second;
    }

    static double make_time(double hour, double min, double sec, double ms) {
        if (!std::isfinite(hour) || !std::isfinite(min) || !std::isfinite(sec) || !std::isfinite(ms)) {
            return NAN;
        }
        return to_integer(hour) * ms_per_hour + to_integer(min) * ms_per_minute + to_integer(sec) * ms_per_second + to_integer(ms);
    }

    static double make_day(double year, double month, double day) {
        if (!std::isfinite(year) || !std::isfinite(month) || !std::isfinite(day)) {
            return NAN;
        }
        const auto y = static_cast<int32_t>(year) + static_cast<int32_t>(month)/12;
        const auto m = static_cast<int32_t>(month) % 12;
        const auto d = static_cast<int32_t>(day);
        // non-standard :/
        std::tm tm;
        std::memset(&tm, 0, sizeof(tm));
        tm.tm_year = y - 1900;
        tm.tm_mon = m;
        tm.tm_mday = d;
        return static_cast<double>(_mkgmtime(&tm)) / 86400;
    }

    static double make_date(double day, double time) {
        if (!std::isfinite(day) || !std::isfinite(time)) {
            return NAN;
        }
        return day * ms_per_day + time;
    }

    static double time_clip(double t) {
        return std::isfinite(t) && std::abs(t) <= 8.64e15 ? t : NAN;
    }

    static double time_from_args(const std::vector<value>& args) {
        assert(args.size() >= 3);
        double year = to_number(args[0]);
        if (double iyear = to_integer(year); !std::isnan(year) && iyear >= 0 && iyear <= 99) {
            year = iyear + 1900;
        }
        const double month   = to_number(args[1]);
        const double day     = to_number(args[2]);
        const double hours   = args.size() > 3 ? to_number(args[3]) : 0;
        const double minutes = args.size() > 4 ? to_number(args[4]) : 0;
        const double seconds = args.size() > 5 ? to_number(args[5]) : 0;
        const double ms      = args.size() > 6 ? to_number(args[6]) : 0;
        return date_helper::make_date(date_helper::make_day(year, month, day), date_helper::make_time(hours, minutes, seconds, ms));
    }

    static string to_string(gc_heap& h, double t) {
        std::wostringstream woss;
        woss << "[Date: " << t << "]";
        return string{h, woss.str()};
    }

    static double utc(double t) {
        // TODO: subtract local time adjustment
        return t;
    }
    static double local_time(double t) {
        // TODO: add local time adjustment
        return t;
    }
};

class date_object : public object {
public:
    friend gc_type_info_registration<date_object>;

    value_type default_value_type() const override {
        // When hint is undefined, assume Number unless it's a Date object in which case assume String
        return value_type::string;
    }

private:
    explicit date_object(const object_ptr& prototype, double val) : object{prototype->class_name(), prototype} {
        internal_value(value{val});
    }
};


//
// Date
//

create_result make_date_object(global_object& global) {
    auto& h = global.heap();
    auto Date_str_ = global.common_string("Date");

    auto prototype = h.make<object>(Date_str_, global.object_prototype());
    prototype->internal_value(value{NAN});

    auto new_date = [prototype, Date_str_](double val) {
        return prototype.heap().make<date_object>(prototype, val);
    };

    auto c = global.make_function([prototype, new_date](const value&, const std::vector<value>&) {
        // Equivalent to (new Date()).toString()
        return value{to_string(prototype.heap(), value{new_date(date_helper::current_time_utc())})};
    }, global.native_function_body(Date_str_), 7);
    global.make_constructable(c, gc_function::make(h, [new_date](const value&, const std::vector<value>& args) {
        if (args.empty()) {
            return value{new_date(date_helper::current_time_utc())};
        } else if (args.size() == 1) {
            auto v = to_primitive(args[0]);
            if (v.type() == value_type::string) {
                // return Date.parse(v)
                NOT_IMPLEMENTED("new Date(string)");
            }
            return value{new_date(to_number(v))};
        } else if (args.size() == 2) {
            NOT_IMPLEMENTED("new Date(value)");
        }
        return value{date_helper::time_clip(date_helper::utc(date_helper::time_from_args(args)))};
    }));

    global.put_native_function(c, "parse", [&h](const value&, const std::vector<value>& args) {
        if (1) NOT_IMPLEMENTED(to_string(h, get_arg(args, 0)));
        return value::undefined;
    }, 1);
    global.put_native_function(c, "UTC", [](const value&, const std::vector<value>& args) {
        if (args.size() < 3) {
            NOT_IMPLEMENTED("Date.UTC() with less than 3 arguments");
        }
        return value{date_helper::time_clip(date_helper::time_from_args(args))};
    }, 7);

    // TODO: Date.parse(string)

    auto check_type = [prototype](const value& this_) {
        validate_type(this_, prototype, "Date");
    };

    auto make_date_getter = [&](const char* name, auto f) {
        global.put_native_function(prototype, name ,[f, check_type](const value& this_, const std::vector<value>&) {
            check_type(this_);
            const double t = this_.object_value()->internal_value().number_value();
            if (std::isnan(t)) {
                return value{t};
            }
            return value{static_cast<double>(f(t))};
        }, 0);
    };
    make_date_getter("valueOf", [](double t) { return t; });
    make_date_getter("getTime", [](double t) { return t; });
    make_date_getter("getYear", [](double t) {
        return date_helper::year_from_time(date_helper::local_time(t)) - 1900;
    });
    make_date_getter("getFullYear", [](double t) {
        return date_helper::year_from_time(date_helper::local_time(t));
    });
    make_date_getter("getUTCFullYear", [](double t) {
        return date_helper::year_from_time(t);
    });
    make_date_getter("getMonth", [](double t) {
        return date_helper::month_from_time(date_helper::local_time(t));
    });
    make_date_getter("getUTCMonth", [](double t) {
        return date_helper::month_from_time(t);
    });
    make_date_getter("getDate", [](double t) {
        return date_helper::date_from_time(date_helper::local_time(t));
    });
    make_date_getter("getUTCDate", [](double t) {
        return date_helper::date_from_time(t);
    });
    make_date_getter("getDay", [](double t) {
        return date_helper::week_day(date_helper::local_time(t));
    });
    make_date_getter("getUTCDay", [](double t) {
        return date_helper::week_day(t);
    });
    make_date_getter("getHours", [](double t) {
        return date_helper::hour_from_time(date_helper::local_time(t));
    });
    make_date_getter("getUTCHours", [](double t) {
        return date_helper::hour_from_time(t);
    });
    make_date_getter("getMinutes", [](double t) {
        return date_helper::min_from_time(date_helper::local_time(t));
    });
    make_date_getter("getUTCMinutes", [](double t) {
        return date_helper::min_from_time(t);
    });
    make_date_getter("getSeconds", [](double t) {
        return date_helper::sec_from_time(date_helper::local_time(t));
    });
    make_date_getter("getUTCSeconds", [](double t) {
        return date_helper::sec_from_time(t);
    });
    make_date_getter("getMilliseconds", [](double t) {
        return date_helper::ms_from_time(date_helper::local_time(t));
    });
    make_date_getter("getUTCMilliseconds", [](double t) {
        return date_helper::ms_from_time(t);
    });
    make_date_getter("getTimezoneOffset", [](double t) {
        return (t - date_helper::local_time(t)) / date_helper::ms_per_minute;
    });

    auto make_date_mutator = [&](const char* name, auto f) {
        global.put_native_function(prototype, name, [f, check_type](const value& this_, const std::vector<value>& args) {
            check_type(this_);
            auto& obj = *this_.object_value();
            f(obj, args);
            return obj.internal_value();
        }, 0);
    };

    // setTime(time)
    make_date_mutator("setTime", [](object& d, const std::vector<value>& args) {
        d.internal_value(value{to_number(get_arg(args, 0))});
    });

    // setMilliseconds(ms)
    // setUTCMilliseconds(ms)
    // ...

    global.put_native_function(prototype, "toString", [check_type](const value& this_, const std::vector<value>&) {
        check_type(this_);
        auto o = this_.object_value();
        return value{date_helper::to_string(o.heap(), o->internal_value().number_value())};
    }, 0);
    // TODO: toLocaleString()
    // TODO: toUTCString()
    // TODO: toGMTString = toUTCString

    return { c, prototype };
}

//
// Math
//

create_result make_math_object(global_object& global) {
    auto& h = global.heap();
    auto math = h.make<object>(global.common_string("Object"), global.object_prototype());

    math->put(string{h, "E"},       value{2.7182818284590452354}, global_object::default_attributes);
    math->put(string{h, "LN10"},    value{2.302585092994046}, global_object::default_attributes);
    math->put(string{h, "LN2"},     value{0.6931471805599453}, global_object::default_attributes);
    math->put(string{h, "LOG2E"},   value{1.4426950408889634}, global_object::default_attributes);
    math->put(string{h, "LOG10E"},  value{0.4342944819032518}, global_object::default_attributes);
    math->put(string{h, "PI"},      value{3.14159265358979323846}, global_object::default_attributes);
    math->put(string{h, "SQRT1_2"}, value{0.7071067811865476}, global_object::default_attributes);
    math->put(string{h, "SQRT2"},   value{1.4142135623730951}, global_object::default_attributes);


    auto make_math_function1 = [&](const char* name, auto f) {
        global.put_native_function(math, name, [f](const value&, const std::vector<value>& args){
            return value{f(to_number(get_arg(args, 0)))};
        }, 1);
    };
    auto make_math_function2 = [&](const char* name, auto f) {
        global.put_native_function(math, name, [f](const value&, const std::vector<value>& args){
            return value{f(to_number(get_arg(args, 0)), to_number(get_arg(args, 1)))};
        }, 2);
    };

#define MATH_IMPL_1(name) make_math_function1(#name, [](double x) { return std::name(x); })
#define MATH_IMPL_2(name) make_math_function2(#name, [](double x, double y) { return std::name(x, y); })

    MATH_IMPL_1(abs);
    MATH_IMPL_1(acos);
    MATH_IMPL_1(asin);
    MATH_IMPL_1(atan);
    MATH_IMPL_2(atan2);
    MATH_IMPL_1(ceil);
    MATH_IMPL_1(cos);
    MATH_IMPL_1(exp);
    MATH_IMPL_1(floor);
    MATH_IMPL_1(log);
    MATH_IMPL_2(pow);
    MATH_IMPL_1(sin);
    MATH_IMPL_1(sqrt);
    MATH_IMPL_1(tan);

#undef MATH_IMPL_1
#undef MATH_IMPL_2

    make_math_function2("min", [](double x, double y) {
        return y >= x ? x: y;
    });
    make_math_function2("max", [](double x, double y) {
        return y < x ? x: y;
    });
    make_math_function1("round", [](double x) {
        if (x >= 0 && x < 0.5) return +0.0;
        if (x < 0 && x >= -0.5) return -0.0;
        return std::floor(x+0.5);
    });

    global.put_native_function(math, "random", [](const value&, const std::vector<value>&){
        return value{static_cast<double>(rand()) / (1.+RAND_MAX)};
    }, 0);

    return { math, nullptr };
}

//
// Console
//

create_result make_console_object(global_object& global) {
    auto& h = global.heap();
    auto console = h.make<object>(global.common_string("Object"), global.object_prototype());

    using timer_clock = std::chrono::steady_clock;

    auto timers = std::make_shared<std::unordered_map<std::wstring, timer_clock::time_point>>();
    global.put_native_function(console, "log", [](const value&, const std::vector<value>& args) {
        for (const auto& a: args) {
            if (a.type() == value_type::string) {
                std::wcout << a.string_value();
            } else {
                mjs::debug_print(std::wcout, a, 4);
            }
            std::wcout << ' ';
        }
        std::wcout << '\n';
        return value::undefined;
    }, 1);
    global.put_native_function(console, "time", [timers, &h](const value&, const std::vector<value>& args) {
        if (args.empty()) {
            THROW_RUNTIME_ERROR("Missing argument to console.time()");
        }
        auto label = to_string(h, args.front());
        (*timers)[std::wstring{label.view()}] = timer_clock::now();
        return value::undefined;
    }, 1);
    global.put_native_function(console, "timeEnd", [timers, &h](const value&, const std::vector<value>& args) {
        const auto end_time = timer_clock::now();
        if (args.empty()) {
            THROW_RUNTIME_ERROR("Missing argument to console.timeEnd()");
        }
        auto label = to_string(h, args.front());
        auto it = timers->find(std::wstring{label.view()});
        if (it == timers->end()) {
            std::wostringstream woss;
            woss << "Timer not found: " << label;
            THROW_RUNTIME_ERROR(woss.str());
        }
        std::wcout << "timeEnd " << label << ": " << show_duration(end_time - it->second) << "\n";
        timers->erase(it);
        return value::undefined;
    }, 1);

    return { console, nullptr };
}

//
// string_cache
//

//#define STRING_CACHE_STATS

struct string_cache_entry {
    uint32_t hash;
    gc_heap_weak_ptr_untracked<gc_string> s;
};

class string_cache : public gc_array_base<string_cache, string_cache_entry> {
public:
    friend gc_type_info_registration<string_cache>;

    using gc_array_base::make;

    string get(const char* name) {
        const auto hash = calc_hash(name);
        auto e = entries();

#ifdef STRING_CACHE_STATS
        ++lookups_;
#endif
        auto& h = heap();

        // In cache already?
        for (uint32_t i = 0; i < length(); ++i) {
            // Check if we encounterd an "lost" weak pointer (only happens first time after a garbage collection)
            if (!e[i].s) {
                // Debugging note: Decrease the size of the cache to force this branch to occur (and increase rate of garbage collection)
                erase(i);
                --i; // Redo this one
                continue;
            }

            if (e[i].hash == hash && string_equal(name, e[i].s.dereference(h).view())) {
#ifdef STRING_CACHE_STATS
                ++hits_;
                dist_ += i;
#endif
                // Move first
                for (; i; --i) {
                    std::swap(e[i], e[i-1]);
                }
                return e[0].s.track(h);
            }
        }

        // No. Move all entries up and insert

        if (length() < capacity()) {
            length(length()+1);
        }

        for (uint32_t i = length(); --i; ) {
            e[i] = e[i-1];
        }


        // Insert
        string s{h, name};
        e[0].hash = hash;
        e[0].s = s.unsafe_raw_get(); 

        return s;
    }

private:
#ifdef STRING_CACHE_STATS
    uint32_t lookups_ = 0;
    uint32_t hits_    = 0;
    uint64_t dist_    = 0;
#endif

    static bool string_equal(const char* s, std::wstring_view v) {
        for (uint32_t i = 0, sz = static_cast<uint32_t>(v.size()); i < sz; ++i, ++s) {
            if (*s != v[i]) return false;
        }
        return !*s;
    }

    static uint32_t calc_hash(const char* s) {
        // FNV
        constexpr uint32_t prime = 16777619;
        constexpr uint32_t offset = 2166136261;
        uint32_t h = offset;
        for (; *s; ++s) {
            h = (h*prime) ^ static_cast<uint8_t>(*s);
        }
        return h;
    }

    using gc_array_base::gc_array_base;

#ifdef STRING_CACHE_STATS
    ~string_cache() {
        std::wcout << "string_cache capcity " << capacity() << " length " << length() << "\n";
        std::wcout << " " << hits_ << " / " << lookups_ << " (" << 100.*hits_/lookups_ << "%) hit rate avg dist: " << 1.*dist_/hits_ << "\n";
    }
#endif

    void fixup() {
        auto e = entries();
        auto& h = heap();
        for (uint32_t i = 0, l = length(); i < l; ++i) {
            e[i].s.fixup(h);
        }
    }
};

#ifndef STRING_CACHE_STATS
static_assert(!gc_type_info_registration<string_cache>::needs_destroy);
#endif
static_assert(gc_type_info_registration<string_cache>::needs_fixup);

} // unnamed namespace

class global_object_impl : public global_object {
public:
    friend gc_type_info_registration<global_object_impl>;

    const object_ptr& object_prototype() const override { return object_prototype_; }

    object_ptr make_raw_function() override {
        auto o = heap().make<object>(function_prototype_->class_name(), function_prototype_);
        o->put(common_string("prototype"), value{heap().make<object>(common_string("Object"), object_prototype_)}, property_attribute::dont_enum);
        return o;
    }

    object_ptr to_object(const value& v) override {
        switch (v.type()) {
            // undefined and null give runtime errors
        case value_type::undefined:
        case value_type::null:
            {
                std::ostringstream oss;
                oss << "Cannot convert " << v.type() << " to object in " << __FUNCTION__;
                THROW_RUNTIME_ERROR(oss.str());
            }
        case value_type::boolean: return new_boolean(boolean_prototype_, v.boolean_value());
        case value_type::number:  return new_number(number_prototype_, v.number_value());
        case value_type::string:  return heap().make<string_object>(string_prototype_, v.string_value());
        case value_type::object:  return v.object_value();
        default:
            NOT_IMPLEMENTED(v.type());
        }
    }

    virtual gc_heap_ptr<global_object> self_ptr() const override {
        return self_;
    }

private:
    gc_heap_ptr<string_cache> string_cache_;
    object_ptr object_prototype_;
    object_ptr function_prototype_;
    object_ptr array_prototype_;
    object_ptr string_prototype_;
    object_ptr boolean_prototype_;
    object_ptr number_prototype_;
    gc_heap_ptr<global_object_impl> self_;

    object_ptr do_make_function(const native_function_type& f, const string& body_text, int named_args) override {
        auto o = make_raw_function();
        put_function(o, f, body_text, named_args);
        return o;
    }

    value array_constructor(const value&, const std::vector<value>& args) override {
        if (args.size() == 1 && args[0].type() == value_type::number) {
            return value{array_object::make(array_prototype_->class_name(), array_prototype_, to_uint32(args[0].number_value()))};
        }
        auto arr = array_object::make(array_prototype_->class_name(), array_prototype_, static_cast<uint32_t>(args.size()));
        for (uint32_t i = 0; i < args.size(); ++i) {
            arr->unchecked_put(i, args[i]);
        }
        return value{arr};
    }

    //
    // Global
    //
    void popuplate_global() {
        // The object and function prototypes are special
        object_prototype_   = heap().make<object>(string{heap(), "ObjectPrototype"}, nullptr);
        function_prototype_ = heap().make<object>(common_string("Function"), object_prototype_);

         
        auto add = [&](const char* name, auto create_func, object_ptr* prototype = nullptr) {
            auto res = create_func(*this);
            put(common_string(name), value{res.obj}, default_attributes);
            if (res.prototype) {
                res.obj->put(common_string("prototype"), value{res.prototype}, prototype_attributes);
            }
            if (prototype) {
                assert(res.prototype);
                *prototype = res.prototype;
            }
        };

        // §15.1
        add("Object", make_object_object, &object_prototype_);          // Resetting it..
        add("Function", [&](auto& g) { return make_function_object(g, function_prototype_); }, &function_prototype_);    // same
        add("Array", make_array_object, &array_prototype_);
        add("String", make_string_object, &string_prototype_);
        add("Boolean", make_boolean_object, &boolean_prototype_);
        add("Number", make_number_object, &number_prototype_);
        add("Math", make_math_object);
        add("Date", make_date_object);
        add("console", make_console_object);

        put(string{heap(), "NaN"}, value{NAN}, default_attributes);
        put(string{heap(), "Infinity"}, value{INFINITY}, default_attributes);
        // Note: eval is added by the interpreter
        put_native_function(*this, "parseInt", [&h=heap()](const value&, const std::vector<value>& args) {
            const auto input = to_string(h, get_arg(args, 0));
            int radix = to_int32(get_arg(args, 1));
            return value{parse_int(input.view(), radix)};
        }, 2);
        put_native_function(*this, "parseFloat", [&h=heap()](const value&, const std::vector<value>& args) {
            const auto input = to_string(h, get_arg(args, 0));
            return value{parse_float(input.view())};
        }, 1);
        put_native_function(*this, "escape", [&h=heap()](const value&, const std::vector<value>& args) {
            const auto input = to_string(h, get_arg(args, 0));
            return value{string{h, escape(input.view())}};
        }, 1);
        put_native_function(*this, "unescape", [&h=heap()](const value&, const std::vector<value>& args) {
            const auto input = to_string(h, get_arg(args, 0));
            return value{string{h, unescape(input.view())}};
        }, 1);
        put_native_function(*this, "isNaN", [](const value&, const std::vector<value>& args) {
            return value(std::isnan(to_number(args.empty() ? value::undefined : args.front())));
        }, 1);
        put_native_function(*this, "isFinite", [](const value&, const std::vector<value>& args) {
            return value(std::isfinite(to_number(args.empty() ? value::undefined : args.front())));
        }, 1);
        put_native_function(*this, "alert", [&h=heap()](const value&, const std::vector<value>& args) {
            std::wcout << "ALERT";
            for (auto& a: args) {
                std::wcout << "\t" << to_string(h, a);
            }
            std::wcout << "\n";
            return value::undefined;
        }, 1);

    }

    string common_string(const char* str) override {
        return string_cache_->get(str);
    }

    explicit global_object_impl(gc_heap& h) : global_object(string{h, "Global"}, object_ptr{}), string_cache_(string_cache::make(h, 16)) {
    }

    global_object_impl(global_object_impl&& other) = default;

    void put_function(const object_ptr& o, const native_function_type& f, const string& body_text, int named_args) override;

    friend global_object;
};

gc_heap_ptr<global_object> global_object::make(gc_heap& h) {
    auto global = h.make<global_object_impl>(h);
    global->self_ = global;
    global->popuplate_global(); // Populate here so the safe_ptr() won't fail the assert
    return global;
}

string global_object::native_function_body(gc_heap& h, const std::wstring_view& s) {
    std::wostringstream oss;
    oss << "function " << s << "() { [native code] }";
    return string{h, oss.str()};
}

void global_object_impl::put_function(const object_ptr& o, const native_function_type& f, const string& body_text, int named_args) {
    assert(o->class_name().view() == L"Function");
    assert(!o->call_function());
    o->put(common_string("length"), value{static_cast<double>(named_args)}, property_attribute::read_only | property_attribute::dont_delete | property_attribute::dont_enum);
    o->put(common_string("arguments"), value::null, property_attribute::read_only | property_attribute::dont_delete | property_attribute::dont_enum);
    o->call_function(f);
    assert(o->internal_value().type() == value_type::undefined);
    o->internal_value(value{body_text});
}

void global_object::make_constructable(const object_ptr& o, const native_function_type& f) {
    assert(o->internal_value().type() == value_type::string);
    o->construct_function(f ? f : o->call_function());
    auto p = o->get(L"prototype");
    assert(p.type() == value_type::object);
    p.object_value()->put(common_string("constructor"), value{o}, global_object::default_attributes);
}

std::wstring index_string(uint32_t index) {
    wchar_t buffer[10], *p = &buffer[10];
    *--p = '\0';
    do {
        *--p = '0' + index % 10;
        index /= 10;
    } while (index);
    return std::wstring{p};
}

} // namespace mjs
