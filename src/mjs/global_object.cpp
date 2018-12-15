#include "global_object.h"
#include "lexer.h" // get_hex_value2/4
#include "gc_vector.h"
#include "array_object.h"
#include "native_object.h"
#include "function_object.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <memory>

#ifndef _WIN32
// TODO: Get rid of this stuff alltogether
#define _mkgmtime timegm
#endif

namespace mjs {

namespace {

value get_arg(const std::vector<value>& args, int index) {
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
        put_native_function(global, prototype, "hasOwnProperty", [](const value& this_, const std::vector<value>& args) {
            assert(this_.type() == value_type::object);
            const auto& o = this_.object_value();
            return value{o->check_own_property_attribute(to_string(o.heap(), get_arg(args, 0)).view(), property_attribute::none, property_attribute::none)};
        }, 1);
        put_native_function(global, prototype, "propertyIsEnumerable", [](const value& this_, const std::vector<value>& args) {
            assert(this_.type() == value_type::object);
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
    }, prototype->class_name().unsafe_raw_get(), nullptr, 1);
    o->default_construct_function();

    // §15.2.4
    prototype->put(global.common_string("constructor"), value{o}, global_object::default_attributes);

    add_object_prototype_functions(global, prototype);

    return { o, prototype };
}

//
// String
//

class string_object : public native_object {
public:

private:
    friend gc_type_info_registration<string_object>;

    value get_length() const {
        return value{static_cast<double>(internal_value().string_value().view().length())};
    }

