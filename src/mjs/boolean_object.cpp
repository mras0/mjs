#include "boolean_object.h"
#include "function_object.h"

namespace mjs {


//
// Boolean
//

class boolean_object : public object {
public:
    friend gc_type_info_registration<boolean_object>;

    bool boolean_value() const {
        return value_;
    }

    value internal_value() const override {
        return value{value_};
    }

private:
    bool value_;

    explicit boolean_object(const string& class_name, const object_ptr& prototype, bool val) : object{class_name, prototype}, value_{val} {
    }

    void do_debug_print_extra(std::wostream& os, int, int, int indent) const override {
        os << std::wstring(indent, ' ') << "[[Value]]: " << (value_?"true":"false") << "\n";
    }
};


object_ptr new_boolean(const object_ptr& prototype, bool val) {
    auto o = prototype.heap().make<boolean_object>(prototype->class_name(), prototype, val);
    return o;
}

global_object_create_result make_boolean_object(const gc_heap_ptr<global_object>& global) {
    auto& h = global.heap();
    auto prototype = h.make<boolean_object>(string{h, "Boolean"}, global->object_prototype(), false);

    auto c = make_function(global, [](const value&, const std::vector<value>& args) {
        return value{!args.empty() && to_boolean(args.front())};
    },  prototype->class_name().unsafe_raw_get(), 1);
    make_constructable(global, c, [prototype](const value&, const std::vector<value>& args) {
        return value{new_boolean(prototype, !args.empty() && to_boolean(args.front()))};
    });

    auto get_bool_obj = [global, prototype](const value& this_) {
        global->validate_type(this_, prototype, "Boolean");
        return gc_heap_ptr<boolean_object>{this_.object_value()};
    };

    put_native_function(global, prototype, global->common_string("toString"), [get_bool_obj](const value& this_, const std::vector<value>&){
        auto o = get_bool_obj(this_);
        return value{string{o.heap(), o->boolean_value() ? L"true" : L"false"}};
    }, 0);

    put_native_function(global, prototype, global->common_string("valueOf"), [get_bool_obj](const value& this_, const std::vector<value>&){
        auto o = get_bool_obj(this_);
        return value{o->boolean_value()};
    }, 0);

    return { c, prototype };
}
} // namespace mjs
