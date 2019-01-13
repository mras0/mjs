#ifndef MJS_OBJECT_H
#define MJS_OBJECT_H

#include "value.h"
#include "value_representation.h"
#include "property_attribute.h"
#include "gc_vector.h"

namespace mjs {

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
    object_ptr prototype() const { return prototype_ ? prototype_.track(heap_) : nullptr; }

    //
    // [[Class]] ()
    //
    // The value of the [[Class]] property is defined by this specification for every kind of built-in object. The value of
    // the  [[Class]]  property  of  a  host  object  may  be  any  value,  even  a  value  used  by  a  built-in  object  for  its  [[Class]]
    // property. Note that this specification does not provide any means for a program to access the value of a [[Class]]
    // property; it is used internally to distinguish different kinds of built-in objects
    string class_name() const { return class_.track(heap_); }

    // [[Get]] (PropertyName)
    virtual value get(const std::wstring_view& name) const {
        auto it = deep_find(name).first;
        return it ? it->get(heap_) : value::undefined;
    }

    bool redefine_own_property(const string& name, const value& val, property_attribute attr) {
        return do_redefine_own_property(name, val, attr);
    }

    // [[Put]] (PropertyName, Value)
    virtual void put(const string& name, const value& val, property_attribute attr = property_attribute::none);

    // [[CanPut]] (PropertyName)
    bool can_put(const std::wstring_view& name) const;

    // [[HasProperty]] (PropertyName)
    bool has_property(const std::wstring_view& name) const;

    // [[Delete]] (PropertyName)
    virtual bool delete_property(const std::wstring_view& name);

    // Get attributes of own property, returns invalid if not found
    property_attribute own_property_attributes(const std::wstring_view& name) const {
        return do_own_property_attributes(name);
    }

    virtual value_type default_value_type() const {
        // When hint is undefined, assume Number unless it's a Date object in which case assume String
        return value_type::number;
    }

    std::vector<string> enumerable_property_names() const;
    std::vector<string> own_property_names(bool check_enumerable) const;

    void debug_print(std::wostream& os, int indent_incr, int max_nest = INT_MAX, int indent = 0) const;

protected:
    explicit object(const string& class_name, const object_ptr& prototype);
    object(object&& o) = default;
    void fixup();

    virtual bool do_redefine_own_property(const string& name, const value& val, property_attribute attr);
    virtual property_attribute do_own_property_attributes(const std::wstring_view& name) const;
    virtual void add_own_property_names(std::vector<string>& names, bool check_enumerable) const;
    virtual void do_debug_print_extra(std::wostream& os, int indent_incr, int max_nest, int indent) const {
        (void)os; (void)indent_incr; (void)max_nest; (void)indent;
    }

private:
    class property {
    public:
        explicit property(const string& key, const mjs::value& v, property_attribute attr)
            : key_{key.unsafe_raw_get()}
            , attributes_{attr}
            , value_{v} {
        }

        void fixup(gc_heap& h);
        
        bool is_accessor() const { return has_attributes(attributes_, property_attribute::accessor); }
        
        property_attribute attributes() const { return attributes_; }
        void attributes(property_attribute a) { attributes_ = a; }

        string key(gc_heap& h) const { return key_.track(h); }

        value get(gc_heap& h) const;
        void put(const value& val);

        bool key_matches(gc_heap& h, const std::wstring_view k) { return key_.dereference(h).view() == k; }

    private:
        gc_heap_ptr_untracked<gc_string>    key_;
        property_attribute                  attributes_;
        value_representation                value_; // holds object with get/set methods if is_accessor()
    };

    gc_heap&                                   heap_;
    gc_heap_ptr_untracked<gc_string>           class_;
    gc_heap_ptr_untracked<object>              prototype_;
    gc_heap_ptr_untracked<gc_vector<property>> properties_;

    std::pair<property*, gc_vector<property>*> find(const std::wstring_view key) const {
        auto& props = properties_.dereference(heap_);
        for (auto& p: props) {
            if (p.key_matches(heap_, key)) {
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