    explicit string_object(const object_ptr& prototype, const string& val) : native_object(prototype->class_name(), prototype) {
        DEFINE_NATIVE_PROPERTY_READONLY(string_object, length);
        internal_value(value{val});
    }
};

create_result make_string_object(global_object& global) {
    auto& h = global.heap();
    auto String_str_ = global.common_string("String");
    auto prototype = h.make<object>(String_str_, global.object_prototype());
    prototype->internal_value(value{string{h, ""}});

    auto c = make_function(global, [&h](const value&, const std::vector<value>& args) {
        return value{args.empty() ? string{h, ""} : to_string(h, args.front())};
    }, String_str_.unsafe_raw_get(), nullptr, 1);
    make_constructable(global, c, [prototype](const value&, const std::vector<value>& args) {
        return value{prototype.heap().make<string_object>(prototype, args.empty() ? string{prototype.heap(), ""} : to_string(prototype.heap(), args.front()))};
    });

    put_native_function(global, c, string{h, "fromCharCode"}, [&h](const value&, const std::vector<value>& args){
        std::wstring s;
        for (const auto& a: args) {
            s.push_back(to_uint16(a));
        }
        return value{string{h, s}};
    }, 0);

    auto check_type = [prototype](const value& this_) {
        validate_type(this_, prototype, "String");
    };

    put_native_function(global, prototype, global.common_string("toString"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return this_.object_value()->internal_value();
    }, 0);
    put_native_function(global, prototype, global.common_string("valueOf"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return this_.object_value()->internal_value();
    }, 0);


    auto make_string_function = [&](const char* name, int num_args, auto f) {
        put_native_function(global, prototype, string{h, name}, [&h, f](const value& this_, const std::vector<value>& args){
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
        auto a = make_array(global->array_prototype(), {});
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

    auto c = make_function(global, [](const value&, const std::vector<value>& args) {
        return value{!args.empty() && to_boolean(args.front())};
    },  bool_str.unsafe_raw_get(), nullptr, 1);
    make_constructable(global, c, [prototype](const value&, const std::vector<value>& args) {
        return value{new_boolean(prototype, !args.empty() && to_boolean(args.front()))};
    });

    auto check_type = [prototype](const value& this_) {
        validate_type(this_, prototype, "Boolean");
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

    auto c = make_function(global, [](const value&, const std::vector<value>& args) {
        return value{args.empty() ? 0.0 : to_number(args.front())};
    }, Number_str_.unsafe_raw_get(), nullptr, 1);
    make_constructable(global, c, [prototype](const value&, const std::vector<value>& args) {
        return value{new_number(prototype, args.empty() ? 0.0 : to_number(args.front()))};
    });

    c->put(string{h, "MAX_VALUE"}, value{1.7976931348623157e308}, global_object::default_attributes);
    c->put(string{h, "MIN_VALUE"}, value{5e-324}, global_object::default_attributes);
    c->put(string{h, "NaN"}, value{NAN}, global_object::default_attributes);
    c->put(string{h, "NEGATIVE_INFINITY"}, value{-INFINITY}, global_object::default_attributes);
    c->put(string{h, "POSITIVE_INFINITY"}, value{INFINITY}, global_object::default_attributes);

    auto check_type = [prototype](const value& this_) {
        validate_type(this_, prototype, "Number");
    };

    put_native_function(global, prototype, global.common_string("toString"), [check_type](const value& this_, const std::vector<value>& args){
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

    auto c = make_function(global, [prototype, new_date](const value&, const std::vector<value>&) {
        // Equivalent to (new Date()).toString()
        return value{to_string(prototype.heap(), value{new_date(date_helper::current_time_utc())})};
    }, Date_str_.unsafe_raw_get(), nullptr, 7);
    make_constructable(global, c, [new_date](const value&, const std::vector<value>& args) {
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
    });

    put_native_function(global, c, "parse", [&h](const value&, const std::vector<value>& args) {
        if (1) NOT_IMPLEMENTED(to_string(h, get_arg(args, 0)));
        return value::undefined;
    }, 1);
    put_native_function(global, c, "UTC", [](const value&, const std::vector<value>& args) {
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
        put_native_function(global, prototype, name ,[f, check_type](const value& this_, const std::vector<value>&) {
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
        put_native_function(global, prototype, name, [f, check_type](const value& this_, const std::vector<value>& args) {
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

    put_native_function(global, prototype, "toString", [check_type](const value& this_, const std::vector<value>&) {
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
//static_assert(!gc_type_info_registration<string_cache>::needs_destroy); // FIXME
#endif
static_assert(!gc_type_info_registration<string_cache>::needs_fixup);

} // unnamed namespace

class global_object_impl : public global_object {
public:
    friend gc_type_info_registration<global_object_impl>;

    object_ptr object_prototype() const override { return object_prototype_.track(heap()); }
    object_ptr function_prototype() const override { return function_prototype_.track(heap()); }
    object_ptr array_prototype() const override { return array_prototype_.track(heap()); }

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
        case value_type::boolean: return new_boolean(boolean_prototype_.track(heap()), v.boolean_value());
        case value_type::number:  return new_number(number_prototype_.track(heap()), v.number_value());
        case value_type::string:  return heap().make<string_object>(string_prototype_.track(heap()), v.string_value());
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

        assert(!get(L"Object").object_value()->can_put(L"prototype"));

        auto self = self_ptr();

        put(string{heap(), "NaN"}, value{NAN}, default_attributes);
        put(string{heap(), "Infinity"}, value{INFINITY}, default_attributes);
        // Note: eval is added by the interpreter
        put_native_function(*this, self, "parseInt", [&h=heap()](const value&, const std::vector<value>& args) {
            const auto input = to_string(h, get_arg(args, 0));
            int radix = to_int32(get_arg(args, 1));
            return value{parse_int(input.view(), radix)};
        }, 2);
        put_native_function(*this, self, "parseFloat", [&h=heap()](const value&, const std::vector<value>& args) {
            const auto input = to_string(h, get_arg(args, 0));
            return value{parse_float(input.view())};
        }, 1);
        put_native_function(*this, self, "escape", [&h=heap()](const value&, const std::vector<value>& args) {
            const auto input = to_string(h, get_arg(args, 0));
            return value{string{h, escape(input.view())}};
        }, 1);
        put_native_function(*this, self, "unescape", [&h=heap()](const value&, const std::vector<value>& args) {
            const auto input = to_string(h, get_arg(args, 0));
            return value{string{h, unescape(input.view())}};
        }, 1);
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
    wchar_t buffer[10], *p = &buffer[10];
    *--p = '\0';
    do {
        *--p = '0' + index % 10;
        index /= 10;
    } while (index);
    return std::wstring{p};
}

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

} // namespace mjs
