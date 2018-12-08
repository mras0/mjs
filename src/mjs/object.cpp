#include "object.h"

namespace mjs {

static_assert(gc_type_info_registration<object>::needs_fixup);
static_assert(!gc_type_info_registration<object>::needs_destroy);

object::object(const string& class_name, const object_ptr& prototype)
    : heap_(class_name.heap()) // A class will always have a class_name - grab heap from that
    , class_(class_name.unsafe_raw_get())
    , prototype_(prototype)
    , properties_(gc_vector<property>::make(heap_, 32))
    , value_(value::undefined) {
}


void object::fixup() {
    class_.fixup(heap_);
    prototype_.fixup(heap_);
    construct_.fixup(heap_);
    call_.fixup(heap_);
    properties_.fixup(heap_);
    value_.fixup(heap_);
}

std::vector<string> object::property_names() const {
    std::vector<string> names;
    add_property_names(names);
    return names;
}

void object::add_property_names(std::vector<string>& names) const {
    for (auto& p : properties_.dereference(heap_)) { 
        if (!p.has_attribute(property_attribute::dont_enum)) {
            names.push_back(p.key.track(heap_));
        }
    }
    if (prototype_) {
        prototype_.dereference(heap()).add_property_names(names);
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
    for (auto& p: properties_.dereference(heap_)) {
        const auto key = p.key.dereference(heap_).view();
        print_prop(key, p.value.get_value(heap_), key == L"constructor");
    }
    print_prop("[[Class]]", class_name(), true);
    print_prop("[[Prototype]]", prototype_ ? value{prototype_.track(heap())} : value::null, true);
    auto val = internal_value();
    if (val.type() != value_type::undefined) {
        print_prop("[[Value]]", val, true);
    }
    os << std::wstring(indent, ' ') << "}";
}

} // namespace mjs
