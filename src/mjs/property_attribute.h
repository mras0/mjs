#ifndef MJS_PROPERTY_ATTRIBUTE
#define MJS_PROPERTY_ATTRIBUTE

namespace mjs {

enum class property_attribute {
    none         = 0,
    read_only    = 1<<0,
    dont_enum    = 1<<1,
    dont_delete  = 1<<2,
    invalid      = 1 + (read_only | dont_enum | dont_delete), // One larger than the sum of the valid flags
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

constexpr inline bool is_valid(property_attribute a) {
    return static_cast<unsigned>(a) < static_cast<unsigned>(property_attribute::invalid);
}

} // namespace mjs

#endif
