#include "lexer.h"
#include "unicode_data.h"
#include <ostream>
#include <sstream>
#include <cstring>
#include <tuple>
#include <climits>
#include <algorithm>
#include <cmath>

namespace mjs {

const token eof_token{token_type::eof};

namespace {

constexpr const wchar_t unicode_ZWNJ = 0x200c;
constexpr const wchar_t unicode_ZWJ  = 0x200d;
constexpr const wchar_t unicode_BOM  = 0xfeff;

void cpp_quote_escape(std::wstring& r, char16_t c) {
    switch (c) {
    case '\'': r += L"\\\'"; break;
    case '\"': r += L"\\\""; break;
    case '\\': r += L"\\\\"; break;
    case '\b': r += L"\\b"; break;
    case '\f': r += L"\\f"; break;
    case '\n': r += L"\\n"; break;
    case '\r': r += L"\\r"; break;
    case '\t': r += L"\\t"; break;
    default:
        constexpr const char* const hexchars = "0123456789ABCDEF";
        r += '\\';
        if (c <= 255) {
            r += 'x';
            r += hexchars[(c>>4)&0xf];
            r += hexchars[c&0xf];
        } else {
            r += 'u';
            r += hexchars[(c>>12)&0xf];
            r += hexchars[(c>>8)&0xf];
            r += hexchars[(c>>4)&0xf];
            r += hexchars[c&0xf];
        }
    }
}

constexpr unicode::classification classify(const uint32_t ch) {
    constexpr auto asize = static_cast<int>(sizeof(unicode::classification_run_start)/sizeof(*unicode::classification_run_start));
    if (ch > unicode::classification_run_start[asize-1]) {
        throw std::runtime_error("Unicode character out of range");
    } else if (ch == unicode::classification_run_start[asize-1]) {
        // Very unlikely
        return unicode::classification_run_type[asize-1];
    }    
    for (uint32_t low = 0, high = asize-1;;) {
        const uint32_t index = low + (high-low) / 2;
        if (ch < unicode::classification_run_start[index]) {
            high = index - 1;
        } else if (ch >= unicode::classification_run_start[index+1]) {
            low = index + 1;
        } else {
            return unicode::classification_run_type[index];
        }
    }
}

constexpr bool is_line_terminator(int ch, version v) {
    if (ch == 0x0A || ch == 0x0D) return true;
    return v != version::es1 && (ch == /*<LS>*/ 0x2028 || ch == /*<PS>*/ 0x2029);
}

constexpr bool is_whitespace_v1(int ch) {
    return (ch == 0x09 || ch == 0x0B || ch == 0x0C || ch == 0x20);
}

constexpr bool is_identifier_start_v1(int ch) {
    return ch == '_' || ch == '$' || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

constexpr bool is_digit(int ch) {
    return ch >= '0' && ch <= '9';
}

constexpr bool is_identifier_part_v1(int ch) {
    return is_identifier_start_v1(ch) || is_digit(ch);
}

constexpr bool is_identifier_part_v3(int ch) {
    switch (classify(ch)) {
    case unicode::classification::id_start:
    case unicode::classification::id_part:
        return true;
    default:
        return false;
    }
}

constexpr bool is_identifier_part_v5(int ch) {
    switch (classify(ch)) {
    case unicode::classification::id_start:
    case unicode::classification::id_part:
        return true;
    case unicode::classification::format:
        // ES5.1, \u200c (<ZWNJ>) and \u200d (<ZWJ>) are considered identifier parts outside literals/comments/etc.
        return ch == unicode_ZWNJ || ch == unicode_ZWJ;
    default:
        return false;
    }
}

constexpr bool is_identifier_part(int ch, version ver) {
    if (is_identifier_part_v1(ch)) return true;
    if (ver < version::es3) return false;
    return ver >= version::es5 ? is_identifier_part_v5(ch) : is_identifier_part_v3(ch);
}

constexpr bool is_form_control(uint32_t ch) {
    // Slight optimization: No form control characters until soft-hypen (0xAD)
    return ch >= 0xAD && classify(ch) == unicode::classification::format;
}

std::tuple<token_type, int> get_punctuation(std::wstring_view s, version v) {
    assert(!s.empty());

#define CHECK_PUNCTUATORS(name, str, ver) if (v >= version::ver && s.length() >= sizeof(str)-1 && s.compare(0, sizeof(str)-1, L##str) == 0) return std::pair<token_type, int>{token_type::name, static_cast<int>(sizeof(str)-1)};
    MJS_PUNCTUATORS(CHECK_PUNCTUATORS)
#undef CHECK_PUNCTUATORS

        std::ostringstream oss;
    auto qs = cpp_quote(s.substr(0, s.length() < 4 ? s.length() : 4));
    oss << "Unhandled character(s) in " << __FUNCTION__ << ": " << std::string(qs.begin(), qs.end()) << "\n";
    throw std::runtime_error(oss.str());
}

std::pair<token, size_t> skip_comment(const std::wstring_view& text, size_t pos, version v) {
    assert(pos < text.size());
    assert(text[pos] == '/' || text[pos] == '*');
    const bool is_single_line = text[pos++] == '/';
    if (is_single_line) {
        for (; pos < text.size(); ++pos) {
            if (is_line_terminator(text[pos], v)) {
                // Don't consume line terminator
                break;
            }
        }
        return {token{token_type::whitespace}, pos};
    } else {
        for (bool last_was_asterisk = false, line_terminator_seen = false; pos < text.size(); ++pos) {
            if (text[pos] == '/' && last_was_asterisk) {
                return {token{line_terminator_seen ? token_type::line_terminator : token_type::whitespace}, ++pos};
            }
            last_was_asterisk = text[pos] == '*';
            if (is_line_terminator(text[pos], v)) {
                line_terminator_seen = true;
            }
        }
        throw std::runtime_error("Unterminated multi-line comment");
    }
}

// Returns decimal value of 'ch' or UINT_MAX on error
unsigned try_get_decimal_value(int ch) {
    return is_digit(ch) ? ch - '0' : UINT_MAX;
}

std::pair<wchar_t, size_t> get_octal_escape_sequence(const std::wstring_view& text, size_t pos) {
    auto value = try_get_decimal_value(text[pos]);
    assert(value < 8);
    const size_t max_len = value < 4 ? 3 : 2;
    size_t len = 1;
    for (; len < max_len && pos+len < text.size(); ++len) {
        const auto this_digit = try_get_decimal_value(text[pos+len]);
        if (this_digit > 7) {
            break;
        }
        value = value*8 + this_digit;
    }
    return { static_cast<wchar_t>(value), len };
}

std::wstring replace_unicode_escape_sequences(const std::wstring& id, version ver) {
    auto idx = id.find_first_of(L'\\');
    assert(idx != std::wstring::npos);
    std::wstring res;
    constexpr const char* const default_error_message = "Illegal unicode escape sequence in identfier";
    std::wstring::size_type last = 0;
    while (idx != std::string::npos) {
        assert(id[idx] == '\\');
        if (idx != last) {
            res += id.substr(last, idx - last);
        }

        last = idx + 6;
        if (last > id.length() || (id[idx+1] != 'u' && id[idx+1] != 'U')) {
            throw std::runtime_error(default_error_message);
        }

        const auto ch = static_cast<char16_t>(get_hex_value4(&id[idx+2]));
        if (!is_identifier_part(ch, ver)) {
            throw std::runtime_error(default_error_message);
        }
        res += ch;

        idx = id.find_first_of(L'\\', idx + 1);
    }

    res += id.substr(last);

    return res;
}

std::pair<token, size_t> get_string_literal(const std::wstring_view text_, const size_t token_start, version ver) {
    bool escape = false;
    std::wstring s;
    const auto ch = text_[token_start];
    assert(ch == '"' || ch == '\'');
    size_t token_end = token_start + 1;
    for ( ;; ++token_end) {
        if (token_end >= text_.size()) {
            throw std::runtime_error("Unterminated string");
        }
        const auto qch = text_[token_end];
        if (is_line_terminator(qch, ver)) {
            throw std::runtime_error("Line terminator in string");
        }
        if (escape) {
            escape = !escape;
            switch (qch) {
            case '\'': s.push_back('\''); break;
            case '\"': s.push_back('\"'); break;
            case '\\': s.push_back('\\'); break;
            case 'b': s.push_back('\b'); break;
            case 'f': s.push_back('\f'); break;
            case 'n': s.push_back('\n'); break;
            case 'r': s.push_back('\r'); break;
            case 't': s.push_back('\t'); break;
            case 'v':
                // '\v' only support in ES3 onwards
                if (ver == version::es1) {
                    goto invalid_escape_sequence;
                }
                s.push_back('\v');
                break;
                // HexEscapeSeqeunce
            case 'x': case 'X':
                ++token_end;
                if (token_end + 2 >= text_.size()) {
                    throw std::runtime_error("Invalid hex escape sequence");
                }
                s.push_back(static_cast<wchar_t>(get_hex_value2(&text_[token_end])));
                token_end += 1; // Incremented in loop
                break;
                // OctalEscapeSequence
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
                {
                    const auto [och, len] =  get_octal_escape_sequence(text_, token_end);
                    token_end += len-1; // Incremented in loop
                    s.push_back(static_cast<wchar_t>(och));
                    break;
                }
                break;
                // UnicodeEscapeSequence
            case 'u': case 'U':
                ++token_end;
                if (token_end + 4 >= text_.size()) {
                    throw std::runtime_error("Invalid unicode escape sequence");
                }
                s.push_back(static_cast<wchar_t>(get_hex_value4(&text_[token_end])));
                token_end += 3; // Incremented in loop
                break;
            default:
            invalid_escape_sequence:
                std::ostringstream oss;
                oss << "Unhandled escape sequence: \\" << (char)qch;
                throw std::runtime_error(oss.str());
            }
        } else if (qch == '\\') {
            escape = true;
        } else if (qch == ch) {
            ++token_end;
            break;
        } else {        
            if (ver == version::es3 && classify(qch) == unicode::classification::format) {
                throw std::runtime_error("Format control characters not allowed in string literals in ES3");
            }
            s.push_back(qch);
        }
    }

    return { token{token_type::string_literal, std::move(s)}, token_end };
}

std::pair<token, size_t> get_identifier(const std::wstring_view text, const size_t token_start, version ver) {
    auto token_end = token_start + 1;
    bool escape_sequence_used = text[token_start] == '\\';

    const auto exended_id_part = ver >= version::es5 ? is_identifier_part_v5 : is_identifier_part_v3;

    for (; token_end < text.size(); ++token_end) {
        const int ch = text[token_end];
        if (is_identifier_part_v1(ch)) {
            // OK - fast path
        } else if (ver < version::es3) {
            break;
        } else if (ch == '\\') {
            escape_sequence_used = true;
        } else if (!exended_id_part(ch)) {
            break;
        }
    }
    auto id = std::wstring{text.begin() + token_start, text.begin() + token_end};
    token tok{token_type::eof};
    if (escape_sequence_used) {
        id = replace_unicode_escape_sequences(id, ver);
    }
    if (0) {}
#define X(rw, first_ver, last_ver) else if (ver >= version::first_ver && ver <= version::last_ver && id == L ## #rw) { tok = token{token_type::rw ## _}; }
    MJS_KEYWORDS(X)
#undef X
else tok = token{token_type::identifier, id};

    if (escape_sequence_used && tok.type() != token_type::identifier) {
        throw std::runtime_error("Unicode escape sequence used in keyword");
    }

    return { tok, token_end };
}

std::pair<token, size_t> get_number_literal(const std::wstring_view text_, const size_t token_start, version) {
    const auto ch = text_[token_start];
    auto token_end = token_start + 1;
    if (ch == '0' && !(token_end < text_.size() && text_[token_end] == '.')) {
        double v = 0;
        if (token_end < text_.size() && tolower(text_[token_end]) == 'x') {
            ++token_end;
            for (; token_end < text_.size(); ++token_end) {
                const auto ch2 = text_[token_end];
                if (!isdigit(ch2) && !isalpha(ch2)) break;
                v = v * 16 + get_hex_value(ch2);
            }
        } else {
            for (; token_end < text_.size(); ++token_end) {
                const auto ch2 = text_[token_end];
                if (!isdigit(ch2)) break;
                if (ch2 > '7') throw std::runtime_error("Invalid octal digit: " + std::string(1, (char)ch));
                v = v * 8 + ch2 - '0';
            }
        }
        return { token{v}, token_end };
    } else {
        bool ndot = ch == '.';
        bool ne = false, last_was_e = false;
        bool es = false;
        for (; token_end < text_.size(); ++token_end) {
            const int ch2 = text_[token_end];
            if (is_digit(ch2)) {
                last_was_e = false;
            } else if (ch2 == '.' && !ndot) {
                last_was_e = false;
                ndot = true;
            } else if ((ch2 == 'e' || ch2 == 'E') && !ne) {
                last_was_e = true;
                ne = true;
            } else if (!es && last_was_e && (ch2 == '+' || ch2 == '-')) {
                es = true;
                last_was_e = false;
            } else {
                break;
            }
        }

        std::string s{text_.begin() + token_start, text_.begin() + token_end};
        char* end;
        errno = 0;
        double v = std::strtod(s.c_str(), &end);
        if (end != s.data() + s.length()) {
            throw std::runtime_error("Invalid string literal " + s);
        }
        if (errno == ERANGE) {
            if (v == HUGE_VAL) {
                v = INFINITY;
            }
            errno = 0;
        }
        return { token{v}, token_end };
    }
}

constexpr bool is_whitespace(char16_t ch, version ver) {
    return is_whitespace_v1(ch) 
        || (ver >= version::es5 && ch == unicode_BOM)
        || (ver >= version::es3 && classify(ch) == unicode::classification::whitespace);
}

std::pair<token, size_t> skip_whitespace(const std::wstring_view text, const size_t token_start, version ver) {
    auto token_end = token_start;
    while (token_end < text.size() && is_whitespace(static_cast<char16_t>(text[token_end]), ver)) {
        ++token_end;
    }
    return { token{token_type::whitespace}, token_end };
}

} // unnamed namespace

bool is_whitespace_or_line_terminator(char16_t ch, version ver) {
    return is_whitespace(ch, ver) || is_line_terminator(ch, ver);
}

std::wstring cpp_quote(const std::wstring_view& s) {
    std::wstring r;
    for (const auto c: s) {
        if (c < 32 || c > 127 || c == '\"' || c == '\\') {
            cpp_quote_escape(r, c);
        } else {
            r += c;
        }
    }
    return r;
}

std::ostream& operator<<(std::ostream& os, const token& t) {
    std::wostringstream oss;
    oss << t;
    auto s = oss.str();
    return os << std::string{s.begin(),s.end()};
}

std::wostream& operator<<(std::wostream& os, const token& t) {
    os << "token{" << t.type();
    if (t.type() == token_type::numeric_literal) {
        os << ", " << t.dvalue_;
    } else if (t.has_text()) {
        os << ", \"" << cpp_quote(t.text()) << "\"";
    }
    return os << "}";
}

std::ostream& operator<<(std::ostream& os, token_type t) {
    switch (t) {
#define CASE_TOKEN_TYPE(n) case token_type::n: return os << #n;
        CASE_TOKEN_TYPE(whitespace)
        CASE_TOKEN_TYPE(line_terminator)
        CASE_TOKEN_TYPE(identifier)
        CASE_TOKEN_TYPE(numeric_literal)
        CASE_TOKEN_TYPE(string_literal)
        CASE_TOKEN_TYPE(eof)
#undef CASE_TOKEN_TYPE
#define CASE_PUNCTUATOR(n, ...) case token_type::n: return os << #n;
        MJS_PUNCTUATORS(CASE_PUNCTUATOR)
#undef CASE_PUNCTUATOR
#define CASE_KEYWORD(n, ...) case token_type::n ## _: return os << #n;
        MJS_KEYWORDS(CASE_KEYWORD)
#undef CASE_KEYWORD
    }
    return os << "token_type{" << (int)t << "}";
}

const char* op_text(token_type tt) {
    switch (tt) {
#define CASE_PUNCTUATOR(name, str, ...) case token_type::name: return str;
        MJS_PUNCTUATORS(CASE_PUNCTUATOR)
#undef CASE_PUNCTUATOR
    case token_type::in_: return " in ";
    case token_type::instanceof_: return " instanceof ";
    default:
        throw std::runtime_error("Invalid token type in op_text: " + std::to_string((int)tt));
    }
}

bool is_assignment_op(token_type t) {
    switch (t) {
    case token_type::equal:
    case token_type::plusequal:
    case token_type::minusequal:
    case token_type::multiplyequal:
    case token_type::divideequal:
    case token_type::modequal:
    case token_type::lshiftequal:
    case token_type::rshiftequal:
    case token_type::rshiftshiftequal:
    case token_type::andequal:
    case token_type::orequal:
    case token_type::xorequal:
        return true;
    default:
        return false;
    }
}

token_type without_assignment(token_type t) {
    assert(is_assignment_op(t));
    switch (t) {
    case token_type::plusequal:         return token_type::plus;
    case token_type::minusequal:        return token_type::minus;
    case token_type::multiplyequal:     return token_type::multiply;
    case token_type::divideequal:       return token_type::divide;
    case token_type::modequal:          return token_type::mod;
    case token_type::lshiftequal:       return token_type::lshift;
    case token_type::rshiftequal:       return token_type::rshift;
    case token_type::rshiftshiftequal:  return token_type::rshiftshift;
    case token_type::andequal:          return token_type::and_;
    case token_type::orequal:           return token_type::or_;
    case token_type::xorequal:          return token_type::xor_;
    default:
        throw std::runtime_error("Invalid token type in without_assignment: " + std::to_string((int)t));
    }
}

std::wostream& operator<<(std::wostream& os, token_type t) {
    std::ostringstream oss;
    oss << t;
    return os << oss.str().c_str();
}

unsigned get_hex_value(int ch) {
    if (is_digit(ch)) return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    throw std::runtime_error("Invalid hex digit: " + std::string(1, (char)ch));
}

unsigned get_hex_value2(const wchar_t* s) {
    return get_hex_value(s[0])<<4 | get_hex_value(s[1]);
}

unsigned get_hex_value4(const wchar_t* s) {
    return get_hex_value(s[0])<<12 | get_hex_value(s[1])<<8 | get_hex_value(s[2])<<4 | get_hex_value(s[3]);
}

std::wstring strip_format_control_characters(const std::wstring_view& s) {
    std::wstring res;
    res.reserve(s.size());
    std::copy_if(s.begin(), s.end(), std::back_inserter(res), [](auto ch) { return !is_form_control(ch); });
    return res;
}

lexer::lexer(const std::wstring_view& text, version ver) : text_(text), version_(ver) {
    next_token();
}

void lexer::next_token() {
    if (text_pos_ == text_.size()) {
        current_token_ = eof_token;
        return;
    }
    assert(text_pos_ < text_.size());

    const int ch = text_[text_pos_];
    size_t token_end = text_pos_ + 1;

    if (ch == '\''  || ch == '\"') {
        std::tie(current_token_, token_end) = get_string_literal(text_, text_pos_, version_);
    } else if (is_line_terminator(ch, version_)) {
        while (token_end < text_.size() && is_line_terminator(text_[token_end], version_)) {
            ++token_end;
        }
        current_token_  = token{token_type::line_terminator};
    } else if (is_digit(ch) || (ch == '.' && token_end < text_.size() && is_digit(text_[token_end]))) {
        std::tie(current_token_, token_end) = get_number_literal(text_, text_pos_, version_);
    } else if (std::strchr("!%&()*+,-./:;<=>?[]^{|}~", ch)) {
        if (ch == '/' && token_end < text_.size() && (text_[token_end] == '/' || text_[token_end] == '*')) {
            std::tie(current_token_, token_end)  = skip_comment(text_, token_end, version_);
        } else {
            auto [tok, len] = get_punctuation(std::wstring_view{&text_[text_pos_], text_.size() - text_pos_}, version_);
            token_end = text_pos_ + len;
            current_token_ = token{tok};
        }
    } else if (is_whitespace_v1(ch) || (version_ >= version::es5 && ch == unicode_BOM)) {
        std::tie(current_token_, token_end) = skip_whitespace(text_, text_pos_, version_);
    } else if (is_identifier_start_v1(ch) || (version_ >= version::es3 && ch == '\\')) {
        std::tie(current_token_, token_end) = get_identifier(text_, text_pos_, version_);
    } else {
        // Do more expensive classification
        switch (version_ >= version::es3 ? classify(ch) : unicode::classification::other) {
        case unicode::classification::whitespace:
            std::tie(current_token_, token_end) = skip_whitespace(text_, text_pos_, version_);
            break;
        case unicode::classification::id_start:
            std::tie(current_token_, token_end) = get_identifier(text_, text_pos_, version_);
            break;
        default:
            std::ostringstream oss;
            oss << "Unhandled character in " << __FUNCTION__ << ": " << ch << " 0x" << std::hex << (int)ch << "\n";
            throw std::runtime_error(oss.str());
        }
    }

    text_pos_ = token_end;
}

std::wstring_view lexer::get_regex_literal() {
    assert(version_ >= version::es3 &&  "Regular expression literals are not support until ES3");
    assert(current_token_.type() == token_type::divide || current_token_.type() == token_type::divideequal);
    const size_t start = text_pos_ - (current_token_.type() == token_type::divide ? 1 : 2);
    assert(start < text_pos_);

    // Body
    for (bool quote = false, in_class = false;; ++text_pos_) {
        if (text_pos_ >= text_.size()) {
            throw std::runtime_error("Unterminated regular expression literal");
        }
        const auto ch = text_[text_pos_];
        if (is_line_terminator(ch, version_)) {
            throw std::runtime_error("Line terminator in regular expression literal");
        } else if (ch == '\\') {
            quote = !quote;
            continue;
        } else if (quote) {
            quote = false;
            continue;
        }
        assert(!quote && ch != '\\');

        if (ch == ']') {
            in_class = false;
        } else if (ch == '[') {
            in_class = true;
        } else if (ch == '/' && (!in_class || version_ < version::es5)) {
            // forward slash can be unquoted in character classes in ES5.1
            ++text_pos_; // The final '/' is part of the regular expression
            break;
        }
    }

    // Flags
    for (; text_pos_ < text_.size(); ++text_pos_) {
        if (!is_identifier_part(text_[text_pos_], version_)) {
            break;
        }
    }

    const std::wstring_view regex_lit{&text_[start], text_pos_ - start};

    next_token();

    return regex_lit;
}

} // namespace mjs
