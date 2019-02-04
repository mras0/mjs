#include "date_object.h"
#include "function_object.h"
#include "error_object.h"
#include <sstream>
#include <ctime>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <iomanip>

#include <iostream> ///TEMP

#ifndef _WIN32
// TODO: Get rid of this stuff alltogether
#define _mkgmtime timegm
#endif

namespace mjs {

namespace {

struct date_time {
    double year, month, day;
    double h, m, s, ms;
};

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
        return std::floor(ms_per_second * std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::system_clock::now().time_since_epoch()).count());
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

    static double time_from_tm(std::tm& tm) {
        return static_cast<double>(_mkgmtime(&tm)) * ms_per_second;
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

    static constexpr const wchar_t* const invalid_date_string = L"Invalid Date";
    static constexpr const wchar_t* const time_format = L"%a %b %d %Y %H:%M:%S";
    static constexpr const wchar_t* const iso_format = L"%Y-%m-%dT%H:%M:%S";

    static double parse(const std::wstring_view s) {
        std::wistringstream wiss{std::wstring{s}};
        std::tm tm;
        if (wiss >> std::get_time(&tm, iso_format)) {
            auto t = time_from_tm(tm);
            if (wiss.peek() == '.') {
                wiss.get();
                const int num_digits = 3;
                wchar_t digits[num_digits];
                if (!wiss.read(digits, num_digits) || !std::all_of(digits, digits+num_digits, isdigit)) {
                    return NAN;
                }
                t += (digits[0]-'0')*100 + (digits[1]-'0')*10 + (digits[2]-'0');
            }
            if (wiss.peek() == 'Z') {
                wiss.get();
            }
            return !wiss.rdbuf()->in_avail() ? t : NAN;
        } 
        wiss.seekg(0);
        wiss.clear();
        assert(wiss);
        if (wiss >> std::get_time(&tm, time_format)) {
            return time_from_tm(tm);
        }
        return NAN;
    }

    static string to_string(gc_heap& h, double t) {
        if (!std::isfinite(t)) return string{h, invalid_date_string};
        const auto tm = tm_from_time(t);
        std::wostringstream woss;
        woss << std::put_time(&tm, time_format) << " UTC";
        return string{h, woss.str()};
    }

    static string to_date_string(gc_heap& h, double t) {
        if (!std::isfinite(t)) return string{h, invalid_date_string};
        const auto tm = tm_from_time(t);
        std::wostringstream woss;
        woss << std::put_time(&tm, L"%a %b %d %Y");
        return string{h, woss.str()};
    }

    static string to_time_string(gc_heap& h, double t) {
        if (!std::isfinite(t)) return string{h, invalid_date_string};
        const auto tm = tm_from_time(t);
        std::wostringstream woss;
        woss << std::put_time(&tm, L"%H:%M:%S");
        return string{h, woss.str()};
    }

    static string to_iso_string(const gc_heap_ptr<global_object>& global, double t) {
        if (!std::isfinite(t)) {
            throw native_error_exception{native_error_type::range, global->stack_trace(), invalid_date_string};
        }
        const auto tm = tm_from_time(t);
        std::wostringstream woss;
        woss << std::put_time(&tm, iso_format) << '.' << std::setw(3) << std::setfill(L'0') << ms_from_time(t) << 'Z';
        return string{global.heap(), woss.str()};
    }

    static value to_json(const gc_heap_ptr<global_object>& global, const value& val) {
        auto o = global->to_object(val);
        auto tv = to_primitive(value{o}, value_type::number);
        if (tv.type() == value_type::number && !std::isfinite(tv.number_value())) {
            return value::null;
        }
        auto to_iso = o->get(L"toISOString");
        return call_function(to_iso, value{o}, {});
    }

    static double utc(double t) {
        // TODO: subtract local time adjustment
        return t;
    }
    
    static double local_time(double t) {
        // TODO: add local time adjustment
        return t;
    }

    static date_time date_time_from_time(double t) {
        const auto tm = tm_from_time(t);
        return {
            static_cast<double>(tm.tm_year+1900), static_cast<double>(tm.tm_mon), static_cast<double>(tm.tm_mday),
            static_cast<double>(tm.tm_hour), static_cast<double>(tm.tm_min), static_cast<double>(tm.tm_sec), static_cast<double>(ms_from_time(t))
        };
    }

    static double time_from_date_time(const date_time& dt) {
        return make_date(make_day(dt.year, dt.month, dt.day), make_time(dt.h, dt.m, dt.s, dt.ms));
    }
};

} // unnamed namespace

