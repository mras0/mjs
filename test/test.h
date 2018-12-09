#ifndef MJS_TEST_H
#define MJS_TEST_H

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string_view>

#include <mjs/value.h>
#include <mjs/version.h>

#define REQUIRE_EQ(l, r) do { auto _l = l; auto _r = r; if (_l != _r) { std::wcerr << __func__ << " in " << __FILE__ << ":" << __LINE__ << ":\n" << #l << " != " << #r << "\n" << _l << " != " << _r << "\n"; std::abort(); } } while (0)
#define REQUIRE(e) REQUIRE_EQ(e, true)

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


extern mjs::version parser_version;

extern void run_test(const std::wstring_view& text, const mjs::value& expected);


#endif