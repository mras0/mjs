#include "function_object.h"
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

value function_object::call(const value& this_, const std::vector<value>& args) {
    if (!call_) throw not_callable_exception{};
    return call_.dereference(heap()).call(this_, args);
}

value function_object::construct(const value& this_, const std::vector<value>& args) {
    if (!construct_) throw not_callable_exception{};
    return construct_.dereference(heap()).call(this_, args);
}

gc_heap_ptr<function_object> make_raw_function(global_object& global) {
    auto fp = global.function_prototype();
    auto o = global.heap().make<function_object>(fp->class_name(), fp);
    o->put_prototype_with_attributes(global.make_object(), global.language_version() >= version::es3 ? property_attribute::dont_delete : property_attribute::dont_enum);
    return o;
}

create_result make_function_object(global_object& global) {
    auto prototype = global.function_prototype();

    // §15.3.4
    assert(prototype.has_type<function_object>());
    static_cast<function_object&>(*prototype).put_function([](const value&, const std::vector<value>&) {
        return value::undefined;
    }, nullptr, nullptr, 0);

    put_native_function(global, prototype, "toString", [prototype](const value& this_, const std::vector<value>&) {
        // HACK to make Function.prototype.toString() work..
        if (this_.type() != value_type::object || this_.object_value().get() != prototype.get()) {
            validate_type(this_, prototype, "Function");
        }
        assert(this_.object_value().has_type<function_object>());
        return value{static_cast<function_object&>(*this_.object_value()).to_string()};
    }, 0);
    auto obj = make_raw_function(global); // Note: function constructor is added by interpreter
    return { obj, prototype };
}

function_object& get_function_object(const value& v) {
    if (v.type() == value_type::object) {
        if (auto o = v.object_value(); o.has_type<function_object>()) {
            return static_cast<function_object&>(*o);
        }
    }
    throw not_callable_exception{};
}

value call_function(const value& v, const value& this_, const std::vector<value>& args) {
    return get_function_object(v).call(this_, args);
}

value construct_function(const value& v, const value& this_, const std::vector<value>& args) {
    return get_function_object(v).construct(this_, args);
}

} // namespace mjs