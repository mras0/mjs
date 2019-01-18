#ifndef MJS_PROPERTY_ATTRIBUTE
#define MJS_PROPERTY_ATTRIBUTE

#include <iosfwd>
#include <cassert>

namespace mjs {

enum class property_attribute {
    none         = 0,
    read_only    = 1<<0, // not writeable
    dont_enum    = 1<<1, // not enumerable
    dont_delete  = 1<<2, // not configurable
    accessor     = 1<<3, // has get/set instead of a value
    valid_mask   = read_only | dont_enum | dont_delete | accessor,
    invalid      = 1 + valid_mask, // One larger than the sum of the valid flags
};

constexpr property_attribute operator|(property_attribute l, property_attribute r) {
    return static_cast<property_attribute>(static_cast<int>(l) | static_cast<int>(r));
}

constexpr property_attribute operator|=(property_attribute& l, property_attribute r) {
    return l = static_cast<property_attribute>(static_cast<int>(l) | static_cast<int>(r));
}

constexpr property_attribute operator&(property_attribute l, property_attribute r) {
    return static_cast<property_attribute>(static_cast<int>(l) & static_cast<int>(r));
}

constexpr property_attribute operator&=(property_attribute& l, property_attribute r) {
    return l = static_cast<property_attribute>(static_cast<int>(l) & static_cast<int>(r));
}

constexpr bool is_valid(property_attribute a) {
    assert(static_cast<unsigned>(a) <= static_cast<unsigned>(property_attribute::invalid));
    return a != property_attribute::invalid;
}

constexpr property_attribute operator~(property_attribute a) {
    assert(is_valid(a));
    return static_cast<property_attribute>(~static_cast<int>(a) & static_cast<int>(property_attribute::valid_mask));
}

constexpr bool has_attributes(property_attribute attributes, property_attribute check) {
    assert(is_valid(attributes) && is_valid(check) && check != property_attribute::none);
    return (attributes & check) == check;
}

template<typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, property_attribute a);

} // namespace mjs

#endif
