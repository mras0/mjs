#ifndef MJS_LEXER_H
#define MJS_LEXER_H

#include <iosfwd>
#include <cassert>
#include <string>
#include "version.h"

namespace mjs {

#define MJS_RESERVED_WORDS(X)   \
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
    X(case,         es3)        \
    X(catch,        es3)        \
    X(default,      es3)        \
    X(do,           es3)        \
    X(finally,      es3)        \
    X(instanceof,   es3)        \
    X(switch,       es3)        \
    X(throw,        es3)        \
    X(try,          es3)        \
    X(abstract,     future)     \
    X(boolean,      future)     \
    X(byte,         future)     \
    X(char,         future)     \
    X(class,        future)     \
    X(const,        future)     \
    X(debugger,     future)     \
    X(double,       future)     \
    X(enum,         future)     \
    X(export,       future)     \
    X(extends,      future)     \
    X(final,        future)     \
    X(float,        future)     \
    X(goto,         future)     \
    X(implements,   future)     \
    X(import,       future)     \
    X(int,          future)     \
    X(interface,    future)     \
    X(long,         future)     \
    X(native,       future)     \
    X(package,      future)     \
    X(private,      future)     \
    X(protected,    future)     \
    X(public,       future)     \
    X(short,        future)     \
    X(static,       future)     \
    X(super,        future)     \
    X(synchronized, future)     \
    X(throws,       future)     \
    X(transient,    future)     \
    X(volatile,     future)


// Must be kept sorted in order of descending length
#define MJS_PUNCTUATORS(X)              \
    X(rshiftshiftequal,   ">>>=", es1)  \
    X(rshiftshift,        ">>>" , es1)  \
    X(lshiftequal,        "<<=" , es1)  \
    X(rshiftequal,        ">>=" , es1)  \
    X(equalequalequal,    "===" , es3)  \
    X(notequalequal,      "!==" , es3)  \
    X(equalequal,         "=="  , es1)  \
    X(notequal,           "!="  , es1)  \
    X(ltequal,            "<="  , es1)  \
    X(gtequal,            ">="  , es1)  \
    X(andand,             "&&"  , es1)  \
    X(oror,               "||"  , es1)  \
    X(plusplus,           "++"  , es1)  \
    X(minusminus,         "--"  , es1)  \
    X(lshift,             "<<"  , es1)  \
    X(rshift,             ">>"  , es1)  \
    X(plusequal,          "+="  , es1)  \
    X(minusequal,         "-="  , es1)  \
    X(multiplyequal,      "*="  , es1)  \
    X(divideequal,        "/="  , es1)  \
    X(andequal,           "&="  , es1)  \
    X(orequal,            "|="  , es1)  \
    X(xorequal,           "^="  , es1)  \
    X(modequal,           "%="  , es1)  \
    X(equal,              "="   , es1)  \
    X(gt,                 ">"   , es1)  \
    X(lt,                 "<"   , es1)  \
    X(comma,              ","   , es1)  \
    X(not_,               "!"   , es1)  \
    X(tilde,              "~"   , es1)  \
    X(question,           "?"   , es1)  \
    X(colon,              ":"   , es1)  \
    X(dot,                "."   , es1)  \
    X(plus,               "+"   , es1)  \
    X(minus,              "-"   , es1)  \
    X(multiply,           "*"   , es1)  \
    X(divide,             "/"   , es1)  \
    X(and_,               "&"   , es1)  \
    X(or_,                "|"   , es1)  \
    X(xor_,               "^"   , es1)  \
    X(mod,                "%"   , es1)  \
    X(lparen,             "("   , es1)  \
    X(rparen,             ")"   , es1)  \
    X(lbrace,             "{"   , es1)  \
    X(rbrace,             "}"   , es1)  \
    X(lbracket,           "["   , es1)  \
    X(rbracket,           "]"   , es1)  \
    X(semicolon,          ";"   , es1)


enum class token_type {
    whitespace,
    line_terminator,
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
