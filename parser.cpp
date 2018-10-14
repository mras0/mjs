#include "parser.h"
#include <sstream>

#define UNHANDLED() unhandled(__FUNCTION__, __LINE__)

namespace mjs {

constexpr int assignment_precedence = 15;
constexpr int comma_precedence      = 16;

int operator_precedence(token_type tt) {
    switch (tt) {
    case token_type::multiply:
    case token_type::divide:
        return 5;
    case token_type::plus:
    case token_type::minus:
        return 6;
    case token_type::equal:
        return assignment_precedence;
    default:
        return comma_precedence + 1;
    }
}

bool is_right_to_left(token_type tt) {
    return operator_precedence(tt) >= assignment_precedence; // HACK
}

template<typename T, typename... Args>
expression_ptr make_expression(Args&&... args) {
    auto e = expression_ptr{new T{std::forward<Args>(args)...}};
    return e;
}

template<typename T, typename... Args>
statement_ptr make_statement(Args&&... args) {
    auto e = statement_ptr{new T{std::forward<Args>(args)...}};
    return e;
}

class parser {
public:
    explicit parser(const std::wstring_view& str) : lexer_(str) {}

    statement_ptr parse() {
        statement_list l;
        skip_whitespace();
        while (lexer_.current_token()) {
            l.push_back(parse_statement_or_function_declaration());
        }
        return make_statement<block_statement>(std::move(l));
    }

private:
    lexer lexer_;

    token_type current_token_type() const {
        return lexer_.current_token().type();
    }

    void skip_whitespace() {
        while (current_token_type() == token_type::whitespace) {
            lexer_.next_token();
        }
    }

    token get_token() {
        auto t = lexer_.current_token();
        lexer_.next_token();
        skip_whitespace();
        return t;
    }

    token accept(token_type tt) {
        if (current_token_type() == tt) {
            return get_token();
        }
        return eof_token;
    }

    token expect(token_type tt, const char* func) {
        auto t = accept(tt);
        if (!t) {
            std::ostringstream oss;
            oss << "Expected " << tt << " in " << func << " got " << lexer_.current_token();
            throw std::runtime_error(oss.str());
        }
        return t;
    }

    expression_ptr parse_primary_expression() {
        // PrimaryExpression :
        //  this
        //  Identifier
        //  Literal
        //  ( Expression )
        if (auto id = accept(token_type::identifier)) {
            return make_expression<identifier_expression>(id.text());
        } else if (accept(token_type::lparen)) {
            auto e = parse_expression();
            expect(token_type::rparen, __FUNCTION__);
            return e;
        } else if (is_literal(current_token_type())) {
            return make_expression<literal_expression>(get_token());
        }
        UNHANDLED();
    }

    expression_ptr parse_postfix_expression() {
        // TODO: ++/--
        return parse_left_hand_side_expression();
    }

    expression_ptr parse_unary_expression() {
        // TODO: Prefixes...
        return parse_postfix_expression();
    }

    expression_ptr parse_expression1(expression_ptr&& lhs, int outer_precedence) {
        for (;;) {
            const auto op = current_token_type();
            const auto precedence = operator_precedence(op);
            if (precedence > outer_precedence) {
                break;
            }
            // TODO: Handle '?'
            get_token();
            auto rhs = parse_unary_expression();
            for (;;) {
                const auto look_ahead = current_token_type();
                const auto look_ahead_precedence = operator_precedence(look_ahead);
                if (look_ahead_precedence > precedence || (look_ahead_precedence == precedence && !is_right_to_left(look_ahead))) {
                    break;
                }
                rhs = parse_expression1(std::move(rhs), look_ahead_precedence);
            }
            lhs = make_expression<binary_expression>(op, std::move(lhs), std::move(rhs));
        }
        return std::move(lhs);
    }

    expression_ptr parse_assignment_expression() {
        return parse_expression1(parse_left_hand_side_expression(), assignment_precedence);
    }

