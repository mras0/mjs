#include "string.h"
#include <ostream>
#include <sstream>
#include <cstring>
#include <cmath>

namespace mjs {

static_assert(!gc_type_info_registration<gc_string>::needs_destroy);
static_assert(!gc_type_info_registration<gc_string>::needs_fixup);

std::ostream& operator<<(std::ostream& os, const string& s) {
    auto v = s.view();
    return os << std::string(v.begin(), v.end());
}

std::wostream& operator<<(std::wostream& os, const string& s) {
    return os << s.view();
}

double to_number(const std::wstring_view& s) {
    // TODO: Implement real algorithm from §9.3.1 ToNumber Applied to the String Type
    if (s.empty()) {
        return 0;
    }
    std::wistringstream wis{std::wstring{s}};
    double d;
    return (wis >> d) && !wis.rdbuf()->in_avail() ? d : NAN;
}

double to_number(const string& s) {
    // TODO: Implement real algorithm from §9.3.1 ToNumber Applied to the String Type
    return to_number(s.view());
}

} // namespace mjs
