#include "test.h"

#include <mjs/lexer.h>
#include <vector>
#include <sstream>

using namespace mjs;

std::vector<token> lex(const std::wstring_view& s) {
    try {
        lexer l{s, tested_version()};
        std::vector<token> ts;
        for (; l.current_token(); l.next_token()) {
            ts.push_back(l.current_token());
        }
        return ts;
    } catch (...) {
        std::wcerr << "Error while processing \"" << cpp_quote(s) << "\"\n";
        std::wcerr << "Lexer version: " << tested_version() << "\n";
        throw;
    }
}

std::string check_lex_fails(const std::wstring_view& s) {
    try {
        lexer l{s, tested_version()};
        std::vector<token> ts;
        for (; l.current_token(); l.next_token()) {
            ts.push_back(l.current_token());
        }
        std::wcerr << "Lexer (version=" << tested_version() << ") should have failed for '" << s << "'\n";
        std::wcerr << "Returned:\n" << ts << "\n";
        std::abort();
    } catch (const std::exception& e) {
        return e.what();
    }
}

#define TOKENS(...) std::vector<token>({__VA_ARGS__});

#define SIMPLE_TEST(text, ...) REQUIRE_EQ(lex(text), TOKENS(__VA_ARGS__))
#define STR(text) token{token_type::string_literal, L ## text}
#define ID(text) token{token_type::identifier, L ## text}
#define T(t) token{token_type::t}
#define WS T(whitespace)
#define NL T(line_terminator)

void basic_tests() {
    // Whitespace
    SIMPLE_TEST(L"\x09\x0b\x0c\x20 ab", WS, ID("ab"));
    if (tested_version() > version::es1) {
        // <NBSP> + <USP> are considered whitespace in ES3+
        SIMPLE_TEST(L"\xA0\x1680\x2000", WS);
    }
    // Line terminators
    SIMPLE_TEST(L"\012x\015", NL, ID("x"), NL);
    if (tested_version() > version::es1) {
        // In ES3+ \u2028 and \u2029 are considered line terminators
        SIMPLE_TEST(L"\x2028\x2029", NL);
        SIMPLE_TEST(L"x//\x2029", ID("x"), WS, NL);
    }
    // Comments
    SIMPLE_TEST(L"/*hello world!*/", WS);
    SIMPLE_TEST(L"/*hello\nworld!*/", NL);
    SIMPLE_TEST(L"x//test", ID("x"), WS);
    SIMPLE_TEST(L"x//test\n", ID("x"), WS, NL);
    // Reserver word
    SIMPLE_TEST(LR"(for)", T(for_));
    // Identifier
    SIMPLE_TEST(LR"(test)", ID("test"));
    SIMPLE_TEST(L"a b cd", ID("a"), WS, ID("b"), WS, ID("cd"));
    SIMPLE_TEST(L"$ _ $foo2_", ID("$"), WS, ID("_"), WS, ID("$foo2_"));
    SIMPLE_TEST(L"1a", token{1.}, ID("a"));
    // Punctuator
    SIMPLE_TEST(LR"(<<>>>=)", T(lshift), T(rshiftshiftequal));
    if (tested_version() == version::es1) {
        SIMPLE_TEST(LR"(= = ===)", T(equal), WS, T(equal), WS, T(equalequal), T(equal));
        SIMPLE_TEST(LR"(!==)", T(notequal), T(equal));
    } else {
        // ===/ !== are new in ES3
        SIMPLE_TEST(LR"(= = ===)", T(equal), WS, T(equal), WS, T(equalequalequal));
        SIMPLE_TEST(LR"(!==)", T(notequalequal));
    }
    // Literal
    //  - null literal
    SIMPLE_TEST(LR"(null)", T(null_));
    //  - boolean literals
    SIMPLE_TEST(LR"(true)", T(true_));
    SIMPLE_TEST(LR"(false)", T(false_));
    //  - numeric literal
    SIMPLE_TEST(LR"(60)", token{60.});
    SIMPLE_TEST(LR"(42.25)", token{42.25});
    SIMPLE_TEST(LR"(1.11e2)", token{111});
    SIMPLE_TEST(LR"(0x100)", token{256});
    SIMPLE_TEST(LR"(0123)", token{83});
    //  - string literal
    SIMPLE_TEST(LR"('test "" str')", STR("test \"\" str"));
    SIMPLE_TEST(LR"("blahblah''")", STR("blahblah''"));
    //      SingleEscapeCharacter
    SIMPLE_TEST(LR"('\'\"\\\b\f\n\r\t')", STR("\'\"\\\b\f\n\r\t"));

    check_lex_fails(L"'\\\n'"); // a line terminator cannot appear in a string literal (even if preceded by a backslash)

    // Vertical tab (<VT>) supported from es3 on
    const auto vert_tab_str = LR"('\v')";
    if (tested_version() == version::es1) {
        check_lex_fails(vert_tab_str);
    } else {
        SIMPLE_TEST(vert_tab_str, STR("\x0B"));
    }
    //      HexEscapeSequence
    SIMPLE_TEST(LR"('\xAB\x0A')", STR("\xAB\n"));
    //      OctalEscapeSequence
    SIMPLE_TEST(LR"('\7\77\123\400')", STR("\x07\x3f\x53\x20\x30"));
    //      UnicodeEscapeSequence
    SIMPLE_TEST(LR"('\u1234')", STR("\x1234"));

    // A combined example
    SIMPLE_TEST(LR"(if (1) foo['x']=bar();)", T(if_), WS, T(lparen), token{1.}, T(rparen), WS, ID("foo"), T(lbracket), STR("x"), T(rbracket), T(equal), ID("bar"), T(lparen), T(rparen), T(semicolon));
}

