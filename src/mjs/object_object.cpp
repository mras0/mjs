#include "object_object.h"
#include "function_object.h"
#include "array_object.h"
#include "error_object.h"
#include <sstream>

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

bool has_own_property(const object_ptr& o, const std::wstring_view p) {
    return is_valid(o->own_property_attributes(p));
}

// ES5.1, 8.10

bool is_accessor_descriptor(const object_ptr& desc) {
    return has_own_property(desc, L"get") || has_own_property(desc, L"set");
}

bool is_data_descriptor(const object_ptr& desc) {
    return has_own_property(desc, L"value") || has_own_property(desc, L"writable");
}

bool is_generic_descriptor(const object_ptr& desc) {
    return !is_accessor_descriptor(desc) && !is_data_descriptor(desc);
}

property_attribute attributes_from_descriptor(const object_ptr& desc) {
    auto a = property_attribute::none;
    if (!to_boolean(desc->get(L"writable"))) a |= property_attribute::read_only;
    if (!to_boolean(desc->get(L"enumerable"))) a |= property_attribute::dont_enum;
    if (!to_boolean(desc->get(L"configurable"))) a |= property_attribute::dont_delete;
    return a;
}

enum class define_own_property_result {
    ok,
    cannot_redefine,
    not_extensible,
};

// ES5.1, 8.12.9
define_own_property_result define_own_property(const object_ptr& o, const string& p, const object_ptr& desc) {
    const auto current_attributes = o->own_property_attributes(p.view());
    const auto new_attributes = attributes_from_descriptor(desc);

    if (!is_valid(current_attributes)) {
        if (!o->is_extensible()) {
            return define_own_property_result::not_extensible;
        }

        // property not already present
        if (is_generic_descriptor(desc) || is_data_descriptor(desc)) {
            o->put(p, desc->get(L"value"), new_attributes);
            return define_own_property_result::ok;
        } else {
            NOT_IMPLEMENTED("accessor descriptor not supported");
        }
    } else if (has_attributes(current_attributes, property_attribute::accessor))  {
        NOT_IMPLEMENTED("accessor descriptor not supported");
    }

    // If descriptor is empty or all fields are equal
    if (!has_own_property(desc, L"writable")
        && !has_own_property(desc, L"enumerable")
        && !has_own_property(desc, L"configurable")
        && !has_own_property(desc, L"value")
        && !has_own_property(desc, L"get")
        && !has_own_property(desc, L"set")) {
        return define_own_property_result::ok;
    } else if (current_attributes == new_attributes && o->get(p.view()) == desc->get(L"value")) {
        return define_own_property_result::ok;
    }

    if (has_attributes(current_attributes, property_attribute::dont_delete)) {
        // 7. If the [[Configurable]] field of current is false then

        if (!has_attributes(new_attributes, property_attribute::dont_delete)) {
            // Reject, if the [[Configurable]] field of Desc is true.
            return define_own_property_result::cannot_redefine;
        }
        if (has_own_property(desc, L"enumerable") && has_attributes(current_attributes, property_attribute::dont_enum) != has_attributes(new_attributes, property_attribute::dont_enum)) {
            // Reject, if the [[Enumerable]] field of Desc is present and the [[Enumerable]] fields of current and Desc are the Boolean negation of each other.
            return define_own_property_result::cannot_redefine;
        }
    }

    if (is_generic_descriptor(desc)) {
        // 8. If IsGenericDescriptor(Desc) is true, then no further validation is required.
        NOT_IMPLEMENTED("generic descriptor");
    } else if (!has_attributes(current_attributes, property_attribute::accessor) != is_data_descriptor(desc)) {
        // 9 Else, if IsDataDescriptor(current) and IsDataDescriptor(Desc) have different results, then 
        if (has_attributes(current_attributes, property_attribute::dont_delete)) {
            // 9.a Reject, if the [[Configurable]] field of current is false.
            return define_own_property_result::cannot_redefine;
        }
        NOT_IMPLEMENTED("changing descriptor type");
    } else if (!has_attributes(current_attributes, property_attribute::accessor)) {
        if (has_attributes(current_attributes, property_attribute::dont_delete)) {
            // 10.a
            if (has_attributes(current_attributes, property_attribute::read_only)) {
                if (!has_attributes(new_attributes, property_attribute::read_only)) {
                    // 10.a.i
                    return define_own_property_result::cannot_redefine;
                }
                if (has_own_property(desc, L"value") && o->get(p.view()) != desc->get(L"value")) {
                    // 10.a.ii
                    return define_own_property_result::cannot_redefine;
                }
            }
        }
    } else {
        assert(has_attributes(current_attributes, property_attribute::accessor) && is_accessor_descriptor(desc));
        NOT_IMPLEMENTED("accessor descriptor");
    }

    property_attribute a = property_attribute::none;
    auto apply_flag = [&](const wchar_t* name, property_attribute f) {
        if (desc->has_property(name)) {
            if (!to_boolean(desc->get(name))) {
                a |= f;
            }
        } else {
            a |= current_attributes & f;
        }
    };

    apply_flag(L"writable", property_attribute::read_only);
    apply_flag(L"enumerable", property_attribute::dont_enum);
    apply_flag(L"configurable", property_attribute::dont_delete);
    if (o->redefine_own_property(p, has_own_property(desc, L"value") ? desc->get(L"value") : o->get(p.view()), a)) {
        return define_own_property_result::ok;
    } else {
        return define_own_property_result::cannot_redefine;
    }
}

