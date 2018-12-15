#ifndef MJS_FUNCTION_OBJECT_H
#define MJS_FUNCTION_OBJECT_H

#include "global_object.h"
#include "native_object.h"

namespace mjs {

class function_object : public native_object {
public:
    void put_function(const native_function_type& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args) {
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

    template<typename F>
    void put_function(const F& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args) {
        put_function(gc_function::make(heap(), f), name, body_text, named_args);
    }

    string to_string() const;

    void put_prototype_with_attributes(const object_ptr& p, property_attribute attributes) {
        assert(!object::check_own_property_attribute(L"prototype", property_attribute::none, property_attribute::none));
        prototype_prop_ = value_representation{value{p}};
        update_property_attributes("prototype", attributes);
    }

    void default_construct_function() {
        assert(!construct_ && call_);
        construct_ = call_;
    }

    void construct_function(const native_function_type& f) {
        assert(!construct_ && f);
        construct_ = f;
    }
    
    native_function_type construct_function() const override {
        return construct_ ? construct_.track(heap()) : nullptr;
    }

    native_function_type call_function() const override {
        return call_ ? call_.track(heap()) : nullptr;
    }

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

    value get_prototype() const {
        return prototype_prop_.get_value(heap());
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
};

gc_heap_ptr<function_object> make_raw_function(global_object& global);
create_result make_function_object(global_object& global);

gc_heap_ptr<function_object> make_function(global_object& global, const native_function_type& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args);

template<typename F>
gc_heap_ptr<function_object> make_function(global_object& global, const F& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args) {
    return make_function(global, gc_function::make(global.heap(), f), name, body_text, named_args);
}

// Add constructor to function object (and prototype.constructor) uses the call_function if f is null
void make_constructable(global_object& global, const object_ptr& o, const native_function_type& f);

template<typename F>
void make_constructable(global_object& global, const object_ptr& o, const F& f) {
    return make_constructable(global, o, gc_function::make(global.heap(), f));
}

template<typename F>
void put_native_function(global_object& global, const object_ptr& obj, const string& name, const F& f, int named_args) {
    obj->put(name, value{make_function(global, f, name.unsafe_raw_get(), nullptr, named_args)}, global_object::default_attributes);
}

template<typename F, typename S>
void put_native_function(global_object& global, const object_ptr& obj, const S& name, const F& f, int named_args) {
    put_native_function(global, obj, string{global.heap(), name}, f, named_args);
}

} // namespace mjs

#endif

