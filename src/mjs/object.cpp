#include "object.h"

namespace mjs {

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
    auto& props = properties_.dereference(heap_);
    for (auto it = props.begin(); it != props.end(); ++it) {
        if (it.key()->view() == L"constructor") {
            print_prop(it.key()->view(), it.value(), true);
        } else {
            print_prop(it.key()->view(), it.value(), false);
        }
    }
    print_prop("[[Class]]", class_name(), true);
    print_prop("[[Prototype]]", prototype_ ? value{prototype_.track(heap())} : value::null, true);
    auto val = internal_value();
    if (val.type() != value_type::undefined) {
        print_prop("[[Value]]", val, true);
    }
    os << std::wstring(indent, ' ') << "}";
}

bool object::fixup(gc_heap& new_heap) {
    class_.fixup_after_move(new_heap, heap());
    prototype_.fixup_after_move(new_heap, heap());
    construct_.fixup_after_move(new_heap, heap());
    call_.fixup_after_move(new_heap, heap());
    properties_.fixup_after_move(new_heap, heap());
    value_.fixup_after_move(new_heap, heap());
    return true;
}


} // namespace mjs
