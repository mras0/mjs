#ifndef MJS_LEXER_H
#define MJS_LEXER_H

#include <iosfwd>
#include <cassert>
#include <string>
#include "version.h"

namespace mjs {

#define MJS_RESERVED_WORDS(X) \
    X(undefined,    es1)        \
    X(null,         es1)        \
    X(false,        es1)        \
    X(true,         es1)        \
    X(break,        es1)        \
    X(continue,     es1)        \
    X(delete,       es1)        \
    X(else,         es1)        \
    X(for,          es1)        \
    X(function,     es1)        \
    X(if,           es1)        \
    X(in,           es1)        \
    X(new,          es1)        \
    X(return,       es1)        \
    X(this,         es1)        \
    X(typeof,       es1)        \
    X(var,          es1)        \
    X(void,         es1)        \
    X(while,        es1)        \
    X(with,         es1)        \
    X(case,         future)     \
    X(catch,        future)     \
    X(class,        future)     \
    X(const,        future)     \
    X(debugger,     future)     \
    X(default,      future)     \
    X(do,           future)     \
    X(enum,         future)     \
    X(export,       future)     \
    X(extends,      future)     \
    X(finally,      future)     \
    X(import,       future)     \
    X(super,        future)     \
    X(switch,       future)     \
    X(throw,        future)     \
    X(try,          future)


// Must be kept sorted in order of descending length
#define MJS_PUNCTUATORS(X)              \
    X(rshiftshiftequal,   ">>>=")       \
    X(rshiftshift,        ">>>")        \
    X(lshiftequal,        "<<=")        \
    X(rshiftequal,        ">>=")        \
    X(equalequal,         "==")         \
    X(ltequal,            "<=")         \
    X(gtequal,            ">=")         \
    X(notequal,           "!=")         \
    X(andand,             "&&")         \
    X(oror,               "||")         \
    X(plusplus,           "++")         \
    X(minusminus,         "--")         \
    X(lshift,             "<<")         \
    X(rshift,             ">>")         \
    X(plusequal,          "+=")         \
    X(minusequal,         "-=")         \
    X(multiplyequal,      "*=")         \
    X(divideequal,        "/=")         \
    X(andequal,           "&=")         \
    X(orequal,            "|=")         \
    X(xorequal,           "^=")         \
    X(modequal,           "%=")         \
    X(equal,              "=")          \
    X(gt,                 ">")          \
    X(lt,                 "<")          \
    X(comma,              ",")          \
    X(not_,               "!")          \
    X(tilde,              "~")          \
    X(question,           "?")          \
    X(colon,              ":")          \
    X(dot,                ".")          \
    X(plus,               "+")          \
    X(minus,              "-")          \
    X(multiply,           "*")          \
    X(divide,             "/")          \
    X(and_,               "&")          \
    X(or_,                "|")          \
    X(xor_,               "^")          \
    X(mod,                "%")          \
    X(lparen,             "(")          \
    X(rparen,             ")")          \
    X(lbrace,             "{")          \
    X(rbrace,             "}")          \
    X(lbracket,           "[")          \
    X(rbracket,           "]")          \
    X(semicolon,          ";")


enum class token_type {
    whitespace,         // TAB/VT/FF/SP \x09, \x0B, \0x0C, \0x20
    line_terminator,    // LF/CR        \x0A, \x0D
    identifier,
    numeric_literal,
    string_literal,
    // Punctuators
#define DEFINE_PUCTUATOR(name, ...) name,
    MJS_PUNCTUATORS(DEFINE_PUCTUATOR)
#undef DEFINE_PUCTUATOR
    // Reserved Words
#define DEFINE_RESERVER_WORD(name, ...) name ## _,
    MJS_RESERVED_WORDS(DEFINE_RESERVER_WORD)
#undef DEFINE_RESERVER_WORD

    eof,
};

std::ostream& operator<<(std::ostream& os, token_type t);
std::wostream& operator<<(std::wostream& os, token_type t);

constexpr bool is_literal(token_type tt) {
    return tt == token_type::undefined_ || tt == token_type::null_ || tt == token_type::true_ || tt == token_type::false_ || tt == token_type::numeric_literal || tt == token_type::string_literal;
}

constexpr bool is_relational(token_type tt) {
    return tt == token_type::lt || tt == token_type::ltequal || tt == token_type::gt || tt == token_type::gtequal;
}

extern token_type without_assignment(token_type t);

const char* op_text(token_type tt);

extern unsigned get_hex_value(int ch);
unsigned get_hex_value2(const wchar_t* s);
unsigned get_hex_value4(const wchar_t* s);

class token {
public:
    explicit token(token_type type) : type_(type) {
        assert(!has_text());
    }
    explicit token(token_type type, const std::wstring& text) : type_(type), text_(text) {
        assert(has_text());
    }
    explicit token(double dval) : type_(token_type::numeric_literal), dvalue_(dval) {
    }
    token(const token& t) : token(token_type::eof) {
        *this = t;
    }
    token(token&& t) noexcept : token(token_type::eof) {
        *this = std::move(t);
    }
    token& operator=(const token& t) {
        if (this != &t) {
            destroy();
            type_ = t.type_;
            if (t.has_text()) {
                new (&text_) std::wstring(t.text_);
            } else {
                ivalue_ = t.ivalue_;
            }
        }
        return *this;
    }
    token& operator=(token&& t) noexcept {
        if (this != &t) {
            destroy();
            type_ = t.type_;
            if (t.has_text()) {
                new (&text_) std::wstring(std::move(t.text_));
            } else {
                ivalue_ = t.ivalue_;
            }
        }
        return *this;
    }

    ~token() {
        destroy();
    }

    token_type type() const { return type_; }

    const std::wstring& text() const {
        assert(has_text());
        return text_;
    }

    double dvalue() const {
        assert(type_ == token_type::numeric_literal);
        return dvalue_;
    }

    explicit operator bool() const {
        return type_ != token_type::eof;
    }

    friend std::ostream& operator<<(std::ostream& os, const token& t);
    friend std::wostream& operator<<(std::wostream& os, const token& t);

    bool has_text() const {
        return type_ == token_type::identifier || type_ == token_type::string_literal;
    }
private:
    token_type type_;
    union {
        uint64_t        ivalue_;
        double          dvalue_;
        std::wstring    text_;
    };

    void destroy() {
        if (has_text()) {
            text_.~basic_string();
        }
    }
};

extern const token eof_token;

class lexer {
public:
    explicit lexer(const std::wstring_view& text, version ver = default_version);

    const token& current_token() const { return current_token_; }

    void next_token();

    std::wstring_view text() const { return text_; }
    uint32_t text_position() const { return static_cast<uint32_t>(text_pos_); }

private:
    std::wstring_view text_;
    version version_;
    size_t text_pos_ = 0;
    token current_token_ = eof_token;
};

std::wstring cpp_quote(const std::wstring_view& s);

} // namespace mjs

#endif
