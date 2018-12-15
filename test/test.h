#ifndef MJS_TEST_H
#define MJS_TEST_H

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string_view>

#include <mjs/value.h>
#include <mjs/version.h>

#define REQUIRE_EQ(l, r) do { auto _l = l; auto _r = r; if (_l != _r) { std::wcerr << __func__ << " in " << __FILE__ << ":" << __LINE__ << ":\n" << #l << " != " << #r << "\n" << _l << " != " << _r << "\n"; std::abort(); } } while (0)
#define REQUIRE(e) REQUIRE_EQ(!!e, true)

template<typename CharT, typename T>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const std::vector<T>& v) {
    os << "{";
    for (size_t i = 0; i < v.size(); ++i) {
        os << (i ? ", " : "") << v[i];
    }
    return os << "}";
}

template<typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const mjs::value& v) {
    debug_print(os, v, 4);
    return os;
}

extern mjs::version tested_version();
extern void tested_version(mjs::version ver);
extern void run_test_debug_pos(const char* func, const char* file, int line);
extern void run_test(const std::wstring_view& text, const mjs::value& expected);
#define RUN_TEST run_test_debug_pos(__func__, __FILE__, __LINE__); run_test// yuck...


namespace mjs {

class token;
bool operator==(const token& l, const token& r);

inline bool operator!=(const token& l, const token& r) {
    return !(l == r);
}

} // namespace mjs



#endif