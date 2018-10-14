#include "lexer.h"
#include <ostream>
#include <sstream>
#include <cstring>

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
        os << ", \"" << cpp_quote(t.text().view()) << "\"";
    }
    return os << "}";
}

std::ostream& operator<<(std::ostream& os, token_type t) {
    switch (t) {
#define CASE_TOKEN_TYPE(n) case token_type::n: return os << #n
        CASE_TOKEN_TYPE(whitespace);
        CASE_TOKEN_TYPE(line_terminator);
        CASE_TOKEN_TYPE(identifier);
        CASE_TOKEN_TYPE(null_literal);
        CASE_TOKEN_TYPE(boolean_literal);
        CASE_TOKEN_TYPE(numeric_literal);
        CASE_TOKEN_TYPE(string_literal);
        CASE_TOKEN_TYPE(comma);
        CASE_TOKEN_TYPE(plus);
        CASE_TOKEN_TYPE(minus);
        CASE_TOKEN_TYPE(multiply);
        CASE_TOKEN_TYPE(divide);
        CASE_TOKEN_TYPE(lparen);
        CASE_TOKEN_TYPE(rparen);
        CASE_TOKEN_TYPE(lbrace);
        CASE_TOKEN_TYPE(rbrace);
        CASE_TOKEN_TYPE(lbracket);
        CASE_TOKEN_TYPE(rbracket);
        CASE_TOKEN_TYPE(semicolon);
        CASE_TOKEN_TYPE(eof);
#undef CASE_TOKEN_TYPE
    }
    return os << "token_type{" << (int)t << "}";
}

const char* op_text(token_type tt) {
    switch (tt) {
    case token_type::plus: return "+";
    case token_type::minus: return "-";
    case token_type::multiply: return "*";
    case token_type::divide: return "/";

    default:
        throw std::runtime_error("Invalid token type: " + std::to_string((int)tt));
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

std::pair<token_type, int> get_punctuation(std::wstring_view v) {
    assert(!v.empty());
    switch (v[0]) {
    case ',': return { token_type::comma, 1 };
    case '+': return { token_type::plus, 1};
    case '-': return { token_type::minus, 1};
    case '*': return { token_type::multiply, 1};
    case '/': return { token_type::divide, 1};
    case '(': return { token_type::lparen, 1};
    case ')': return { token_type::rparen, 1};
    case '{': return { token_type::lbrace, 1};
    case '}': return { token_type::rbrace, 1};
    case '[': return { token_type::lbracket, 1};
    case ']': return { token_type::rbracket, 1};
    case ';': return { token_type::semicolon, 1};
    }
    std::ostringstream oss;
    oss << "Unhandled character in " << __FUNCTION__ << ": " << v[0] << " 0x" << std::hex << (int)v[0] << "\n";
    throw std::runtime_error(oss.str());
}

lexer::lexer(const std::wstring_view& text) : text_(text), current_token_{eof_token} {
    next_token();
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
                const auto qch = text_[token_end];
                if (token_end >= text_.size()) {
                    throw std::runtime_error("Unterminated string");
                } else if (is_line_terminator(qch)) {
                    throw std::runtime_error("Line temrinator in string");
                }
                if (escape) {
                    std::ostringstream oss;
                    oss << "Unahdled escape sequence: \\" << qch;
                    throw std::runtime_error(oss.str());
                    //escape = !escape;
                } else if (qch == '\\') {
                    escape = true;
                } else if (qch == ch) {
                    ++token_end;
                    break;
                } else {
                    s.push_back(static_cast<unsigned char>(qch));
                }
            }
            current_token_ = token{token_type::string_literal, string{std::move(s)}};
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
            current_token_  = token{token_type::identifier, string{std::wstring{text_.begin() + text_pos_, text_.begin() + token_end}}};
        } else if (is_digit(ch) /*|| (ch == '.' && token_end < text_.size() && is_digit(text_[token_end]))*/) {
            // TODO: Handle HexIntegerLiteral and exponent/decimal point
            while (token_end < text_.size() && is_digit(text_[token_end])) {
                ++token_end;
            }
            std::string s{text_.begin() + text_pos_, text_.begin() + token_end};
            size_t len;
            const double v = std::stod(s, &len);
            if (len != s.length()) {
                throw std::runtime_error("Invalid string literal " + s);
            }
            current_token_  = token{v};
        } else if (std::strchr("!%&()*+,-./:;<=>?[]^{|}~", ch)) {
            auto [tok, len] = get_punctuation(std::wstring_view{&text_[text_pos_], text_.size() - text_pos_});
            token_end = text_pos_ + len;
            current_token_ = token{tok};
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
