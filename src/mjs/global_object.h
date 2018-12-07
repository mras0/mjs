#ifndef MJS_GLOBAL_OBJECT_H
#define MJS_GLOBAL_OBJECT_H

#include "value.h"
#include "object.h"

namespace mjs {

class global_object : public object {
public:
    static gc_heap_ptr<global_object> make(gc_heap& h);
    virtual ~global_object() {}

    virtual const object_ptr& object_prototype() const = 0;
    virtual object_ptr make_raw_function() = 0;
    virtual object_ptr to_object(const value& v) = 0;

    static string native_function_body(gc_heap& h, const std::wstring_view& s);
    static string native_function_body(const string& s) { return native_function_body(s.heap(), s.view()); }

    static constexpr auto prototype_attributes = property_attribute::dont_enum | property_attribute::dont_delete | property_attribute::read_only;
    static constexpr auto default_attributes = property_attribute::dont_enum;

    virtual void put_function(const object_ptr& o, const native_function_type& f, const string& body_text, int named_args) = 0;
    virtual value array_constructor(const value&, const std::vector<value>& args) = 0; // FIXME: String.split() needs this
    virtual gc_heap_ptr<global_object> self_ptr() const = 0; // FIXME: Remove need for this

    template<typename F>
    void put_native_function(object& obj, const string& name, const F& f, int named_args) {
        obj.put(name, value{make_function(f, native_function_body(name), named_args)}, default_attributes);
    }

    template<typename F>
    void put_native_function(object& obj, const char* name, const F& f, int named_args) {
        put_native_function(obj, common_string(name), f, named_args);
    }

    template<typename F, typename String>
    void put_native_function(const object_ptr& obj, String&& name, const F& f, int named_args) {
        put_native_function(*obj, std::forward<String>(name), f, named_args);
    }

    template<typename F>
    object_ptr make_function(const F& f, const string& body_text, int named_args) {
        return do_make_function(gc_function::make(heap(), f), body_text, named_args);
    }

    object_ptr make_function(const native_function_type& f, const string& body_text, int named_args) {
        return do_make_function(f, body_text, named_args);
    }

    // Add constructor to function object (and prototype.constructor) uses the call_function if f is null
    void make_constructable(const object_ptr& o, const native_function_type& f = nullptr);

    // Make a string (to make it possible to use a cache)
    virtual string common_string(const char* str) = 0;

protected:
    using object::object;
    global_object(global_object&&) = default;

    virtual object_ptr do_make_function(const native_function_type& f, const string& body_text, int named_args) = 0;
};

extern std::wstring index_string(uint32_t index);

} // namespace mjs

#endif
