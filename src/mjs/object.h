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
    static auto make(gc_heap& h, const string& class_name, const object_ptr& prototype) {
        return h.make<object>(h, class_name, prototype);
    }

    virtual ~object() {}

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
    value get(const std::wstring_view& name) const {
        auto [it, pp] = deep_find(name);
        return it != pp->end() ? it.value() : value::undefined;
    }

    // [[Put]] (PropertyName, Value)
    virtual void put(const string& name, const value& val, property_attribute attr = property_attribute::none) {
        // See if there is already a property with this name
        auto& props = properties_.dereference(heap_);
        if (auto [it, pp] = deep_find(name.view()); it != pp->end()) {
            // CanPut?
            if (it.has_attribute(property_attribute::read_only)) {
                return;
            }
            // Did the property come from this object's property list?
            if (pp == &props) {
                // Yes, update
                it.value(val);
                return;
            }
            // Handle as insertion
        }
        // Room to insert another element?
        if (props.length() != props.capacity()) {
            // Yes, insert into existing table
            props.insert(name, val, attr);
        } else {
            // No, increase the capacity
            properties_ = props.copy_with_increased_capacity();
            // let props (old properties_) be collected
            // MUST dereference again here
            properties_.dereference(heap_).insert(name, val, attr);
        }
    }

    // [[CanPut]] (PropertyName)
    bool can_put(const std::wstring_view& name) const {
        auto [it, pp] = deep_find(name);
        return it != pp->end() ? !it.has_attribute(property_attribute::read_only) : true;
    }

    // [[HasProperty]] (PropertyName)
    bool has_property(const std::wstring_view& name) const {
        auto [it, pp] = deep_find(name);
        return it != pp->end();
    }

    // [[Delete]] (PropertyName)
    bool delete_property(const std::wstring_view& name) {
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

    std::vector<string> property_names() const;

    virtual void debug_print(std::wostream& os, int indent_incr, int max_nest = INT_MAX, int indent = 0) const;

protected:
    explicit object(gc_heap& heap, const string& class_name, const object_ptr& prototype);
    object(object&& o) = default;
    void fixup();

private:
    gc_heap& heap_;
    gc_heap_ptr_untracked<gc_string>    class_;
    gc_heap_ptr_untracked<object>       prototype_;
    gc_heap_ptr_untracked<gc_function>  construct_;
    gc_heap_ptr_untracked<gc_function>  call_;
    gc_heap_ptr_untracked<gc_table>     properties_;
    value_representation                value_;

    void add_property_names(std::vector<string>& names) const;

    std::pair<gc_table::entry, gc_table*> deep_find(const std::wstring_view& key) const {
        auto& props = properties_.dereference(heap_);
        auto it = props.find(key);
        return it != props.end() || !prototype_ ? std::make_pair(it, &props) : prototype_.dereference(heap_).deep_find(key);
    }
};

} // namespace mjs

#endif
