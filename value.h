#ifndef MJS_VALUE_H
#define MJS_VALUE_H

#include <iosfwd>
#include <string>
#include <string_view>
#include <cassert>
#include <stdint.h>

namespace mjs {

enum class value_type {
    undefined, null, boolean, number, string, object
};
const char* string_value(value_type t);
template<typename CharT, typename CharTraitsT>
std::basic_ostream<CharT, CharTraitsT>& operator<<(std::basic_ostream<CharT, CharTraitsT>& os, value_type t) {
    return os << string_value(t);
}

class string {
public:
    explicit string(const char* str);
    explicit string(const std::wstring_view& s) : s_(s) {}
    explicit string(std::wstring&& s) : s_(std::move(s)) {}
    string(const string& s) : s_(s.s_) {}
    string(string&& s) : s_(std::move(s.s_)) {}

    string& operator+=(const string& rhs) {
        s_ += rhs.s_;
        return *this;
    }

    std::wstring_view view() const { return s_; }
    const std::wstring& str() const { return s_; }
private:
    std::wstring s_;
};
std::ostream& operator<<(std::ostream& os, const string& s);
std::wostream& operator<<(std::wostream& os, const string& s);
inline bool operator==(const string& l, const string& r) { return l.view() == r.view(); }
inline string operator+(const string& l, const string& r) { auto res = l; return res += r; } 

double to_number(const string& s);

class value {
public:
    explicit value(bool b) : type_(value_type::boolean), b_(b) {}
    explicit value(double n) : type_(value_type::number), n_(n) {}
    explicit value(const string& s) : type_(value_type::string), s_(new string{s}) {}
    value(const value& rhs);
    value(value&& rhs);
    ~value() { destroy(); }

    value& operator=(const value& rhs);
    value& operator=(value&& rhs);

    value_type type() const { return type_; }
    bool boolean_value() const { assert(type_ == value_type::boolean); return b_; }
    double number_value() const {assert(type_ == value_type::number); return n_; }
    const string& string_value() const {assert(type_ == value_type::string); return *s_; }

    static const value undefined;
    static const value null;
private:
    explicit value(value_type t) : type_(t) { assert(t == value_type::undefined || t == value_type::null); }

    void destroy();

    value_type type_;
    union {
        bool b_;
        double n_;
        string* s_;
    };
};
std::ostream& operator<<(std::ostream& os, const value& v);
std::wostream& operator<<(std::wostream& os, const value& v);
bool operator==(const value& l, const value& r);

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
// TODO: §9.9 ToObject


} // namespace mjs

#endif
