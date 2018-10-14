#ifndef MJS_VALUE_H
#define MJS_VALUE_H

#include <iosfwd>
#include <string>
#include <string_view>
#include <cassert>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>

#include "string.h"

// TODO: Garbage collect / cycle breaking...

namespace mjs {

enum class value_type {
    undefined, null, boolean, number, string, object,
    // internal
    reference, native_function
};
const char* string_value(value_type t);
template<typename CharT, typename CharTraitsT>
std::basic_ostream<CharT, CharTraitsT>& operator<<(std::basic_ostream<CharT, CharTraitsT>& os, value_type t) {
    return os << string_value(t);
}

enum class property_attribute {
    none = 0,
    read_only = 1<<0,
    dont_enum = 1<<1,
    dont_delete = 1<<2,
    internal = 1<<3,
};

inline property_attribute operator|(property_attribute l, property_attribute r) {
    return static_cast<property_attribute>(static_cast<int>(l) | static_cast<int>(r));
}

inline property_attribute operator&(property_attribute l, property_attribute r) {
    return static_cast<property_attribute>(static_cast<int>(l) & static_cast<int>(r));
}

class object;
class value;
using object_ptr = std::shared_ptr<object>;
using native_function_type = std::function<value (const value&, const std::vector<value>&)>;

// §8.7
class reference {
public:
    explicit reference(const object_ptr& base, const string& property_name) : base_(base), property_name_(property_name) {
        assert(base);
    }

    const object_ptr& base() const { return base_; }
    const string& property_name() const { return property_name_; }

    const value& get_value() const;
    void put_value(const value& val) const;

private:
    object_ptr base_;
    string property_name_;
};


class value {
public:
    explicit value() : type_(value_type::undefined) {}
    explicit value(bool b) : type_(value_type::boolean), b_(b) {}
    explicit value(double n) : type_(value_type::number), n_(n) {}
    explicit value(const string& s) : type_(value_type::string), s_(s) {}
    explicit value(const object_ptr& o) : type_(value_type::object), o_(o) {}
    explicit value(const reference& r) : type_(value_type::reference), r_(r) {}
    explicit value(const native_function_type& f) : type_(value_type::native_function), f_(f) {}
    value(const value& rhs);
    value(value&& rhs);
    ~value() { destroy(); }

    value& operator=(const value& rhs);
    value& operator=(value&& rhs);

    value_type type() const { return type_; }
    bool boolean_value() const { assert(type_ == value_type::boolean); return b_; }
    double number_value() const { assert(type_ == value_type::number); return n_; }
    const string& string_value() const { assert(type_ == value_type::string); return s_; }
    const object_ptr& object_value() const { assert(type_ == value_type::object); return o_; }
    const reference& reference_value() const { assert(type_ == value_type::reference); return r_; }
    const native_function_type& native_function_value() const { assert(type_ == value_type::native_function); return f_; }

    static const value undefined;
    static const value null;
private:
    explicit value(value_type t) : type_(t) { assert(t == value_type::undefined || t == value_type::null); }

    void destroy();

    value_type type_;
    union {
        bool b_;
        double n_;
        string s_;
        object_ptr o_;
        reference r_;
        native_function_type f_;
    };
};
std::ostream& operator<<(std::ostream& os, const value& v);
std::wostream& operator<<(std::wostream& os, const value& v);
bool operator==(const value& l, const value& r);
inline bool operator!=(const value& l, const value& r) {
    return !(l == r);
}

inline value get_value(const value& v) {
    return v.type() == value_type::reference ? v.reference_value().get_value() : v;
}

inline value get_value(value&& v) {
    return v.type() == value_type::reference ? v.reference_value().get_value() : std::move(v);
}

[[nodiscard]] bool put_value(const value& ref, const value& val);

class object {
public:
    static object_ptr make(const string& class_name, const object_ptr& prototype = nullptr) {
        struct make_shared_helper : object {
            make_shared_helper(const string& class_name, const object_ptr& prototype) : object(class_name, prototype) {}
        };

        return std::make_shared<make_shared_helper>(class_name, prototype);
    }

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

    // [[Get]] (PropertyName)
    const value& get(const string& name) {
        if (auto it = properties_.find(name); it != properties_.end()) {
            return it->second.val;
        }
        return prototype_ ? prototype_->get(name) : value::undefined;
    }
 
    // [[Put]] (PropertyName, Value)
    void put(const string& name, const value& val, property_attribute attr = property_attribute::none) {
        if (!can_put(name)) {
            return;
        }
        if (auto it = properties_.find(name); it != properties_.end()) {
            it->second.val = val;
        } else {
            properties_[name] = property{val, attr};
        }
    }

    // [[CanPut]] (PropertyName)
    bool can_put(const string& name) const {
        if (auto it = properties_.find(name); it != properties_.end()) {
            return !it->second.has_attribute(property_attribute::read_only);
        }
        return prototype_ ? prototype_->can_put(name) : true;
    }

    // [[HasProperty]] (PropertyName)
    bool has_property(const string& name) const {
        if (auto it = properties_.find(name); it != properties_.end()) {
            return true;
        }
        return prototype_ ? prototype_->has_property(name) : false;
    }

    // [[Delete]] (PropertyName)
    bool delete_property(const string& name) {
        auto it = properties_.find(name);
        if (it == properties_.end()) {
            return true;

        }
        if (it->second.has_attribute(property_attribute::dont_delete)) {
            return false;
        }
        properties_.erase(it);
        return true;
    }

    // [[DefaultValue]] (Hint)
    value default_value(value_type hint) const {
        throw std::runtime_error(std::string("Not implemented. default_value hint=") + string_value(hint));
    }

    // [[Construct]] (Arguments...)
    void construct_function(const native_function_type& f) { construct_ = f; }
    const native_function_type& construct_function() const { return construct_; }

    // [[Call]] (Arguments...)
    void call_function(const native_function_type& f) { call_ = f; }
    const native_function_type& call_function() const { return call_; }

private:
    struct property {
        value val;
        property_attribute attr;

        bool has_attribute(property_attribute a) const { return (attr & a) == a; }
    };

    explicit object(const string& class_name, const object_ptr& prototype) : class_(class_name), prototype_(prototype){}

    string class_;
    object_ptr prototype_;
    native_function_type construct_;
    native_function_type call_;
    std::unordered_map<string, property> properties_;
};

// §9 Type Conversions
value to_primitive(const value& v, value_type hint = value_type::undefined);
bool to_boolean(const value& v);
double to_number(const value& v);
double to_integer(double n);
double to_integer(const value& v);
int32_t to_int32(double n);
int32_t to_int32(const value& v);
uint32_t to_uint32(double n);
uint32_t to_uint32(const value& v);
uint16_t to_uint16(double n);
uint16_t to_uint16(const value& v);
string to_string(double n);
string to_string(const value& v);
// TODO: §9.9 ToObject


} // namespace mjs

#endif
