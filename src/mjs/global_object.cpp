#include "global_object.h"
#include "lexer.h" // get_hex_value2/4
#include "gc_vector.h"
#include "array_object.h"
#include "native_object.h"
#include "function_object.h"
#include "regexp_object.h"
#include "error_object.h"
#include "string_object.h"
#include "number_object.h"
#include "date_object.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <memory>

namespace mjs {

namespace {

inline const value& get_arg(const std::vector<value>& args, int index) {
    return index < static_cast<int>(args.size()) ? args[index] : value::undefined;
}

//
// Object
//

// TODO: This is a bit of a hacky way to get the global object to have the same behavior as a normal object.
//       Could probably share the functions..
void add_object_prototype_functions(global_object& global, const object_ptr& prototype) {
    auto& h = global.heap();
    put_native_function(global, prototype, "toString", [&h](const value& this_, const std::vector<value>&){
        return value{string{h, "[object "} + this_.object_value()->class_name() + string{h, "]"}};
    }, 0);
    put_native_function(global, prototype, "valueOf", [](const value& this_, const std::vector<value>&){
        return this_;
    }, 0);

    if (global.language_version() >= version::es3) {
        prototype->put(global.common_string("toLocaleString"), prototype->get(L"toString"), global_object::default_attributes);
        put_native_function(global, prototype, "hasOwnProperty", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            const auto& o = this_.object_value();
            return value{o->check_own_property_attribute(to_string(o.heap(), get_arg(args, 0)).view(), property_attribute::none, property_attribute::none)};
        }, 1);
        put_native_function(global, prototype, "propertyIsEnumerable", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            global->validate_object(this_);
            const auto& o = this_.object_value();
            return value{o->check_own_property_attribute(to_string(o.heap(), get_arg(args, 0)).view(), property_attribute::dont_enum, property_attribute::none)};
        }, 1);
    }
}

