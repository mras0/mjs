#ifndef MJS_VALUE_H
#define MJS_VALUE_H

#include <iosfwd>
#include <string>
#include <string_view>
#include <cassert>
#include <stdint.h>
#include <climits>
#include <memory>
#include <vector>

#include "string.h"

namespace mjs {

enum class value_type {
    undefined, null, boolean, number, string, object,
    // internal
    reference
};

constexpr bool is_primitive(value_type t) {
    return t == value_type::undefined ||t == value_type::null || t == value_type::boolean || t == value_type::number || t == value_type::string;
}

const char* string_value(value_type t);
template<typename CharT, typename CharTraitsT>
std::basic_ostream<CharT, CharTraitsT>& operator<<(std::basic_ostream<CharT, CharTraitsT>& os, value_type t) {
    return os << string_value(t);
}

enum class property_attribute {
    none = 0,
    read_only = 1<<0,
    dont_enum = 1<<1,
    dont_delete = 1<<2,
    internal = 1<<3,
};

constexpr inline property_attribute operator|(property_attribute l, property_attribute r) {
    return static_cast<property_attribute>(static_cast<int>(l) | static_cast<int>(r));
}

constexpr inline property_attribute operator&(property_attribute l, property_attribute r) {
    return static_cast<property_attribute>(static_cast<int>(l) & static_cast<int>(r));
}

class object;
class value;
using object_ptr = gc_heap_ptr<object>;

// §8.7
class reference {
public:
    explicit reference(const object_ptr& base, const string& property_name) : base_(base), property_name_(property_name) {
        assert(base);
    }

    const object_ptr& base() const { return base_; }
    const string& property_name() const { return property_name_; }

    value get_value() const;
    void put_value(const value& val) const;

private:
    object_ptr base_;
    string property_name_;
};


class value {
public:
    explicit value() : type_(value_type::undefined) {}
    explicit value(bool b) : type_(value_type::boolean), b_(b) {}
    explicit value(double n) : type_(value_type::number), n_(n) {}
    explicit value(const string& s) : type_(value_type::string), s_(s) {}
    explicit value(const object_ptr& o) : type_(value_type::object), o_(o) {}
    explicit value(const reference& r) : type_(value_type::reference), r_(r) {}
    explicit value(const void*) = delete;
    value(const value& rhs);
    value(value&& rhs);
    ~value() { destroy(); }

    value& operator=(const value& rhs);
    value& operator=(value&& rhs);

    value_type type() const { return type_; }
    bool boolean_value() const { assert(type_ == value_type::boolean); return b_; }
    double number_value() const { assert(type_ == value_type::number); return n_; }
    const string& string_value() const { assert(type_ == value_type::string); return s_; }
    const object_ptr& object_value() const { assert(type_ == value_type::object); return o_; }
    const reference& reference_value() const { assert(type_ == value_type::reference); return r_; }

    static const value undefined;
    static const value null;
private:
    explicit value(value_type t) : type_(t) { assert(t == value_type::undefined || t == value_type::null); }

    void destroy();

    value_type type_;
    union {
        bool b_;
        double n_;
        string s_;
        object_ptr o_;
        reference r_;
    };
};
std::ostream& operator<<(std::ostream& os, const value& v);
std::wostream& operator<<(std::wostream& os, const value& v);
bool operator==(const value& l, const value& r);
inline bool operator!=(const value& l, const value& r) {
    return !(l == r);
}

inline value get_value(const value& v) {
    return v.type() == value_type::reference ? v.reference_value().get_value() : v;
}

inline value get_value(value&& v) {
    return v.type() == value_type::reference ? v.reference_value().get_value() : std::move(v);
}

[[nodiscard]] bool put_value(const value& ref, const value& val);

// §9 Type Conversions
value to_primitive(const value& v, value_type hint = value_type::undefined);
bool to_boolean(const value& v);
double to_number(const value& v);
double to_integer(double n);
double to_integer(const value& v);
int32_t to_int32(double n);
int32_t to_int32(const value& v);
uint32_t to_uint32(double n);
uint32_t to_uint32(const value& v);
uint16_t to_uint16(double n);
uint16_t to_uint16(const value& v);
string to_string(double n);
string to_string(const value& v);

void debug_print(std::wostream& os, const value& v, int indent_incr, int max_nest = INT_MAX, int indent = 0);
std::wstring debug_string(const value& v);

[[noreturn]] void throw_runtime_error(const std::string_view& s, const char* file, int line);
[[noreturn]] void throw_runtime_error(const std::wstring_view& s, const char* file, int line);

#define THROW_RUNTIME_ERROR(msg) ::mjs::throw_runtime_error(msg, __FILE__, __LINE__)
#define NOT_IMPLEMENTED(o) do { std::wostringstream _woss; _woss << "Not implemented: " << o; THROW_RUNTIME_ERROR(_woss.str()); } while (0)



} // namespace mjs

#endif
