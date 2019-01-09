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

uint32_t index_value_from_string(const std::wstring_view& str) {
    const auto len = str.length();
    if (len == 0 || len > 10) {
        assert(len); // Shouldn't be passed the empty string
        return invalid_index_value;
    }
    uint32_t index = 0;
    for (uint32_t i = 0; i < len; ++i) {
        const auto ch = str[i];
        if (ch < L'0' || ch > L'9') {
            return invalid_index_value;
        }
        const auto last = index;
        index = index*10 + (ch - L'0');
        if (index < last) {
            // Overflow
            return invalid_index_value;
        }
    }
    return index;
}

} // namespace mjs
