#include "test.h"
#include "test_spec.h"
#include <sstream>
#include <mjs/parser.h>
#include <mjs/platform.h>

namespace mjs {

template<typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, expression_type e) {
    return os << "expression_type{" << (int)e << "}";
}

template<typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, statement_type s) {
    return os << "statement_type{" << (int)s << "}";
}

}

using namespace mjs;

struct parse_failure {};

auto make_source(const std::wstring_view& text) {
    return std::make_shared<source_file>(L"test", text, tested_version());
}

auto make_source(const char* text) {
    return make_source(std::wstring(text, text+std::strlen(text)));
}

template<typename T>
auto parse_text(T&& text) {
    return parse(make_source(std::forward<T>(text)));
}

template<typename CharT>
auto parse_one_statement(const CharT* text) {
    try {
        auto bs = parse_text(text);
        REQUIRE_EQ(bs->l().size(), 1U);
        return std::move(const_cast<statement_ptr&>(bs->l().front()));
    } catch (...) {
        std::wcerr << "Error while parsing \"" << text << "\"\n";
        throw;
    }
}

void test_parse_fails(const std::wstring_view text) {
    try {
        auto res = parse_text(text);
        std::wcout << "Parsed:\n" << *res << "\n";
    } catch ([[maybe_unused]] const std::exception& e) {
        //std::wcerr << "\n'" << text << "' ---->\n" << e.what() << "\n\n";
        return;
    }
    std::wostringstream woss;
    woss << "Unexpected parse success for '" << cpp_quote(text) << "' parser_version " << tested_version();
    auto s = woss.str();
    throw std::runtime_error(std::string(s.begin(),s.end()));
}

void test_parse_fails(const char* text) {
    test_parse_fails(std::wstring{text, text+std::strlen(text)});
}

template<typename CharT>
std::unique_ptr<expression_statement> parse_expression(const CharT* text) {
    try {
        auto s = parse_one_statement(text);
        REQUIRE_EQ(s->type(), statement_type::expression);
        return std::unique_ptr<expression_statement>{static_cast<expression_statement*>(s.release())};
    } catch (...) {
        std::wcerr << "Parse failed for '" << text << "' parser version " << tested_version() << "\n";
        throw;
    }
}

void test_semicolon_insertion() {
    test_parse_fails(R"({ 1 2 } 3)");
    RUN_TEST(LR"({ 1
2 } 3)", value{3.});
    RUN_TEST(LR"({ 1
;2 ;} 3;)", value{3.}); // Valid
    test_parse_fails(R"(for (a; b
))");

    RUN_TEST(LR"(function f(a, b) { return
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

    {
        auto cont_s = parse_one_statement("continue");
        REQUIRE_EQ(cont_s->type(), statement_type::continue_);
        REQUIRE_EQ(static_cast<const continue_statement&>(*cont_s).id(), L"");
    }
    {
        auto break_s = parse_one_statement("break");
        REQUIRE_EQ(break_s->type(), statement_type::break_);
        REQUIRE_EQ(static_cast<const break_statement&>(*break_s).id(), L"");
    }

    {
        auto es = parse_text("continue\nid");
        REQUIRE_EQ(es->l().size(), 2U);
        REQUIRE_EQ(es->l()[0]->type(), statement_type::continue_);
        REQUIRE_EQ(static_cast<const continue_statement&>(*es->l()[0]).id(), L"");
        REQUIRE_EQ(es->l()[1]->type(), statement_type::expression);
        
    }
    {
        auto es = parse_text("break\nid");
        REQUIRE_EQ(es->l().size(), 2U);
        REQUIRE_EQ(es->l()[0]->type(), statement_type::break_);
        REQUIRE_EQ(static_cast<const break_statement&>(*es->l()[0]).id(), L"");
        REQUIRE_EQ(es->l()[1]->type(), statement_type::expression);
    }

    if (tested_version() < version::es3) {
        test_parse_fails("continue id");
        test_parse_fails("break id");
    } else {
        {
            auto cont_s = parse_one_statement("continue id");
            REQUIRE_EQ(cont_s->type(), statement_type::continue_);
            REQUIRE_EQ(static_cast<const continue_statement&>(*cont_s).id(), L"id");
        }
        {
            auto break_s = parse_one_statement("break id");
            REQUIRE_EQ(break_s->type(), statement_type::break_);
            REQUIRE_EQ(static_cast<const break_statement&>(*break_s).id(), L"id");
        }
        // TODO: Check no-line-break between throw and expression (like return)
    }
}

void test_fails_with_es3_constructors() {
    test_parse_fails("1===2");
    test_parse_fails("1!==2");
    test_parse_fails("[1]");
    test_parse_fails("[1,2]");
    test_parse_fails("({a:42})");
    test_parse_fails("({1:2})");
    RUN_TEST(L"{1,2,3}", value{3.}); // Not an object literal!
    test_parse_fails("a in x");
    test_parse_fails("a instanceof x");
    test_parse_fails("switch (x){}");
    test_parse_fails("do {} while(1);");
    test_parse_fails("lab: foo();");
    test_parse_fails("for(;;){ break test;}");
    test_parse_fails("try {} catch(e) {}");
    test_parse_fails("throw 42;");
    test_parse_fails("a = function x() {}");
    test_parse_fails("a = function() {}");
    test_parse_fails("a = /4/;");
}

