#include "version.h"
#include <ostream>

namespace mjs {

std::wostream& operator<<(std::wostream& os, version v) {
    return os << "version{" << (int)v << "}";
}

} // namespace mjs
