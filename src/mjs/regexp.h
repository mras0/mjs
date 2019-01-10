#ifndef MJS_REGEXP_H
#define MJS_REGEXP_H

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <stdint.h>

namespace mjs {

enum class regexp_flag {
    none        = 0,
    global      = 1<<0,
    ignore_case = 1<<1,
    multiline   = 1<<2,
};

constexpr inline regexp_flag operator|(regexp_flag l, regexp_flag r) {
    return static_cast<regexp_flag>(static_cast<int>(l) | static_cast<int>(r));
}

constexpr inline regexp_flag operator&(regexp_flag l, regexp_flag r) {
    return static_cast<regexp_flag>(static_cast<int>(l) & static_cast<int>(r));
}

// Convert character to regexp_flag, returns regexp_flag::none on conversion error
regexp_flag regexp_flag_from_char(char ch);

std::wstring regexp_flags_to_string(regexp_flag flags);

// throws a runtime_error on conversion failure
regexp_flag regexp_flags_from_string(std::wstring_view s);

struct regexp_match {
    const wchar_t* first;
    const wchar_t* second;

    std::wstring_view str() const {
        return std::wstring_view{first, static_cast<size_t>(second-first)};
    }
};

class regexp {
public:
    explicit regexp(std::wstring_view pattern, regexp_flag flags);
    ~regexp();

    static constexpr uint32_t npos = 0xffff'ffff;

    std::wstring_view pattern() const { return pattern_; }
    regexp_flag flags() const { return flags_; }

    // Returns match groups from running the regular expression on the given string, returns an empty vector on no match
    std::vector<regexp_match> exec(std::wstring_view str) const;

    // Returns index of match, npos if no match was found
    uint32_t search(std::wstring_view haystack) const;

private:
    class impl;
    std::unique_ptr<impl> impl_;
    const std::wstring pattern_;
    const regexp_flag flags_;
};

} // namespace mjs

#endif