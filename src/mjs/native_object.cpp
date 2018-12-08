#include "native_object.h"
#include <cstring>

namespace mjs {

native_object::native_object(const string& class_name, const object_ptr& prototype)
    : object(class_name, prototype)
    , native_properties_(gc_vector<native_object_property>::make(heap(), 16)) {
}

void native_object::fixup() {
    native_properties_.fixup(heap());
    object::fixup();
}

bool native_object::delete_property(const std::wstring_view& name) {
    if (auto it = find(name)) {
        if (it->has_attribute(property_attribute::dont_delete)) {
            return false;
        }
        assert(false);
        throw std::runtime_error("TODO: Handle delete of native property");
    }
    return object::delete_property(name);
}


native_object::native_object_property* native_object::find(const char* name) const {
    for (auto& p: native_properties_.dereference(heap())) {
        if (!strcmp(p.name, name)) {
            return &p;
        }
    }
    return nullptr;
}

native_object::native_object_property* native_object::find(const std::wstring_view& v) const {
    const auto len = v.length();
    if (len >= sizeof(native_object_property::name)) {
        return nullptr;
    }
    char name[sizeof(native_object_property::name)];
    for (uint32_t i = 0; i < len; ++i) {
        if (v[i] < 32 || v[i] >= 127) {
            return nullptr;
        }
        name[i] = static_cast<char>(v[i]);
    }
    name[len] = '\0';
    return find(name);
}

void native_object::do_add_native_property(const char* name, property_attribute attributes, get_func get, put_func put) {
    assert(find(name) == nullptr);
    assert(((attributes & property_attribute::read_only) != property_attribute::none) == !put);
    native_properties_.dereference(heap()).push_back(native_object_property(name, attributes, get, put));
}

void native_object::add_property_names(std::vector<string>& names) const {
    for (const auto& p: native_properties_.dereference(heap())) {
        if (!p.has_attribute(property_attribute::dont_enum)) {
            names.emplace_back(heap(), p.name);
        }
    }
    object::add_property_names(names);
}

native_object::native_object_property::native_object_property(const char* name, property_attribute attributes, get_func get, put_func put)
    : attributes(attributes)
    , get(get)
    , put(put) {
    assert(std::strlen(name) < sizeof(this->name));
    strcpy(this->name, name);
}


} // namespace mjs