class date_object : public object {
public:
    friend gc_type_info_registration<date_object>;

    value_type default_value_type() const override {
        // When hint is undefined, assume Number unless it's a Date object in which case assume String
        return value_type::string;
    }

    double date_value() const {
        return value_;
    }

    void date_value(double val) {
        value_ = val;
    }

private:
    double value_;

    explicit date_object(const string& class_name, const object_ptr& prototype, double val) : object{class_name, prototype}, value_{val} {
    }

    void do_debug_print_extra(std::wostream& os, int, int, int indent) const override {
        char buffer[32] = "Invalid Date";
        if (!std::isnan(value_)) {
            std::snprintf(buffer, sizeof(buffer), "%.17g", value_);
        }
        os << std::wstring(indent, ' ') << "[[Value]]: " << buffer << "\n";
    }
};

//
// Date
//

global_object_create_result make_date_object(const gc_heap_ptr<global_object>& global) {
    auto& h = global.heap();

    auto prototype = h.make<date_object>(string{h, "Date"}, global->object_prototype(), NAN);

    auto new_date = [prototype](double val) {
        return prototype.heap().make<date_object>(prototype->class_name(), prototype, val);
    };

    auto c = make_function(global, [prototype, new_date](const value&, const std::vector<value>&) {
        // Equivalent to (new Date()).toString()
        return value{to_string(prototype.heap(), value{new_date(date_helper::current_time_utc())})};
    }, prototype->class_name().unsafe_raw_get(), 7);
    make_constructable(global, c, [new_date](const value&, const std::vector<value>& args) {
        if (args.empty()) {
            return value{new_date(date_helper::current_time_utc())};
        } else if (args.size() == 1) {
            auto v = to_primitive(args[0]);
            if (v.type() == value_type::string) {
                // return Date.parse(v)
                return value{new_date(date_helper::parse(v.string_value().view()))};
            }
            return value{new_date(to_number(v))};
        } else if (args.size() == 2) {
            NOT_IMPLEMENTED("new Date(value)");
        }
        return value{date_helper::time_clip(date_helper::utc(date_helper::time_from_args(args)))};
    });

    prototype->put(global->common_string("constructor"), value{c}, global_object::default_attributes);

    put_native_function(global, c, "parse", [&h](const value&, const std::vector<value>& args) {
        if (!args.empty()) {
            const auto s = to_string(h, args.front());
            return value{date_helper::parse(s.view())};
        }
        return value{NAN};
    }, 1);
    put_native_function(global, c, "UTC", [](const value&, const std::vector<value>& args) {
        if (args.size() < 3) {
            NOT_IMPLEMENTED("Date.UTC() with less than 3 arguments");
        }
        return value{date_helper::time_clip(date_helper::time_from_args(args))};
    }, 7);

    auto get_data_object = [global, prototype](const value& this_) {
        global->validate_type(this_, prototype, "Date");
        return gc_heap_ptr<date_object>{this_.object_value()};
    };

    auto make_date_getter = [&](const char* name, auto f) {
        put_native_function(global, prototype, name ,[f, get_data_object](const value& this_, const std::vector<value>&) {
            const double t = get_data_object(this_)->date_value();
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
        put_native_function(global, prototype, name, [f, get_data_object](const value& this_, const std::vector<value>& args) {
            auto d = get_data_object(this_);
            const auto v = f(d->date_value(), !args.empty() ? to_number(args[0]) : NAN, args);
            d->date_value(v);
            return value{v};
        }, 0);
    };

    auto copy_func = [&](const wchar_t* from, const wchar_t* to) {
        prototype->put(string{h, to}, prototype->get(from));
    };

    using field_ptr = double date_time::*;

    auto make_simple_date_mutator = [&](const char* name, field_ptr field0, field_ptr field1 = nullptr, field_ptr field2 = nullptr, field_ptr field3 = nullptr) {
        make_date_mutator(name, [field0, field1, field2, field3](double current, double arg, const std::vector<value>& args) {
            auto dt = date_helper::date_time_from_time(current);
            dt.*field0 = arg;
            if (field1 && args.size() > 1) dt.*field1 = to_number(args[1]);
            if (field2 && args.size() > 2) dt.*field2 = to_number(args[2]);
            if (field3 && args.size() > 3) dt.*field3 = to_number(args[3]);
            return date_helper::time_from_date_time(dt);
        });
        const std::wstring from{name, name+std::strlen(name)};
        assert(from.compare(0, 3, L"set") == 0);
        copy_func(from.c_str(), (L"setUTC" + from.substr(3)).c_str());
    };

    make_date_mutator("setTime", [](double, double arg, const std::vector<value>&) {
        return arg;
    });

    make_simple_date_mutator("setMilliseconds", &date_time::ms);
    make_simple_date_mutator("setSeconds", &date_time::s, &date_time::ms);
    make_simple_date_mutator("setMinutes", &date_time::m, &date_time::s, &date_time::ms);
    make_simple_date_mutator("setHours", &date_time::h, &date_time::m, &date_time::s, &date_time::ms);
    make_simple_date_mutator("setDate", &date_time::day);
    make_simple_date_mutator("setMonth", &date_time::month, &date_time::day);
    make_simple_date_mutator("setFullYear", &date_time::year, &date_time::month, &date_time::day);

    make_date_mutator("setYear", [](double current, double arg, const std::vector<value>&) {
        auto dt = date_helper::date_time_from_time(current);
        dt.year = arg + 1900;
        return date_helper::time_from_date_time(dt);
    });

    put_native_function(global, prototype, "toString", [get_data_object](const value& this_, const std::vector<value>&) {
        auto o = get_data_object(this_);
        return value{date_helper::to_string(o.heap(), o->date_value())};
    }, 0);

    copy_func(L"toString", L"toLocaleString");
    copy_func(L"toString", L"toUTCString");
    copy_func(L"toString", L"toGMTString");

    if (global->language_version() >= version::es3) {
        put_native_function(global, prototype, "toTimeString", [get_data_object](const value& this_, const std::vector<value>&) {
            auto o = get_data_object(this_);
            return value{date_helper::to_time_string(o.heap(), o->date_value())};
        }, 0);

        put_native_function(global, prototype, "toDateString", [get_data_object](const value& this_, const std::vector<value>&) {
            auto o = get_data_object(this_);
            return value{date_helper::to_date_string(o.heap(), o->date_value())};
        }, 0);

        copy_func(L"toTimeString", L"toLocaleTimeString");
        copy_func(L"toDateString", L"toLocaleDateString");
    }

    if (global->language_version() >= version::es5) {
        put_native_function(global, c, "now", [](const value&, const std::vector<value>&) {
            return value{date_helper::current_time_utc()};
        }, 0);

        put_native_function(global, prototype, "toISOString", [get_data_object, global](const value& this_, const std::vector<value>&) {
            auto o = get_data_object(this_);
            return value{date_helper::to_iso_string(global, o->date_value())};
        }, 0);

        put_native_function(global, prototype, "toJSON", [global](const value& this_, const std::vector<value>&) {
            return date_helper::to_json(global, this_);
        }, 1);
    }

    return { c, prototype };
}

} // namespace mjs
