#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cassert>

#include "value.h"
#include "parser.h"

auto make_runtime_error(const std::wstring_view& s) {
    return std::runtime_error(std::string(s.begin(), s.end()));
}

#define NOT_IMPLEMENTED(o) do { std::wostringstream woss; woss << "Not implemented: " << o << " in " << __FUNCTION__ << " " << __FILE__ << ":" << __LINE__; throw make_runtime_error(woss.str()); } while (0) 

class print_visitor {
public:
    void operator()(const mjs::identifier_expression& e) {
        std::wcout << e.id();
    }

    void operator()(const mjs::literal_expression& e) {
        switch (e.t().type()) {
        case mjs::token_type::numeric_literal:
            std::wcout << e.t().dvalue();
            return;
        case mjs::token_type::string_literal:
            std::wcout << '"' << cpp_quote(e.t().text()) << '"';
            return;
        }
        NOT_IMPLEMENTED(e);
    }
    void operator()(const mjs::call_expression& e) {
        accept(e.member(), *this);
        std::wcout << '(';
        const auto& args = e.arguments();
        for (size_t i = 0; i < args.size(); ++i) {
            std::wcout << (i ? ", " : "");
            accept(*args[i], *this);
        }
        std::wcout << ')';
    }

    void operator()(const mjs::binary_expression& e) {
        const int precedence = mjs::operator_precedence(e.op());
        print_with_parentheses(e.lhs(), precedence);
        std::wcout << op_text(e.op());
        print_with_parentheses(e.rhs(), precedence);
    }

    void operator()(const mjs::expression& e) {
        NOT_IMPLEMENTED(e);
    }

    void operator()(const mjs::expression_statement& s) {
        accept(s.e(), *this);
    }

    void operator()(const mjs::statement& s) {
        NOT_IMPLEMENTED(s);
    }

private:
    void print_with_parentheses(const mjs::expression& e, int outer_precedence) {
        const int inner_precedence = e.type() == mjs::expression_type::binary ? mjs::operator_precedence(static_cast<const mjs::binary_expression&>(e).op()) : 0;
        if (inner_precedence > outer_precedence) std::wcout << '(';
        accept(e, *this);
        if (inner_precedence > outer_precedence) std::wcout << ')';
    }
};

class eval_visitor {
public:
    explicit eval_visitor(const mjs::object_ptr& global) : global_(global) {}

    mjs::value operator()(const mjs::identifier_expression& e) {
        return mjs::value{mjs::reference{global_, e.id()}};
    }

    mjs::value operator()(const mjs::literal_expression& e) {
         switch (e.t().type()) {
         case mjs::token_type::numeric_literal: return mjs::value{e.t().dvalue()};
         case mjs::token_type::string_literal:  return mjs::value{e.t().text()};
         }
         NOT_IMPLEMENTED(e);
    }

    mjs::value operator()(const mjs::call_expression& e) {
        auto member = accept(e.member(), *this);
        auto mval = get_value(member);
        std::vector<mjs::value> args;
        for (const auto& a: e.arguments()) {
            args.push_back(get_value(accept(*a, *this)));
        }
        if (mval.type() != mjs::value_type::object) {
            std::wostringstream woss;
            woss << e.member() << " is not a function";
            throw make_runtime_error(woss.str());
        }
        auto c = mval.object_value()->call_function();
        if (!c) {
            std::wostringstream woss;
            woss << e.member() << " is not callable";
            throw make_runtime_error(woss.str());
        }

        // TODO: scope etc.
        // TODO: Handle this as 11.2.3 specifies
        return c(args);
    }

    mjs::value operator()(const mjs::binary_expression& e) {
        auto l = get_value(accept(e.lhs(), *this));
        auto r = get_value(accept(e.rhs(), *this));
        if (e.op() == mjs::token_type::plus) {
            l = to_primitive(l);
            r = to_primitive(r);
            if (l.type() == mjs::value_type::string || r.type() == mjs::value_type::string) {
                auto ls = to_string(l);
                auto rs = to_string(r);
                return mjs::value{ls + rs};
            }
            // Otherwise handle like the other operators
        }
        const auto ln = to_number(l);
        const auto rn = to_number(r);
        switch (e.op()) {
        case mjs::token_type::plus:     return mjs::value{ln + rn};
        case mjs::token_type::minus:    return mjs::value{ln - rn};
        case mjs::token_type::multiply: return mjs::value{ln * rn};
        case mjs::token_type::divide:   return mjs::value{ln / rn};
        }
        NOT_IMPLEMENTED(e);
    }

    mjs::value operator()(const mjs::expression& e) {
        NOT_IMPLEMENTED(e);
    }

    mjs::value operator()(const mjs::expression_statement& s) {
        return accept(s.e(), *this);
    }

    mjs::value operator()(const mjs::statement& s) {
        NOT_IMPLEMENTED(s);
    }

private:
    mjs::object_ptr global_;
};

auto make_function(const mjs::native_function_type& f) {
    auto o = mjs::object::make(mjs::string{"Function"}, nullptr); // TODO
    o->call_function(f);
    return o;
}

auto make_global() {
    auto global = mjs::object::make(mjs::string{"Object"}, nullptr); // TODO
    global->put(mjs::string{"x"}, mjs::value{42.0});
    global->put(mjs::string{"alert"}, mjs::value{make_function(
        [](const std::vector<mjs::value>& args) {
        std::wcout << "ALERT";
        if (!args.empty()) std::wcout << ": " << args[0];
        std::wcout << "\n";
        return mjs::value::undefined; 
    })});
    return global;
}

bool test_expression(const std::wstring_view& text, const mjs::value& expected) {
    auto ss = mjs::parse(text);
    assert(ss.size() == 1);
    auto global = make_global();
    eval_visitor ev{global};
    auto res = accept(*ss[0], ev);
    if (res != expected) {
        std::wcout << "Test failed: " << text << " expecting " << expected << " got " << res << "\n";
        return false;
    }
    return true;
}

int main() {
    try {
        assert(test_expression(L"'test ' + 2 * (6 - 4 + 1) + ' ' + x", mjs::value{mjs::string{"test 6 42"}}));
        auto ss = mjs::parse(L"alert()");
        auto global = make_global();
        eval_visitor e{global};
        print_visitor p;
        for (const auto& s: ss) {
            std::wcout << "> ";
            accept(*s, p);
            std::wcout << "\n";
            std::wcout << accept(*s, e) << "\n";
        }
    } catch (const std::exception& e) {
        std::wcout << e.what() << "\n";
        return 1;
    }
}