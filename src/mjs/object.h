#ifndef MJS_OBJECT_H
#define MJS_OBJECT_H

#include "value.h"
#include "gc_table.h"
#include "gc_function.h"

namespace mjs {

using native_function_type = gc_heap_ptr<gc_function>;

class object {
public:
    friend gc_type_info_registration<object>;

    gc_heap& heap() const {
        return heap_;
    }

    // TODO: Remove this
    static auto make(const string& class_name, const object_ptr& prototype) {
        return class_name.unsafe_raw_get().heap().make<object>(class_name, prototype);
    }

    virtual ~object() {}

    // §8.6.2, Page 22: Internal Properties and Methods

    //
    // [[Prototype]] ()
    //
    // The value of the [[Prototype]] property must be either an object or null , and every [[Prototype]] chain must have
    // finite  length  (that  is,  starting  from  any  object,  recursively  accessing  the  [[Prototype]]  property  must  eventually
    // lead to a null value). Whether or not a native object can have a host object as its [[Prototype]] depends on the implementation
    object_ptr prototype() { return prototype_.track(heap_); }

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
    void internal_value(const value& v) { value_ = v; }

    // [[Get]] (PropertyName)
    value get(const string& name) const {
        auto& props = properties_.dereference(heap_);
        auto it = props.find(name);
        if (it != props.end()) {
            return it.value();
        }
        return prototype_ ? prototype_.dereference(heap_).get(name) : value::undefined;
    }

    // [[Put]] (PropertyName, Value)
    virtual void put(const string& name, const value& val, property_attribute attr = property_attribute::none) {
        auto& props = properties_.dereference(heap_);
        auto it = props.find(name);
        if (it != props.end()) {
            // CanPut?
            if (!it.has_attribute(property_attribute::read_only)) {
                it.value(val);
            }
        } else {
            if (props.length() == props.capacity()) {
                properties_ = props.copy_with_increased_capacity();
            }

            // properties_ could have changed, so don't use props
            properties_.dereference(heap_).insert(name, val, attr);
        }
    }

    // [[CanPut]] (PropertyName)
    bool can_put(const string& name) const {
        auto& props = properties_.dereference(heap_);
        auto it = props.find(name);
        if (it != props.end()) {
            return !it.has_attribute(property_attribute::read_only);
        }
        return prototype_ ? prototype_.dereference(heap_).can_put(name) : true;
    }

    // [[HasProperty]] (PropertyName)
    bool has_property(const string& name) const {
        auto& props = properties_.dereference(heap_);
        if (props.find(name) != props.end()) {
            return true;
        }
        return prototype_ ? prototype_.dereference(heap_).has_property(name) : false;
    }

    // [[Delete]] (PropertyName)
    bool delete_property(const string& name) {
        auto& props = properties_.dereference(heap_);
        auto it = props.find(name);
        if (it == props.end()) {
            return true;

        }
        if (it.has_attribute(property_attribute::dont_delete)) {
            return false;
        }
        it.erase();
        return true;
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

    std::vector<string> property_names() const {
        std::vector<string> names;
        add_property_names(names);
        return names;
    }

    virtual void debug_print(std::wostream& os, int indent_incr, int max_nest = INT_MAX, int indent = 0) const;

protected:
    explicit object(const string& class_name, const object_ptr& prototype)
        : heap_(class_name.unsafe_raw_get().heap())
        , class_(class_name.unsafe_raw_get())
        , prototype_(prototype)
        , properties_(gc_table::make(heap_, 32))
        , value_(value::undefined) {
    }

    object(object&& o) = default;

    bool fixup(gc_heap& new_heap);
private:
    gc_heap& heap_;
    gc_heap_ptr_untracked<gc_string>    class_;
    gc_heap_ptr_untracked<object>       prototype_;
    gc_heap_ptr_untracked<gc_function>  construct_;
    gc_heap_ptr_untracked<gc_function>  call_;
    gc_heap_ptr_untracked<gc_table>     properties_;
    value_representation                value_;

    void add_property_names(std::vector<string>& names) const {
        auto& props = properties_.dereference(heap_);
        for (auto it = props.begin(); it != props.end(); ++it) {
            if (!it.has_attribute(property_attribute::dont_enum)) {
                names.push_back(it.key());
            }
        }
        if (prototype_) {
            prototype_.dereference(heap()).add_property_names(names);
        }
    }

    gc_heap_ptr_untyped move(gc_heap& new_heap) {
        return new_heap.make<object>(std::move(*this));
    }
};

} // namespace mjs

#endif
