#ifndef MJS_VALUE_H
#define MJS_VALUE_H

#include <iosfwd>
#include <string>
#include <string_view>
#include <cassert>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
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

constexpr bool is_primitive(value_type t) {
    return t == value_type::undefined ||t == value_type::null || t == value_type::boolean || t == value_type::number || t == value_type::string;
}

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

constexpr inline property_attribute operator|(property_attribute l, property_attribute r) {
    return static_cast<property_attribute>(static_cast<int>(l) | static_cast<int>(r));
}

constexpr inline property_attribute operator&(property_attribute l, property_attribute r) {
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
    explicit value(const void*) = delete;
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

class object : public std::enable_shared_from_this<object> {
public:
    static object_ptr make(const string& class_name, const object_ptr& prototype) {
        struct make_shared_helper : object {
            make_shared_helper(const string& class_name, const object_ptr& prototype) : object(class_name, prototype) {}
        };

        return std::make_shared<make_shared_helper>(class_name, prototype);
    }

    virtual ~object() {
        all_objects_.erase(this);
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
    const value& internal_value() const { return value_; }
    void internal_value(const value& v) { value_ = v; }

    // [[Get]] (PropertyName)
    const value& get(const string& name) const {
        if (auto it = properties_.find(name); it != properties_.end()) {
            return it->second.val;
        }
        return prototype_ ? prototype_->get(name) : value::undefined;
    }

    // [[Put]] (PropertyName, Value)
    virtual void put(const string& name, const value& val, property_attribute attr = property_attribute::none) {
        if (auto it = properties_.find(name); it != properties_.end()) {
            // CanPut?
            if (!it->second.has_attribute(property_attribute::read_only)) {
                it->second.val = val;
            }
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
    virtual value default_value(value_type hint) {
        if (hint == value_type::undefined) {
            // When hint is undefined, assume Number unless it's a Date object in which case assume String
            hint = value_type::number;
        }

        assert(hint == value_type::number || hint == value_type::string);
        for (int i = 0; i < 2; ++i) {
            const char* id = (hint == value_type::string) ^ i ? "toString" : "valueOf";
            const auto& fo = get(mjs::string{id});
            if (fo.type() != value_type::object) {
                continue;
            }
            const auto& f = fo.object_value()->call_function();
            if (!f) {
                continue;
            }
            auto v = f(mjs::value{shared_from_this()}, {});
            if (!is_primitive(v.type())) {
                continue;
            }
            return v;
        }

        throw std::runtime_error("default_value() not implemented");
    }

    // [[Construct]] (Arguments...)
    void construct_function(const native_function_type& f) { construct_ = f; }
    const native_function_type& construct_function() const { return construct_; }

    // [[Call]] (Arguments...)
    void call_function(const native_function_type& f) { call_ = f; }
    const native_function_type& call_function() const { return call_; }

    std::vector<string> property_names() const {
        std::unordered_set<string> names;
        add_property_names(names);
        return std::vector<string>(std::make_move_iterator(names.begin()), std::make_move_iterator(names.end()));
    }

    static size_t object_count() { return all_objects_.size(); }

    static void garbage_collect(const std::vector<object_ptr>& roots);

    virtual void debug_print(std::wostream& os, int indent_incr, int max_nest = INT_MAX, int indent = 0) const;

protected:
    explicit object(const string& class_name, const object_ptr& prototype) : class_(class_name), prototype_(prototype) {
        all_objects_.insert(this);
    }

    bool has_own_property(const string& name) const {
        return properties_.find(name) != properties_.end();
    }

    void gc_visit(std::unordered_set<const object*>& live_objects) const;
    void clear();

private:
    object(const object&) = delete;
    static std::unordered_set<object*> all_objects_;

    struct property {
        value val;
        property_attribute attr;

        bool has_attribute(property_attribute a) const { return (attr & a) == a; }
    };
    using property_map = std::unordered_map<string, property>;

    string class_;
    object_ptr prototype_;
    value value_;
    native_function_type construct_;
    native_function_type call_;
    property_map properties_;

    void add_property_names(std::unordered_set<string>& names) const {
        for (const auto& p: properties_) {
            if (!p.second.has_attribute(property_attribute::dont_enum)) {
                names.insert(p.first);
            }
        }
        if (prototype_) {
            prototype_->add_property_names(names);
        }
    }
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

void debug_print(std::wostream& os, const value& v, int indent_incr, int max_nest = INT_MAX, int indent = 0);

[[noreturn]] void throw_runtime_error(const std::string_view& s, const char* file, int line);
[[noreturn]] void throw_runtime_error(const std::wstring_view& s, const char* file, int line);

#define THROW_RUNTIME_ERROR(msg) ::mjs::throw_runtime_error(msg, __FILE__, __LINE__)
#define NOT_IMPLEMENTED(o) do { std::wostringstream _woss; _woss << "Not implemented: " << o; THROW_RUNTIME_ERROR(_woss.str()); } while (0)



} // namespace mjs

#endif
