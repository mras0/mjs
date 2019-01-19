#include "printer.h"
#include "parser.h"
#include "value.h"
#include <sstream>

namespace mjs {

class print_visitor {
public:
    explicit print_visitor(std::wostream& os) : os_(os) {}

    //
    // Expressions
    //

    void operator()(const identifier_expression& e) {
        os_ << e.id();
    }

    void operator()(const literal_expression& e) {
        switch (e.t().type()) {
        case token_type::null_:            os_ << "null"; return;
        case token_type::true_:            os_ << "true"; return;
        case token_type::false_:           os_ << "false"; return;
        case token_type::numeric_literal:  os_ << e.t().dvalue(); return;
        case token_type::string_literal:   os_ << '"' << cpp_quote(e.t().text()) << '"'; return;
        default:
            NOT_IMPLEMENTED(e);
        }
    }

    void operator()(const array_literal_expression& e) {
        os_ << '[';
        const auto& es = e.elements();
        for (size_t i = 0; i < es.size(); ++i) {
            os_ << (i ? ", " : "");
            if (es[i]) {
                accept(*es[i], *this);
            }
        }
        os_ << ']';
    }

    void operator()(const object_literal_expression& e) {
        os_ << '{';
        const auto& es = e.elements();
        for (size_t i = 0; i < es.size(); ++i) {
            os_ << (i ? ", " : "");
            if (es[i].type() == property_assignment_type::normal) {
                accept(es[i].name(), *this);
                os_ << ": ";
                accept(es[i].value(), *this);
            } else {
                assert(!"Not implemented: get/set for object literal");
            }
        }
        os_ << '}';
    }

    void operator()(const regexp_literal_expression& e) {
        os_ << '/' << e.re().pattern() << '/' << e.re().flags();
    }

    void operator()(const call_expression& e) {
        accept(e.member(), *this);
        os_ << '(';
        const auto& args = e.arguments();
        for (size_t i = 0; i < args.size(); ++i) {
            os_ << (i ? ", " : "");
            accept(*args[i], *this);
        }
        os_ << ')';
    }

    void operator()(const prefix_expression& e) {
        switch (e.op()) {
        case token_type::delete_: os_ << "delete "; break;
        case token_type::typeof_: os_ << "typeof "; break;
        case token_type::void_:   os_ << "void "; break;
        case token_type::new_:    os_ << "new "; break;
        default:                  os_ << op_text(e.op());
        }
        accept(e.e(), *this);
    }

    void operator()(const postfix_expression& e) {
        accept(e.e(), *this);
        os_ << op_text(e.op());
    }

    void operator()(const binary_expression& e) {
        const int precedence = operator_precedence(e.op());
        print_with_parentheses(e.lhs(), precedence);
        if (e.op() == token_type::lbracket) {
            os_ << '[';
            accept(e.rhs(), *this);
            os_ << ']';
        } else if (e.op() == token_type::dot) {
            os_ << '.';
            if (e.rhs().type() == expression_type::literal) {
                if (const auto& t = static_cast<const literal_expression&>(e.rhs()).t(); t.type() == token_type::string_literal) {
                    os_ << t.text();
                    return;
                }
            }
            accept(e.rhs(), *this);
        } else {
            os_ << op_text(e.op());
            print_with_parentheses(e.rhs(), precedence);
        }
    }

    void operator()(const conditional_expression& e) {
        accept(e.cond(), *this);
        os_ << " ? ";
        accept(e.lhs(), *this);
        os_ << " : ";
        accept(e.rhs(), *this);
    }

    void operator()(const function_expression& e) {
        handle_function(e);
    }

    void operator()(const expression& e) {
        NOT_IMPLEMENTED(e);
    }

    //
    // Statements
    //

    void operator()(const block_statement& s) {
        os_ << '{';
        for (const auto& bs: s.l()) {
            accept(*bs, *this);
        }
        os_ << '}';
    }

    void operator()(const variable_statement& s) {
        os_ << "var";
        for (size_t i = 0; i < s.l().size(); ++i) {
            const auto& d = s.l()[i];
            os_ << (i ? ", ": " ") << d.id();
            if (d.init()) {
                os_ << " = ";
                accept(*d.init(), *this);
            }
        }
        os_ << ';';
    }

