#include "value.h"
#include "object.h"
#include "function_object.h"
#include "number_to_string.h"
#include <ostream>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <cstring>

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
    case value_type::reference: return "reference";
    }
    NOT_IMPLEMENTED((int)t);
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
        case value_type::string:    new (&s_) string{rhs.s_}; break;
        case value_type::object:    new (&o_) object_ptr{rhs.o_}; break;
        case value_type::reference: new (&r_) reference{rhs.r_}; break;
        default: NOT_IMPLEMENTED(rhs.type_);
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
    case value_type::string:    new (&s_) string{std::move(rhs.s_)}; break;
    case value_type::object:    new (&o_) object_ptr{std::move(rhs.o_)}; break;
    case value_type::reference: new (&r_) reference{std::move(rhs.r_)}; break;
    default: NOT_IMPLEMENTED(rhs.type_);
    }
    type_ = rhs.type_;
    return *this;
}

void value::destroy() {
    switch (type_) {
    case value_type::undefined: break;
    case value_type::null: break;
    case value_type::boolean: break;
    case value_type::number: break;
    case value_type::string: s_.~string(); break;
    case value_type::object: o_.~gc_heap_ptr(); break;
    case value_type::reference: r_.~reference(); break;
    default: NOT_IMPLEMENTED(type_);
    }
    type_ = value_type::undefined;
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
        return lv == rv || (std::isnan(lv) && std::isnan(rv));
    }
    case value_type::string:    return l.string_value().view() == r.string_value().view();
    case value_type::object:    return l.object_value().get() == r.object_value().get();
    case value_type::reference: break;
    }
    NOT_IMPLEMENTED(l.type());
}

//
// Type Conversions
//

value to_primitive(const value& v, value_type hint) {
    if (v.type() != value_type::object) {
        return v;
    }

    auto o = v.object_value();

    // [[DefaultValue]] (Hint)
    if (hint == value_type::undefined) {
        // When hint is undefined, assume Number unless it's a Date object in which case assume String
        hint = o->default_value_type();
    }

    assert(hint == value_type::number || hint == value_type::string);
    for (int i = 0; i < 2; ++i) {
        const wchar_t* const id = (hint == value_type::string) ^ i ? L"toString" : L"valueOf";
        const auto fo = o->get(id);
        if (fo.type() != value_type::object) {
            continue;
        }
        auto res = call_function(fo, v, {});
        if (!is_primitive(res.type())) {
            continue;
        }
        return res;
    }

    throw to_primitive_failed_error{};
}

bool to_boolean(const value& v) {
    switch (v.type()) {
    case value_type::undefined: return false;
    case value_type::null:      return false;
    case value_type::boolean:   return v.boolean_value();
    case value_type::number:    return v.number_value() != 0 && !std::isnan(v.number_value());
    case value_type::string:    return !v.string_value().view().empty();
    case value_type::object:    return true;
    case value_type::reference: break;
    }
    NOT_IMPLEMENTED(v.type());
}

double to_number(const value& v) {
    switch (v.type()) {
    case value_type::undefined: return NAN;
    case value_type::null:      return +0.0;
    case value_type::boolean:   return v.boolean_value() ? 1.0 : +0.0;
    case value_type::number:    return v.number_value();
    case value_type::string:    return to_number(v.string_value());
    case value_type::object:    return to_number(to_primitive(v, value_type::number));
    case value_type::reference: break;
    }
    NOT_IMPLEMENTED(v.type());
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

string to_string(gc_heap& h, double m) {
    return string{h, number_to_string(m)};
}

string to_string(gc_heap& h, const value& v) {
    switch (v.type()) {
    case value_type::undefined: return string{h, "undefined"};
    case value_type::null:      return string{h, "null"};
    case value_type::boolean:   return string{h, v.boolean_value() ? "true" : "false"};
    case value_type::number:    return to_string(h, v.number_value());
    case value_type::string:    return v.string_value();
    case value_type::object:    return to_string(h, to_primitive(v, value_type::string));
    case value_type::reference: break;
    }
    NOT_IMPLEMENTED(v.type());
}

void debug_print(std::wostream& os, const value& v, int indent_incr, int max_nest, int indent) {
    switch (v.type()) {
    case value_type::undefined: [[fallthrough]];
    case value_type::null:
        os << v.type();
        break;
    case value_type::boolean:
        os << (v.boolean_value() ? "true" : "false");
        break;
    case value_type::number:
        os << number_to_string(v.number_value());
        break;
    case value_type::string:
        os << "'";
        for (const auto& ch: v.string_value().view()) {
            switch (ch) {
            case '\'': os << "\\'"; break;
            case '\\': os << "\\\\"; break;
            case '\b': os << "\\b"; break;
            case '\f': os << "\\f"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default:
                os << ch;
            }
        }
        os << "'";
        break;
    case value_type::object:
        v.object_value()->debug_print(os, indent_incr, max_nest, indent);
        break;
    case value_type::reference:
        os << "reference to ";
        if (auto b = v.reference_value().base()) {
            os << b->class_name();
        } else {
            os << "null";
        }
        os << " '" << v.reference_value().property_name().view() << "'";
        break;
    default:
        assert(false);
        os << "[Unhandled type " << v.type() << "]";
    }
}

std::wstring debug_string(const value& v) {
    std::wostringstream woss;
    debug_print(woss, v, 4, 0);
    return woss.str();
}

[[noreturn]] void throw_runtime_error(const std::string_view& s, const char* file, int line) {
    std::ostringstream oss;
    oss << file << ":" << line << ": " << s;
    throw std::runtime_error(oss.str());
}

[[noreturn]] void throw_runtime_error(const std::wstring_view& s, const char* file, int line) {
    throw_runtime_error(std::string(s.begin(), s.end()), file, line);
}


} // namespace mjs
