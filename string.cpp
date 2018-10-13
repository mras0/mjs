#include "string.h"
#include <ostream>
#include <sstream>
#include <cstring>
#include <cmath>

namespace mjs {

string::string(const char* str) : s_(str, str+std::strlen(str)) {
}

std::ostream& operator<<(std::ostream& os, const string& s) {
    auto v = s.view();
    return os << std::string(v.begin(), v.end());
}

std::wostream& operator<<(std::wostream& os, const string& s) {
    return os << s.view();
}

double to_number(const string& s) {
    // TODO: Implement real algorithm from §9.3.1 ToNumber Applied to the String Type
    std::wistringstream wis{s.str()};
    double d;
    return wis >> d ? d : NAN;
}

} // namespace mjs
