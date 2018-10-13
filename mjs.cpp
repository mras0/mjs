#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cassert>

#include "value.h"
#include "parser.h"

#define NOT_IMPLEMENTED(o) do { std::wostringstream woss; woss << "Not implemented: " << o << " in " << __FUNCTION__ << " " << __FILE__ << ":" << __LINE__; auto s_ = woss.str(); throw std::runtime_error(std::string(s_.begin(), s_.end())); } while (0) 

struct print_visitor {
public:
    void operator()(const mjs::literal_expression& e) {
        assert(e.t().type() == mjs::token_type::numeric_literal);
        std::wcout << e.t().dvalue();
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

struct eval_visitor {
public:
     mjs::value operator()(const mjs::literal_expression& e) {
        assert(e.t().type() == mjs::token_type::numeric_literal);
        return mjs::value{e.t().dvalue()};
    }

    mjs::value operator()(const mjs::binary_expression& e) {
        auto l = get_value(accept(e.lhs(), *this));
        auto r = get_value(accept(e.rhs(), *this));
        if (e.op() == mjs::token_type::plus) {
            l = to_primitive(l);
            r = to_primitive(r);
            if (l.type() == mjs::value_type::string || r.type() == mjs::value_type::string) {
                NOT_IMPLEMENTED(e);
            }
            // Otherwise handle like the other operators
        }
        const auto ln = to_number(l);
        const auto rn = to_number(r);
        switch (e.op()) {
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
};

int main() {
    try {
        auto ss = mjs::parse(L"2 * (6- 4)");
        print_visitor p;
        eval_visitor e;
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