void test_fails_with_es5_constructors() {
    test_parse_fails("debugger;");
}

template<typename T>
const T& expression_cast(const expression& e, expression_type expected) {
    REQUIRE_EQ(e.type(), expected);
    return static_cast<const T&>(e);
}

template<typename T>
const T& expression_cast(const expression_ptr& e, expression_type expected) {
    REQUIRE(e);
    REQUIRE_EQ(e->type(), expected);
    return static_cast<const T&>(*e);
}

#define CHECK_EXPR_TYPE(e, t) expression_cast<t ## _expression>(e, expression_type::t)

void test_array_literal() {
    auto parse_array_literal = [](const char* text) {
        auto es = parse_expression(text);
        return std::make_pair(std::move(es), &CHECK_EXPR_TYPE(es->e(), array_literal));
    };

    {
        auto [es, a] = parse_array_literal("[]");
        (void)es;
        REQUIRE_EQ(a->elements().size(), 0U);
    }

    {
        auto [es, a] = parse_array_literal("[1]");
        (void)es;
        REQUIRE_EQ(a->elements().size(), 1U);
        auto& ale = CHECK_EXPR_TYPE(a->elements().front(), literal);
        REQUIRE_EQ(ale.t(), token{1.});
    }

    // Elision

    {
        auto [es, a] = parse_array_literal("[,]");
        (void)es;
        REQUIRE_EQ(a->elements().size(), 1U);
        REQUIRE(!a->elements().front());
    }

    {
        const auto& [bs, e] = parse_array_literal("[,,]");
        (void)bs;
        REQUIRE_EQ(e->elements().size(), 2U);
        REQUIRE(!e->elements()[0]);
        REQUIRE(!e->elements()[1]);
    }

    // More complicated example
    RUN_TEST_SPEC(R"(
a = [x=2,3,'test,',,];
x; //$number 2
a.length; //$number 4
a[0]; //$number 2
a[1]; //$number 3
a[2]; //$string 'test,'
a[3]; //$undefined
)");

    // Nested
    RUN_TEST_SPEC(R"(
a=[,,[,42,]]
a.length; //$number 3
a.toString(); //$string ',,,42'
a[0]; //$undefined
a[1]; //$undefined
a[2].length; //$number 2
a[2][0]; //$undefined
a[2][1]; //$number 42
)");
}

void test_object_literal() {
    auto parse_object_literal = [](const char* text) {
        auto es = parse_expression(text);
        return std::make_pair(std::move(es), &CHECK_EXPR_TYPE(es->e(), object_literal));
    };

    {
        const auto& [bs, e] = parse_object_literal("({ })");
        (void)bs;
        REQUIRE_EQ(e->elements().size(), 0U);
    }

    {
        const auto& [bs, e] = parse_object_literal("({1:2})");
        (void)bs;
        REQUIRE_EQ(e->elements().size(), 1U);
        const auto& [p0, v0] = e->elements()[0];
        REQUIRE(p0);
        REQUIRE(v0);
        REQUIRE_EQ(CHECK_EXPR_TYPE(p0, literal).t(), token{1.0});
        REQUIRE_EQ(CHECK_EXPR_TYPE(v0, literal).t(), token{2.0});
    }

    RUN_TEST(L"{1,2,3}", value{3.}); // Not an object literal!

    test_parse_fails("{,})");

    if (tested_version() == version::es3) {
        test_parse_fails("({33:66,})"); // Trailing comma not allowed until ES5
        test_parse_fails("{if:2})"); // Rserved words not allowed as property name until ES5
        test_parse_fails("global.for"); // Rserved words not allowed as property name until ES5
    } else {
        const auto& [bs, e] = parse_object_literal("({33:66,})");
        (void)bs;
        REQUIRE_EQ(e->elements().size(), 1U);
        const auto& [p0, v0] = e->elements()[0];
        REQUIRE(p0);
        REQUIRE(v0);
        REQUIRE_EQ(CHECK_EXPR_TYPE(p0, literal).t(), token{33.0});
        REQUIRE_EQ(CHECK_EXPR_TYPE(v0, literal).t(), token{66.0});

        RUN_TEST(L"({if:2}).if", value{2.0}); // Reserved words can be used as property names
        RUN_TEST(L"global.for", value::undefined);
    }

    test_parse_fails("({test})"); // No shorthand property name syntax until ES2015

    // More complicated examples + nesting
    RUN_TEST_SPEC(R"(
o1 = {1e3:'test'};
o1['1000'];//$string 'test'

o2 = {1:2,3:4,'test':'hest'};
o2[1]; //$number 2
o2[3]; //$number 4
o2['test']; //$string 'hest'

o3 = {x: {1:2}, y: {3:4}};
o3['x']['1']; //$number 2
o3['y']['3']; //$number 4
)");
}

