#include "global_object.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cmath>

#include <iostream>

namespace mjs {

string index_string(uint32_t index) {
    wchar_t buffer[10], *p = &buffer[10];
    *--p = '\0';
    do {
        *--p = '0' + index % 10;
        index /= 10;
    } while (index);
    return string{p};
}

#define THROW_RUNTIME_ERROR(msg)  throw_runtime_error(msg, __FILE__, __LINE__)

[[noreturn]] void throw_runtime_error(const std::string_view& s, const char* file, int line) {
    std::ostringstream oss;
    oss << file << ":" << line << ": " << s;
    throw std::runtime_error(oss.str());
}

[[noreturn]] void throw_runtime_error(const std::wstring_view& s, const char* file, int line) {
    throw_runtime_error(std::string(s.begin(), s.end()), file, line);
}

class array_object : public object {
public:
    static std::shared_ptr<array_object> make(const object_ptr& prototype, uint32_t length) {
        struct make_shared_helper : array_object {
            make_shared_helper(const object_ptr& prototype, uint32_t length) : array_object(prototype, length) {}
        };
        return std::make_shared<make_shared_helper>(prototype, length);
    }

    void put(const string& name, const value& val, property_attribute attr) override {
        if (!can_put(name)) {
            return;
        }

        if (name.view() == L"length") {
            const uint32_t length = to_uint32(object::get(string{"length"}));
            const uint32_t new_length = to_uint32(val);
            if (new_length < length) {
                for (uint32_t i = new_length; i < length; ++i) {
                    const bool res = object::delete_property(index_string(i));
                    assert(res); (void)res;
                }
            }
            object::put(string{"length"}, value{static_cast<double>(new_length)});
        } else {
            object::put(name, val, attr);
            uint32_t index = to_uint32(value{name});
            if (name.view() == to_string(index).view() && index != UINT32_MAX && index >= to_uint32(object::get(string{"length"}))) {
                object::put(string{"length"}, value{static_cast<double>(index+1)});
            }
        }
    }

