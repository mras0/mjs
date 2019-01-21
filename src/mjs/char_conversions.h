#ifndef MJS_CHAR_CONVERSIONS_H
#define MJS_CHAR_CONVERSIONS_H

#include <cstdint>
#include <cassert>
#include <string>
#include <string_view>

namespace mjs::unicode {

constexpr unsigned invalid_length = 0;

constexpr unsigned utf8_max_length = 4;
constexpr unsigned utf16_max_length = 2;

constexpr char32_t max_code_point = 0x110000;

struct conversion_result {
    unsigned length;
    char32_t code_point; // could use a union with this field to signal error
};

constexpr inline auto invalid_conversion = conversion_result{invalid_length,0};

constexpr unsigned utf8_length_from_lead_code_unit(uint8_t first_code_point) {
    if ((first_code_point & 0b1000'0000) == 0b0000'0000) return 1;
    if ((first_code_point & 0b1110'0000) == 0b1100'0000) return 2;
    if ((first_code_point & 0b1111'0000) == 0b1110'0000) return 3;
    if ((first_code_point & 0b1111'1000) == 0b1111'0000) return 4;
    return invalid_length;
}

template<typename CharT>
constexpr conversion_result utf8_to_utf32(const CharT* src, unsigned max_length) {
    static_assert(sizeof(CharT) == 1);
    assert(max_length != 0);
    const auto first_code_unit = static_cast<uint8_t>(src[0]);
    const auto l = utf8_length_from_lead_code_unit(first_code_unit);
    if (!l) {
        // Invalid leading byte
        return invalid_conversion;
    } else if (l > max_length) {
        // Not enough source characters
        return invalid_conversion;
    } else if (l == 1) {
        // Simple case
        return { 1, first_code_unit };
    }
    char32_t code_point = first_code_unit & (0b1111'1100 >> l);
    for (unsigned i = 1; i < l; ++i) {
        const auto here = static_cast<uint8_t>(src[i]);
        if ((here & 0b1100'0000) != 0b1000'0000) {
            // Invalid continuation byte
            return invalid_conversion;
        }
        code_point <<= 6;
        code_point |= here & 0x3f;
    }
    
    return { l, code_point };
}

constexpr unsigned utf8_code_unit_count(char32_t code_point) {
    return code_point < 0x80           ? 1 : 
           code_point < 0x800          ? 2 : 
           code_point < 0x10000        ? 3 : 
           code_point < max_code_point ? 4 : invalid_length;
}

template<typename CharT>
unsigned utf32_to_utf8(char32_t code_point, CharT* out) {
    if (code_point < 0x80) {
        out[0] = static_cast<CharT>(static_cast<uint8_t>(code_point));
        return 1;
    } else if (code_point < 0x800) {
        out[0] = static_cast<CharT>(static_cast<uint8_t>(0b11000000 | (code_point >> 6)));
        out[1] = static_cast<CharT>(static_cast<uint8_t>(0b10000000 | (code_point & 0x3f)));
        return 2;
    } else if (code_point < 0x10000) {
        out[0] = static_cast<CharT>(static_cast<uint8_t>(0b11100000 | (code_point >> 12)));
        out[1] = static_cast<CharT>(static_cast<uint8_t>(0b10000000 | ((code_point >> 6) & 0x3f)));
        out[2] = static_cast<CharT>(static_cast<uint8_t>(0b10000000 | (code_point & 0x3f)));
        return 3;
    } else if (code_point < max_code_point) {
        out[0] = static_cast<CharT>(static_cast<uint8_t>(0b11110000 | (code_point >> 18)));
        out[1] = static_cast<CharT>(static_cast<uint8_t>(0b10000000 | ((code_point >> 12) & 0x3f)));
        out[2] = static_cast<CharT>(static_cast<uint8_t>(0b10000000 | ((code_point >> 6) & 0x3f)));
        out[3] = static_cast<CharT>(static_cast<uint8_t>(0b10000000 | (code_point & 0x3f)));
        return 4;
    }
    return 0;
}

constexpr char16_t utf16_high_surrogate  = 0xd800;
constexpr char16_t utf16_low_surrogate   = 0xdc00;
constexpr char16_t utf16_last_surrogate  = 0xdfff;

constexpr bool utf16_is_surrogate(char16_t code_unit) {
    return code_unit >= utf16_high_surrogate && code_unit <= utf16_last_surrogate;
}

constexpr unsigned utf16_length_from_lead_code_unit(char16_t lead) {
    if (lead < utf16_high_surrogate || lead > utf16_last_surrogate) {
        return 1;
    } else {
        return lead < utf16_low_surrogate ? 2 : invalid_length;
    }
}

constexpr unsigned utf16_code_unit_count(char32_t code_point) {
    return code_point < utf16_high_surrogate ? 1 :
           code_point <= utf16_last_surrogate ? invalid_length :
           code_point < 0x10000 ? 1 :
           code_point < max_code_point ? 2 : invalid_length;
}

template<typename CharT>
constexpr conversion_result utf16_to_utf32(const CharT* src, unsigned max_length) {
    static_assert(sizeof(CharT) >= sizeof(char16_t));
    assert(max_length != 0);
    const auto first_code_unit = static_cast<char16_t>(src[0]);
    const auto l = utf16_length_from_lead_code_unit(first_code_unit);
    if (!l) {
        // Invalid leading byte
        return invalid_conversion;
    } else if (l > max_length) {
        // Not enough source characters
        return invalid_conversion;
    } else if (l == 1) {
        // Simple case
        return { 1, first_code_unit };
    }
    assert(first_code_unit >= utf16_high_surrogate && first_code_unit < utf16_low_surrogate);
    const auto second_code_unit = static_cast<char16_t>(src[1]);
    if (second_code_unit < utf16_low_surrogate || second_code_unit > utf16_last_surrogate) {
        // Invalid surrogate pair
        return invalid_conversion;
    }

    return { 2, static_cast<char32_t>(0x10000 + ((first_code_unit - utf16_high_surrogate) << 10) + (second_code_unit - utf16_low_surrogate)) };
}

template<typename CharT>
constexpr unsigned utf32_to_utf16(char32_t code_point, CharT* out) {
    static_assert(sizeof(CharT) >= sizeof(char16_t));
    const auto l = utf16_code_unit_count(code_point);
    if (l == 1) {
        out[0] = static_cast<CharT>(static_cast<char16_t>(code_point));
    } else if (l == 2) {
        code_point -= 0x10000;
        out[0] = static_cast<CharT>(static_cast<char16_t>(utf16_high_surrogate | ((code_point >> 10) & 0x3ff)));
        out[1] = static_cast<CharT>(static_cast<char16_t>(utf16_low_surrogate | (code_point & 0x3ff)));
    } else {
        assert(l == invalid_length);
    }
    return l;
}

std::wstring utf8_to_utf16(const std::string_view in);
std::string utf16_to_utf8(const std::wstring_view in);

} // namespace mjs::unicode

#endif
