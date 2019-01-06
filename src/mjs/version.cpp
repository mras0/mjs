#include "version.h"
#include <ostream>
#include <cassert>

namespace mjs {

template<typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, version v) {
    switch (v) {
    case version::es1:      return os << "ES1";
    case version::es3:      return os << "ES3";
    case version::es5:      return os << "ES5";
    case version::future:   return os << "Future";
    }
    assert(false);
    return os << "version{" << (int)v << "}";
}

template std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, version v);
template std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t>& os, version v);

} // namespace mjs
