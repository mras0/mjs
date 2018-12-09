#include "test.h"
#include "test_spec.h"
#include <sstream>
#include <mjs/parser.h>

using namespace mjs;

struct parse_failure {};

void test_parse_fails(const char* text) {
    try {
        parse(std::make_shared<source_file>(L"test", std::wstring(text, text+std::strlen(text))));
    } catch (const std::exception&) {
        // std::wcerr << "\n'" << text << "' ---->\n" << e.what() << "\n\n";
        return;
    }
    std::ostringstream oss;
    oss << "Unexpected parse success for '" << text << "' parser_version " << parser_version;
    throw std::runtime_error(oss.str());
}

void test_semicolon_insertion() {
    // TODO: Check other examples:
    test_parse_fails(R"({ 1 2 } 3)");
    run_test(LR"({ 1
2 } 3)", value{3.});
    run_test(LR"({ 1
;2 ;} 3;)", value{3.}); // Valid
    test_parse_fails(R"(for (a; b
))");

    run_test(LR"(function f(a, b) { return
        a+b;}
        f(1,2))", value::undefined);

    RUN_TEST_SPEC(R"(
a=1;b=2;c=3;
a = b
++c
; a //$ number 2
; b //$ number 2
; c //$ number 4
)");

    test_parse_fails(R"(if (a > b)
else c = d;)");

    RUN_TEST_SPEC(R"(
function c(n) { return n+1; }
a=1;b=2;d=4;e=5;
a = b + c
(d + e).toString()      //$ string '210'
)");

    RUN_TEST_SPEC(R"(x=0; x=x+1 //$ number 1
x++; x //$ number 2
)");
}

void test_es1_fails_with_new_constructs() {
    parser_version = version::es1;

    test_parse_fails("[1]");
    test_parse_fails("[1,2]");
    test_parse_fails("{a:42}");
    test_parse_fails("{1:2}");
    test_parse_fails("switch (x){}");
    test_parse_fails("do {} while(1);");
    test_parse_fails("lab: foo();");
    test_parse_fails("for(;;){ break test;}");
    test_parse_fails("try {} catch(e) {}");
}

int main() {
    try {
        for (const auto ver:  { version::es1, version::es3 }) {
            parser_version = ver;
            test_semicolon_insertion();
        }
        test_es1_fails_with_new_constructs();
    } catch (const std::runtime_error& e) {
        std::wcerr << e.what() << "\n";
        return 1;
    }
}