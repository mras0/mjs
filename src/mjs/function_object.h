#ifndef MJS_FUNCTION_OBJECT_H
#define MJS_FUNCTION_OBJECT_H

#include "global_object.h"
#include "native_object.h"
#include "gc_function.h"

namespace mjs {

class function_object : public native_object {
public:
    template<typename F>
    void put_function(const F& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args) {
        put_function(gc_function::make(heap(), f), name, body_text, named_args);
    }

    template<typename F>
    void construct_function(const F& f) {
        construct_function(gc_function::make(heap(), f));
    }

    string to_string() const;

    value get_prototype() const {
        return prototype_prop_.get_value(heap());
    }

    void put_prototype_with_attributes(const object_ptr& p, property_attribute attributes) {
        assert(!object::check_own_property_attribute(L"prototype", property_attribute::none, property_attribute::none));
        prototype_prop_ = value_representation{value{p}};
        update_property_attributes("prototype", attributes);
    }

    void default_construct_function() {
        assert(!construct_ && call_);
        construct_ = call_;
    }

    value call(const value& this_, const std::vector<value>& args) const;
    value construct(const value& this_, const std::vector<value>& args) const;

private:
    friend gc_type_info_registration<function_object>;
    gc_heap_ptr_untracked<gc_function> construct_;
    gc_heap_ptr_untracked<gc_function> call_;
    gc_heap_ptr_untracked<gc_string>   text_;
    value_representation               prototype_prop_;
    int named_args_ = 0;
    bool is_native_ = false;

    value get_length() const {
        return value{static_cast<double>(named_args_)};
    }

    value get_arguments() const {
        return value::null;
    }

    void put_prototype(const value& val) {
        prototype_prop_ = value_representation{val};
    }

    explicit function_object(const string& class_name, const object_ptr& prototype) : native_object(class_name, prototype) {
        DEFINE_NATIVE_PROPERTY_READONLY(function_object, length);
        DEFINE_NATIVE_PROPERTY_READONLY(function_object, arguments);
        DEFINE_NATIVE_PROPERTY(function_object, prototype);

        static_assert(!gc_type_info_registration<function_object>::needs_destroy);
        static_assert(gc_type_info_registration<function_object>::needs_fixup);
    }

    void fixup() {
        auto& h = heap();
        construct_.fixup(h);
        call_.fixup(h);
        text_.fixup(h);
        prototype_prop_.fixup(h);
        native_object::fixup();
    }

    void put_function(const gc_heap_ptr<gc_function>& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args);

    void construct_function(const gc_heap_ptr<gc_function>& f) {
        assert(!construct_ && f);
        construct_ = f;
    }

};

gc_heap_ptr<function_object> make_raw_function(global_object& global);
global_object_create_result make_function_object(global_object& global);

template<typename F>
gc_heap_ptr<function_object> make_function(global_object& global, const F& f, const gc_heap_ptr<gc_string>& name, int named_args) {
    auto o = make_raw_function(global);
    o->put_function(f, name, nullptr, named_args);
    return o;
}

// Add constructor to function object
template<typename F>
void make_constructable(global_object& global, gc_heap_ptr<function_object>& o, const F& f) {
    o->construct_function(f);
    auto p = o->get_prototype();
    assert(p.type() == value_type::object);
    if (global.language_version() < version::es3) {
        p.object_value()->put(global.common_string("constructor"), value{o}, property_attribute::dont_enum);
    }
}

template<typename F>
void put_native_function(global_object& global, const object_ptr& obj, const string& name, const F& f, int named_args) {
    obj->put(name, value{make_function(global, f, name.unsafe_raw_get(), named_args)}, global_object::default_attributes);
}

template<typename F>
void put_native_function(global_object& global, const object_ptr& obj, const char* name, const F& f, int named_args) {
    put_native_function(global, obj, string{global.heap(), name}, f, named_args);
}

class not_callable_exception : public std::exception {
public:
    explicit not_callable_exception(value_type type = value_type::object) : type_{type} {}
    const char* what() const noexcept override {
        return "Not callable";
    }

    value_type type() const { return type_; }
private:
    value_type type_;
};

value call_function(const value& v, const value& this_, const std::vector<value>& args);
value construct_function(const value& v, const value& this_, const std::vector<value>& args);

} // namespace mjs

#endif

