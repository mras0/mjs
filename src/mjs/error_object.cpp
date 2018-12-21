#include "error_object.h"
#include "native_object.h"
#include "function_object.h"
#include <sstream>

namespace mjs {

class error_object : public native_object {
public:

    string to_string() const {
        return class_name();
    }

private:
    friend gc_type_info_registration<error_object>;
    value_representation name_;
    value_representation message_;

    void fixup() {
        auto& h = heap();
        name_.fixup(h);
        message_.fixup(h);
    }

    value get_name() const {
        return name_.get_value(heap());
    }

    void put_name(const value& v) {
        name_ = value_representation{v};
    }
    
    value get_message() const {
        return message_.get_value(heap());
    }

    void put_message(const value& v) {
        message_ = value_representation{v};
    }

    explicit error_object(const string& class_name, const object_ptr& prototype, const value& message) : native_object{class_name, prototype}, name_(value_representation{value{class_name}}), message_(value_representation{message}) {
        DEFINE_NATIVE_PROPERTY(error_object, name);
        DEFINE_NATIVE_PROPERTY(error_object, message);
    }
};
static_assert(!gc_type_info_registration<error_object>::needs_destroy);
static_assert(gc_type_info_registration<error_object>::needs_fixup);


create_result make_error_object(global_object& global) {
    auto prototype = global.heap().make<error_object>(global.common_string("Error"), global.object_prototype(), value::undefined);

    gc_heap_ptr<function_object> error_constructor;
    for (const char* name: {"Error", "EvalError", "RangeError", "ReferenceError", "SyntaxError", "TypeError", "URIError"}) {
        auto n = global.common_string(name);
        auto constructor = make_function(global, [n, global = global.self_ptr()](const value&, const std::vector<value>& args) {
            auto& h = global.heap();
            auto prototype = global->error_prototype();
            return value{h.make<error_object>(n, prototype, value{args.empty() || args[0].type() == value_type::undefined ? string{h, ""} : to_string(h, args[0])})};
        }, prototype->class_name().unsafe_raw_get(), 1);
        constructor->default_construct_function();
        if (!error_constructor) {
            assert(!strcmp(name, "Error"));
            error_constructor = constructor;
        } else {
            global.put(n, value{constructor}, property_attribute::dont_enum);
        }
    }

    put_native_function(global, prototype, "toString", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
        if (this_.type() == value_type::object) {
            auto& o = this_.object_value();
            if (o.has_type<error_object>()) {
                return value{static_cast<const error_object&>(*o).to_string()};
            }
        }
        return value{global->common_string("Error")};
    }, 0);

    prototype->put(global.common_string("constructor"), value{error_constructor}, global_object::prototype_attributes);

    return { error_constructor, prototype };
}

} // namespace mjs
