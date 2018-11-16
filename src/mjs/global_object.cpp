#include "global_object.h"
#include "lexer.h" // get_hex_value2/4
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

std::wstring index_string(uint32_t index) {
    wchar_t buffer[10], *p = &buffer[10];
    *--p = '\0';
    do {
        *--p = '0' + index % 10;
        index /= 10;
    } while (index);
    return std::wstring{p};
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

class array_object : public object {
public:
    friend gc_type_info_registration<array_object>;

    static constexpr const std::wstring_view length_str{L"length", 6};

    static gc_heap_ptr<array_object> make(const string& class_name, const object_ptr& prototype, uint32_t length) {
        return gc_heap::local_heap().make<array_object>(class_name, prototype, length);
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
            object::put(string{length_str}, value{static_cast<double>(new_length)});
        } else {
            object::put(name, val, attr);
            uint32_t index = to_uint32(value{name});
            if (name.view() == to_string(index).view() && index != UINT32_MAX && index >= length()) {
                object::put(string{length_str}, value{static_cast<double>(index+1)});
            }
        }
    }

    void unchecked_put(uint32_t index, const value& val) {
        const auto name = index_string(index);
        assert(index < to_uint32(get(length_str)) && can_put(name));
        object::put(string{name}, val);
    }

private:
    uint32_t length() {
        return to_uint32(object::get(length_str));
    }

    explicit array_object(const string& class_name, const object_ptr& prototype, uint32_t length) : object{class_name, prototype} {
        object::put(string{length_str}, value{static_cast<double>(length)}, property_attribute::dont_enum | property_attribute::dont_delete);
    }
};

