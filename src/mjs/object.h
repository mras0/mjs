#ifndef MJS_OBJECT_H
#define MJS_OBJECT_H

// TODO: Allow expansion of properties table (!)

#include "value.h"
#include "gc_table.h"
#include "gc_function.h"

namespace mjs {

using native_function_type = gc_heap_ptr<gc_function>;

class object {
public:
    friend gc_type_info_registration<object>;

    // TODO: Remove this
    static auto make(const string& class_name, const object_ptr& prototype) {
        return gc_heap::local_heap().make<object>(class_name, prototype);
    }

    virtual ~object() {}

    // §8.6.2, Page 22: Internal Properties and Methods

    //
    // [[Prototype]] ()
    //
    // The value of the [[Prototype]] property must be either an object or null , and every [[Prototype]] chain must have
    // finite  length  (that  is,  starting  from  any  object,  recursively  accessing  the  [[Prototype]]  property  must  eventually
    // lead to a null value). Whether or not a native object can have a host object as its [[Prototype]] depends on the implementation
    const object_ptr& prototype() { return prototype_; }

    //
    // [[Class]] ()
    //
    // The value of the [[Class]] property is defined by this specification for every kind of built-in object. The value of
    // the  [[Class]]  property  of  a  host  object  may  be  any  value,  even  a  value  used  by  a  built-in  object  for  its  [[Class]]
    // property. Note that this specification does not provide any means for a program to access the value of a [[Class]]
    // property; it is used internally to distinguish different kinds of built-in objects
    const string& class_name() const { return class_; }

    // [[Value]] ()
    const value& internal_value() const { return value_; }
    void internal_value(const value& v) { value_ = v; }

    // [[Get]] (PropertyName)
    value get(const string& name) const {
        auto it = properties_->find(name);
        if (it != properties_->end()) {
            return it.value();
        }
        return prototype_ ? prototype_->get(name) : value::undefined;
    }

    // [[Put]] (PropertyName, Value)
    virtual void put(const string& name, const value& val, property_attribute attr = property_attribute::none) {
        auto it = properties_->find(name);
        if (it != properties_->end()) {
            // CanPut?
            if (!it.has_attribute(property_attribute::read_only)) {
                it.value(val);
            }
        } else {
            properties_->insert(name, val, attr);
        }
    }

    // [[CanPut]] (PropertyName)
    bool can_put(const string& name) const {
        auto it = properties_->find(name);
        if (it != properties_->end()) {
            return !it.has_attribute(property_attribute::read_only);
        }
        return prototype_ ? prototype_->can_put(name) : true;
    }

    // [[HasProperty]] (PropertyName)
    bool has_property(const string& name) const {
        if (properties_->find(name) != properties_->end()) {
            return true;
        }
        return prototype_ ? prototype_->has_property(name) : false;
    }

    // [[Delete]] (PropertyName)
    bool delete_property(const string& name) {
        auto it = properties_->find(name);
        if (it == properties_->end()) {
            return true;

        }
        if (it.has_attribute(property_attribute::dont_delete)) {
            return false;
        }
        it.erase();
        return true;
    }

    // [[DefaultValue]] (Hint)
    virtual value_type default_value_type() const {
        // When hint is undefined, assume Number unless it's a Date object in which case assume String
        return value_type::number;
    }

    // [[Construct]] (Arguments...)
    void construct_function(const native_function_type& f) { construct_ = f; }
    const native_function_type& construct_function() const { return construct_; }

    // [[Call]] (Arguments...)
    void call_function(const native_function_type& f) { call_ = f; }
    const native_function_type& call_function() const { return call_; }

    std::vector<string> property_names() const {
        std::vector<string> names;
        add_property_names(names);
        return names;
    }

    virtual void debug_print(std::wostream& os, int indent_incr, int max_nest = INT_MAX, int indent = 0) const;

protected:
    explicit object(const string& class_name, const object_ptr& prototype)
        : class_(class_name)
        , prototype_(prototype)
        , properties_(gc_table::make(gc_heap::local_heap(), 32)) {
    }

    object(object&&) = default;

private:
    string class_;
    object_ptr prototype_;
    value value_;
    native_function_type construct_;
    native_function_type call_;
    gc_heap_ptr<gc_table> properties_;

    void add_property_names(std::vector<string>& names) const {
        for (auto it = properties_->begin(); it != properties_->end(); ++it) {
            if (!it.has_attribute(property_attribute::dont_enum)) {
                names.push_back(it.key());
            }
        }
        if (prototype_) {
            prototype_->add_property_names(names);
        }
    }

    gc_heap_ptr_untyped move(gc_heap& new_heap) {
        return new_heap.make<object>(std::move(*this));
    }
};

} // namespace mjs

#endif
