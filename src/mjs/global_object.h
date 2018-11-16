#ifndef MJS_GLOBAL_OBJECT_H
#define MJS_GLOBAL_OBJECT_H

#include "value.h"
#include "object.h"

namespace mjs {

class global_object : public object {
public:
    static gc_heap_ptr<global_object> make();
    virtual ~global_object() {}

    virtual const object_ptr& object_prototype() const = 0;
    virtual object_ptr make_raw_function() = 0;
    virtual object_ptr to_object(const value& v) = 0;

    static string native_function_body(const string& name);

    static constexpr auto prototype_attributes = property_attribute::dont_enum | property_attribute::dont_delete | property_attribute::read_only;
    static constexpr auto default_attributes = property_attribute::dont_enum;

    virtual void put_function(const object_ptr& o, const native_function_type& f, const string& body_text, int named_args) = 0;

    template<typename F>
    void put_native_function(object& obj, const string& name, const F& f, int named_args) {
        obj.put(name, value{make_function(f, native_function_body(name), named_args)}, default_attributes);
    }

    template<typename F>
    void put_native_function(object& obj, const char* name, const F& f, int named_args) {
        put_native_function(obj, string{name}, f, named_args);
    }

    template<typename F, typename String>
    void put_native_function(const object_ptr& obj, String&& name, const F& f, int named_args) {
        put_native_function(*obj, std::forward<String>(name), f, named_args);
    }

protected:
    using object::object;
    global_object(global_object&&) = default;

    virtual object_ptr do_make_function(const native_function_type& f, const string& body_text, int named_args) = 0;

    template<typename F>
    object_ptr make_function(const F& f, const string& body_text, int named_args) {
        return do_make_function(gc_function::make(gc_heap::local_heap(), f), body_text, named_args);
    }

    object_ptr make_function(const native_function_type& f, const string& body_text, int named_args) {
        return do_make_function(f, body_text, named_args);
    }
};

extern std::wstring index_string(uint32_t index);

} // namespace mjs

#endif
