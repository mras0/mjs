#include "regexp.h"
#include <regex>
#include <sstream>
#include <cassert>

namespace mjs {

regexp_flag regexp_flag_from_char(char ch) {
    switch (ch) {
    case 'g': return regexp_flag::global;
    case 'i': return regexp_flag::ignore_case;
    case 'm': return regexp_flag::multiline;
    default:  return regexp_flag::none;
    }
}

std::wstring regexp_flags_to_string(regexp_flag flags) {
    std::wstring res;
    if ((flags & regexp_flag::global) != regexp_flag::none)      res.push_back('g');
    if ((flags & regexp_flag::ignore_case) != regexp_flag::none) res.push_back('i');
    if ((flags & regexp_flag::multiline) != regexp_flag::none)   res.push_back('m');
    return res;
}

regexp_flag regexp_flags_from_string(std::wstring_view s) {
    auto f = regexp_flag::none;
    for (const auto ch: s) {
        const regexp_flag here = regexp_flag_from_char(static_cast<char>(ch));
        if (here == regexp_flag::none || (f & here) != regexp_flag::none) {
            std::ostringstream oss;
            assert(static_cast<char16_t>(ch) <= 0x7f); // TODO: Report better...
            oss << (here == regexp_flag::none ? "Invalid" : "Duplicate") << " flag '" << static_cast<char>(ch) << "' given to RegExp constructor";
            throw std::runtime_error(oss.str());
        }
        f = f | here;
    }
    return f;
}

namespace {

std::wregex make_regex(const std::wstring_view pattern, regexp_flag flags) {
    auto options = std::regex_constants::ECMAScript;
    if ((flags & regexp_flag::ignore_case) != regexp_flag::none) options |= std::regex_constants::icase;
    // TODO: Not implemented in MSVC yet
    //if ((flags_ & regexp_flag::multiline) != regexp_flag::none) options |= std::regex_constants::multiline;
    return std::wregex{pattern.begin(), pattern.end(), options};
}

} // unnamed namespace

class regexp::impl {
public:
    explicit impl(std::wregex&& r) : r_(std::move(r)) {}

    uint32_t search(const std::wstring_view haystack) const {
        std::wcmatch match;
        const wchar_t* const str_beg = haystack.data();
        const wchar_t* const str_end = str_beg + haystack.length();
        if (!std::regex_search(str_beg, str_end, match, r_)) {
            return npos;
        }
        return static_cast<uint32_t>(match[0].first - str_beg);
    }

    std::vector<regexp_match> exec(const std::wstring_view str) const {
        std::wcmatch match;
        if (!std::regex_search(str.data(), str.data()+str.length(), match, r_)) {
            return {};
        }
        std::vector<regexp_match> res;
        for (const auto& m: match) {
            res.push_back(regexp_match{m.first, m.second});
        }
        return res;
    }
private:
    std::wregex r_;
};

regexp::regexp(std::wstring_view pattern, regexp_flag flags)
    : impl_{new impl{make_regex(pattern, flags)}}
    , pattern_{pattern}
    , flags_{flags} {
}

regexp::~regexp() = default;

uint32_t regexp::search(std::wstring_view haystack) const {
    return impl_->search(haystack);
}

std::vector<regexp_match> regexp::exec(std::wstring_view str) const {
    return impl_->exec(str);
}

} // namespace mjs
