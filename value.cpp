#include "value.h"
#include <ostream>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <cstring>

#define NOT_IMPLEMENTED() do { assert(false); throw std::runtime_error("Not implemented"); } while (0)

namespace mjs {

const value value::undefined{value_type::undefined};
const value value::null{value_type::null};

//
// value_type
//

const char* string_value(value_type t) {
    switch (t) {
    case value_type::undefined: return "undefined";
    case value_type::null:      return "null";
    case value_type::boolean:   return "boolean";
    case value_type::number:    return "number";
    case value_type::string:    return "string";
    case value_type::object:    return "object";
    }
    NOT_IMPLEMENTED();
}

//
// string
//
string::string(const char* str) : s_(str, str+std::strlen(str)) {
}

std::ostream& operator<<(std::ostream& os, const string& s) {
    auto v = s.view();
    return os << std::string(v.begin(), v.end());
}

std::wostream& operator<<(std::wostream& os, const string& s) {
    return os << s.view();
}

double to_number(const string& s) {
    // TODO: Implement real algorithm from §9.3.1 ToNumber Applied to the String Type
    std::wistringstream wis{s.str()};
    double d;
    return wis >> d ? d : NAN;
}

//
// value
//

value::value(const value& rhs) : type_(value_type::undefined) { *this = rhs; }
value::value(value&& rhs) : type_(value_type::undefined) { *this = std::move(rhs); }

value& value::operator=(const value& rhs) {
    if (this != &rhs) {
        destroy();
        switch (rhs.type_) {
        case value_type::undefined: break;
        case value_type::null:      break;
        case value_type::boolean:   b_ = rhs.b_; break;
        case value_type::number:    n_ = rhs.n_; break;
        case value_type::string:    s_ = new string{*rhs.s_}; break;
        //case value_type::object:    break;
        default:
            NOT_IMPLEMENTED();
        }
        type_ = rhs.type_;
    }
    return *this;
}
value& value::operator=(value&& rhs) {
    destroy();
    switch (rhs.type_) {
    case value_type::undefined: break;
    case value_type::null:      break;
    case value_type::boolean:   b_ = rhs.b_; break;
    case value_type::number:    n_ = rhs.n_; break;
    case value_type::string:    s_ = rhs.s_; break;
        //case value_type::object:    break;
    default:
        NOT_IMPLEMENTED();
    }
    type_ = rhs.type_;
    rhs.type_ = value_type::undefined;
    return *this;
}

void value::destroy() {
    switch (type_) {
    case value_type::undefined: break;
    case value_type::null: break;
    case value_type::boolean: break;
    case value_type::number: break;
    case value_type::string: delete s_; break;
    default:
        NOT_IMPLEMENTED();
    }
    type_ = value_type::undefined;
}

std::ostream& operator<<(std::ostream& os, const value& v) {
    return os << to_string(v);
}

std::wostream& operator<<(std::wostream& os, const value& v) {
    return os << to_string(v);
}

bool operator==(const value& l, const value& r) {
    if (l.type() != r.type()) {
        return false;
    }

    switch (l.type()) {
    case value_type::undefined: return true;
    case value_type::null:      return true;
    case value_type::boolean:   return l.boolean_value() == r.boolean_value();        
    case value_type::number: {
        const double lv = l.number_value(), rv = r.number_value();
        return std::memcmp(&lv, &rv, sizeof(lv)) == 0;
    }
    case value_type::string:    return l.string_value().view() == r.string_value().view();
    case value_type::object: break;
    }
    NOT_IMPLEMENTED();
}

//
// Type Conversions
//

value to_primitive(const value& v, value_type hint) {
    (void)hint;
    if (v.type() != value_type::object) {
        return v;
    }
    NOT_IMPLEMENTED();
}

bool to_boolean(const value& v) {
    switch (v.type()) {
    case value_type::undefined: return false;
    case value_type::null:      return false;
    case value_type::boolean:   return v.boolean_value();
    case value_type::number:    return v.number_value() != 0 && !std::isnan(v.number_value());
    case value_type::string:    return !v.string_value().view().empty();
    case value_type::object:    return true;
    }
    NOT_IMPLEMENTED();
}

double to_number(const value& v) {
    switch (v.type()) {
    case value_type::undefined: return NAN;
    case value_type::null:      return +0.0;
    case value_type::boolean:   return v.boolean_value() ? 1.0 : +0.0;
    case value_type::number:    return v.number_value();
    case value_type::string:    return to_number(v.string_value());
    case value_type::object:    return to_number(to_primitive(v, value_type::number));
    }
    NOT_IMPLEMENTED();
}

double to_integer_inner(double n) {
    return (n < 0 ? -1 : 1) * std::floor(std::abs(n));
}

double to_integer(double n) {
    if (std::isnan(n)) {
        return 0;
    }
    if (n == 0 || std::isinf(n)) {
        return n;
    }
    return to_integer_inner(n);
}

double to_integer(const value& v) {
    return to_integer(to_number(v));
}

int32_t to_int32(double n) {
    return static_cast<int32_t>(to_uint32(n));
}

int32_t to_int32(const value& v) {
    return to_int32(to_number(v));
}

uint32_t to_uint32(double n) {
    if (std::isnan(n) || n == 0 || std::isinf(n)) {
        return 0;
    }
    n = to_integer_inner(n);
    constexpr double max = 1ULL << 32;
    n = n - max * std::floor(n / max);
    assert(n >= 0.0 && n < max);
    return static_cast<uint32_t>(n);
}

uint32_t to_uint32(const value& v) {
    return to_uint32(to_number(v));
}

uint16_t to_uint16(double n) {
    return static_cast<uint16_t>(to_uint32(n));
}

uint16_t to_uint16(const value& v) {
    return to_uint16(to_number(v));
}

string to_string(double n) {
    if (std::isnan(n)) {
        return string{"NaN"};
    }
    if (n == 0) {
        return string{"0"};
    }
    if (n < 0) {
        return string{"-"} + to_string(-n);
    }
    if (std::isinf(n)) {
        return string{"Infinity"};
    }

    // TODO: Implement algorithm in 9.8.1 ToString Applied to the Number Type
    std::wostringstream woss;
    woss << n;
    return string{woss.str()};
}

string to_string(const value& v) {
    switch (v.type()) {
    case value_type::undefined: return string{"undefined"};
    case value_type::null:      return string{"null"};
    case value_type::boolean:   return string{v.boolean_value() ? "true" : "false"};
    case value_type::number:    return to_string(v.number_value());
    case value_type::string:    return v.string_value();
    case value_type::object:    break;
    }
    NOT_IMPLEMENTED();
}

} // namespace mjs
