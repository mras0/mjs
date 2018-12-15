#include "function_object.h"

namespace mjs {

create_result make_function_object(global_object& global) {
    auto prototype = global.function_prototype();
    auto& h = global.heap();

    auto c = global.make_raw_function(); // Note: function constructor is added by interpreter

    // §15.3.4
    static_cast<function_object&>(*prototype).call_function(gc_function::make(h, [](const value&, const std::vector<value>&) {
        return value::undefined;
    }));
    global.put_native_function(prototype, "toString", [prototype](const value& this_, const std::vector<value>&) {
        validate_type(this_, prototype, "Function");
        assert(this_.object_value().has_type<function_object>());
        return static_cast<function_object&>(*this_.object_value()).to_string();
    }, 0);
    return { c, prototype };
}

} // namespace mjs