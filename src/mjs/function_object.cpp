#include "function_object.h"
#include <sstream>

namespace mjs {

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


gc_heap_ptr<function_object> make_raw_function(global_object& global) {
    auto fp = global.function_prototype();
    auto o = global.heap().make<function_object>(fp->class_name(), fp);
    o->put_prototype_with_attributes(global.make_object(), global.language_version() >= version::es3 ? property_attribute::dont_delete : property_attribute::dont_enum);
    return o;
}

gc_heap_ptr<function_object> make_function(global_object& global, const native_function_type& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args) {
    auto o = make_raw_function(global);
    o->put_function(f, name, body_text, named_args);
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

void make_constructable(global_object& global, const object_ptr& o, const native_function_type& f) {
    assert(o.has_type<function_object>());
    auto& fo = static_cast<function_object&>(*o);
    if (f) {
        fo.construct_function(f);
    } else {
        fo.default_construct_function();
    }
    auto p = o->get(L"prototype");
    assert(p.type() == value_type::object);
    if (global.language_version() < version::es3) {
        p.object_value()->put(global.common_string("constructor"), value{o}, property_attribute::dont_enum);
    }
}

} // namespace mjs