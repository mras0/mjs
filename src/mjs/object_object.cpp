#include "object_object.h"
#include "function_object.h"
#include "array_object.h"

namespace mjs {

namespace {

value get_property_names(const gc_heap_ptr<global_object>& global, const value& o, bool check_enumerable) {
    global->validate_object(o);
    const auto n = o.object_value()->own_property_names(check_enumerable);
    const auto ns = static_cast<uint32_t>(n.size());
    auto a = make_array(global, ns);
    auto& h = global.heap();
    for (uint32_t i = 0; i < ns; ++i) {
        a->put(string{h,index_string(i)}, value{n[i]});
    }
    return value{a};
}


inline const value& get_arg(const std::vector<value>& args, int index) {
    return index < static_cast<int>(args.size()) ? args[index] : value::undefined;
}

} // unnamed namespace

global_object_create_result make_object_object(global_object& global) {
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

    auto& h = global.heap();
    put_native_function(global, prototype, "toString", [&h](const value& this_, const std::vector<value>&){
        return value{string{h, "[object "} + this_.object_value()->class_name() + string{h, "]"}};
    }, 0);
    put_native_function(global, prototype, "valueOf", [](const value& this_, const std::vector<value>&){
        return this_;
    }, 0);

    if (global.language_version() >= version::es3) {
        put_native_function(global, prototype, "toLocaleString", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
            auto o = global->to_object(this_);
            return call_function(o->get(L"toString"), value{o}, {});
        }, 0);
        put_native_function(global, prototype, "hasOwnProperty", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            auto o = global->validate_object(this_);
            return value{o->own_property_attributes(to_string(o.heap(), get_arg(args, 0)).view()) != property_attribute::invalid};
        }, 1);
        put_native_function(global, prototype, "propertyIsEnumerable", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            auto o = global->validate_object(this_);
            const auto a = o->own_property_attributes(to_string(o.heap(), get_arg(args, 0)).view());
            return value{is_valid(a) && (a & property_attribute::dont_enum) == property_attribute::none};
        }, 1);
    }

    if (global.language_version() >= version::es5) {
        put_native_function(global, o, "getPrototypeOf", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            auto o = global->validate_object(get_arg(args, 0));
            return value{o->prototype()};
        }, 1);

        put_native_function(global, o, "getOwnPropertyNames", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            return get_property_names(global, get_arg(args, 0), false);
        }, 1);

        put_native_function(global, o, "keys", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            return get_property_names(global, get_arg(args, 0), true);
        }, 1);

        put_native_function(global, o, "getOwnPropertyDescriptor", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            auto o = global->validate_object(get_arg(args, 0));
            auto& h = global.heap();
            auto p = to_string(h, get_arg(args, 1));
            const auto a = o->own_property_attributes(p.view());
            if (!is_valid(a)) {
                // Seems like we have to return undefined in ES5.1 (15.10.4)
                return value::undefined;
            }
            auto desc = global->make_object();
            if (has_attributes(a, property_attribute::accessor)) {
                assert(!"Not implemented"); // See ES5.1, 8.10.4
            } else {
                desc->put(global->common_string("value"), o->get(p.view()));
                desc->put(global->common_string("writable"), value{(a & property_attribute::read_only) == property_attribute::none});
            }
            desc->put(global->common_string("enumerable"), value{(a & property_attribute::dont_enum) == property_attribute::none});
            desc->put(global->common_string("configurable"), value{(a & property_attribute::dont_delete) == property_attribute::none});
            return value{desc};
        }, 2);
    }

    return { o, prototype };
}

} // namespace mjs

