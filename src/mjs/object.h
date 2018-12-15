#ifndef MJS_OBJECT_H
#define MJS_OBJECT_H

#include "value_representation.h"
#include "property_attribute.h"
#include "gc_vector.h"
#include "gc_function.h"

namespace mjs {

using native_function_type = gc_heap_ptr<gc_function>;

class object {
public:
    friend gc_type_info_registration<object>;

    gc_heap& heap() const {
        return heap_;
    }

    // §8.6.2, Page 22: Internal Properties and Methods

    //
    // [[Prototype]] ()
    //
    // The value of the [[Prototype]] property must be either an object or null , and every [[Prototype]] chain must have
    // finite  length  (that  is,  starting  from  any  object,  recursively  accessing  the  [[Prototype]]  property  must  eventually
    // lead to a null value). Whether or not a native object can have a host object as its [[Prototype]] depends on the implementation
    object_ptr prototype() { return prototype_ ? prototype_.track(heap_) : nullptr; }

    //
    // [[Class]] ()
    //
    // The value of the [[Class]] property is defined by this specification for every kind of built-in object. The value of
    // the  [[Class]]  property  of  a  host  object  may  be  any  value,  even  a  value  used  by  a  built-in  object  for  its  [[Class]]
    // property. Note that this specification does not provide any means for a program to access the value of a [[Class]]
    // property; it is used internally to distinguish different kinds of built-in objects
    string class_name() const { return class_.track(heap_); }

    // [[Value]] ()
    value internal_value() const { return value_.get_value(heap_); }
    void internal_value(const value& v) { value_ = value_representation{v}; }

    // [[Get]] (PropertyName)
    virtual value get(const std::wstring_view& name) const {
        auto it = deep_find(name).first;
        return it ? it->value.get_value(heap_) : value::undefined;
    }

    // [[Put]] (PropertyName, Value)
    virtual void put(const string& name, const value& val, property_attribute attr = property_attribute::none);

    // [[CanPut]] (PropertyName)
    virtual bool can_put(const std::wstring_view& name) const {
        auto it = deep_find(name).first;
        return it ? !it->has_attribute(property_attribute::read_only) : true;
    }

    // [[HasProperty]] (PropertyName)
    virtual bool has_property(const std::wstring_view& name) const {
        return deep_find(name).first;
    }

    // [[Delete]] (PropertyName)
    virtual bool delete_property(const std::wstring_view& name);

    // Check if the object has a property with matching attributes
    virtual bool check_own_property_attribute(const std::wstring_view& name, property_attribute mask, property_attribute expected) const {
        if (auto it = find(name).first; it) {
            return (it->attributes & mask) == expected;
        }
        return false;
    }

    virtual value_type default_value_type() const {
        // When hint is undefined, assume Number unless it's a Date object in which case assume String
        return value_type::number;
    }

    // [[Construct]] (Arguments...)
    void construct_function(const native_function_type& f) { construct_ = f; }
    native_function_type construct_function() const { return construct_ ? construct_.track(heap_) : nullptr; }

    // [[Call]] (Arguments...)
    void call_function(const native_function_type& f) { call_ = f; }
    native_function_type call_function() const { return call_ ? call_.track(heap_) : nullptr; }

    std::vector<string> property_names() const;

    virtual void debug_print(std::wostream& os, int indent_incr, int max_nest = INT_MAX, int indent = 0) const;

protected:
    explicit object(const string& class_name, const object_ptr& prototype);
    object(object&& o) = default;
    void fixup();
    virtual void add_property_names(std::vector<string>& names) const;

private:
    struct property {
        gc_heap_ptr_untracked<gc_string> key;
        property_attribute               attributes;
        value_representation             value;

        explicit property(const string& key, const mjs::value& v, property_attribute attr) : key(key.unsafe_raw_get()), attributes(attr), value(v) {}

        void fixup(gc_heap& h) {
            key.fixup(h);
            value.fixup(h);
        }

        bool has_attribute(property_attribute a) const {
            return (attributes & a) == a;
        }
    };

    gc_heap&                                   heap_;
    gc_heap_ptr_untracked<gc_string>           class_;
    gc_heap_ptr_untracked<object>              prototype_;
    gc_heap_ptr_untracked<gc_function>         construct_;
    gc_heap_ptr_untracked<gc_function>         call_;
    gc_heap_ptr_untracked<gc_vector<property>> properties_;
    value_representation                       value_;

    std::pair<property*, gc_vector<property>*> find(const std::wstring_view& key) const {
        auto& props = properties_.dereference(heap_);
        for (auto& p: props) {
            if (p.key.dereference(heap_).view() == key) {
                return { &p, &props };
            }
        }
        return {nullptr, &props};
    }

    std::pair<property*, gc_vector<property>*> deep_find(const std::wstring_view& key) const {
        auto fr = find(key);
        return fr.first || !prototype_ ? fr : prototype_.dereference(heap_).deep_find(key);
    }
};

} // namespace mjs

#endif
