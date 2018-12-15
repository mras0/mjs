#include "test.h"
#include "test_spec.h"
#include <sstream>
#include <mjs/parser.h>

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

auto make_source(const char* text) {
    return std::make_shared<source_file>(L"test", std::wstring(text, text+std::strlen(text)));
}

auto parse_text(const char* text) {
    return parse(make_source(text), tested_version());
}


auto parse_one_statement(const char* text) {
    auto bs = parse_text(text);
    REQUIRE_EQ(bs->l().size(), 1U);
    return std::move(const_cast<statement_ptr&>(bs->l().front()));
}


void test_parse_fails(const char* text) {
    try {
        parse_text(text);
    } catch ([[maybe_unused]] const std::exception& e) {
        //std::wcerr << "\n'" << text << "' ---->\n" << e.what() << "\n\n";
        return;
    }
    std::ostringstream oss;
    oss << "Unexpected parse success for '" << text << "' parser_version " << tested_version();
    throw std::runtime_error(oss.str());
}

std::unique_ptr<expression_statement> parse_expression(const char* text) {
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

void test_es1_fails_with_new_constructs() {
    tested_version(version::es1);

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
    test_parse_fails("({1:2,})"); // Trailing comma not allowed until ES5
    test_parse_fails("({test})"); // No shorthand property name syntaax until ES2015

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


int main() {
    try {
        for (const auto ver: supported_versions) {
            tested_version(ver);
            test_semicolon_insertion();
        }

        test_es1_fails_with_new_constructs();

        for (const auto ver: { version::es3 }) {
            tested_version(ver);
            test_array_literal();
            test_object_literal();
            test_labelled_statements();
        }

    } catch (const std::runtime_error& e) {
        std::wcerr << e.what() << "\n";
        return 1;
    }
}
