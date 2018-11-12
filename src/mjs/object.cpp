#include "object.h"

namespace mjs {

void object::debug_print(std::wostream& os, int indent_incr, int max_nest, int indent) const {
    if (indent / indent_incr > 4 || max_nest <= 0) {
        os << "[Object " << class_ << "]";
        return;
    }
    auto indent_string = std::wstring(indent + indent_incr, ' ');
    auto print_prop = [&](const auto& name, const auto& val, bool internal) {
        os << indent_string << name << ": ";
        mjs::debug_print(os, mjs::value{val}, indent_incr, max_nest > 1 && internal ? 1 : max_nest - 1, indent + indent_incr);
        os << "\n";
    };
    os << "{\n";
    //for (const auto& p : properties_) {
    for (auto it = properties_->begin(); it != properties_->end(); ++it) {
        if (it.key()->view() == L"constructor") {
            print_prop(it.key()->view(), it.value(), true);
        } else {
            print_prop(it.key()->view(), it.value(), false);
        }
    }
    print_prop("[[Class]]", class_, true);
    print_prop("[[Prototype]]", prototype_ ? value{prototype_} : value::null, true);
    if (value_.type() != value_type::undefined) {
        print_prop("[[Value]]", value_, true);
    }
    os << std::wstring(indent, ' ') << "}";
}

} // namespace mjs
