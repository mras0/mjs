#include "number_object.h"
#include "function_object.h"
#include "error_object.h"
#include "number_to_string.h"
#include <cmath>
#include <sstream>

namespace mjs {

namespace {

int get_int_arg(const std::vector<value>& args) {
    return static_cast<int>(args.empty() ? /*undefined->0*/ 0 : to_integer(args.front()));
}

} // unnamed namespace

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
    }, Number_str_.unsafe_raw_get(), 1);
    make_constructable(global, c, [prototype](const value&, const std::vector<value>& args) {
        return value{new_number(prototype, args.empty() ? 0.0 : to_number(args.front()))};
    });

    c->put(string{h, "MAX_VALUE"}, value{1.7976931348623157e308}, global_object::default_attributes);
    c->put(string{h, "MIN_VALUE"}, value{5e-324}, global_object::default_attributes);
    c->put(string{h, "NaN"}, value{NAN}, global_object::default_attributes);
    c->put(string{h, "NEGATIVE_INFINITY"}, value{-INFINITY}, global_object::default_attributes);
    c->put(string{h, "POSITIVE_INFINITY"}, value{INFINITY}, global_object::default_attributes);

    auto make_number_function = [&](const char* name, int num_args, auto f) {
        put_native_function(global, prototype, string{h, name}, [prototype, global = global.self_ptr(), f](const value& this_, const std::vector<value>& args){
            global->validate_type(this_, prototype, "Number");
            return value{f(this_.object_value()->internal_value().number_value(), args)};
        }, num_args);
    };


    make_number_function("toString", 1, [&h](double num, const std::vector<value>& args) {
        const int radix = args.empty() ? 10 : to_int32(args.front());
        if (radix < 2 || radix > 36) {
            std::wostringstream woss;
            woss << "Invalid radix in Number.toString: " << to_string(h, args.front());
            THROW_RUNTIME_ERROR(woss.str());
        }
        if (radix != 10) {
            NOT_IMPLEMENTED(radix);
        }
        return to_string(h, num);
    });
    make_number_function("valueOf", 0, [](double num, const std::vector<value>&) {
        return num;
    });

    if (global.language_version() >= version::es3) {
        make_number_function("toLocaleString", 0, [&h](double num, const std::vector<value>&) {
            return to_string(h, num);
        });
        make_number_function("toFixed", 1, [global = global.self_ptr()](double num, const std::vector<value>& args) {
            const auto f = get_int_arg(args);
            if (f < 0 || f > 20) {
                throw native_error_exception{native_error_type::range, global->stack_trace(), L"fractionDigits out of range in Number.toFixed()"};
            }
            auto& h = global.heap();
            if (std::isnan(num) || std::fabs(num) >= 1e21) {
                return to_string(h, num);
            }
            return string{h, number_to_fixed(num, f)};
        });
        make_number_function("toExponential", 1, [global = global.self_ptr()](double num, const std::vector<value>& args) {
            const auto f = get_int_arg(args);
            if (f < 0 || f > 20) {
                throw native_error_exception{native_error_type::range, global->stack_trace(), L"fractionDigits out of range in Number.toExponential()"};
            }
            auto& h = global.heap();
            if (!std::isfinite(num)) {
                return to_string(h, num);
            }
            return string{h, number_to_exponential(num, f)};
        });
        make_number_function("toPrecision", 1, [global = global.self_ptr()](double num, const std::vector<value>& args) {
            auto& h = global.heap();
            if (args.empty()) {
                return to_string(h, num);
            }
            const auto p = get_int_arg(args);
            if (p < 1 || p > 21) {
                throw native_error_exception{native_error_type::range, global->stack_trace(), L"precision out of range in Number.toPrecision()"};
            }
            if (!std::isfinite(num)) {
                return to_string(h, num);
            }
            return string{h, number_to_precision(num, p)};
        });
    }

    return { c, prototype };
}

} // namespace mjs