#ifndef MJS_LEXER_H
#define MJS_LEXER_H

#include <iosfwd>
#include <cassert>
#include "string.h"

namespace mjs {

enum class token_type {
    whitespace,         // TAB/VT/FF/SP \x09, \x0B, \0x0C, \0x20
    line_terminator,    // LF/CR        \x0A, \x0D
    identifier,
    null_literal,
    boolean_literal,
    numeric_literal,
    string_literal,
    // Punctuators
    equal, // =
    // >
    // <
    // ==
    // <=
    // >=
    // !=
    comma, // ,
    // !
    // ~
    // ?
    // :
    // .
    // &&
    // ||
    // ++
    // --
    plus, // +
    minus, // -
    multiply, // *
    divide, // /
    // &
    // |
    // ^
    // %
    // <<
    // >>
    // >>>
    // +=
    // -=
    // *=
    // /=
    // &=
    // |=
    // ^=
    // %=
    // <<=
    // >>=
    // >>>=
    lparen,             // (
    rparen,             // )
    lbrace,             // {
    rbrace,             // }
    lbracket,           // [
    rbracket,           // ]
    semicolon,          // ;

    // Reserved Words
    break_,
    continue_,
    delete_,
    else_,
    for_,
    function_,
    if_,
    in_,
    new_,
    return_,
    this_,
    typeof_,
    var_,
    void_,
    while_,
    with_,

    eof,
};

std::ostream& operator<<(std::ostream& os, token_type t);
std::wostream& operator<<(std::wostream& os, token_type t);

constexpr bool is_literal(token_type tt) {
    return tt == token_type::null_literal || tt == token_type::boolean_literal || tt == token_type::numeric_literal || tt == token_type::string_literal;
}

const char* op_text(token_type tt);

class token {
public:
    explicit token(token_type type) : type_(type) {
        assert(!has_text());
    }
    explicit token(token_type type, const string& text) : type_(type), text_(text) {
        assert(has_text());
    }
    explicit token(double dval) : type_(token_type::numeric_literal), dvalue_(dval) {
    }
    token(const token& t) : token(token_type::eof) {
        *this = t;
    }
    token(token&& t) : token(token_type::eof) {
        *this = std::move(t);
    }
    token& operator=(const token& t) {
        if (this != &t) {
            destroy();
            type_ = t.type_;
            if (t.has_text()) {
                new (&text_) string(t.text_);
            } else {
                ivalue_ = t.ivalue_;
            }
        }
        return *this;
    }
    token& operator=(token&& t) {
        if (this != &t) {
            destroy();
            type_ = t.type_;
            if (t.has_text()) {
                new (&text_) string(std::move(t.text_));
            } else {
                ivalue_ = t.ivalue_;
            }
            t.type_ = token_type::eof;
        }
        return *this;
    }

    ~token() {
        destroy();
    }

    token_type type() const { return type_; }

    const string& text() const {
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

private:
    token_type type_;
    union {
        uint64_t ivalue_;
        double   dvalue_;
        string   text_;
    };

    void destroy() {
        if (has_text()) {
            text_.~string();
        }
    }

    bool has_text() const {
        return type_ == token_type::identifier || type_ == token_type::string_literal;
    }
};

extern const token eof_token;

class lexer {
public:
    explicit lexer(const std::wstring_view& text);

    const token& current_token() const { return current_token_; }

    void next_token();

private:
    std::wstring_view text_;
    size_t text_pos_ = 0;

    token current_token_;
};

std::wstring cpp_quote(const std::wstring_view& s);
inline std::wstring cpp_quote(const string& s) { return cpp_quote(s.view()); }

} // namespace mjs

#endif
