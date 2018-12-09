#include <cstdlib>
#include <iostream>

#define REQUIRE_EQ(l, r) do { auto _l = l; auto _r = r; if (_l != _r) { std::wcerr << __func__ << " in " << __FILE__ << ":" << __LINE__ << ":\n" << #l << " != " << #r << "\n" << _l << " != " << _r << "\n"; std::abort(); } } while (0)
#define REQUIRE(e) REQUIRE_EQ(e, true)

// TODO: move this
#include <vector>
template<typename CharT, typename T>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const std::vector<T>& v) {
    os << "{";
    for (size_t i = 0; i < v.size(); ++i) {
        os << (i ? ", " : "") << v[i];
    }
    return os << "}";
}

#include <mjs/value.h>
template<typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const mjs::value& v) {
    debug_print(os, v, 4);
    return os;
}