    expression_ptr parse_expression() {
        return parse_expression1(parse_left_hand_side_expression(), comma_precedence);
    }

    expression_ptr parse_member_expression() {
        // MemberExpression :
        //  PrimaryExpression
        //  MemberExpression [ Expression ]
        //  MemberExpression . Identifier
        //  new MemberExpression Arguments
        return parse_primary_expression();
    }

    expression_list parse_argument_list() {
        expect(token_type::lparen, __FUNCTION__);
        expression_list l;
        if (!accept(token_type::rparen)) {
            do {
                l.push_back(parse_assignment_expression());
            } while (accept(token_type::comma));
            expect(token_type::rparen, __FUNCTION__);
        }
        return l;
    }

    expression_ptr parse_left_hand_side_expression() {
        // LeftHandSideExpression :
        //  NewExpression
        //  CallExpression

        // NewExpression :
        //  MemberExpression
        //  new NewExpression

        // CallExpression :
        //  MemberExpression Arguments
        //  CallExpression Arguments
        //  CallExpression [ Expression ]
        //  CallExpression . Identifier

        auto m = parse_member_expression();
        if (current_token_type() == token_type::lparen) {
            return make_expression<call_expression>(std::move(m), parse_argument_list());
        } else {
            return std::move(m);
        }
    }

    statement_ptr parse_block() {
        expect(token_type::lbrace, __FUNCTION__);
        statement_list l;
        while (!accept(token_type::rbrace)) {
            l.push_back(parse_statement_or_function_declaration());
        }
        return make_statement<block_statement>(std::move(l));
    }

    variable_statement::declaration_list parse_variable_declaration_list() {
        variable_statement::declaration_list l;
        do {
            auto id = expect(token_type::identifier, __FUNCTION__).text();
            expression_ptr init{};
            if (accept(token_type::equal)) {
                init = parse_assignment_expression();
            }
            l.push_back(variable_statement::declaration{id, std::move(init)});
        } while (accept(token_type::comma));
        return l;
    }

    statement_ptr parse_statement() {
        // Statement :
        //  Block
        //  VariableStatement
        //  EmptyStatement
        //  ExpressionStatement
        //  IfStatement
        //  IterationStatement
        //  ContinueStatement
        //  BreakStatement
        //  ReturnStatement
        if (current_token_type() == token_type::lbrace) {
            return parse_block();
        } else if (accept(token_type::var_)) {
            return make_statement<variable_statement>(parse_variable_declaration_list());
        } else if (accept(token_type::return_)) {
            expression_ptr e{};
            if (current_token_type() != token_type::semicolon) {
                e = parse_expression();
            }
            return make_statement<return_statement>(std::move(e));
        } else {
            return make_statement<expression_statement>(parse_expression());
        }
    }

    statement_ptr parse_function() {
        // function Identifier ( FormalParameterListopt ) Block
        expect(token_type::function_, __FUNCTION__);
        auto id = expect(token_type::identifier, __FUNCTION__).text();
        expect(token_type::lparen, __FUNCTION__);
        std::vector<string> params;
        if (!accept(token_type::rparen)) {
            do {
                params.push_back(expect(token_type::identifier, __FUNCTION__).text());
            } while (accept(token_type::comma));
            expect(token_type::rparen, __FUNCTION__);
        }
        return make_statement<function_definition>(id, std::move(params), parse_block());
    }

    statement_ptr parse_statement_or_function_declaration() {
        auto s = current_token_type() == token_type::function_ ? parse_function() : parse_statement();
        accept(token_type::semicolon);
        return s;
    }

    [[noreturn]] void unhandled(const char* function, int line) {
        std::ostringstream oss;
        oss << "Unhandled token in " << function  << " line " << line << " " << lexer_.current_token();
        throw std::runtime_error(oss.str());
    }
};

statement_ptr parse(const std::wstring_view& str) {
    return parser{str}.parse();
}

} // namespace mjs
