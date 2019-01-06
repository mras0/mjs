#ifndef MJS_PROPERTY_ATTRIBUTE
#define MJS_PROPERTY_ATTRIBUTE

namespace mjs {

enum class property_attribute {
    none = 0,
    read_only = 1<<0,
    dont_enum = 1<<1,
    dont_delete = 1<<2,
    internal = 1<<3,
};

constexpr inline property_attribute operator|(property_attribute l, property_attribute r) {
    return static_cast<property_attribute>(static_cast<int>(l) | static_cast<int>(r));
}

constexpr inline property_attribute operator|=(property_attribute& l, property_attribute r) {
    return l = static_cast<property_attribute>(static_cast<int>(l) | static_cast<int>(r));
}

constexpr inline property_attribute operator&(property_attribute l, property_attribute r) {
    return static_cast<property_attribute>(static_cast<int>(l) & static_cast<int>(r));
}

} // namespace mjs

#endif
