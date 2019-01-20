#include "test.h"
#include "test_spec.h"
#include <sstream>
#include <exception>
#include <mjs/parser.h>

#define STR(s) token{token_type::string_literal, s}

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

std::exception_ptr test_parse_fails(const std::wstring_view text) {
    try {
        auto res = parse_text(text);
        std::wcout << "Parsed:\n" << *res << "\n";
    } catch (...) {
        return std::current_exception();
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
        const auto& elem = e->elements()[0];
        REQUIRE_EQ(elem.type(), property_assignment_type::normal);
        REQUIRE_EQ(CHECK_EXPR_TYPE(elem.name(), literal).t(), token{1.0});
        REQUIRE_EQ(CHECK_EXPR_TYPE(elem.value(), literal).t(), token{2.0});
    }

    {
        const auto& [bs, e] = parse_object_literal("({get:1,set:2})");
        (void)bs;
        REQUIRE_EQ(e->elements().size(), 2U);
        const auto& es = e->elements();
        REQUIRE_EQ(es[0].type(), property_assignment_type::normal);
        REQUIRE_EQ(CHECK_EXPR_TYPE(es[0].name(), identifier).id(), L"get");
        REQUIRE_EQ(CHECK_EXPR_TYPE(es[0].value(), literal).t(), token{1.0});
        REQUIRE_EQ(es[1].type(), property_assignment_type::normal);
        REQUIRE_EQ(CHECK_EXPR_TYPE(es[1].name(), identifier).id(), L"set");
        REQUIRE_EQ(CHECK_EXPR_TYPE(es[1].value(), literal).t(), token{2.0});
    }

    RUN_TEST(L"{1,2,3}", value{3.}); // Not an object literal!

    test_parse_fails("{,})");

    test_parse_fails("({get q:2})");

    const char* const get_lit = "({get x  ( \n )\t{}})";
    const char* const set_lit = "({set x  (  x )  {}})";

    if (tested_version() == version::es3) {
        test_parse_fails("({33:66,})"); // Trailing comma not allowed until ES5
        test_parse_fails("{if:2})"); // Rserved words not allowed as property name until ES5
        test_parse_fails("global.for"); // Rserved words not allowed as property name until ES5
        test_parse_fails(get_lit);
        test_parse_fails(set_lit);
    } else {
        const auto& [bs, e] = parse_object_literal("({33:66,})");
        (void)bs;
        REQUIRE_EQ(e->elements().size(), 1U);
        const auto& elem = e->elements()[0];
        REQUIRE_EQ(elem.type(), property_assignment_type::normal);
        REQUIRE_EQ(CHECK_EXPR_TYPE(elem.name(), literal).t(), token{33.0});
        REQUIRE_EQ(CHECK_EXPR_TYPE(elem.value(), literal).t(), token{66.0});

        RUN_TEST(L"({if:2}).if", value{2.0}); // Reserved words can be used as property names
        RUN_TEST(L"global.for", value::undefined);

        {
            const auto& [_, ge] = parse_object_literal(get_lit); (void)_;
            const auto& ges = ge->elements();
            REQUIRE_EQ(ges.size(), 1U);
            REQUIRE_EQ(ges[0].type(), property_assignment_type::get);
            REQUIRE_EQ(CHECK_EXPR_TYPE(ges[0].name(), identifier).id(), L"x");
            auto f = CHECK_EXPR_TYPE(ges[0].value(), function);
            REQUIRE_EQ(f.id(), L"get x");
            REQUIRE_EQ(f.params().size(), 0U);
            REQUIRE_EQ(f.body_extend().source_view(), L"( \n )\t{}");
        }


        {
            const auto& [_, se] = parse_object_literal(set_lit); (void)_;
            const auto& ses = se->elements();
            REQUIRE_EQ(ses.size(), 1U);
            REQUIRE_EQ(ses[0].type(), property_assignment_type::set);
            REQUIRE_EQ(CHECK_EXPR_TYPE(ses[0].name(), identifier).id(), L"x");
            CHECK_EXPR_TYPE(ses[0].value(), function);
            auto f = CHECK_EXPR_TYPE(ses[0].value(), function);
            REQUIRE_EQ(f.id(), L"set x");
            REQUIRE_EQ(f.params().size(), 1U);
            REQUIRE_EQ(f.params()[0], L"x");
            REQUIRE_EQ(f.body_extend().source_view(), L"(  x )  {}");
        }

        // SyntaxError on wrong paramter count
        test_parse_fails("({get x(a){}})");
        test_parse_fails("({get x(a,b){}})");
        test_parse_fails("({set x(){}})");
        test_parse_fails("({set x(a,b){}})");

        {
            const auto& [_, oe] = parse_object_literal("({get if(){return 42;}, set if(x){}, set if(y){}, x:42})"); (void)_;
            const auto& oes = oe->elements();
            REQUIRE_EQ(oes.size(), 4U);

            REQUIRE_EQ(oes[0].type(), property_assignment_type::get);
            REQUIRE_EQ(CHECK_EXPR_TYPE(oes[0].name(), identifier).id(), L"if");
            CHECK_EXPR_TYPE(oes[0].value(), function);

            REQUIRE_EQ(oes[1].type(), property_assignment_type::set);
            REQUIRE_EQ(CHECK_EXPR_TYPE(oes[1].name(), identifier).id(), L"if");
            CHECK_EXPR_TYPE(oes[1].value(), function);

            REQUIRE_EQ(oes[2].type(), property_assignment_type::set);
            REQUIRE_EQ(CHECK_EXPR_TYPE(oes[2].name(), identifier).id(), L"if");
            CHECK_EXPR_TYPE(oes[2].value(), function);

            REQUIRE_EQ(oes[3].type(), property_assignment_type::normal);
            REQUIRE_EQ(CHECK_EXPR_TYPE(oes[3].name(), identifier).id(), L"x");
            REQUIRE_EQ(CHECK_EXPR_TYPE(oes[3].value(), literal).t(), token{42.});
        }

        {
            const auto& [_, oe] = parse_object_literal("({get 1e3(){}, set 'test x\\u1234'(a){}})"); (void)_;
            const auto& oes = oe->elements();
            REQUIRE_EQ(oes.size(), 2U);

            REQUIRE_EQ(oes[0].type(), property_assignment_type::get);
            REQUIRE_EQ(CHECK_EXPR_TYPE(oes[0].name(), literal).t(), token{1e3});
            REQUIRE_EQ(oes[0].name_str(), L"1000");
            CHECK_EXPR_TYPE(oes[0].value(), function);

            REQUIRE_EQ(oes[1].type(), property_assignment_type::set);
            REQUIRE_EQ(CHECK_EXPR_TYPE(oes[1].name(), literal).t(), STR(L"test x\x1234"));
            CHECK_EXPR_TYPE(oes[1].value(), function);
        }
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
    {
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
        REQUIRE_EQ(rle.re().pattern(), LR"(a*b\/)");
        REQUIRE_EQ(rle.re().flags(), regexp_flag::global);
    }

    {
        auto es = parse_expression("/abc/gim");
        REQUIRE_EQ(es->e().type(), expression_type::regexp_literal);
        const auto& re = static_cast<const regexp_literal_expression&>(es->e()).re();
        REQUIRE_EQ(re.pattern(), L"abc");
        REQUIRE_EQ(re.flags(), regexp_flag::global | regexp_flag::ignore_case | regexp_flag::multiline);
    }

    // TODO: Error may be deferred in ES3, could support that
    test_parse_fails(L"/re/x");
    test_parse_fails(L"/(/");
    test_parse_fails(L"function f() { /re/x; }");
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
        REQUIRE_EQ(re.re().pattern(), L"\xFEFF\x200c\xADtest");
        REQUIRE_EQ(re.re().flags(), regexp_flag::global | regexp_flag::ignore_case);

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

namespace identifier_classification {

enum word_class { none, keyword, reserved, strict };

template<typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, word_class c) {
    switch (c) {
    case none:      return os << "none";
    case keyword:   return os << "keyword";
    case reserved:  return os << "reserved";
    case strict:    return os << "strict";
    }
    assert(false);
    return os << "word_class{" << static_cast<int>(c) << "}";
}

constexpr struct {
    const char* const str;
    word_class es1;
    word_class es3;
    word_class es5;

    word_class get_class() const {
        switch (tested_version()) {
        case version::es1: return es1;
        case version::es3: return es3;
        default:
            assert(false);
            [[fallthrough]];
        case version::es5: return es5;
        }
    }
} classifications[] = {
    { "abstract"     , none     , reserved , none     },
    { "boolean"      , none     , reserved , none     },
    { "break"        , keyword  , keyword  , keyword  },
    { "byte"         , none     , reserved , none     },
    { "case"         , reserved , keyword  , keyword  },
    { "catch"        , reserved , keyword  , keyword  },
    { "char"         , none     , reserved , none     },
    { "class"        , reserved , reserved , reserved },
    { "const"        , reserved , reserved , reserved },
    { "continue"     , keyword  , keyword  , keyword  },
    { "debugger"     , reserved , reserved , keyword  },
    { "default"      , reserved , keyword  , keyword  },
    { "delete"       , keyword  , keyword  , keyword  },
    { "do"           , reserved , keyword  , keyword  },
    { "double"       , none     , reserved , none     },
    { "else"         , keyword  , keyword  , keyword  },
    { "enum"         , reserved , reserved , reserved },
    { "export"       , reserved , reserved , reserved },
    { "extends"      , reserved , reserved , reserved },
    { "final"        , none     , reserved , none     },
    { "finally"      , reserved , keyword  , keyword  },
    { "float"        , none     , reserved , none     },
    { "for"          , keyword  , keyword  , keyword  },
    { "function"     , keyword  , keyword  , keyword  },
    { "goto"         , none     , reserved , none     },
    { "if"           , keyword  , keyword  , keyword  },
    { "implements"   , none     , reserved , strict   },
    { "import"       , reserved , reserved , reserved },
    { "in"           , keyword  , keyword  , keyword  },
    { "instanceof"   , none     , keyword  , keyword  },
    { "int"          , none     , reserved , none     },
    { "interface"    , none     , reserved , strict   },
    { "let"          , none     , none     , strict   },
    { "long"         , none     , reserved , none     },
    { "native"       , none     , reserved , none     },
    { "new"          , keyword  , keyword  , keyword  },
    { "package"      , none     , reserved , strict   },
    { "private"      , none     , reserved , strict   },
    { "protected"    , none     , reserved , strict   },
    { "public"       , none     , reserved , strict   },
    { "return"       , keyword  , keyword  , keyword  },
    { "short"        , none     , reserved , none     },
    { "static"       , none     , reserved , strict   },
    { "super"        , reserved , reserved , reserved },
    { "switch"       , reserved , keyword  , keyword  },
    { "synchronized" , none     , reserved , none     },
    { "this"         , keyword  , keyword  , keyword  },
    { "throw"        , reserved , keyword  , keyword  },
    { "throws"       , none     , reserved , none     },
    { "transient"    , none     , reserved , none     },
    { "try"          , reserved , keyword  , keyword  },
    { "typeof"       , keyword  , keyword  , keyword  },
    { "var"          , keyword  , keyword  , keyword  },
    { "void"         , keyword  , keyword  , keyword  },
    { "volatile"     , none     , reserved , none     },
    { "while"        , keyword  , keyword  , keyword  },
    { "with"         , keyword  , keyword  , keyword  },
    { "yield"        , none     , none     , strict   },
};

word_class classify(const char* str, bool use_strict) {
    try {
        const auto bs = parse_text(use_strict ? (std::string{"'use strict';"} + str).c_str() : str);
        REQUIRE(bs);
        REQUIRE_EQ(bs->l().size(), use_strict ? 2U : 1U);
        const auto& s = *bs->l()[use_strict ? 1 : 0];
        if (s.type() == statement_type::expression) {
            const auto& e = static_cast<const expression_statement&>(s).e();
            if (e.type() == expression_type::identifier) {
                return none;
            }
        }
        return keyword;
    } catch (const std::exception& e) {
        const auto estr = std::string{e.what()};
        if (estr.find("reserved") != std::string::npos) {
            REQUIRE(estr.find(str) != std::string::npos);
            if (estr.find("strict") != std::string::npos) {
                return strict;
            }
            return reserved;
        }
        return keyword;
    }
}

} // identifier_classification

void check_resered_words() {
    using namespace identifier_classification;
    for (const auto& in: classifications) {
        auto expect = in.get_class();
        if (expect != strict) {
            REQUIRE_EQ(classify(in.str, false), expect);
            REQUIRE_EQ(classify(in.str, true), expect);
        } else {
            REQUIRE_EQ(classify(in.str, false), none);
            REQUIRE_EQ(classify(in.str, true),  strict);
        }
    }
}

void test_strict_mode() {
    const auto v = tested_version();
    auto is_strict_global = [](const char* text) { return parse_text(text)->strict_mode(); };
    REQUIRE_EQ(is_strict_global("'use strict'"), v >= version::es5);
    REQUIRE_EQ(is_strict_global("\"use strict\""), v >= version::es5);
    REQUIRE_EQ(is_strict_global("\"use strict\";"), v >= version::es5);
    REQUIRE_EQ(is_strict_global("'\\u0075se strict';"), false);
    REQUIRE_EQ(is_strict_global("'use'+'strict'"), false);

    {
        auto bs = parse_text("if (1) { 'use strict'; }");
        REQUIRE_EQ(bs->strict_mode(), false);
        REQUIRE_EQ(bs->l().size(), 1U);
        REQUIRE_EQ(bs->l()[0]->type(), statement_type::if_);
        const auto& if_s = static_cast<const if_statement&>(*bs->l()[0]);
        REQUIRE(!if_s.else_s());
        REQUIRE_EQ(if_s.if_s().type(), statement_type::block);
        REQUIRE(!static_cast<const block_statement&>(if_s.if_s()).strict_mode());
    }

    auto is_strict_function = [](const std::string& body) {
        auto bs = parse_text(("function f(){" + body + "}").c_str());
        REQUIRE(!bs->strict_mode());
        REQUIRE_EQ(bs->l().size(), 1U);
        REQUIRE_EQ(bs->l()[0]->type(), statement_type::function_definition);
        const auto& f = static_cast<const function_definition&>(*bs->l()[0]);
        REQUIRE_EQ(f.id(), L"f");
        REQUIRE_EQ(f.params().size(), 0U);
        return f.block().strict_mode();
    };

    REQUIRE_EQ(is_strict_function("'use strict'"), v >= version::es5);
    REQUIRE_EQ(is_strict_function("1;'use strict'"), false);
    REQUIRE_EQ(is_strict_function("'\\x75se strict'"), false);
}

void test_main() {
    check_resered_words();
    test_semicolon_insertion();
    test_form_control_characters();
    if (tested_version() > version::es1) {
        test_array_literal();
        test_object_literal();
        test_labelled_statements();
        test_regexp_literal();
    }
    if (tested_version() > version::es3) {
        test_debugger_statement();
    }
    if (tested_version() < version::es3) {
        test_fails_with_es3_constructors();
    }
    if (tested_version() < version::es5) {
        test_fails_with_es5_constructors();
    }
    test_strict_mode();
}