string join(const object_ptr& o, const std::wstring_view& sep) {
    const uint32_t l = to_uint32(o->get(array_object::length_str));
    std::wstring s;
    for (uint32_t i = 0; i < l; ++i) {
        if (i) s += sep;
        const auto& oi = o->get(index_string(i));
        if (oi.type() != value_type::undefined && oi.type() != value_type::null) {
            s += to_string(oi).view();
        }
    }
    return string{s};
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

value get_arg(const std::vector<value>& args, int index) {
    return index < static_cast<int>(args.size()) ? args[index] : value::undefined;
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

    static string to_string(double t) {
        std::wostringstream woss;
        woss << "[Date: " << t << "]";
        return string{woss.str()};
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
    explicit date_object(const string& class_name, const object_ptr& prototype, double val) : object{class_name, prototype} {
        internal_value(value{val});
    }
};

class global_object_impl : public global_object {
public:
    friend gc_type_info_registration<global_object_impl>;

    const object_ptr& object_prototype() const override { return object_prototype_; }

    object_ptr make_raw_function() override {
        auto o = object::make(Function_str_, function_prototype_);
        o->put(prototype_str_, value{object::make(Object_str_, object_prototype_)}, property_attribute::dont_enum);
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
        case value_type::boolean: return new_boolean(v.boolean_value());
        case value_type::number:  return new_number(v.number_value());
        case value_type::string:  return new_string(v.string_value());
        case value_type::object:  return v.object_value();
        default:
            NOT_IMPLEMENTED(v);
        }
    }

private:
    object_ptr object_prototype_;
    object_ptr function_prototype_;
    object_ptr array_prototype_;
    object_ptr string_prototype_;
    object_ptr boolean_prototype_;
    object_ptr number_prototype_;
    object_ptr date_prototype_;
    gc_heap_ptr<global_object_impl> self_;

    // Well know, commonly used strings
#define DEFINE_STRING(x) string x ## _str_{#x}
    DEFINE_STRING(Object);
    DEFINE_STRING(Function);
    DEFINE_STRING(Array);
    DEFINE_STRING(String);
    DEFINE_STRING(Boolean);
    DEFINE_STRING(Number);
    DEFINE_STRING(Date);
    DEFINE_STRING(prototype);
    DEFINE_STRING(constructor);
    DEFINE_STRING(length);
    DEFINE_STRING(arguments);
    DEFINE_STRING(toString);
    DEFINE_STRING(valueOf);
#undef DEFINE_STRING

    // FIXME: Is this sufficient to guard against clever users?
    static void validate_type(const value& v, const object_ptr& expected_prototype, const char* expected_type) {
        if (v.type() == value_type::object && v.object_value()->prototype().get() == expected_prototype.get()) {
            return;
        }
        std::wostringstream woss;
        mjs::debug_print(woss, v, 2, 1);
        woss << " is not a " << expected_type;
        THROW_RUNTIME_ERROR(woss.str());
    }

    //
    // Function
    //

    object_ptr do_make_function(const native_function_type& f, const string& body_text, int named_args) override {
        auto o = make_raw_function();
        put_function(o, f, body_text, named_args);
        return o;
    }

    object_ptr make_function_object() {
        auto o = make_raw_function(); // Note: function constructor is added by interpreter
        o->put(prototype_str_, value{function_prototype_}, prototype_attributes);

        // §15.3.4
        function_prototype_->call_function(gc_function::make(gc_heap::local_heap(), [](const value&, const std::vector<value>&) {
            return value::undefined;
        }));
        function_prototype_->put(constructor_str_, value{o}, default_attributes);
        put_native_function(function_prototype_, toString_str_, [function_prototype = function_prototype_](const value& this_, const std::vector<value>&) {
            validate_type(this_, function_prototype, "Function");
            assert(this_.object_value()->internal_value().type() == value_type::string);
            return this_.object_value()->internal_value();
        }, 0);
        return o;
    }

    //
    // Object
    //

    object_ptr make_object_object() {
        auto o = make_function([global = self_](const value&, const std::vector<value>& args) {
            if (args.empty() || args.front().type() == value_type::undefined || args.front().type() == value_type::null) {
                auto o = object::make(global->Object_str_, global->object_prototype_);
                return value{o};
            }
            return value{global->to_object(args.front())};
        }, native_function_body(Object_str_), 1);
        o->put(prototype_str_, value{object_prototype_}, prototype_attributes);

        // §15.2.4
        object_prototype_->put(constructor_str_, value{o}, default_attributes);
        put_native_function(object_prototype_, toString_str_, [](const value& this_, const std::vector<value>&){
            return value{string{"[object "} + this_.object_value()->class_name() + string{"]"}};
        }, 0);
        put_native_function(object_prototype_, valueOf_str_, [](const value& this_, const std::vector<value>&){
            return this_;
        }, 0);
        return o;
    }

    //
    // Boolean
    //

    object_ptr new_boolean(bool val) {
        auto o = object::make(Boolean_str_, boolean_prototype_);
        o->internal_value(value{val});
        return o;
    }

    object_ptr make_boolean_object() {
        boolean_prototype_ = object::make(Boolean_str_, object_prototype_);
        boolean_prototype_->internal_value(value{false});

        auto c = make_function([global = self_](const value&, const std::vector<value>& args) {
            return value{global->new_boolean(!args.empty() && to_boolean(args.front()))};
        },  native_function_body(Boolean_str_), 1);
        c->call_function(gc_function::make(gc_heap::local_heap(), [](const value&, const std::vector<value>& args) {
            return value{!args.empty() && to_boolean(args.front())};
        }));
        c->put(prototype_str_, value{boolean_prototype_}, prototype_attributes);

        auto check_type = [p = boolean_prototype_](const value& this_) {
            validate_type(this_, p, "Boolean");
        };

        boolean_prototype_->put(constructor_str_, value{c}, default_attributes);

        put_native_function(boolean_prototype_, toString_str_, [check_type](const value& this_, const std::vector<value>&){
            check_type(this_);
            return value{string{this_.object_value()->internal_value().boolean_value() ? L"true" : L"false"}};
        }, 0);

        put_native_function(boolean_prototype_, valueOf_str_, [check_type](const value& this_, const std::vector<value>&){
            check_type(this_);
            return this_.object_value()->internal_value();
        }, 0);

        return c;
    }

    //
    // Number
    //

    object_ptr new_number(double val) {
        auto o = object::make(Number_str_, number_prototype_);
        o->internal_value(value{val});
        return o;
    }

    object_ptr make_number_object() {
        number_prototype_ = object::make(Number_str_, object_prototype_);
        number_prototype_->internal_value(value{0.});

        auto c = make_function([global = self_](const value&, const std::vector<value>& args) {
            return value{global->new_number(args.empty() ? 0.0 : to_number(args.front()))};
        }, native_function_body(Number_str_), 1);
        c->call_function(gc_function::make(gc_heap::local_heap(), [](const value&, const std::vector<value>& args) {
            return value{args.empty() ? 0.0 : to_number(args.front())};
        }));
        c->put(prototype_str_, value{number_prototype_}, prototype_attributes);
        c->put(string{"MAX_VALUE"}, value{1.7976931348623157e308}, default_attributes);
        c->put(string{"MIN_VALUE"}, value{5e-324}, default_attributes);
        c->put(string{"NaN"}, value{NAN}, default_attributes);
        c->put(string{"NEGATIVE_INFINITY"}, value{-INFINITY}, default_attributes);
        c->put(string{"POSITIVE_INFINITY"}, value{INFINITY}, default_attributes);

        auto check_type = [p = number_prototype_](const value& this_) {
            validate_type(this_, p, "Number");
        };

        number_prototype_->put(constructor_str_, value{c}, default_attributes);
        put_native_function(number_prototype_, toString_str_, [check_type](const value& this_, const std::vector<value>& args){
            check_type(this_);
            const int radix = args.empty() ? 10 : to_int32(args.front());
            if (radix < 2 || radix > 36) {
                std::wostringstream woss;
                woss << "Invalid radix in Number.toString: " << args.front();
                THROW_RUNTIME_ERROR(woss.str());
            }
            if (radix != 10) {
                NOT_IMPLEMENTED(radix);
            }
            return value{to_string(this_.object_value()->internal_value())};
        }, 1);
        put_native_function(number_prototype_, valueOf_str_,[check_type](const value& this_, const std::vector<value>&){
            check_type(this_);
            return this_.object_value()->internal_value();
        }, 0);

        return c;
    }

    //
    // String
    //


    object_ptr new_string(const string& val) {
        auto o = object::make(String_str_, string_prototype_);
        o->internal_value(value{val});
        o->put(length_str_, value{static_cast<double>(val.view().length())}, prototype_attributes);
        return o;
    }

    object_ptr make_string_object() {
        string_prototype_ = object::make(String_str_, object_prototype_);
        string_prototype_->internal_value(value{string{""}});

        auto c = make_function([global = self_](const value&, const std::vector<value>& args) {
            return value{global->new_string(args.empty() ? string{""} : to_string(args.front()))};
        }, native_function_body(String_str_), 1);
        c->call_function(gc_function::make(gc_heap::local_heap(), [](const value&, const std::vector<value>& args) {
            return value{args.empty() ? string{""} : to_string(args.front())};
        }));
        c->put(prototype_str_, value{string_prototype_}, prototype_attributes);
        put_native_function(c, string{"fromCharCode"}, [](const value&, const std::vector<value>& args){
            std::wstring s;
            for (const auto& a: args) {
                s.push_back(to_uint16(a));
            }
            return value{string{s}};
        }, 0);

        auto check_type = [p = string_prototype_](const value& this_) {
            validate_type(this_, p, "String");
        };

        string_prototype_->put(constructor_str_, value{c}, default_attributes);
        put_native_function(string_prototype_, toString_str_, [check_type](const value& this_, const std::vector<value>&){
            check_type(this_);
            return this_.object_value()->internal_value();
        }, 0);
        put_native_function(string_prototype_, valueOf_str_, [check_type](const value& this_, const std::vector<value>&){
            check_type(this_);
            return this_.object_value()->internal_value();
        }, 0);


        auto make_string_function = [global = self_](const char* name, int num_args, auto f) {
            global->put_native_function(global->string_prototype_, string{name}, [f](const value& this_, const std::vector<value>& args){
                return value{f(to_string(this_).view(), args)};
            }, num_args);
        };

        make_string_function("charAt", 1, [](const std::wstring_view& s, const std::vector<value>& args){
            const int position = to_int32(get_arg(args, 0));
            if (position < 0 || position >= static_cast<int>(s.length())) {
                return string{""};
            }
            return string{s.substr(position, 1)};
        });

        make_string_function("charCodeAt", 1, [](const std::wstring_view& s, const std::vector<value>& args){
            const int position = to_int32(get_arg(args, 0));
            if (position < 0 || position >= static_cast<int>(s.length())) {
                return static_cast<double>(NAN);
            }
            return static_cast<double>(s[position]);
        });

        make_string_function("indexOf", 2, [](const std::wstring_view& s, const std::vector<value>& args){
            const auto& search_string = to_string(get_arg(args, 0));
            const int position = to_int32(get_arg(args, 1));
            auto index = s.find(search_string.view(), position);
            return index == std::wstring_view::npos ? -1. : static_cast<double>(index);
        });

        make_string_function("lastIndexOf", 2, [](const std::wstring_view& s, const std::vector<value>& args){
            const auto& search_string = to_string(get_arg(args, 0));
            double position = to_number(get_arg(args, 1));
            const int ipos = std::isnan(position) ? INT_MAX : to_int32(position);
            auto index = s.rfind(search_string.view(), ipos);
            return index == std::wstring_view::npos ? -1. : static_cast<double>(index);
        });

        make_string_function("split", 1, [global = self_](const std::wstring_view& s, const std::vector<value>& args){
            auto a = global->array_constructor(value::null, {}).object_value();
            if (args.empty()) {
                a->put(string{index_string(0)}, value{string{s}});
            } else {
                const auto sep = to_string(args.front());
                if (sep.view().empty()) {
                    for (uint32_t i = 0; i < s.length(); ++i) {
                        a->put(string{index_string(i)}, value{string{ s.substr(i,1) }});
                    }
                } else {
                    size_t pos = 0;
                    uint32_t i = 0;
                    for (; pos < s.length(); ++i) {
                        const auto next_pos = s.find(sep.view(), pos);
                        if (next_pos == std::wstring_view::npos) {
                            break;
                        }
                        a->put(string{index_string(i)}, value{string{ s.substr(pos, next_pos-pos) }});
                        pos = next_pos + 1;
                    }
                    if (pos < s.length()) {
                        a->put(string{index_string(i)}, value{string{ s.substr(pos) }});
                    }
                }
            }
            return a;
        });

        make_string_function("substring", 1, [](const std::wstring_view& s, const std::vector<value>& args){
            int start = std::min(std::max(to_int32(get_arg(args, 0)), 0), static_cast<int>(s.length()));
            if (args.size() < 2) {
                return string{s.substr(start)};
            }
            int end = std::min(std::max(to_int32(get_arg(args, 1)), 0), static_cast<int>(s.length()));
            if (start > end) {
                std::swap(start, end);
            }
            return string{s.substr(start, end-start)};
        });

        make_string_function("toLowerCase", 0, [](const std::wstring_view& s, const std::vector<value>&){
            std::wstring res;
            for (auto c: s) {
                res.push_back(towlower(c));
            }
            return string{res};
        });

        make_string_function("toUpperCase", 0, [](const std::wstring_view& s, const std::vector<value>&){
            std::wstring res;
            for (auto c: s) {
                res.push_back(towupper(c));
            }
            return string{res};
        });

        return object_ptr{c};
    }

    //
    // Array
    //

    value array_constructor(const value&, const std::vector<value>& args) {
        if (args.size() == 1 && args[0].type() == value_type::number) {
            return value{array_object::make(Array_str_, array_prototype_, to_uint32(args[0].number_value()))};
        }
        auto arr = array_object::make(Array_str_, array_prototype_, static_cast<uint32_t>(args.size()));
        for (uint32_t i = 0; i < args.size(); ++i) {
            arr->unchecked_put(i, args[i]);
        }
        return value{arr};
    }

    object_ptr make_array_object() {
        array_prototype_ = array_object::make(Array_str_, object_prototype_, 0);

        auto o = make_function([global = self_](const value& this_, const std::vector<value>& args) {
            return global->array_constructor(this_, args);
        }, native_function_body(Array_str_), 1);
        o->put(prototype_str_, value{array_prototype_}, prototype_attributes);

        array_prototype_->put(constructor_str_, value{o}, default_attributes);
        put_native_function(array_prototype_, toString_str_, [](const value& this_, const std::vector<value>&) {
            assert(this_.type() == value_type::object);
            return value{join(this_.object_value(), L",")};
        }, 0);
        put_native_function(array_prototype_, "join", [](const value& this_, const std::vector<value>& args) {
            assert(this_.type() == value_type::object);
            string sep{L","};
            if (!args.empty()) {
                sep = to_string(args.front());
            }
            return value{join(this_.object_value(), sep.view())};
        }, 1);
        put_native_function(array_prototype_, "reverse", [](const value& this_, const std::vector<value>&) {
            assert(this_.type() == value_type::object);
            const auto& o = this_.object_value();
            const uint32_t length = to_uint32(o->get(array_object::length_str));
            for (uint32_t k = 0; k != length / 2; ++k) {
                const auto i1 = index_string(k);
                const auto i2 = index_string(length - k - 1);
                auto v1 = o->get(i1);
                auto v2 = o->get(i2);
                o->put(string{i1}, v2);
                o->put(string{i2}, v1);
            }
            return this_;
        }, 0);
        put_native_function(array_prototype_, "sort", [](const value& this_, const std::vector<value>& args) {
            assert(this_.type() == value_type::object);
            const auto& o = this_.object_value();
            const uint32_t length = to_uint32(o->get(array_object::length_str));

            native_function_type comparefn{};
            if (!args.empty()) {
                if (args.front().type() == value_type::object) {
                    comparefn = args.front().object_value()->call_function();
                }
                if (!comparefn) {
                    NOT_IMPLEMENTED("Invalid compare function given to sort");
                }
            }

            auto sort_compare = [comparefn](const value& x, const value& y) {
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
                    const auto xs = to_string(x);
                    const auto ys = to_string(y);
                    if (xs.view() < ys.view()) return -1;
                    if (xs.view() > ys.view()) return 1;
                    return 0;
                }
            };

            std::vector<value> values(length);
            for (uint32_t i = 0; i < length; ++i) {
                values[i] = o->get(index_string(i));
            }
            std::stable_sort(values.begin(), values.end(), [&](const value& x, const value& y) {
                return sort_compare(x, y) < 0;
            });
            for (uint32_t i = 0; i < length; ++i) {
                o->put(string{index_string(i)}, values[i]);
            }
            return this_;
        }, 1);
        return o;
    }

    //
    // Console
    //
    auto make_console_object() {
        auto console = object::make(Object_str_, object_prototype_);

        using timer_clock = std::chrono::steady_clock;

        auto timers = std::make_shared<std::unordered_map<std::wstring, timer_clock::time_point>>();
        put_native_function(console, "log", [](const value&, const std::vector<value>& args) {
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
        put_native_function(console, "time", [timers](const value&, const std::vector<value>& args) {
            if (args.empty()) {
                THROW_RUNTIME_ERROR("Missing argument to console.time()");
            }
            auto label = to_string(args.front());
            (*timers)[std::wstring{label.view()}] = timer_clock::now();
            return value::undefined;
        }, 1);
        put_native_function(console, "timeEnd", [timers](const value&, const std::vector<value>& args) {
            const auto end_time = timer_clock::now();
            if (args.empty()) {
                THROW_RUNTIME_ERROR("Missing argument to console.timeEnd()");
            }
            auto label = to_string(args.front());
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

        return console;
    }

    //
    // Math
    //
    auto make_math_object() {
        auto math = object::make(Object_str_, object_prototype_);

        math->put(string{"E"},       value{2.7182818284590452354}, default_attributes);
        math->put(string{"LN10"},    value{2.302585092994046}, default_attributes);
        math->put(string{"LN2"},     value{0.6931471805599453}, default_attributes);
        math->put(string{"LOG2E"},   value{1.4426950408889634}, default_attributes);
        math->put(string{"LOG10E"},  value{0.4342944819032518}, default_attributes);
        math->put(string{"PI"},      value{3.14159265358979323846}, default_attributes);
        math->put(string{"SQRT1_2"}, value{0.7071067811865476}, default_attributes);
        math->put(string{"SQRT2"},   value{1.4142135623730951}, default_attributes);


        auto make_math_function1 = [&](const char* name, auto f) {
            put_native_function(math, name, [f](const value&, const std::vector<value>& args){
                return value{f(to_number(get_arg(args, 0)))};
            }, 1);
        };
        auto make_math_function2 = [&](const char* name, auto f) {
            put_native_function(math, name, [f](const value&, const std::vector<value>& args){
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

        put_native_function(math, "random", [](const value&, const std::vector<value>&){
            return value{static_cast<double>(rand()) / (1.+RAND_MAX)};
        }, 0);

        return math;
    }

    //
    // Date
    //

    auto new_date(double val) {
        return gc_heap::local_heap().make<date_object>(Date_str_, date_prototype_, val);
    }

    auto make_date_object() {
        //const auto attr = property_attribute::dont_enum | property_attribute::dont_delete | property_attribute::read_only;

        date_prototype_ = object::make(Date_str_, object_prototype_);
        date_prototype_->internal_value(value{NAN});

        auto c = make_function([global = self_](const value&, const std::vector<value>& args) {
            if (args.empty()) {
                return value{global->new_date(date_helper::current_time_utc())};
            } else if (args.size() == 1) {
                auto v = to_primitive(args[0]);
                if (v.type() == value_type::string) {
                    // return Date.parse(v)
                    NOT_IMPLEMENTED("new Date(string)");
                }
                return value{global->new_date(to_number(v))};
            } else if (args.size() == 2) {
                NOT_IMPLEMENTED("new Date(value)");
            }
            return value{date_helper::time_clip(date_helper::utc(date_helper::time_from_args(args)))};
        }, native_function_body(Date_str_), 7);
        c->call_function(gc_function::make(gc_heap::local_heap(), [global = self_](const value&, const std::vector<value>&) {
            // Equivalent to (new Date()).toString()
            return value{to_string(value{global->new_date(date_helper::current_time_utc())})};
        }));
        c->put(prototype_str_, value{date_prototype_}, prototype_attributes);
        put_native_function(c, "parse", [](const value&, const std::vector<value>& args) {
            if (1) NOT_IMPLEMENTED(get_arg(args, 0));
            return value::undefined;
        }, 1);
        put_native_function(c, "UTC", [](const value&, const std::vector<value>& args) {
            if (args.size() < 3) {
                NOT_IMPLEMENTED("Date.UTC() with less than 3 arguments");
            }
            return value{date_helper::time_clip(date_helper::time_from_args(args))};
        }, 7);

        date_prototype_->put(constructor_str_, value{c}, default_attributes);
        // TODO: Date.parse(string)

        auto check_type = [p = date_prototype_](const value& this_) {
            validate_type(this_, p, "Date");
        };

        auto make_date_getter = [&](const char* name, auto f) {
            put_native_function(date_prototype_, name ,[f, check_type](const value& this_, const std::vector<value>&) {
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
            put_native_function(date_prototype_, name, [f, check_type](const value& this_, const std::vector<value>& args) {
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

        put_native_function(date_prototype_, "toString", [check_type](const value& this_, const std::vector<value>&) {
            check_type(this_);
            return value{date_helper::to_string(this_.object_value()->internal_value().number_value())};
        }, 0);
        // TODO: toLocaleString()
        // TODO: toUTCString()
        // TODO: toGMTString = toUTCString

        return c;
    }

    //
    // Global
    //
    void popuplate_global() {
        object_prototype_   = object::make(string{"ObjectPrototype"}, nullptr);
        function_prototype_ = object::make(Function_str_, object_prototype_);

        // §15.1
        put(Object_str_, value{make_object_object()}, default_attributes);
        put(Function_str_, value{make_function_object()}, default_attributes);
        put(Array_str_, value{make_array_object()}, default_attributes);
        put(String_str_, value{make_string_object()}, default_attributes);
        put(Boolean_str_, value{make_boolean_object()}, default_attributes);
        put(Number_str_, value{make_number_object()}, default_attributes);
        put(string{"Math"}, value{make_math_object()}, default_attributes);
        put(Date_str_, value{make_date_object()}, default_attributes);

        put(string{"NaN"}, value{NAN}, default_attributes);
        put(string{"Infinity"}, value{INFINITY}, default_attributes);
        // Note: eval is added by the interpreter
        put_native_function(*this, "parseInt", [](const value&, const std::vector<value>& args) {
            const auto input = to_string(get_arg(args, 0));
            int radix = to_int32(get_arg(args, 1));
            return value{parse_int(input.view(), radix)};
        }, 2);
        put_native_function(*this, "parseFloat", [](const value&, const std::vector<value>& args) {
            const auto input = to_string(get_arg(args, 0));
            return value{parse_float(input.view())};
        }, 1);
        put_native_function(*this, "escape", [](const value&, const std::vector<value>& args) {
            const auto input = to_string(get_arg(args, 0));
            return value{string{escape(input.view())}};
        }, 1);
        put_native_function(*this, "unescape", [](const value&, const std::vector<value>& args) {
            const auto input = to_string(get_arg(args, 0));
            return value{string{unescape(input.view())}};
        }, 1);
        put_native_function(*this, "isNaN", [](const value&, const std::vector<value>& args) {
            return value(std::isnan(to_number(args.empty() ? value::undefined : args.front())));
        }, 1);
        put_native_function(*this, "isFinite", [](const value&, const std::vector<value>& args) {
            return value(std::isfinite(to_number(args.empty() ? value::undefined : args.front())));
        }, 1);
        put_native_function(*this, "alert", [](const value&, const std::vector<value>& args) {
            std::wcout << "ALERT";
            if (!args.empty()) std::wcout << ": " << args[0];
            std::wcout << "\n";
            return value::undefined;
        }, 1);

        put(string{"console"}, value{make_console_object()}, default_attributes);
    }

    explicit global_object_impl() : global_object(string{"Global"}, object_ptr{}) {
    }

    global_object_impl(global_object_impl&& other) = default;

    bool fixup(gc_heap& new_heap) {
        object::fixup(new_heap);
        return false; // This object is lazy
    }

    void put_function(const object_ptr& o, const native_function_type& f, const string& body_text, int named_args) override;

    friend global_object;
};

gc_heap_ptr<global_object> global_object::make() {
    auto global = gc_heap::local_heap().make<global_object_impl>();
    global->self_ = global;
    global->popuplate_global(); // Populate here so the safe_ptr() won't fail the assert
    return global;
}

string global_object::native_function_body(const string& name) {
    std::wostringstream oss;
    oss << "function " << name.view() << "() { [native code] }";
    return string{oss.str()};
}

void global_object_impl::put_function(const object_ptr& o, const native_function_type& f, const string& body_text, int named_args) {
    assert(o->class_name().view() == Function_str_.view());
    assert(!o->call_function());
    assert(o->get(prototype_str_.view()).type() == value_type::object);
    o->put(length_str_, value{static_cast<double>(named_args)}, property_attribute::read_only | property_attribute::dont_delete | property_attribute::dont_enum);
    o->put(arguments_str_, value::null, property_attribute::read_only | property_attribute::dont_delete | property_attribute::dont_enum);
    o->call_function(f);
    o->construct_function(f);
    assert(o->internal_value().type() == value_type::undefined);
    o->internal_value(value{body_text});
    auto p = o->get(prototype_str_.view()).object_value();
    p->put(constructor_str_, value{o}, global_object_impl::default_attributes);
}


} // namespace mjs