std::wstring get_one_regexp_literal(std::wstring_view text) {
    lexer l{text, tested_version()};
    const auto regex_lit = l.get_regex_literal();
    REQUIRE(!l.current_token()); // Must be at end
    return std::wstring{regex_lit};
}

void test_get_regex_literal(std::wstring_view text) {
    REQUIRE_EQ(text, get_one_regexp_literal(text));
}

void test_get_regex_literal_fails(std::wstring_view text) {
    try {
        lexer l{text, tested_version()};
        l.get_regex_literal();
        if (l.current_token()) {
            return;
        }
        std::wcerr << "Should have failed while processing regex literal '" << text << "'\n";
        std::abort();
    } catch (...) {
        // OK
    }
}

void test_regexp_literals() {
    test_get_regex_literal(LR"(/(?:)/)");
    test_get_regex_literal(LR"(/=X/)");
    test_get_regex_literal(LR"(/[123]xx/g)");
    test_get_regex_literal(LR"(/a/test)");
    test_get_regex_literal(LR"(/\||1212/)");
    test_get_regex_literal(LR"(/abc\//)");
    test_get_regex_literal(LR"(/[\/\[]]/)");
    test_get_regex_literal_fails(LR"(/abc)");
    test_get_regex_literal_fails(LR"(/abc\/)");
    test_get_regex_literal_fails(L"/a\nbc/");
    const wchar_t* const unsescaped_slash_in_character_class = L"/[/]/";
    // In ES5 '/' can be unescaped in a character class
    if (tested_version() < version::es5) {
        test_get_regex_literal_fails(unsescaped_slash_in_character_class);
    } else {
        test_get_regex_literal(unsescaped_slash_in_character_class);
    }
}

template<size_t size>
void check_keywords(const char* const (&list)[size]) {
    for (auto s: list) {
        const auto tt = std::wstring(s, s+std::strlen(s));
        const auto ts = lex(tt);
        REQUIRE_EQ(ts.size(), 1U);
        REQUIRE(!ts[0].has_text());
        std::wostringstream woss;
        woss << ts[0];
        REQUIRE_EQ(woss.str(), L"token{" + tt + L"}");
    }
}

template<size_t size>
void check_reserved_words(const char* const (&list)[size]) {
    for (auto s: list) {
        auto exc = check_lex_fails(std::wstring(s, s+std::strlen(s)));
        if (exc.find(exc) == std::string::npos) {
            std::wcerr << "Did not find " << s << " in exception message '" << exc.c_str() << "' in " << __func__ << "\n";
            std::abort();
        }
    }
}

