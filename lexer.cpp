#include "lexer.h"
#include <ostream>
#include <sstream>
#include <cstring>

#define RESERVED_WORDS(X) \
    X(undefined)          \
    X(null)               \
    X(false)              \
    X(true)               \
    X(break)              \
    X(continue)           \
    X(delete)             \
    X(else)               \
    X(for)                \
    X(function)           \
    X(if)                 \
    X(in)                 \
    X(new)                \
    X(return)             \
    X(this)               \
    X(typeof)             \
    X(var)                \
    X(void)               \
    X(while)              \
    X(with)


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
        CASE_TOKEN_TYPE(numeric_literal);
        CASE_TOKEN_TYPE(string_literal);
        CASE_TOKEN_TYPE(equal);              // =
        CASE_TOKEN_TYPE(gt);                 // >
        CASE_TOKEN_TYPE(lt);                 // <
        CASE_TOKEN_TYPE(equalequal);         // ==
        CASE_TOKEN_TYPE(ltequal);            // <=
        CASE_TOKEN_TYPE(gtequal);            // >=
        CASE_TOKEN_TYPE(notequal);           // !=
        CASE_TOKEN_TYPE(comma);              // ,
        CASE_TOKEN_TYPE(not_);               // !
        CASE_TOKEN_TYPE(tilde);              // ~
        CASE_TOKEN_TYPE(question);           // ?
        CASE_TOKEN_TYPE(colon);              // :
        CASE_TOKEN_TYPE(dot);                // .
        CASE_TOKEN_TYPE(andand);             // &&
        CASE_TOKEN_TYPE(oror);               // ||
        CASE_TOKEN_TYPE(plusplus);           // ++
        CASE_TOKEN_TYPE(minusminus);         // --
        CASE_TOKEN_TYPE(plus);               // +
        CASE_TOKEN_TYPE(minus);              // -
        CASE_TOKEN_TYPE(multiply);           // *
        CASE_TOKEN_TYPE(divide);             // /
        CASE_TOKEN_TYPE(and_);               // &
        CASE_TOKEN_TYPE(or_);                // |
        CASE_TOKEN_TYPE(xor_);               // ^
        CASE_TOKEN_TYPE(mod);                // %
        CASE_TOKEN_TYPE(lshift);             // <<
        CASE_TOKEN_TYPE(rshift);             // >>
        CASE_TOKEN_TYPE(rshiftshift);        // >>>
        CASE_TOKEN_TYPE(plusequal);          // +=
        CASE_TOKEN_TYPE(minusequal);         // -=
        CASE_TOKEN_TYPE(multiplyequal);      // *=
        CASE_TOKEN_TYPE(divideequal);        // /=
        CASE_TOKEN_TYPE(andequal);           // &=
        CASE_TOKEN_TYPE(orequal);            // |=
        CASE_TOKEN_TYPE(xorequal);           // ^=
        CASE_TOKEN_TYPE(modequal);           // %=
        CASE_TOKEN_TYPE(lshiftequal);        // <<=
        CASE_TOKEN_TYPE(rshiftequal);        // >>=
        CASE_TOKEN_TYPE(rshiftshiftequal);   // >>>=
        CASE_TOKEN_TYPE(lparen);             // (
        CASE_TOKEN_TYPE(rparen);             // )
        CASE_TOKEN_TYPE(lbrace);             // {
        CASE_TOKEN_TYPE(rbrace);             // }
        CASE_TOKEN_TYPE(lbracket);           // [
        CASE_TOKEN_TYPE(rbracket);           // ]
        CASE_TOKEN_TYPE(semicolon);          // ;
        CASE_TOKEN_TYPE(eof);
#define X(rw) case token_type::rw ## _: return os << #rw;
        RESERVED_WORDS(X)
#undef X
#undef CASE_TOKEN_TYPE
    }
    return os << "token_type{" << (int)t << "}";
}

