#ifndef MJS_LEXER_H
#define MJS_LEXER_H

#include <iosfwd>
#include <cassert>
#include <string>
#include "version.h"

namespace mjs {

#define MJS_KEYWORDS(X)              \
    X( abstract     , es3 , es3    ) \
    X( boolean      , es3 , es3    ) \
    X( break        , es1 , latest ) \
    X( byte         , es3 , es3    ) \
    X( case         , es1 , latest ) \
    X( catch        , es1 , latest ) \
    X( char         , es3 , es3    ) \
    X( class        , es1 , latest ) \
    X( const        , es1 , latest ) \
    X( continue     , es1 , latest ) \
    X( debugger     , es1 , latest ) \
    X( default      , es1 , latest ) \
    X( delete       , es1 , latest ) \
    X( do           , es1 , latest ) \
    X( double       , es3 , es3    ) \
    X( else         , es1 , latest ) \
    X( enum         , es1 , latest ) \
    X( export       , es1 , latest ) \
    X( extends      , es1 , latest ) \
    X( false        , es1 , latest ) \
    X( final        , es3 , es3    ) \
    X( finally      , es1 , latest ) \
    X( float        , es3 , es3    ) \
    X( for          , es1 , latest ) \
    X( function     , es1 , latest ) \
    X( goto         , es3 , es3    ) \
    X( if           , es1 , latest ) \
    X( implements   , es3 , latest ) \
    X( import       , es1 , latest ) \
    X( in           , es1 , latest ) \
    X( instanceof   , es3 , latest ) \
    X( int          , es3 , es3    ) \
    X( interface    , es3 , latest ) \
    X( let          , es5 , latest ) \
    X( long         , es3 , es3    ) \
    X( native       , es3 , es3    ) \
    X( new          , es1 , latest ) \
    X( null         , es1 , latest ) \
    X( package      , es3 , latest ) \
    X( private      , es3 , latest ) \
    X( protected    , es3 , latest ) \
    X( public       , es3 , latest ) \
    X( return       , es1 , latest ) \
    X( short        , es3 , es3    ) \
    X( static       , es3 , latest ) \
    X( super        , es1 , latest ) \
    X( switch       , es1 , latest ) \
    X( synchronized , es3 , es3    ) \
    X( this         , es1 , latest ) \
    X( throw        , es1 , latest ) \
    X( throws       , es3 , es3    ) \
    X( transient    , es3 , es3    ) \
    X( true         , es1 , latest ) \
    X( try          , es1 , latest ) \
    X( typeof       , es1 , latest ) \
    X( var          , es1 , latest ) \
    X( void         , es1 , latest ) \
    X( volatile     , es3 , es3    ) \
    X( while        , es1 , latest ) \
    X( with         , es1 , latest ) \
    X( yield        , es5 , latest )

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
    // Keywords
#define DEFINE_KEYWORD(name, ...) name ## _,
    MJS_KEYWORDS(DEFINE_KEYWORD)
#undef DEFINE_KEYWORD

    eof,
};

std::ostream& operator<<(std::ostream& os, token_type t);
std::wostream& operator<<(std::wostream& os, token_type t);

constexpr bool is_literal(token_type tt) {
    return tt == token_type::null_ || tt == token_type::true_ || tt == token_type::false_ || tt == token_type::numeric_literal || tt == token_type::string_literal;
}

constexpr bool is_relational(token_type tt) {
    return tt == token_type::lt || tt == token_type::ltequal || tt == token_type::gt || tt == token_type::gtequal;
}

extern token_type without_assignment(token_type t);

const char* op_text(token_type tt);

extern unsigned get_hex_value(int ch);
unsigned get_hex_value2(const wchar_t* s);
unsigned get_hex_value4(const wchar_t* s);

extern bool is_whitespace_or_line_terminator(char16_t ch, version ver);

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
    explicit lexer(const std::wstring_view& text, version ver);

    const token& current_token() const { return current_token_; }
    std::wstring_view text() const { return text_; }
    uint32_t text_position() const { return static_cast<uint32_t>(text_pos_); }

    void next_token();

    // Since the grammar for regular expression literals is context sensitive
    // the parser has to explicitly request parsing of them.
    // The current token must be either divide or divideequal when calling
    // method. The complete regular expression literal text is returned and
    // the scanner advanced to the next token (i.e. current_token() will 
    // return the first token following the regular expression)
    std::wstring_view get_regex_literal();

private:
    std::wstring_view text_;
    version version_;
    size_t text_pos_ = 0;
    token current_token_ = eof_token;
};

std::wstring cpp_quote(const std::wstring_view& s);

// Remove Unicode Format-Control Characters (ES3, 7.1)
std::wstring strip_format_control_characters(const std::wstring_view& s);

} // namespace mjs

#endif