void test_labelled_statements() {
    auto s = parse_one_statement("x:y:;");
    REQUIRE_EQ(s->type(), statement_type::labelled);
    const auto& lsx = static_cast<const labelled_statement&>(*s);
    REQUIRE_EQ(lsx.id(), L"x");
    REQUIRE_EQ(lsx.s().type(), statement_type::labelled);
    const auto& lsy = static_cast<const labelled_statement&>(lsx.s());
    REQUIRE_EQ(lsy.id(), L"y");
    REQUIRE_EQ(lsy.s().type(), statement_type::empty);
}

void test_regexp_literal() {
    auto s = parse_one_statement(R"(a = /a*b\//g;)");
    REQUIRE_EQ(s->type(), statement_type::expression);
    const auto& es = static_cast<const expression_statement&>(*s);
    REQUIRE_EQ(es.e().type(), expression_type::binary);
    const auto& be = static_cast<const binary_expression&>(es.e());
    REQUIRE_EQ(be.op(), token_type::equal);
    REQUIRE_EQ(be.lhs().type(), expression_type::identifier);
    REQUIRE_EQ(static_cast<const identifier_expression&>(be.lhs()).id(), L"a");
    REQUIRE_EQ(be.rhs().type(), expression_type::regexp_literal);
    const auto& rle = static_cast<const regexp_literal_expression&>(be.rhs());
    REQUIRE_EQ(rle.pattern(), LR"(a*b\/)");
    REQUIRE_EQ(rle.flags(), L"g");
}

void test_form_control_characters() {
    const wchar_t* const text = L"t\x200cq12\xADst\xFEFFww;";
    if (tested_version() != version::es3) {
        test_parse_fails(text);
    } else {
        auto s = parse_one_statement(text);
        REQUIRE_EQ(s->type(), statement_type::expression);
        const auto& e = static_cast<const expression_statement&>(*s).e();
        REQUIRE_EQ(e.type(), expression_type::identifier);
        REQUIRE_EQ(static_cast<const identifier_expression&>(e).id(), L"tq12stww");
    }

    // Format control characters were stripped in ES3
    {
        const wchar_t* const lit = L"'\xFEFF\x200c\xADtest!!'";
        const wchar_t* const expected_lit = tested_version() == version::es3 ? L"test!!" : L"\xFEFF\x200c\xADtest!!";
        auto s = parse_one_statement(lit);
        REQUIRE_EQ(s->type(), statement_type::expression);
        const auto& e = static_cast<const expression_statement&>(*s).e();
        REQUIRE_EQ(e.type(), expression_type::literal);
        const auto& le = static_cast<const literal_expression&>(e);
        REQUIRE_EQ(le.t().type(), token_type::string_literal);
        REQUIRE_EQ(le.t().text(), expected_lit);
    }

    if (tested_version() >= version::es5) {
        // Cf in expression regular expression literals
        auto s = parse_one_statement(L"\xFEFF/\xFEFF\x200c\xADtest/gi");
        REQUIRE_EQ(s->type(), statement_type::expression);
        const auto& e = static_cast<const expression_statement&>(*s).e();
        REQUIRE_EQ(e.type(), expression_type::regexp_literal);
        const auto& re = static_cast<const regexp_literal_expression&>(e);
        REQUIRE_EQ(re.pattern(), L"\xFEFF\x200c\xADtest");
        REQUIRE_EQ(re.flags(), L"gi");

        // Zero-width (non)-joiner in identifier
        auto s2 = parse_one_statement(L"test\\u200c\x200dxyz");
        REQUIRE_EQ(s2->type(), statement_type::expression);
        const auto& e2 = static_cast<const expression_statement&>(*s2).e();
        REQUIRE_EQ(e2.type(), expression_type::identifier);
        REQUIRE_EQ(static_cast<const identifier_expression&>(e2).id(), L"test\x200c\x200dxyz");
    }

}

void test_debugger_statement() {
    auto s = parse_one_statement("debugger;");
    REQUIRE_EQ(s->type(), statement_type::debugger);
}

int main() {
    platform_init();
    try {
        for (const auto ver: supported_versions) {
            tested_version(ver);
            test_semicolon_insertion();
            test_form_control_characters();
            if (ver > version::es1) {
                test_array_literal();
                test_object_literal();
                test_labelled_statements();
                test_regexp_literal();
            }
            if (ver > version::es3) {
                test_debugger_statement();
            }
            if (ver < version::es3) {
                test_fails_with_es3_constructors();
            }
            if (ver < version::es5) {
                test_fails_with_es5_constructors();
            }
        }

    } catch (const std::runtime_error& e) {
        std::wcerr << e.what() << "\n";
        std::wcerr << "Tested version: " << tested_version() << "\n";
        return 1;
    }
}