    void unchecked_put(uint32_t index, const value& val) {
        const auto name = index_string(index);
        assert(index < to_uint32(get(string{"length"})) && can_put(name));
        object::put(name, val);
    }

private:
    explicit array_object(const object_ptr& prototype, uint32_t length) : object{string{"Array"}, prototype} {
        object::put(string{"length"}, value{static_cast<double>(length)}, property_attribute::dont_enum | property_attribute::dont_delete);
    }
};

string join(const object_ptr& o, const std::wstring_view& sep) {
    const uint32_t l = to_uint32(o->get(string{"length"}));
    std::wstring s;
    for (uint32_t i = 0; i < l; ++i) {
        if (i) s += sep;
        const auto& oi = o->get(string{std::to_wstring(i)});
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


class global_object_impl : public global_object {
public:
    explicit global_object_impl() : global_object(string{"Global"}, object_ptr{}) {
        object_prototype_   = object::make(string{"ObjectPrototype"}, nullptr);
        function_prototype_ = object::make(string{"Function"}, object_prototype_);
        popuplate_global();
    }

    const object_ptr& object_prototype() const override { return object_prototype_; }

    object_ptr make_raw_function() override {
        auto o = object::make(string{"Function"}, function_prototype_);
        o->put(string{"prototype"}, value{object::make(string{"Object"}, object_prototype_)}, property_attribute::dont_enum);
        return o;
    }

    object_ptr make_function(const native_function_type& f, int named_args) override {
        auto o = make_raw_function();
        put_function(o, f, named_args);
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

    static constexpr auto prototype_attributes = property_attribute::dont_enum | property_attribute::dont_delete | property_attribute::read_only;

    static void validate_type(const value& v, const wchar_t* expected_type) {
        if (v.type() == value_type::object && v.object_value()->class_name().view() == expected_type) {
            return;
        }
        std::wostringstream woss;
        woss << v << " is not a " << expected_type;
        THROW_RUNTIME_ERROR(woss.str());
    }

    //
    // Function
    //

    value function_constructor(const value&, const std::vector<value>&) {
        NOT_IMPLEMENTED("function constructor not implemented");
    }

    object_ptr make_function_object() {
        auto o = make_function(std::bind(&global_object_impl::function_constructor, this, std::placeholders::_1, std::placeholders::_2), 1);
        o->put(string{"prototype"}, value{function_prototype_}, prototype_attributes);
        o->put(string{"length"}, value{1.0});

        // §15.3.4
        function_prototype_->call_function([](const value&, const std::vector<value>&) {
            return value::undefined;
        });
        function_prototype_->put(string{"constructor"}, value{o});
        // TODO: toString()
        return o;
    }

    //
    // Object
    //

    value object_constructor(const value&, const std::vector<value>& args) {
        if (args.empty() || args.front().type() == value_type::undefined || args.front().type() == value_type::null) {
            auto o = object::make(string{"Object"}, object_prototype_);
            return value{o};
        }
        return value{to_object(args.front())};
    }

    object_ptr make_object_object() {
        auto o = make_function(std::bind(&global_object_impl::object_constructor, this, std::placeholders::_1, std::placeholders::_2), 1);
        o->put(string{"prototype"}, value{object_prototype_}, prototype_attributes);

        // §15.2.4
        object_prototype_->put(string{"constructor"}, value{o});
        object_prototype_->put(string{"toString"}, value{make_function([](const value& this_, const std::vector<value>&){
            return value{string{"[object "} + this_.object_value()->class_name() + string{"]"}};
        }, 0)});
        object_prototype_->put(string{"valueOf"}, value{make_function([](const value& this_, const std::vector<value>&){
            return this_;
        }, 0)});
        return o;
    }

    //
    // Boolean
    //

    object_ptr new_boolean(bool val) {
        auto o = object::make(string{"Boolean"}, boolean_prototype_);
        o->internal_value(value{val});
        return o;
    }

    object_ptr make_boolean_object() {
        boolean_prototype_ = object::make(string{"Boolean"}, object_prototype_);
        boolean_prototype_->internal_value(value{false});

        auto c = make_function([this](const value&, const std::vector<value>& args) {
            return value{new_boolean(!args.empty() && to_boolean(args.front()))};
        }, 1);
        c->call_function([](const value&, const std::vector<value>& args) {
            return value{!args.empty() && to_boolean(args.front())};
        });
        c->put(string{"prototype"}, value{boolean_prototype_}, prototype_attributes);

        boolean_prototype_->put(string{"constructor"}, value{c});
        boolean_prototype_->put(string{"toString"}, value{make_function([](const value& this_, const std::vector<value>&){
            validate_type(this_, L"Boolean");
            return value{string{this_.object_value()->internal_value().boolean_value() ? L"true" : L"false"}};
        }, 0)});
        boolean_prototype_->put(string{"valueOf"}, value{make_function([](const value& this_, const std::vector<value>&){
            validate_type(this_, L"Boolean");
            return this_.object_value()->internal_value();
        }, 0)});

        return c;
    }

    //
    // Number
    //

    object_ptr new_number(double val) {
        auto o = object::make(string{"Number"}, number_prototype_);
        o->internal_value(value{val});
        return o;
    }

    object_ptr make_number_object() {
        number_prototype_ = object::make(string{"Number"}, object_prototype_);
        number_prototype_->internal_value(value{0.});

        auto c = make_function([this](const value&, const std::vector<value>& args) {
            return value{new_number(args.empty() ? 0.0 : to_number(args.front()))};
        }, 1);
        c->call_function([](const value&, const std::vector<value>& args) {
            return value{args.empty() ? 0.0 : to_number(args.front())};
        });
        c->put(string{L"prototype"}, value{number_prototype_}, prototype_attributes);
        c->put(string{"MAX_VALUE"}, value{1.7976931348623157e308});
        c->put(string{"MIN_VALUE"}, value{5e-324});
        c->put(string{"NaN"}, value{NAN});
        c->put(string{"NEGATIVE_INFINITY"}, value{-INFINITY});
        c->put(string{"POSITIVE_INFINITY"}, value{INFINITY});

        number_prototype_->put(string{L"constructor"}, value{c});
        number_prototype_->put(string{"toString"}, value{make_function([](const value& this_, const std::vector<value>& args){
            validate_type(this_, L"Number");
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
        }, 1)});
        number_prototype_->put(string{"valueOf"}, value{make_function([](const value& this_, const std::vector<value>&){
            validate_type(this_, L"Number");
            return this_.object_value()->internal_value();
        }, 0)});

        return c;
    }

    //
    // String
    //


    object_ptr new_string(const string& val) {
        auto o = object::make(string{"String"}, string_prototype_);
        o->internal_value(value{val});
        o->put(string{"length"}, value{static_cast<double>(val.view().length())}, prototype_attributes);
        return o;
    }

    static value get_arg(const std::vector<value>& args, int index) {
        return index < static_cast<int>(args.size()) ? args[index] : value::undefined;
    }

    object_ptr make_string_object() {
        string_prototype_ = object::make(string{"String"}, object_prototype_);
        string_prototype_->internal_value(value{string{""}});

        auto c = make_function([this](const value&, const std::vector<value>& args) {
            return value{new_string(args.empty() ? string{""} : to_string(args.front()))};
        }, 1);
        c->call_function([](const value&, const std::vector<value>& args) {
            return value{args.empty() ? string{""} : to_string(args.front())};
        });
        c->put(string{"prototype"}, value{string_prototype_}, prototype_attributes);
        c->put(string{"fromCharCode"}, value{make_function([](const value&, const std::vector<value>& args){
            std::wstring s;
            for (const auto& a: args) {
                s.push_back(to_uint16(a));
            }
            return value{string{s}};
        }, 0)});

        string_prototype_->put(string{"constructor"}, value{c});
        string_prototype_->put(string{"toString"}, value{make_function([](const value& this_, const std::vector<value>&){
            validate_type(this_, L"String");
            return this_.object_value()->internal_value();
        }, 0)});
        string_prototype_->put(string{"valueOf"}, value{make_function([](const value& this_, const std::vector<value>&){
            validate_type(this_, L"String");
            return this_.object_value()->internal_value();
        }, 0)});


        auto make_string_function = [this](const char* name, int num_args, auto f) {
            string_prototype_->put(string{name}, value{make_function([f](const value& this_, const std::vector<value>& args){
                return value{f(to_string(this_).view(), args)};
            }, num_args)});
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

        make_string_function("split", 1, [this](const std::wstring_view& s, const std::vector<value>& args){
            auto a = array_constructor(value::null, {}).object_value();
            if (args.empty()) {
                a->put(index_string(0), value{string{s}});
            } else {
                const auto sep = to_string(args.front());
                if (sep.view().empty()) {
                    for (uint32_t i = 0; i < s.length(); ++i) {
                        a->put(index_string(i), value{string{ s.substr(i,1) }});
                    }
                } else {
                    size_t pos = 0;
                    uint32_t i = 0;
                    for (; pos < s.length(); ++i) {
                        const auto next_pos = s.find(sep.view(), pos);
                        if (next_pos == std::wstring_view::npos) {
                            break;
                        }
                        a->put(index_string(i), value{string{ s.substr(pos, next_pos-pos) }});
                        pos = next_pos + 1;
                    }
                    if (pos < s.length()) {
                        a->put(index_string(i), value{string{ s.substr(pos) }});
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
            return value{array_object::make(array_prototype_, to_uint32(args[0].number_value()))};
        }
        auto arr = array_object::make(array_prototype_, static_cast<uint32_t>(args.size()));
        for (uint32_t i = 0; i < args.size(); ++i) {
            arr->unchecked_put(i, args[i]);
        }
        return value{arr};
    }

    object_ptr make_array_object() {
        array_prototype_ = array_object::make(object_prototype_, 0);

        auto o = make_function(std::bind(&global_object_impl::array_constructor, this, std::placeholders::_1, std::placeholders::_2), 1);
        o->put(string{"prototype"}, value{array_prototype_}, prototype_attributes);

        array_prototype_->put(string{"constructor"}, value{o});
        array_prototype_->put(string{"toString"}, value{make_function([](const value& this_, const std::vector<value>&) {
            assert(this_.type() == value_type::object);
            return value{join(this_.object_value(), L",")};
        }, 0)});
        array_prototype_->put(string{"join"}, value{make_function([](const value& this_, const std::vector<value>& args) {
            assert(this_.type() == value_type::object);
            string sep{L","};
            if (!args.empty()) {
                sep = to_string(args.front());
            }
            return value{join(this_.object_value(), sep.view())};
        }, 1)});
        array_prototype_->put(string{"reverse"}, value{make_function([](const value& this_, const std::vector<value>&) {
            assert(this_.type() == value_type::object);
            const auto& o = this_.object_value();
            const uint32_t length = to_uint32(o->get(string{L"length"}));
            for (uint32_t k = 0; k != length / 2; ++k) {
                const auto i1 = index_string(k);
                const auto i2 = index_string(length - k - 1);
                auto v1 = o->get(i1);
                auto v2 = o->get(i2);
                o->put(i1, v2);
                o->put(i2, v1);
            }
            return this_;
        }, 0)});
        array_prototype_->put(string{"sort"}, value{make_function([](const value& this_, const std::vector<value>& args) {
            assert(this_.type() == value_type::object);
            const auto& o = this_.object_value();
            const uint32_t length = to_uint32(o->get(string{L"length"}));

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
                    const auto r = to_number(comparefn(value::null, {x,y}));
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
                o->put(index_string(i), values[i]);
            }
            return this_;
        }, 1)});
        return o;
    }

    //
    // Console
    //
    auto make_console_object() {
        const auto attr = property_attribute::dont_enum;
        auto console = object::make(string{"Object"}, object_prototype_);

        using timer_clock = std::chrono::steady_clock;

        auto timers = std::make_shared<std::unordered_map<std::wstring, timer_clock::time_point>>();
        console->put(string{"log"}, value{make_function(
            [](const value&, const std::vector<value>& args) {
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
        }, 1)}, attr);
        console->put(string{"time"}, value{make_function(
            [timers](const value&, const std::vector<value>& args) {
            if (args.empty()) {
                THROW_RUNTIME_ERROR("Missing argument to console.time()");
            }
            auto label = to_string(args.front());
            (*timers)[label.str()] = timer_clock::now();
            return value::undefined; 
        }, 1)}, attr);
        console->put(string{"timeEnd"}, value{make_function(
            [timers](const value&, const std::vector<value>& args) {
            const auto end_time = timer_clock::now();
            if (args.empty()) {
                THROW_RUNTIME_ERROR("Missing argument to console.timeEnd()");
            }
            auto label = to_string(args.front());
            auto it = timers->find(label.str());
            if (it == timers->end()) {
                std::wostringstream woss;
                woss << "Timer not found: " << label;
                THROW_RUNTIME_ERROR(woss.str());
            }
            std::wcout << "timeEnd " << label << ": " << show_duration(end_time - it->second) << "\n";
            timers->erase(it);
            return value::undefined; 
        }, 1)}, attr);

        return console;
    }


    //
    // Global
    //
    void popuplate_global() {
        // §15.1
        const auto attr = property_attribute::dont_enum;
        put(string{"Object"}, value{make_object_object()}, attr);
        put(string{"Function"}, value{make_function_object()}, attr);
        put(string{"Array"}, value{make_array_object()}, attr);
        put(string{"String"}, value{make_string_object()}, attr);
        put(string{"Boolean"}, value{make_boolean_object()}, attr);
        put(string{"Number"}, value{make_number_object()}, attr);

        put(string{"NaN"}, value{NAN}, attr);
        put(string{"Infinity"}, value{INFINITY}, attr);
        // Note: eval is added by the eval visitor
        put(string{"isNaN"}, value{make_function(
            [](const value&, const std::vector<value>& args) {
            return value(std::isnan(to_number(args.empty() ? value::undefined : args.front())));
        }, 1)}, attr);
        put(string{"isFinite"}, value{make_function(
            [](const value&, const std::vector<value>& args) {
            return value(std::isfinite(to_number(args.empty() ? value::undefined : args.front())));
        }, 1)}, attr);
        put(string{"alert"}, value{make_function(
            [](const value&, const std::vector<value>& args) {
            std::wcout << "ALERT";
            if (!args.empty()) std::wcout << ": " << args[0];
            std::wcout << "\n";
            return value::undefined; 
        }, 1)}, attr);

        put(string{"console"}, value{make_console_object()}, attr);
    }
};

std::shared_ptr<global_object> global_object::make() {
    return std::make_shared<global_object_impl>();
}

void global_object::put_function(const object_ptr& o, const native_function_type& f, int named_args) {
    assert(o->class_name().str() == L"Function");
    assert(!o->call_function());
    assert(o->get(string{"prototype"}).type() == value_type::object);
    o->put(string{"length"}, value{static_cast<double>(named_args)}, property_attribute::read_only | property_attribute::dont_delete | property_attribute::dont_enum);
    o->put(string{"arguments"}, value::null, property_attribute::read_only | property_attribute::dont_delete | property_attribute::dont_enum);
    o->call_function(f);
    o->construct_function(f);
    auto& p = o->get(string{"prototype"}).object_value();
    p->put(string{"constructor"}, value{o}, property_attribute::dont_enum);
}

} // namespace mjs