const char* const es1_keywords[] = { "break", "continue", "delete", "else", "for", "function", "if", "in", "new", "return", "this", "typeof", "var", "void", "while", "with" };
const char* const es1_reserved_words[] = { "case", "catch", "class", "const", "debugger", "default", "do", "enum", "export", "extends", "finally", "import", "super", "switch", "throw", "try" };

const char* const es3_keywords[] = { "case", "catch", "default", "do", "finally", "instanceof", "switch", "throw", "try", };
const char* const es3_reserved_words[] = { "abstract", "boolean", "byte", "char", "class", "const", "debugger", "double", "enum", "export", "extends", "final", "float", "goto", "implements", "import", "int", "interface", "long", "native", "package", "private", "protected", "public", "short", "static", "super", "synchronized", "throws", "transient", "volatile" };

const char* const es5_keywords[] = { "debugger" };
const char* const es5_reserved_words[] = {
    // ES5.1, 7.6.1.2 FutureReservedWord
    "class", "const", "enum", "export", "extends", "import", "super",
    // FutureReservedWord when in strict mode
    "implements", "interface", "let", "package", "private", "protected", "public", "static", "yield",
};

void check_keywords() {
    check_keywords(es1_keywords);
    if (tested_version() >= version::es3) {
        check_keywords(es3_keywords);
    }
    if (tested_version() >= version::es5) {
        check_keywords(es5_keywords);
    }

    switch (tested_version()) {
    case version::es1: check_reserved_words(es1_reserved_words); break;
    case version::es3: check_reserved_words(es3_reserved_words); break;
    case version::es5: check_reserved_words(es5_reserved_words); break;
    default:
        assert(false);
    }
}

void test_unicode_escape_sequence_in_identifier() {
    check_lex_fails(LR"(\u0069\u0066)"); // Must not spell 'if' like this
    check_lex_fails(LR"(\u0069\u0000)");
    check_lex_fails(LR"(\u0069\u00)");
    check_lex_fails(LR"(\u0069\u0)");
    check_lex_fails(LR"(\u0069\u)");
    check_lex_fails(LR"(\u0069\)");
    check_lex_fails(LR"(\u0069\x66)");

    const auto uid = LR"(\u0073X\u0031d)";
    if (tested_version() == version::es1) {
        check_lex_fails(uid);
    } else {
        SIMPLE_TEST(LR"(\u0069)", ID("i"));
        SIMPLE_TEST(uid, ID("sX1d"));
        SIMPLE_TEST(L"\\u1234\x7533", ID("\x1234\x7533"));
    }
}

void test_format_control_characters() {
    REQUIRE_EQ(strip_format_control_characters(L"te\xADst\x600zz"), L"testzz");
    check_lex_fails(L"1\xAD"); // Soft-hypen
    check_lex_fails(L"a\x600");
    check_lex_fails(L"\x200c");
    const wchar_t * const in_id = L"a\x200c\x200dz";
    const wchar_t * const bom_is_ws = L"\xFEFF  \xFEFF" L"abc\t\xFEFF";
    const wchar_t * const cf_in_lit = L"'\x600\x200c\xFEFF hello'";
    const wchar_t * const cf_in_relit = L"/\x600\x200c\xFEFF/";

    // It's only in ES3 that we can't have format control characters in string literals
    if (tested_version() != version::es3) {
        SIMPLE_TEST(cf_in_lit, STR("\x600\x200c\xFEFF hello"));
    } else {
        check_lex_fails(cf_in_lit);
    }


    if (tested_version() < version::es5) {
        check_lex_fails(in_id);
        check_lex_fails(bom_is_ws);
        check_lex_fails(cf_in_relit);
    } else {
        SIMPLE_TEST(in_id, ID("a\x200c\x200dz"));
        SIMPLE_TEST(bom_is_ws, WS, ID("abc"), WS);
        test_get_regex_literal(cf_in_relit);
    }
}

void test_main() {
    basic_tests();
    test_unicode_escape_sequence_in_identifier();
    test_format_control_characters();
    if (tested_version() > version::es1) {
        test_regexp_literals();
    }
    check_keywords();
}
