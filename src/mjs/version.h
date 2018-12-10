#ifndef MJS_VERSION_H
#define MJS_VERSION_H

#include <iosfwd>

namespace mjs {

enum class version {
    es1,
    es3,

    future,
};

constexpr version default_version = version::es3;
constexpr version supported_versions[] = { version::es1, version::es3 };


template<typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, version v);

} // namespace mjs

#endif