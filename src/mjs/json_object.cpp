#include "json_object.h"
#include "function_object.h"
#include "error_object.h"
#include <sstream>

namespace mjs {

[[noreturn]] void throw_not_implement(const char* name, const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    std::wostringstream woss;
    woss << name << " is not yet implemented for arguments: {";
    for (const auto& a: args) {
        woss << " ";
        debug_print(woss, a, 4);
    }
    woss << "}";
    throw native_error_exception{native_error_type::assertion, global->stack_trace(), woss.str()};
}


value json_parse(const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    throw_not_implement("parse", global, args);
}

value json_stringify(const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    throw_not_implement("stringify", global, args);
}

global_object_create_result make_json_object(global_object& global) {
    assert(global.language_version() >= version::es5);
    auto& h = global.heap();
    auto json = h.make<object>(string{h,"JSON"}, global.object_prototype());

    put_native_function(global, json, "parse", [global=global.self_ptr()](const value&, const std::vector<value>& args) {
        return json_parse(global, args);
    }, 2);
    
    put_native_function(global, json, "stringify", [global=global.self_ptr()](const value&, const std::vector<value>& args) {
        return json_stringify(global, args);
    }, 3);

    return { json, nullptr };
}

} // namespace mjs

