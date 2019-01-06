#include "function_object.h"
#include "error_object.h"
#include <sstream>

namespace mjs {

void function_object::put_function(const gc_heap_ptr<gc_function>& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args) {
    assert(!call_ && !text_ && !named_args_ && !is_native_);
    call_ = f;
    if (!body_text) {
        text_ = name;
        is_native_ = true;
    } else {
        assert(!name);
        text_ = body_text;
    }
    named_args_ = named_args;
}


string function_object::to_string() const {
    auto& h = heap();
    if (is_native_) {
        std::wostringstream oss;
        oss << "function ";
        if (text_) {
            oss << text_.dereference(h).view();
        }
        oss << "() { [native code] }";
        return string{h, oss.str()};
    } else {
        return text_.track(h);
    }
}

value function_object::call(const value& this_, const std::vector<value>& args) const {
    if (!call_) throw not_callable_exception{};
    return call_.dereference(heap()).call(this_, args);
}

value function_object::construct(const value& this_, const std::vector<value>& args) const {
    if (!construct_) throw not_callable_exception{};
    return construct_.dereference(heap()).call(this_, args);
}

gc_heap_ptr<function_object> make_raw_function(global_object& global) {
    auto fp = global.function_prototype();
    auto o = global.heap().make<function_object>(fp->class_name(), fp);
    o->put_prototype_with_attributes(global.make_object(), global.language_version() >= version::es3 ? property_attribute::dont_delete : property_attribute::dont_enum);
    return o;
}

static value get_this_arg(const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    value this_arg = args.size() < 1 ? value::undefined : args[0];
    if (this_arg.type() == value_type::undefined || this_arg.type() == value_type::null) {
        return value{global};
    } else {
        return value{global->to_object(this_arg)};
    }
}

create_result make_function_object(global_object& global) {
    auto prototype = global.function_prototype();

    // §15.3.4
    assert(prototype.has_type<function_object>());
    static_cast<function_object&>(*prototype).put_function([](const value&, const std::vector<value>&) {
        return value::undefined;
    }, nullptr, nullptr, 0);

    put_native_function(global, prototype, "toString", [global = global.self_ptr(), prototype](const value& this_, const std::vector<value>&) {
        // HACK to make Function.prototype.toString() work..
        if (this_.type() != value_type::object || this_.object_value().get() != prototype.get()) {
            global->validate_type(this_, prototype, "function");
        }
        assert(this_.object_value().has_type<function_object>());
        return value{static_cast<function_object&>(*this_.object_value()).to_string()};
    }, 0);

    if (global.language_version() >= version::es3) {
        put_native_function(global, prototype, "call", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            global->validate_type(this_, global->function_prototype(), "function");
            std::vector<value> new_args;
            if (args.size() > 1) {
                new_args.insert(new_args.end(), args.cbegin() + 1, args.cend());
            }
            return static_cast<const function_object&>(*this_.object_value()).call(get_this_arg(global, args), new_args);
        }, 1);

        put_native_function(global, prototype, "apply", [global = global.self_ptr()](const value& this_, const std::vector<value>& args) {
            global->validate_type(this_, global->function_prototype(), "function");
            std::vector<value> new_args;
            if (args.size() > 1 && args[1].type() != value_type::undefined && args[1].type() != value_type::null) {
                if (args[1].type() != value_type::object) {
                    std::wostringstream woss;
                    debug_print(woss, args[1], 4);
                    woss << " is not an array";
                    throw native_error_exception(native_error_type::type, global->stack_trace(), woss.str());
                }
                auto a = args[1].object_value();
                const uint32_t len = to_uint32(a->get(L"length"));
                new_args.resize(len);
                for (uint32_t i = 0; i < len; ++i) {
                    new_args[i] = a->get(index_string(i));
                }
            }
            return static_cast<const function_object&>(*this_.object_value()).call(get_this_arg(global, args), new_args);
        }, 2);
    }

    auto obj = make_raw_function(global); // Note: function constructor is added by interpreter
    return { obj, prototype };
}

function_object& get_function_object(const value& v) {
    if (v.type() == value_type::object) {
        if (auto o = v.object_value(); o.has_type<function_object>()) {
            return static_cast<function_object&>(*o);
        }
    }
    throw not_callable_exception{v.type()};
}

value call_function(const value& v, const value& this_, const std::vector<value>& args) {
    return get_function_object(v).call(this_, args);
}

value construct_function(const value& v, const value& this_, const std::vector<value>& args) {
    return get_function_object(v).construct(this_, args);
}

} // namespace mjs