#include "object.h"

namespace mjs {

static_assert(gc_type_info_registration<object>::needs_fixup);
static_assert(!gc_type_info_registration<object>::needs_destroy);

object::object(const string& class_name, const object_ptr& prototype)
    : heap_{class_name.heap()} // A class will always have a class_name - grab heap from that
    , class_{class_name.unsafe_raw_get()}
    , prototype_{prototype}
    , properties_{gc_vector<property>::make(heap_, 4)}
    , extensible_{true} {
}


void object::fixup() {
    class_.fixup(heap_);
    prototype_.fixup(heap_);
    properties_.fixup(heap_);
}

bool object::has_property(const std::wstring_view& name) const {
    if (own_property_attributes(name) != property_attribute::invalid) {
        return true;
    }
    return prototype_ && prototype_.dereference(heap()).has_property(name);
}

std::vector<string> object::enumerable_property_names() const {
    std::vector<string> names;
    add_own_property_names(names, true);
    if (prototype_) {
        prototype_.dereference(heap()).add_own_property_names(names, true);
    }
    return names;
}

std::vector<string> object::own_property_names(bool check_enumerable) const {
    std::vector<string> names;
    add_own_property_names(names, check_enumerable);
    return names;
}

bool object::do_redefine_own_property(const string& name, const value& val, property_attribute attr) {
    if (auto it = find(name.view()).first; it) {
        it->put(val);
        it->attributes(attr);
        return true;
    }
    assert(!has_property(name.view()));
    put(name, val, attr);
    return true;
}

property_attribute object::do_own_property_attributes(const std::wstring_view& name) const {
    if (auto it = find(name).first; it) {
        return it->attributes();
    }
    return property_attribute::invalid;
}

void object::add_own_property_names(std::vector<string>& names, bool check_enumerable) const {
    for (auto& p : properties_.dereference(heap_)) { 
        if (!check_enumerable || !has_attributes(p.attributes(), property_attribute::dont_enum)) {
            names.push_back(p.key(heap_));
        }
    }
}

void object::debug_print(std::wostream& os, int indent_incr, int max_nest, int indent) const {
    if (indent / indent_incr > 4 || max_nest <= 0) {
        os << "[Object " << class_name() << "]";
        return;
    }
    auto indent_string = std::wstring(indent + indent_incr, ' ');
    auto print_prop = [&](const auto& name, const auto& val, bool internal) {
        os << indent_string << name << ": ";
        mjs::debug_print(os, mjs::value{val}, indent_incr, max_nest > 1 && internal ? 1 : max_nest - 1, indent + indent_incr);
        os << "\n";
    };
    os << "{\n";
    print_prop("[[Class]]", class_name(), true);
    print_prop("[[Prototype]]", prototype_ ? value{prototype_.track(heap())} : value::null, true);
    for (auto& p: properties_.dereference(heap_)) {
        const auto key = p.key(heap_).view();
        print_prop(key, p.get(heap_), key == L"constructor");
    }
    do_debug_print_extra(os, indent_incr, max_nest, indent+indent_incr);
    os << std::wstring(indent, ' ') << "}";
}

bool object::can_put(const std::wstring_view& name) const {
    auto a = own_property_attributes(name);
    if (is_valid(a)) {
        assert(!has_attributes(a, property_attribute::accessor));
        return !has_attributes(a, property_attribute::read_only);
    }
    if (!prototype_) {
        return extensible_;
    }
    return prototype_.dereference(heap()).can_put(name);
}

void object::put(const string& name, const value& val, property_attribute attr) {
    // See if there is already a property with this name
    auto& props = properties_.dereference(heap_);
    if (auto [it, pp] = deep_find(name.view()); it) {
        // CanPut?
        if (has_attributes(it->attributes(), property_attribute::read_only)) {
            return;
        }
        // Did the property come from this object's property list?
        if (pp == &props) {
            // Yes, update
            it->put(val);
            return;
        }
        // Handle as insertion
    }
    if (!extensible_) {
        return;
    }
    props.emplace_back(name, val, attr);
}

bool object::delete_property(const std::wstring_view& name) {
    auto [it, props] = find(name);
    if (!it) {
        return true;

    }
    if (has_attributes(it->attributes(), property_attribute::dont_delete)) {
        return false;
    }
    props->erase(it);
    return true;
}

void object::property::fixup(gc_heap& h) {
    key_.fixup(h);
    value_.fixup(h);
}

value object::property::get(gc_heap& h) const {
    assert(!is_accessor());
    return value_.get_value(h);
}

void object::property::put(const value& val) {
    assert(!is_accessor());
    value_ = value_representation{val};
}



} // namespace mjs
