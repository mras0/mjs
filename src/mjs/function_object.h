#ifndef MJS_FUNCTION_OBJECT_H
#define MJS_FUNCTION_OBJECT_H

#include "global_object.h"
#include "native_object.h"

namespace mjs {

class function_object : public native_object {
public:
    void put_function(const native_function_type& f, const string& body_text, int named_args) {
        assert(!call_ && !body_text_ && !named_args_);
        call_ = f;
        body_text_ = body_text.unsafe_raw_get();
        named_args_ = named_args;
    }

    value to_string() const {
        assert(body_text_);
        return value{body_text_.track(heap())};
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

    friend create_result make_function_object(global_object& global);

private:
    friend gc_type_info_registration<function_object>;
    gc_heap_ptr_untracked<gc_function> construct_;
    gc_heap_ptr_untracked<gc_function> call_;
    gc_heap_ptr_untracked<gc_string>   body_text_;
    value_representation               prototype_prop_;
    int named_args_ = 0;

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
        body_text_.fixup(h);
        prototype_prop_.fixup(h);
        native_object::fixup();
    }

    void call_function(const native_function_type& f) {
        assert(!call_);
        call_ = f;
    }
};

create_result make_function_object(global_object& global);

} // namespace mjs

#endif

