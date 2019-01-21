#include "char_conversions.h"

namespace mjs::unicode {

class conversion_error : public std::exception {
public:
    explicit conversion_error() {}
    const char* what() const noexcept override { return "Unicode conversion failed"; }
};

std::wstring utf8_to_utf16(const std::string_view in) {
    std::wstring res;
    const auto l = static_cast<unsigned>(in.length());
    wchar_t buffer[utf16_max_length];
    for (unsigned i = 0; i < l;) {
        const auto conv = utf8_to_utf32(&in[i], l - i);
        if (conv.length == invalid_length) {
            throw conversion_error{};
        }
        const auto out_len = utf32_to_utf16(conv.code_point, buffer);
        if (!out_len) {
            throw conversion_error{};
        }
        res.insert(res.end(), buffer, buffer + out_len);
        i += conv.length;
    }

    return res;
}

std::string utf16_to_utf8(const std::wstring_view in) {
    std::string res;
    const auto l = static_cast<unsigned>(in.length());
    char buffer[utf8_max_length];
    for (unsigned i = 0; i < l;) {
        const auto conv = utf16_to_utf32(&in[i], l - i);
        if (conv.length == invalid_length) {
            throw conversion_error{};
        }
        const auto out_len = utf32_to_utf8(conv.code_point, buffer);
        if (!out_len) {
            throw conversion_error{};
        }
        res.insert(res.end(), buffer, buffer + out_len);
        i += conv.length;
    }

    return res;
}

} // namespace mjs::unicode
