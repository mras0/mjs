#include "test.h"

#include <mjs/lexer.h>
#include <vector>

namespace mjs {
bool operator==(const token& l, const token& r) {
    if (l.type() != r.type()) {
        return false;
    } else if (l.has_text()) {
        return l.text() == r.text();
    } else if (l.type() == token_type::numeric_literal) {
        return l.dvalue() == r.dvalue();
    }
    return true;
}

bool operator!=(const token& l, const token& r) {
    return !(l == r);
}
}

using namespace mjs;

std::vector<token> lex(const std::wstring_view& s) {
    try {
        std::vector<token> ts;
        lexer l{s};
        for (; l.current_token(); l.next_token()) {
            ts.push_back(l.current_token());
        }
        return ts;
    } catch (...) {
        std::wcerr << "Error while processing " << s << "\n";
        throw;
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
    // Line terminators
    SIMPLE_TEST(L"\012x\015", NL, ID("x"), NL);
    // Comments
    SIMPLE_TEST(L"/*hello world!*/", WS);
    SIMPLE_TEST(L"/*hello\nworld!*/", NL);
    SIMPLE_TEST(L"x//test", ID("x"), WS);
    SIMPLE_TEST(L"x//test\n", ID("x"), WS, NL);
    // Reserver word
    SIMPLE_TEST(LR"(undefined)", T(undefined_));
    // Identifier
    SIMPLE_TEST(LR"(test)", ID("test"));
    SIMPLE_TEST(L"a b cd", ID("a"), WS, ID("b"), WS, ID("cd"));
    // Punctuator
    SIMPLE_TEST(LR"(<<>>>=)", T(lshift), T(rshiftshiftequal));
    SIMPLE_TEST(LR"(= = ===)", T(equal), WS, T(equal), WS, T(equalequal), T(equal));
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
    //      HexEscapeSequence
    SIMPLE_TEST(LR"('\xAB\x0A')", STR("\xAB\n"));
    //      OctalEscapeSequence
    SIMPLE_TEST(LR"('\7\77\123')", STR("\x07\x3f\x53"));
    //      UnicodeEscapeSequence
    SIMPLE_TEST(LR"('\u1234')", STR("\x1234"));

    SIMPLE_TEST(LR"(if (1) foo['x']=bar();)", T(if_), WS, T(lparen), token{1.}, T(rparen), WS, ID("foo"), T(lbracket), STR("x"), T(rbracket), T(equal), ID("bar"), T(lparen), T(rparen), T(semicolon));
}

int main() {
    try {
        basic_tests();
    } catch (const std::exception& e) {
        std::wcerr << e.what() << "\n";
        return 1;
    }
}