    void operator()(const debugger_statement&) {
        os_ << "debugger;";
    }

    void operator()(const empty_statement&) {
        os_ << ';';
    }

    void operator()(const if_statement& s) {
        os_ << "if (";
        accept(s.cond(), *this);
        os_ << ") ";
        accept(s.if_s(), *this);
        if (auto e = s.else_s()) {
            os_ << " else ";
            accept(*e, *this);
        }
    }

    void operator()(const do_statement& s) {
        os_ << "do ";
        accept(s.s(), *this);
        os_ << " while (";
        accept(s.cond(), *this);
        os_ << ") ";
    }

    void operator()(const while_statement& s) {
        os_ << "while (";
        accept(s.cond(), *this);
        os_ << ") ";
        accept(s.s(), *this);
    }

    void operator()(const for_statement& s) {
        os_ << "for (";
        if (auto is = s.init()) {
            accept(*is, *this);
        } else {
            os_ << ";";
        }
        os_ << " ";
        if (auto cs = s.cond()) {
            accept(*cs, *this);
        }
        os_ << "; ";
        if (auto is = s.iter()) {
            accept(*is, *this);
        }
        os_ << ") ";
        accept(s.s(), *this);
    }

    void operator()(const for_in_statement& s){
        os_ << "for (";
        accept(s.init(), *this);
        os_ << " in ";
        accept(s.e(), *this);
        os_ << ") ";
        accept(s.s(), *this);
    }

    void operator()(const expression_statement& s) {
        accept(s.e(), *this);
        os_ << ';';
    }

    void operator()(const continue_statement&) {
        os_ << "continue;";
    }

    void operator()(const break_statement&) {
        os_ << "break;";
    }

    void operator()(const return_statement& s) {
        os_ << "return";
        if (s.e()) {
            os_ << " ";
            accept(*s.e(), *this);
        }
        os_ << ';';
    }

    void operator()(const with_statement& s) {
        os_ << "with (";
        accept(s.e(), *this);
        os_ << ") ";
        accept(s.s(), *this);
    }

    void operator()(const labelled_statement& s) {
        NOT_IMPLEMENTED(s);
    }

    void operator()(const switch_statement& s) {
        os_ << "switch (";
        accept(s.e(), *this);
        os_ << ") {";
        for (const auto& c: s.cl()) {
            os_ << " ";
            if (const auto& e = c.e()) {
                os_ << "case ";
                accept(*e, *this);
            } else {
                os_ << "default";
            }
            os_ << ": ";
            for (const auto& cs: c.sl()) {
                accept(*cs, *this);
            }
        }
        os_ << "}";
    }

    void operator()(const throw_statement& s) {
        os_ << "throw ";
        accept(s.e(), *this);
        os_ << ";";
    }

    void operator()(const try_statement& s) {
        os_ << "try ";
        accept(s.block(), *this);
        if (auto c = s.catch_block()) {
            os_ << " catch(" << s.catch_id() << ") ";
            accept(*c, *this);
        }
        if (auto f = s.finally_block()) {
            os_ << " finally ";
            accept(*f, *this);
        }
    }

    void operator()(const function_definition& s) {
        handle_function(s);
    }

    void operator()(const statement& s) {
        NOT_IMPLEMENTED(s);
    }

private:
    std::wostream& os_;

    void handle_function(const function_base& s) {
        os_ << "function" << (s.id().empty() ? L"" : L" " + s.id()) << "(";
        for (size_t i = 0; i < s.params().size(); ++i) {
            os_ << (i?", ":"") << s.params()[i];
        }
        os_ << ")";
        (*this)(s.block());
    }

    void print_with_parentheses(const expression& e, int outer_precedence) {
        const int inner_precedence = e.type() == expression_type::binary ? operator_precedence(static_cast<const binary_expression&>(e).op()) : 0;
        if (inner_precedence > outer_precedence) os_ << '(';
        accept(e, *this);
        if (inner_precedence > outer_precedence) os_ << ')';
    }
};

void print(std::wostream& os, const expression& e) {
    print_visitor pv{os};
    accept(e, pv);
}

void print(std::wostream& os, const statement& s) {
    print_visitor pv{os};
    accept(s, pv);
}

} // namespace mjs
