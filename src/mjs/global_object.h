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

    static string native_function_body(const char* name);

    static constexpr auto prototype_attributes = property_attribute::dont_enum | property_attribute::dont_delete | property_attribute::read_only;
    static constexpr auto default_attributes = property_attribute::dont_enum;

    static void put_function(const object_ptr& o, const native_function_type& f, const string& body_text, int named_args);
    template<typename F>
    void put_native_function(object& obj, const char* name, const F& f, int named_args) {
        obj.put(string{name}, value{make_function(f, native_function_body(name), named_args)}, default_attributes);
    }

    template<typename F>
    void put_native_function(const object_ptr& obj, const char* name, const F& f, int named_args) {
        put_native_function(*obj, name, f, named_args);
    }

protected:
    using object::object;
    global_object(global_object&&) = default;

    virtual object_ptr make_function(const native_function_type& f, const string& body_text, int named_args) = 0;
};

extern string index_string(uint32_t index);

} // namespace mjs

#endif
