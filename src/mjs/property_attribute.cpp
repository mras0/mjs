#include "property_attribute.h"
#include <ostream>

namespace mjs {

template<typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, property_attribute a) {
    if (!is_valid(a)) {
        return os << "property_attribute::invalid";
    } else if (a == property_attribute::none) {
        return os << "property_attribute::none";
    }


    bool empty = true;
    for (const auto f: { property_attribute::read_only, property_attribute::dont_enum, property_attribute::dont_delete, property_attribute::accessor }) {
        if (has_attributes(a, f)) {
            if (!empty) {
                os << "|";
            }
            os << "property_attribute::";
            switch (f) {
            case property_attribute::read_only: os << "read_only"; break;
            case property_attribute::dont_enum: os << "dont_enum"; break;
            case property_attribute::dont_delete: os << "dont_delete"; break;
            case property_attribute::accessor: os << "accessor"; break;
            default:
                assert(false);
            }
            empty = false;
        }
    }

    return os;
}

template std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, property_attribute a);
template std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t>& os, property_attribute a);

} // namespace mjs