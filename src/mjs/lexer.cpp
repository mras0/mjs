#include "lexer.h"
#include <ostream>
#include <sstream>
#include <cstring>
#include <tuple>
#include <climits>

namespace mjs {

const token eof_token{token_type::eof};

std::wstring cpp_quote(const std::wstring_view& s) {
    std::wstring r;
    for (const auto c: s) {
        if (c < 32 || c > 127) {
            constexpr const char* const hexchars = "0123456789ABCDEF";
            r += '\\';
            r += 'u';
            r += hexchars[(c>>12)&0xf];
            r += hexchars[(c>>8)&0xf];
            r += hexchars[(c>>4)&0xf];
            r += hexchars[c&0xf];
        } else if (c == '\"') {
            r += '\\';
            r += c;
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
#define CASE_RESERVED_WORD(n, ...) case token_type::n ## _: return os << #n;
        MJS_RESERVED_WORDS(CASE_RESERVED_WORD)
#undef CASE_RESERVED_WORD
    }
    return os << "token_type{" << (int)t << "}";
}

const char* op_text(token_type tt) {
    switch (tt) {
#define CASE_PUNCTUATOR(name, str) case token_type::name: return str;
        MJS_PUNCTUATORS(CASE_PUNCTUATOR)
#undef CASE_PUNCTUATOR
    default:
        throw std::runtime_error("Invalid token type in op_text: " + std::to_string((int)tt));
    }
}

token_type without_assignment(token_type t) {
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

constexpr bool is_whitespace(int ch) {
    return ch == 0x09 || ch == 0x0B || ch == 0x0C || ch == 0x20;
}

constexpr bool is_line_terminator(int ch) {
    return ch == 0x0A || ch == 0x0D;
}

constexpr bool is_identifier_letter(int ch) {
    return ch == '_' || ch == '$' || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

constexpr bool is_digit(int ch) {
    return ch >= '0' && ch <= '9';
}

std::tuple<token_type, int> get_punctuation(std::wstring_view v) {
    assert(!v.empty());

#define CHECK_PUNCTUATORS(name, str) if (v.length() >= sizeof(str)-1 && v.compare(0, sizeof(str)-1, L##str) == 0) return std::pair<token_type, int>{token_type::name, static_cast<int>(sizeof(str)-1)};
    MJS_PUNCTUATORS(CHECK_PUNCTUATORS)
#undef CHECK_PUNCTUATORS

    std::ostringstream oss;
    auto s = cpp_quote(v.substr(0, 4));
    oss << "Unhandled character(s) in " << __FUNCTION__ << ": " << std::string(s.begin(), s.end()) << "\n";
    throw std::runtime_error(oss.str());   
}

std::pair<token, size_t> skip_comment(const std::wstring_view& text, size_t pos) {
    assert(pos < text.size());
    assert(text[pos] == '/' || text[pos] == '*');
    const bool is_single_line = text[pos++] == '/';
    if (is_single_line) {
        for (; pos < text.size(); ++pos) {
            if (is_line_terminator(text[pos])) {
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
            if (is_line_terminator(text[pos])) {
                line_terminator_seen = true;
            }
        }
        throw std::runtime_error("Unterminated multi-line comment");
    }
}

lexer::lexer(const std::wstring_view& text, version ver) : text_(text), version_(ver) {
    next_token();
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

void lexer::next_token() {
    if (text_pos_ < text_.size()) {
        const int ch = text_[text_pos_];
        size_t token_end = text_pos_ + 1;
        std::wstring token_text;
        if (ch == '\''  || ch == '\"') {
            bool escape = false;
            std::wstring s;
            for ( ;; ++token_end) {
                if (token_end >= text_.size()) {
                    throw std::runtime_error("Unterminated string");
                }
                const auto qch = text_[token_end];
                if (is_line_terminator(qch)) {
                    throw std::runtime_error("Line temrinator in string");
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
                        std::ostringstream oss;
                        oss << "Unahdled escape sequence: \\" << (char)qch;
                        throw std::runtime_error(oss.str());
                    }
                } else if (qch == '\\') {
                    escape = true;
                } else if (qch == ch) {
                    ++token_end;
                    break;
                } else {
                    s.push_back(qch);
                }
            }
            current_token_ = token{token_type::string_literal, std::move(s)};
        } else if (is_whitespace(ch)) {
            while (token_end < text_.size() && is_whitespace(text_[token_end])) {
                ++token_end;
            }
            current_token_  = token{token_type::whitespace};
        } else if (is_line_terminator(ch)) {
            while (token_end < text_.size() && is_line_terminator(text_[token_end])) {
                ++token_end;
            }
            current_token_  = token{token_type::line_terminator};
        } else if (is_identifier_letter(ch)) {
            while (token_end < text_.size() && (is_identifier_letter(text_[token_end]) || is_digit(text_[token_end]))) {
                ++token_end;
            }
            auto id = std::wstring{text_.begin() + text_pos_, text_.begin() + token_end};
            if (0) {}
#define X(rw, ver) else if (id == L ## #rw) { if (version::ver > version_) { std::ostringstream oss; oss << #rw << " is not available until " << version::ver; throw std::runtime_error(oss.str()); }  current_token_ = token{token_type::rw ## _}; }
            MJS_RESERVED_WORDS(X)
#undef X
            else current_token_  = token{token_type::identifier, id};
        } else if (is_digit(ch) || (ch == '.' && token_end < text_.size() && is_digit(text_[token_end]))) {
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
                current_token_  = token{v};
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

                std::string s{text_.begin() + text_pos_, text_.begin() + token_end};
                size_t len;
                const double v = std::stod(s, &len);
                if (len != s.length()) {
                    throw std::runtime_error("Invalid string literal " + s);
                }
                current_token_  = token{v};
            }
        } else if (std::strchr("!%&()*+,-./:;<=>?[]^{|}~", ch)) {
            if (ch == '/' && token_end < text_.size() && (text_[token_end] == '/' || text_[token_end] == '*')) {
                std::tie(current_token_, token_end)  = skip_comment(text_, token_end);
            } else {
                auto [tok, len] = get_punctuation(std::wstring_view{&text_[text_pos_], text_.size() - text_pos_});
                token_end = text_pos_ + len;
                current_token_ = token{tok};
            }
        } else {
            std::ostringstream oss;
            oss << "Unhandled character in " << __FUNCTION__ << ": " << ch << " 0x" << std::hex << (int)ch << "\n";
            throw std::runtime_error(oss.str());
        }

        text_pos_ += token_end - text_pos_;
    } else {
        current_token_ = eof_token;
    }
}

} // namespace mjs