void define_own_property_checked(const gc_heap_ptr<global_object>& global, const object_ptr& o, const string& p, const value& desc) {
    const auto res = define_own_property(o, p, global->validate_object(desc));
    if (res == define_own_property_result::ok) {
        return;
    }

    std::wostringstream woss;
    if (res == define_own_property_result::cannot_redefine) {
        woss << "cannot redefine property: " << p.view();
    } else {
        assert(res == define_own_property_result::not_extensible);
        woss << "cannot define property: " << p.view() << ", object is not extensible";
    }
    throw native_error_exception{native_error_type::type, global->stack_trace(), woss.str()};
}

bool check_for_immutability(const object_ptr& o, property_attribute attr) {
    if (o->is_extensible()) {
        return false;
    }
    for (const auto& p : o->own_property_names(false)) {
        if (!has_attributes(o->own_property_attributes(p.view()), attr)) {
            return false;
        }
    }
    return true;
}

void make_immutable(const object_ptr& o, property_attribute attr) {
    for (const auto& p : o->own_property_names(false)) {
        const auto attrs = o->own_property_attributes(p.view());
        if (!has_attributes(attrs, attr)) {
            o->redefine_own_property(p, o->get(p.view()), attrs | attr);
        }
    }
    o->prevent_extensions();
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
            return value{has_own_property(o, to_string(o.heap(), get_arg(args, 0)).view())};
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
            auto p = o->prototype();
            return p ? value{p} : value::null;
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

        put_native_function(global, o, "defineProperty", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            auto& h = global.heap();
            auto o = global->validate_object(get_arg(args, 0));
            auto p = to_string(h, get_arg(args, 1));
            define_own_property_checked(global, o, p, get_arg(args, 2));
            return value{o};
        }, 3);

        put_native_function(global, o, "defineProperties", [global= global.self_ptr()](const value&, const std::vector<value>& args) {
            auto o = global->validate_object(get_arg(args, 0));
            auto props = global->to_object(get_arg(args, 1));
            for (const auto& p : props->own_property_names(false)) {
                define_own_property_checked(global, o, p, props->get(p.view()));
            }
            return value{o};
        }, 2);

        put_native_function(global, o, "isExtensible", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            return value{global->validate_object(get_arg(args, 0))->is_extensible()};
        }, 1);

        put_native_function(global, o, "preventExtensions", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            auto o = global->validate_object(get_arg(args, 0));
            o->prevent_extensions();
            return value{o};
        }, 1);

        put_native_function(global, o, "isSealed", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            return value{check_for_immutability(global->validate_object(get_arg(args, 0)), property_attribute::dont_delete)};
        }, 1);

        put_native_function(global, o, "seal", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            auto o = global->validate_object(get_arg(args, 0));
            make_immutable(o, property_attribute::dont_delete);
            return value{o};
        }, 1);

        put_native_function(global, o, "isFrozen", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            return value{check_for_immutability(global->validate_object(get_arg(args, 0)), property_attribute::read_only | property_attribute::dont_delete)};
        }, 1);

        put_native_function(global, o, "freeze", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            auto o = global->validate_object(get_arg(args, 0));
            make_immutable(o, property_attribute::read_only | property_attribute::dont_delete);
            return value{o};
        }, 1);

        put_native_function(global, o, "create", [global = global.self_ptr()](const value&, const std::vector<value>& args) {
            if (args.empty() || (args[0].type() != value_type::null && args[0].type() != value_type::object)) {
                throw native_error_exception{native_error_type::type, global->stack_trace(), L"Invalid object prototype"};
            }
            auto o = global.heap().make<object>(global->object_prototype()->class_name(), args[0].type() == value_type::object ? args[0].object_value() : nullptr);
            if (args.size() > 1) {
                auto props = global->to_object(args[1]);
                for (const auto& p : props->own_property_names(false)) {
                    define_own_property_checked(global, o, p, props->get(p.view()));
                }
            }
            return value{o};
        }, 2);
    }

    return { o, prototype };
}

} // namespace mjs