const char* op_text(token_type tt) {
    switch (tt) {
    case token_type::equal:              return "=";
    case token_type::gt:                 return ">";
    case token_type::lt:                 return "<";
    case token_type::equalequal:         return "==";
    case token_type::ltequal:            return "<=";
    case token_type::gtequal:            return ">=";
    case token_type::notequal:           return "!=";
    case token_type::comma:              return ":";
    case token_type::not_:               return "!";
    case token_type::tilde:              return "~";
    case token_type::question:           return "?";
    case token_type::colon:              return ":";
    case token_type::dot:                return ". ";
    case token_type::andand:             return "&&";
    case token_type::oror:               return "||";
    case token_type::plusplus:           return "++";
    case token_type::minusminus:         return "--";
    case token_type::plus:               return "+";
    case token_type::minus:              return "-";
    case token_type::multiply:           return "*";
    case token_type::divide:             return "/";
    case token_type::and_:               return "&";
    case token_type::or_:                return "|";
    case token_type::xor_:               return "^";
    case token_type::mod:                return "%";
    case token_type::lshift:             return "<<";
    case token_type::rshift:             return ">>";
    case token_type::rshiftshift:        return ">>>";
    case token_type::plusequal:          return "+=";
    case token_type::minusequal:         return "-=";
    case token_type::multiplyequal:      return "*=";
    case token_type::divideequal:        return "/=";
    case token_type::andequal:           return "&=";
    case token_type::orequal:            return "|=";
    case token_type::xorequal:           return "^=";
    case token_type::modequal:           return "%=";
    case token_type::lshiftequal:        return "<<=";
    case token_type::rshiftequal:        return ">>=";
    case token_type::rshiftshiftequal:   return ">>>=";
    case token_type::lparen:             return "(";
    case token_type::rparen:             return ")";
    case token_type::lbrace:             return "{";
    case token_type::rbrace:             return "}";
    case token_type::lbracket:           return "[";
    case token_type::rbracket:           return "]";
    case token_type::semicolon:          return ";";
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

std::pair<token_type, int> get_punctuation(std::wstring_view v) {
    using p = std::pair<token_type, int>;
    assert(!v.empty());
    switch (v[0]) {
#if 0

#endif
    case '=': return v.length() > 1 && v[1] == '=' ? p{ token_type::equalequal, 2} : p{ token_type::equal, 1};
    case '>':
        if (v.length() > 1 && v[1] == '>') {
            if (v.length() > 2 && v[2] == '>') {
                if(v.length() > 3 && v[3] == '=') {
                    return p{token_type::rshiftshiftequal, 4};
                } else {
                    return p{token_type::rshiftshift, 3};
                }                
            } else if (v.length() > 2 && v[2] == '=') {
                return p{token_type::rshiftequal, 3};
            } else {
                return p{token_type::rshift, 2};
            }
        } else if (v.length() > 1 && v[1] == '=') {
            return p{token_type::gtequal, 2};
        } else {
            return p{token_type::gt, 1};
        }
    case '<':
        if (v.length() > 1 && v[1] == '<') {
            if (v.length() > 2 && v[2] == '=') {
                return p{token_type::lshiftequal, 3};
            } else {
                return p{token_type::lshift, 2};
            }
        } else if (v.length() > 1 && v[1] == '=') {
            return p{token_type::ltequal, 2};
        } else {
            return p{token_type::lt, 1};
        }
    case ',': return { token_type::comma, 1 };
    case '!': return v.length() > 1 && v[1] == '=' ? p{ token_type::notequal, 2} : p{ token_type::not_, 1};
    case '~': return { token_type::tilde, 1 };
    case '?': return { token_type::question, 1 };
    case ':': return { token_type::colon, 1 };
    case '.': return { token_type::dot, 1 };
    case '+':
        if (v.length() > 1 && v[1] == '+') {
            return p{ token_type::plusplus, 2};
        } else if (v.length() > 1 && v[1] == '=') {
            return p{ token_type::plusequal, 2};
        } else {
            return p{ token_type::plus, 1};
        }
    case '-': 
        if (v.length() > 1 && v[1] == '-') {
            return p{ token_type::minusminus, 2};
        } else if (v.length() > 1 && v[1] == '=') {
            return p{ token_type::minusequal, 2};
        } else {
            return p{ token_type::minus, 1};
        }
    case '*': return v.length() > 1 && v[1] == '=' ? p{ token_type::multiplyequal, 2} : p{ token_type::multiply, 1};
    case '/': return v.length() > 1 && v[1] == '=' ? p{ token_type::divideequal, 2} : p{ token_type::divide, 1};
    case '%': return v.length() > 1 && v[1] == '=' ? p{ token_type::modequal, 2} : p{ token_type::mod, 1};
    case '&':
        if (v.length() > 1 && v[1] == '&') {
            return p{ token_type::andand, 2};
        } else if (v.length() > 1 && v[1] == '=') {
            return p{ token_type::andequal, 2};
        } else {
            return p{ token_type::and_, 1};
        }
    case '^': return v.length() > 1 && v[1] == '=' ? p{ token_type::xorequal, 2} : p{ token_type::xor_, 1};
    case '|':
        if (v.length() > 1 && v[1] == '|') {
            return p{ token_type::oror, 2};
        } else if (v.length() > 1 && v[1] == '=') {
            return p{ token_type::orequal, 2};
        } else {
            return p{ token_type::or_, 1};
        }
    case '(': return { token_type::lparen, 1};
    case ')': return { token_type::rparen, 1};
    case '{': return { token_type::lbrace, 1};
    case '}': return { token_type::rbrace, 1};
    case '[': return { token_type::lbracket, 1};
    case ']': return { token_type::rbracket, 1};
    case ';': return { token_type::semicolon, 1};
    }
    std::ostringstream oss;
    oss << "Unhandled character in " << __FUNCTION__ << ": " << (char)v[0] << " 0x" << std::hex << (int)v[0] << "\n";
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
            auto id = std::wstring{text_.begin() + text_pos_, text_.begin() + token_end};
            if (0) {}
#define X(rw) else if (id == L ## #rw) current_token_ = token{token_type::rw ## _};
            RESERVED_WORDS(X)
#undef X
            else current_token_  = token{token_type::identifier, string{id}};
        } else if (is_digit(ch) || (ch == '.' && token_end < text_.size() && is_digit(text_[token_end]))) {
            // TODO: Handle HexIntegerLiteral and exponent/decimal point
            bool ndot = ch == '.';
            bool ne = false;
            for (; token_end < text_.size(); ++token_end) {
                const int ch2 = text_[token_end];
                if (is_digit(ch2)) {
                } else if (ch2 == '.' && !ndot) {
                    ndot = true;
                } else if ((ch2 == 'e' || ch2 == 'E') && !ne) {
                    ne = true;
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
