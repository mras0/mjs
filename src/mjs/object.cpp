#include "object.h"
#include "function_object.h"

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

void object::define_accessor_property(const string& name, const object_ptr& accessor, property_attribute attr) {
    assert(is_valid(attr));
    assert(accessor && !accessor->prototype() && (is_function(accessor->get(L"get")) || is_function(accessor->get(L"set"))));
    attr |= property_attribute::accessor;
    if (accessor->get(L"set").type() != value_type::undefined) {
        attr &= ~property_attribute::read_only;
    } else {
        attr |= property_attribute::read_only;
    }
    if (auto it = find(name.view()).first) {
        it->raw_put(value{accessor});
        it->attributes(attr);
        return;
    }
    assert(!has_property(name.view()));
    properties_.dereference(heap()).emplace_back(name, value{accessor}, attr);
}

void object::modify_accessor_object(const std::wstring_view name, const value& new_val, bool is_get) {
    assert(new_val.type() == value_type::undefined || is_function(new_val));
    auto it = find(name).first;
    if (!has_attributes(it->attributes(), property_attribute::accessor)) {
        throw std::logic_error{"modify_accessor_object called on non-accessor property"};
    }

    auto a = it->raw_get(heap_).object_value();
    assert(a && a->class_name().view() == L"Accessor" && !a->prototype());
    auto p = a->find(is_get ? L"get" : L"set").first;
    assert(p);
    p->raw_put(new_val);

    // Maintain invariant regarding the read_only attribute
    if (!is_get) {
        auto attr = it->attributes() & ~property_attribute::read_only;
        if (new_val.type() == value_type::undefined) {
            attr |= property_attribute::read_only;
        }
        it->attributes(attr);
    }
}

object_ptr object::get_accessor_property_object(const std::wstring_view name) {
    auto it = find(name).first;
    if (!has_attributes(it->attributes(), property_attribute::accessor)) {
        throw std::logic_error{"get_accessor_property_fields called on non-accessor property"};
    }
    return it->raw_get(heap_).object_value();
}

bool object::do_redefine_own_property(const string& name, const value& val, property_attribute attr) {
    if (auto it = find(name.view()).first) {
        it->put(*this, val);
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
        print_prop(key, p.get(*this), key == L"constructor");
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
            it->put(*this, val);
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

value object::property::get(const object& self) const {
    auto& h = self.heap();
    auto v = raw_get(h);
    if (is_accessor()) {
        assert(v.type() == value_type::object);
        auto g = v.object_value()->get(L"get");
        return g.type() != value_type::undefined ? call_function(g, value{h.unsafe_track(self)}, {}) : g;
    }
    return v;
}

value object::property::raw_get(gc_heap& heap) const {
    return value_.get_value(heap);
}

void object::property::raw_put(const value& val) {
    value_ = value_representation{val};
}

void object::property::put(const object& self, const value& val) {
    assert(!has_attributes(attributes_, property_attribute::read_only));
    if (is_accessor()) {
        auto& h = self.heap();
        auto s = value_.get_value(h).object_value()->get(L"set");
        call_function(s, value{h.unsafe_track(self)}, {val});
    } else {
        assert(val.type() != value_type::object || val.object_value()->class_name().view() != L"Accessor");
        raw_put(val);
    }
}

} // namespace mjs
