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

    void set_strict() {
        assert(!strict_);
        strict_ = true;
    }

    string to_string() const;

    value get_prototype() const {
        return prototype_prop_.get_value(heap());
    }

    void put_prototype_with_attributes(const object_ptr& p, property_attribute attributes) {
        assert(is_valid(object::own_property_attributes(L"prototype")));
        prototype_prop_ = value_representation{value{p}};
        update_property_attributes("prototype", attributes);
    }

    void default_construct_function() {
        assert(!construct_ && call_);
        construct_ = call_;
    }

    value call(const value& this_, const std::vector<value>& args) const;
    value construct(const value& this_, const std::vector<value>& args) const;

    static object_ptr bind(const gc_heap_ptr<function_object>& f, const std::vector<value>& args);

private:
    friend gc_type_info_registration<function_object>;
    gc_heap_ptr_untracked<global_object> global_;
    gc_heap_ptr_untracked<gc_function>   construct_;
    gc_heap_ptr_untracked<gc_function>   call_;
    gc_heap_ptr_untracked<gc_string>     text_;
    value_representation                 prototype_prop_;
    int named_args_ = 0;
    bool is_native_ = false;
    bool strict_ = false;

    value get_length() const {
        return value{static_cast<double>(named_args_)};
    }

    void put_prototype(const value& val) {
        prototype_prop_ = value_representation{val};
    }

    explicit function_object(const gc_heap_ptr<global_object>& global, const string& class_name, const object_ptr& prototype);

    void fixup();

    void put_function(const gc_heap_ptr<gc_function>& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args);

    void construct_function(const gc_heap_ptr<gc_function>& f) {
        assert(!construct_ && f);
        construct_ = f;
    }
};

gc_heap_ptr<function_object> make_raw_function(const gc_heap_ptr<global_object>& global);
global_object_create_result make_function_object(const gc_heap_ptr<global_object>& global);

template<typename F>
gc_heap_ptr<function_object> make_function(const gc_heap_ptr<global_object>& global, const F& f, const gc_heap_ptr<gc_string>& name, int named_args) {
    auto o = make_raw_function(global);
    o->put_function(f, name, nullptr, named_args);
    return o;
}

// Add constructor to function object
template<typename F>
void make_constructable(const gc_heap_ptr<global_object>& global, gc_heap_ptr<function_object>& o, const F& f) {
    o->construct_function(f);
    auto p = o->get_prototype();
    assert(p.type() == value_type::object);
    if (global->language_version() < version::es3) {
        p.object_value()->put(global->common_string("constructor"), value{o}, property_attribute::dont_enum);
    }
}

template<typename F>
void put_native_function(const gc_heap_ptr<global_object>& global, const object_ptr& obj, const string& name, const F& f, int named_args) {
    obj->put(name, value{make_function(global, f, name.unsafe_raw_get(), named_args)}, global_object::default_attributes);
}

template<typename F>
void put_native_function(const gc_heap_ptr<global_object>& global, const object_ptr& obj, const char* name, const F& f, int named_args) {
    put_native_function(global, obj, string{global.heap(), name}, f, named_args);
}

class not_callable_exception : public std::runtime_error {
public:
    explicit not_callable_exception(const value& v) : std::runtime_error(format_error_message(v)) {
    }
private:
    static std::string format_error_message(const value& v);
};

inline bool is_function(const object_ptr& o) {
    return o.has_type<function_object>();
}

inline bool is_function(const value& v) {
    return v.type() == value_type::object && is_function(v.object_value());
}

value call_function(const value& v, const value& this_, const std::vector<value>& args);
value construct_function(const value& v, const value& this_, const std::vector<value>& args);

} // namespace mjs

#endif