create_result make_object_object(global_object& global) {
    auto prototype = global.object_prototype();
    auto o = make_function(global, [global = global.self_ptr()](const value&, const std::vector<value>& args) {
        if (args.empty() || args.front().type() == value_type::undefined || args.front().type() == value_type::null) {
            return value{global->make_object()};
        }
        return value{global->to_object(args.front())};
    }, prototype->class_name().unsafe_raw_get(), 1);
    o->default_construct_function();

    // §15.2.4
    prototype->put(global.common_string("constructor"), value{o}, global_object::default_attributes);

    add_object_prototype_functions(global, prototype);

    return { o, prototype };
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

    auto c = make_function(global, [](const value&, const std::vector<value>& args) {
        return value{!args.empty() && to_boolean(args.front())};
    },  bool_str.unsafe_raw_get(), 1);
    make_constructable(global, c, [prototype](const value&, const std::vector<value>& args) {
        return value{new_boolean(prototype, !args.empty() && to_boolean(args.front()))};
    });

    auto check_type = [global = global.self_ptr(), prototype](const value& this_) {
        global->validate_type(this_, prototype, "Boolean");
    };

    put_native_function(global, prototype, global.common_string("toString"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return value{string{this_.object_value().heap(), this_.object_value()->internal_value().boolean_value() ? L"true" : L"false"}};
    }, 0);

    put_native_function(global, prototype, global.common_string("valueOf"), [check_type](const value& this_, const std::vector<value>&){
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

value parse_float(const gc_heap_ptr<global_object>&, const std::wstring_view s) {
    std::wistringstream wiss{std::wstring{ltrim(s)}};
    double val;
    return value{wiss >> val ? val : NAN};
}

namespace {

void put_hex_byte(std::wstring& res, int val) {
    constexpr const char* const hexchars = "0123456789ABCDEF";
    res.push_back(hexchars[(val>>4)&0xf]);
    res.push_back(hexchars[val&0xf]);
}

void put_percent_hex_byte(std::wstring& res, int val) {
    res.push_back('%');
    put_hex_byte(res, val);
}

class encoder_exception : public std::exception {
public:
    explicit encoder_exception() {}

    const char* what() const noexcept override {
        return "Encoding error";
    }
};

struct escape_encoder {
    void operator()(std::wstring& res, int ch) const {
        if (ch > 255) {
            res.push_back('%');
            res.push_back('u');
            put_hex_byte(res, ch>>8);
            put_hex_byte(res, ch);
        } else {
            put_percent_hex_byte(res, ch);
        }
    }
};

struct uri_encoder {
    void operator()(std::wstring& res, int ch) const {
        assert(ch >= 0);
        if (ch <= 0x7F) {
            // One byte
            put_percent_hex_byte(res, ch);
            return;
        } else if (ch <= 0x7FF) {
            // Two byte
            put_percent_hex_byte(res, 0b11000000 | (ch >> 6));
            put_percent_hex_byte(res, 0b10000000 | (ch & 0x3f));
            return;
        } else if (ch <= 0xD7FF || (ch >= 0xE000 && ch <= 0xFFFF)) {
            // Three byte
            put_percent_hex_byte(res, 0b11100000 | (ch >> 12));
            put_percent_hex_byte(res, 0b10000000 | ((ch >> 6) & 0x3f));
            put_percent_hex_byte(res, 0b10000000 | (ch & 0x3f));
            return;
        } else if (ch <= 0xDBFF) {
            // Surrogate pair
        } else if (ch <= 0xDFFF) {
            throw encoder_exception{};
        }
        std::wostringstream woss;
        woss << "Not implemented in uri_encoder: 0x" << std::hex << ch << " '" << static_cast<wchar_t>(ch) << "'";
        NOT_IMPLEMENTED(woss.str());
    }
};

template<typename Pred, typename Encoder = uri_encoder>
std::wstring encode_inner(const std::wstring_view s, Pred pred, Encoder enc = Encoder{}) {
    std::wstring res;
    for (uint16_t ch: s) {
        if (pred(ch)) {
            res.push_back(ch);
        } else {
            enc(res, ch);
        }
    }
    return res;
}

[[noreturn]] void throw_uri_error(const gc_heap_ptr<global_object>& global) {
    throw native_error_exception{native_error_type::uri, global->stack_trace(), L"URI malformed"};
}

template<typename Pred>
value encode_uri_helper(const gc_heap_ptr<global_object>& global, const std::wstring_view s, Pred pred) {
    try {
        return value{string{global.heap(), encode_inner(s, pred)}};
    } catch (const encoder_exception&) {
        throw_uri_error(global);
    }
}


constexpr bool is_alpha_or_digit(int ch) {
    return (ch >= 'A' && ch <= 'Z')
        || (ch >= 'a' && ch <= 'z')
        || (ch >= '0' && ch <= '9');
}

template<typename CharT, size_t Size>
constexpr bool is_in_list(int ch, CharT (&str)[Size]) {
    return std::find(str, str + Size - 1, static_cast<CharT>(ch)) != str + Size - 1;
}

constexpr bool is_uri_unescaped(int ch) {
    return is_alpha_or_digit(ch) || is_in_list(ch, "-_.!~*'()");
}

constexpr bool is_uri_reserved(int ch) {
    return is_in_list(ch, ";/?:@&=+$,");
}

} // unnamed namespace

value escape(const gc_heap_ptr<global_object>& global, const std::wstring_view s) {
    return value{string{global.heap(), encode_inner(s, [](int ch) { return is_alpha_or_digit(ch) || is_in_list(ch, "@*_+-./"); }, escape_encoder{})}};
}

value unescape(const gc_heap_ptr<global_object>& global, const std::wstring_view s) {
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
    return value{string{global.heap(), res}};
}

value encode_uri(const gc_heap_ptr<global_object>& global, const std::wstring_view s) {
    return encode_uri_helper(global, s, [](int ch) { return is_uri_unescaped(ch) || is_uri_reserved(ch) || ch == '#'; } );
}

value encode_uri_component(const gc_heap_ptr<global_object>& global, const std::wstring_view s) {
    return encode_uri_helper(global, s, &is_uri_unescaped);
}

value decode_uri(const gc_heap_ptr<global_object>& global, const std::wstring_view s) {
    std::wstring res;
    for (size_t i = 0; i < s.length();) {
        if (s[i] != '%') {
            res += s[i];
            ++i;
            continue;
        }
        auto get_byte = [&] () {
            assert(s[i] == '%');
            ++i;
            if (i + 2 > s.length()) {
                throw_uri_error(global);
            }
            i += 2;
            return static_cast<uint8_t>(get_hex_value2(&s[i-2]));
        };
        auto get_continuation_byte = [&] () {
            auto b = get_byte();
            if ((b & 0b11000000) != 0b10000000) {
                throw_uri_error(global);
            }
            return static_cast<uint8_t>(b & 0x3f);
        };

        const auto first = get_byte();
        if (!(first & 0x80)) {
            // One byte
            res.push_back(first);
         } else if ((first & 0b11100000) == 0b11000000) {
            // Two byte
            char16_t c = first & 0x1f;
            c = (c << 6) | get_continuation_byte();
            res.push_back(c);
        } else if ((first & 0b11110000) == 0b11100000) {
            char16_t c = first & 0xf;
            c = (c << 6) | get_continuation_byte();
            c = (c << 6) | get_continuation_byte();
            res.push_back(c);
        } else {
            std::wostringstream woss;
            woss << "Unsupported: " << "0x" << std::hex << first;
            NOT_IMPLEMENTED(woss.str());
        }
    }
    return value{string{global.heap(), res}};
}

value decode_uri_component(const gc_heap_ptr<global_object>& global, const std::wstring_view s) {
    return decode_uri(global, s);
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

//
// Math
//

create_result make_math_object(global_object& global) {
    auto& h = global.heap();
    auto math = global.make_object();

    math->put(string{h, "E"},       value{2.7182818284590452354}, global_object::default_attributes);
    math->put(string{h, "LN10"},    value{2.302585092994046}, global_object::default_attributes);
    math->put(string{h, "LN2"},     value{0.6931471805599453}, global_object::default_attributes);
    math->put(string{h, "LOG2E"},   value{1.4426950408889634}, global_object::default_attributes);
    math->put(string{h, "LOG10E"},  value{0.4342944819032518}, global_object::default_attributes);
    math->put(string{h, "PI"},      value{3.14159265358979323846}, global_object::default_attributes);
    math->put(string{h, "SQRT1_2"}, value{0.7071067811865476}, global_object::default_attributes);
    math->put(string{h, "SQRT2"},   value{1.4142135623730951}, global_object::default_attributes);


    auto make_math_function1 = [&](const char* name, auto f) {
        put_native_function(global, math, name, [f](const value&, const std::vector<value>& args){
            return value{f(to_number(get_arg(args, 0)))};
        }, 1);
    };
    auto make_math_function2 = [&](const char* name, auto f) {
        put_native_function(global, math, name, [f](const value&, const std::vector<value>& args){
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

    put_native_function(global, math, "random", [](const value&, const std::vector<value>&){
        return value{static_cast<double>(rand()) / (1.+RAND_MAX)};
    }, 0);

    return { math, nullptr };
}

//
// Console
//

create_result make_console_object(global_object& global) {
    auto& h = global.heap();
    auto console = global.make_object();

    put_native_function(global, console, "assert", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
        const auto val = !args.empty() ? args.front() : value::undefined;
        if (to_boolean(val)) {
            return value::undefined;
        }
        std::wostringstream woss;
        if (args.size() > 1) {
            auto&h = global.heap();
            for (size_t i = 1; i < args.size(); ++i) {
                if (i > 1) woss << " ";
                woss << to_string(h, args[i]).view();
            }
        } else {
            debug_print(woss, val, 4);
            woss << " == true";
        }
        throw native_error_exception{native_error_type::assertion, global->stack_trace(), woss.str()};
    }, 1);

    using timer_clock = std::chrono::steady_clock;

    auto timers = std::make_shared<std::unordered_map<std::wstring, timer_clock::time_point>>();
    put_native_function(global, console, "log", [](const value&, const std::vector<value>& args) {
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
    put_native_function(global, console, "time", [timers, &h](const value&, const std::vector<value>& args) {
        if (args.empty()) {
            THROW_RUNTIME_ERROR("Missing argument to console.time()");
        }
        auto label = to_string(h, args.front());
        (*timers)[std::wstring{label.view()}] = timer_clock::now();
        return value::undefined;
    }, 1);
    put_native_function(global, console, "timeEnd", [timers, &h](const value&, const std::vector<value>& args) {
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
        std::wcout << label << ": " << show_duration(end_time - it->second) << "\n";
        timers->erase(it);
        return value::undefined;
    }, 1);

    return { console, nullptr };
}

//
// string_cache
//

//#define STRING_CACHE_STATS

class string_cache {
public:
    explicit string_cache(gc_heap& h, uint32_t capacity) : entries_(gc_vector<entry>::make(h, capacity)) {}
#ifdef STRING_CACHE_STATS
    ~string_cache() {
        std::wcout << "string_cache capcity " << entries_->capacity() << " length " << entries_->length() << "\n";
        std::wcout << " " << hits_ << " / " << lookups_ << " (" << 100.*hits_/lookups_ << "%) hit rate avg dist: " << 1.*dist_/hits_ << "\n";
    }
#endif

    void fixup(gc_heap& h) {
        entries_.fixup(h);
    }

    string get(gc_heap& h, const char* name) {
        const auto hash = calc_hash(name);

#ifdef STRING_CACHE_STATS
        ++lookups_;
#endif
        auto& es = entries_.dereference(h);
        auto es_data = es.data(); // Size we never go beyond the initial capacity the data pointer can't change in below

        // In cache already?
        for (uint32_t i = 0, l = es.length(); i < l; ++i) {
            // Check if we encounterd a "lost" weak pointer (only happens first time after a garbage collection)
            if (!es[i].s) {
                // Debugging note: Decrease the size of the cache to force this branch to occur (and increase rate of garbage collection)
                es.erase(i);
                --i; // Redo this one
                --l; // Length changed but we don't want to call es.length() again
                continue;
            }

            if (es_data[i].hash == hash && string_equal(name, es_data[i].s.dereference(h).view())) {
#ifdef STRING_CACHE_STATS
                ++hits_;
                dist_ += i;
#endif
                // Move first
                for (; i; --i) {
                    std::swap(es_data[i], es_data[i-1]);
                }
                return es_data[0].s.track(h);
            }
        }

        // No. Move existing entries up
        string s{h, name};

        if (es.length() < es.capacity()) {
            es.push_back(entry{0, nullptr});
            assert(es_data == es.data());
        }

        for (uint32_t i = es.length(); --i; ) {
            es_data[i] = es_data[i-1];
        }

        es_data[0] = { hash, s.unsafe_raw_get() };

        return s;
    }

private:
    struct entry {
        uint32_t hash;
        gc_heap_weak_ptr_untracked<gc_string> s;

        void fixup(gc_heap& h) {
            s.fixup(h);
        }
    };
    gc_heap_ptr_untracked<gc_vector<entry>> entries_;

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
};

#ifndef STRING_CACHE_STATS
static_assert(!gc_type_info_registration<string_cache>::needs_destroy);
#endif
static_assert(!gc_type_info_registration<string_cache>::needs_fixup);

} // unnamed namespace

class global_object_impl : public global_object {
public:
    friend gc_type_info_registration<global_object_impl>;

    object_ptr object_prototype() const override { return object_prototype_.track(heap()); }
    object_ptr function_prototype() const override { return function_prototype_.track(heap()); }
    object_ptr array_prototype() const override { return array_prototype_.track(heap()); }
    object_ptr regexp_prototype() const override { assert(language_version() >= version::es3); return regexp_prototype_.track(heap()); }
    object_ptr error_prototype() const override { assert(language_version() >= version::es3); return error_prototype_.track(heap()); }

    object_ptr to_object(const value& v) override {
        switch (v.type()) {
            // undefined and null give runtime errors
        case value_type::undefined:
        case value_type::null:
            {
                std::wostringstream woss;
                woss << "Cannot convert " << v.type() << " to object";
                throw native_error_exception{native_error_type::type, stack_trace(), woss.str()};
            }
        case value_type::boolean: return new_boolean(boolean_prototype_.track(heap()), v.boolean_value());
        case value_type::number:  return new_number(number_prototype_.track(heap()), v.number_value());
        case value_type::string:  return new_string(string_prototype_.track(heap()), v.string_value());
        case value_type::object:  return v.object_value();
        default:
            NOT_IMPLEMENTED(v.type());
        }
    }

    virtual gc_heap_ptr<global_object> self_ptr() const override {
        return self_.track(heap());
    }

private:
    string_cache string_cache_;
    gc_heap_ptr_untracked<object> object_prototype_;
    gc_heap_ptr_untracked<object> function_prototype_;
    gc_heap_ptr_untracked<object> array_prototype_;
    gc_heap_ptr_untracked<object> string_prototype_;
    gc_heap_ptr_untracked<object> boolean_prototype_;
    gc_heap_ptr_untracked<object> number_prototype_;
    gc_heap_ptr_untracked<object> regexp_prototype_;
    gc_heap_ptr_untracked<object> error_prototype_;
    gc_heap_ptr_untracked<global_object_impl> self_;

    void fixup() {
        auto& h = heap();
        string_cache_.fixup(h);
        object_prototype_.fixup(h);
        function_prototype_.fixup(h);
        array_prototype_.fixup(h);
        string_prototype_.fixup(h);
        boolean_prototype_.fixup(h);
        number_prototype_.fixup(h);
        regexp_prototype_.fixup(h);
        error_prototype_.fixup(h);
        self_.fixup(h);
        global_object::fixup();
    }

    //
    // Global
    //
    void popuplate_global() {
        // The object and function prototypes are special
        auto obj_proto = heap().make<object>(common_string("Object"), nullptr);
        object_prototype_   = obj_proto;
        function_prototype_ = static_cast<object_ptr>(heap().make<function_object>(common_string("Function"), obj_proto));

         
        auto add = [&](const char* name, auto create_func, gc_heap_ptr_untracked<object>* prototype = nullptr) {
            auto res = create_func(*this);
            put(common_string(name), value{res.obj}, default_attributes);
            if (res.prototype) {
                assert(res.obj.template has_type<function_object>());
                auto& f = static_cast<function_object&>(*res.obj);
                f.put_prototype_with_attributes(res.prototype, prototype_attributes);
            }
            if (prototype) {
                assert(res.prototype);
                *prototype = res.prototype;
            }
        };

        // §15.1
        add("Object", make_object_object, &object_prototype_);          // Resetting it..
        add("Function", make_function_object, &function_prototype_);    // same
        add("Array", make_array_object, &array_prototype_);
        add("String", make_string_object, &string_prototype_);
        add("Boolean", make_boolean_object, &boolean_prototype_);
        add("Number", make_number_object, &number_prototype_);
        add("Math", make_math_object);
        add("Date", make_date_object);
        add("console", make_console_object);

        if (language_version() >= version::es3) {
            add("RegExp", make_regexp_object, &regexp_prototype_);
            add("Error", make_error_object, &error_prototype_);
        }

        assert(!get(L"Object").object_value()->can_put(L"prototype"));

        auto self = self_ptr();

        auto global_const_attrs = default_attributes;
        if (version_ >= version::es3) {
            global_const_attrs |= property_attribute::dont_delete;
        }
        if (version_ >= version::es5) {
            global_const_attrs |= property_attribute::read_only;
        }

        put(string{heap(), "NaN"}, value{NAN}, global_const_attrs);
        put(string{heap(), "Infinity"}, value{INFINITY}, global_const_attrs);
        put(string{heap(), "undefined"}, value{value::undefined}, global_const_attrs);

        // Note: eval is added by the interpreter

        put_native_function(*this, self, "parseInt", [&h=heap()](const value&, const std::vector<value>& args) {
            const auto input = to_string(h, get_arg(args, 0));
            int radix = to_int32(get_arg(args, 1));
            return value{parse_int(input.view(), radix)};
        }, 2);
        put_native_function(*this, self, "isNaN", [](const value&, const std::vector<value>& args) {
            return value(std::isnan(to_number(args.empty() ? value::undefined : args.front())));
        }, 1);
        put_native_function(*this, self, "isFinite", [](const value&, const std::vector<value>& args) {
            return value(std::isfinite(to_number(args.empty() ? value::undefined : args.front())));
        }, 1);
        put_native_function(*this, self, "alert", [&h=heap()](const value&, const std::vector<value>& args) {
            std::wcout << "ALERT";
            for (auto& a: args) {
                std::wcout << "\t" << to_string(h, a);
            }
            std::wcout << "\n";
            return value::undefined;
        }, 1);

        auto put_string_function = [&](const char* name, auto f) {
            put_native_function(*this, self, name, [self, f](const value&, const std::vector<value>& args) {
                const auto input = to_string(self.heap(), get_arg(args, 0));
                return f(self, input.view());
            }, 1);
        };

        put_string_function("parseFloat", &parse_float);
        put_string_function("escape", &escape);
        put_string_function("unescape", &unescape);

        if (version_ >= version::es3) {
            put_string_function("encodeURI", &encode_uri);
            put_string_function("encodeURIComponent", &encode_uri_component);
            put_string_function("decodeURI", &decode_uri);
            put_string_function("decodeURIComponent", &decode_uri_component);
        }

        // Add this class as the global object
        put(common_string("global"), value{self}, property_attribute::dont_delete | property_attribute::read_only);
        // Give it the same properties as a normal object
        add_object_prototype_functions(*this, self);
    }

    string common_string(const char* str) override {
        return string_cache_.get(heap(), str);
    }

    explicit global_object_impl(gc_heap& h, version ver) : global_object(string{h, "Global"}, object_ptr{}), string_cache_(h, 16) {
        version_ = ver;
    }

    global_object_impl(global_object_impl&& other) = default;

    friend global_object;
};

gc_heap_ptr<global_object> global_object::make(gc_heap& h, version ver) {
    auto global = h.make<global_object_impl>(h, ver);
    global->self_ = global;
    global->popuplate_global(); // Populate here so the self_ptr() won't fail the assert
    return global;
}

object_ptr global_object::make_object() {
    auto op = object_prototype();
    return heap().make<object>(op->class_name(), op);
}

std::wstring index_string(uint32_t index) {
    assert(index != UINT32_MAX);
    wchar_t buffer[12], *p = &buffer[sizeof(buffer)/sizeof(*buffer)];
    *--p = '\0';
    do {
        *--p = '0' + index % 10;
        index /= 10;
    } while (index);
    return std::wstring{p};
}

void global_object::validate_object(const value& v) const {
    if (v.type() == value_type::object) {
        return;
    }
    std::wostringstream woss;
    mjs::debug_print(woss, v, 2, 1);
    woss << " is not an object";
    throw native_error_exception(native_error_type::type, stack_trace(), woss.str());
}

void global_object::validate_type(const value& v, const object_ptr& expected_prototype, const char* expected_type) const {
    std::wostringstream woss;
    if (v.type() == value_type::object) {
        auto oval = v.object_value();
        assert(oval);
        auto p = oval->prototype();
        if (p && p.get() == expected_prototype.get()) {
            return;
        }
        woss << v.object_value()->class_name();
    } else {
        mjs::debug_print(woss, v, 2, 1);
    }
    woss << " is not a" << (strchr("aeiou", expected_type[0]) ? "n" : "") << " " << expected_type;
    throw native_error_exception(native_error_type::type, stack_trace(), woss.str());
}

class arguments_array_object : public object {
public:
    friend gc_type_info_registration<arguments_array_object>;
private:
    explicit arguments_array_object(const string& class_name, const object_ptr& prototype) : object{class_name, prototype} {
    }
};

object_ptr global_object::make_arguments_array() {
    return heap().make<arguments_array_object>(common_string("Arguments"), object_prototype());
}

bool global_object::is_arguments_array(const object_ptr& o) {
    return o.has_type<arguments_array_object>();
}

} // namespace mjs
