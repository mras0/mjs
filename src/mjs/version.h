#ifndef MJS_VERSION_H
#define MJS_VERSION_H

#include <iosfwd>

namespace mjs {

enum class version {
    es1,
    es3,
};

constexpr auto default_version = version::es3;

std::wostream& operator<<(std::wostream& os, version v);

} // namespace mjs

#endif