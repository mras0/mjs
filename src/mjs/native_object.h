#ifndef MJS_NATIVE_OBJECT_H
#define MJS_NATIVE_OBJECT_H

#include "object.h"

namespace mjs {

#define DEFINE_NATIVE_PROPERTY(Class, Property) add_native_property<Class, &Class::get_##Property, &Class::put_##Property>(#Property, property_attribute::dont_enum | property_attribute::dont_delete)
#define DEFINE_NATIVE_PROPERTY_READONLY(Class, Property) add_native_property<Class, &Class::get_##Property>(#Property, property_attribute::dont_enum | property_attribute::dont_delete | property_attribute::read_only)

class native_object : public object {
public:
    value get(const std::wstring_view& name) const override {
        if (auto it = find(name)) {
            return it->get(*this);
        }
        return object::get(name);
    }

    bool do_put(const string& name, const value& val) {
        if (auto it = find(name.view())) {
            if (!it->has_attribute(property_attribute::read_only)) {
                it->put(*this, val);
            }
            return true;
        } else {
            return false;
        }
    }

    void put(const string& name, const value& val, property_attribute attr) override {
        if (!do_put(name, val)) {
            object::put(name, val, attr);
        }
    }

    bool can_put(const std::wstring_view& name) const override {
        if (auto it = find(name)) {
            return !it->has_attribute(property_attribute::read_only);
        }
        return object::can_put(name);
    }

    bool has_property(const std::wstring_view& name) const override {
        return find(name) || object::has_property(name);
    }

    bool delete_property(const std::wstring_view& name) override;

    bool check_own_property_attribute(const std::wstring_view& name, property_attribute mask, property_attribute expected) const override {
        if (auto it = find(name)) {
            return (it->attributes & mask) == expected;
        }
        return object::check_own_property_attribute(name, mask, expected);
    }

private:
    using get_func = value (*)(const native_object&);
    using put_func = void (*)(native_object&, const value&);

    struct native_object_property {
        char               name[32];
        property_attribute attributes;
        get_func           get;
        put_func           put;

        bool has_attribute(property_attribute a) const { return (attributes & a) == a; }

        native_object_property(const char* name, property_attribute attributes, get_func get, put_func put);
    };
    gc_heap_ptr_untracked<gc_vector<native_object_property>> native_properties_;

    native_object_property* find(const char* name) const;
    native_object_property* find(const std::wstring_view& v) const;

    void do_add_native_property(const char* name, property_attribute attributes, get_func get, put_func put);

protected:
    explicit native_object(const string& class_name, const object_ptr& prototype);

    template<typename T, value (T::* Get)() const, void (T::* Put)(const value&) = nullptr>
    void add_native_property(const char* name, property_attribute attributes) {
        static_assert(std::is_base_of_v<native_object, T>);

        put_func put = nullptr;
        if (Put != nullptr) {
            put = [](native_object& o, const value& v) {
                (static_cast<T&>(o).*Put)(v);
            };
        }

        do_add_native_property(name, attributes, [](const native_object& o) {
            return (static_cast<const T&>(o).*Get)();
        }, put);
    }

    void add_property_names(std::vector<string>& names) const override;

    void update_property_attributes(const char* name, property_attribute attributes) {
        auto it = find(name);
        assert(it);
        assert(((attributes & property_attribute::read_only) != property_attribute::none) || it->put);
        it->attributes = attributes;
    }

    void fixup();

    void do_debug_print_extra(std::wostream& os, int indent_incr, int max_nest, int indent) const override;
};

} // namespace mjs

#endif