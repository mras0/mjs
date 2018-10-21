#include "parser.h"
#include <sstream>
#include <algorithm>

//#define PARSER_DEBUG

#ifdef PARSER_DEBUG
#include <iostream>
#endif

#define UNHANDLED() unhandled(__FUNCTION__, __LINE__)
#define EXPECT(tt) expect(tt, __FUNCTION__, __LINE__)

namespace mjs {

source_position calc_source_position(const std::wstring_view& t, uint32_t start_pos, uint32_t end_pos, const source_position& start) {
    assert(start_pos <= t.size() && end_pos <= t.size() && start_pos <= end_pos);
    int cr = start.line-1, lf = start.line-1;
    int column = start.column-1;
    constexpr int tabstop = 8;
    for (uint32_t i = start_pos; i < end_pos; ++i) {
        switch (t[i]) {
        case '\n': ++lf; column = 0; break;
        case '\r': ++cr; column = 0; break;
        case '\t': column += tabstop - (column % tabstop); break;
        default: ++column;
        }
    }
    return {1 + std::max(cr, lf), 1 + column};
}

std::pair<source_position, source_position> extend_to_positions(const std::wstring_view& t, uint32_t start_pos, uint32_t end_pos) {
    auto start = calc_source_position(t, 0, start_pos, {1,1});
    auto end = calc_source_position(t, start_pos, end_pos, start);
    if (start.line == end.line && start.column != end.column) {
        end.column--;
    }
    return {start, end};
}

int operator_precedence(token_type tt) {
    switch (tt) {
        // Don't handle dot here
    case token_type::multiply:
    case token_type::divide:
    case token_type::mod:
        return 5;
    case token_type::plus:
    case token_type::minus:
        return 6;
    case token_type::lshift:
    case token_type::rshift:
    case token_type::rshiftshift:
        return 7;
    case token_type::lt:
    case token_type::ltequal:
    case token_type::gt:
    case token_type::gtequal:
        return 8;
    case token_type::equalequal:
    case token_type::notequal:
        return 9;
    case token_type::and_:
        return 10;
    case token_type::xor_:
        return 11;
    case token_type::or_:
        return 12;
    case token_type::andand:
        return 13;
    case token_type::oror:
        return 13;
    case token_type::question:
    case token_type::equal:
    case token_type::plusequal:
    case token_type::minusequal:
    case token_type::multiplyequal:
    case token_type::divideequal:
    case token_type::modequal:
    case token_type::lshiftequal:
    case token_type::rshiftequal:
    case token_type::rshiftshiftequal:
    case token_type::andequal:
    case token_type::orequal:
    case token_type::xorequal:
        return assignment_precedence;
    case token_type::comma:
        return comma_precedence;
    default:
        return comma_precedence + 1;
    }
}

bool is_right_to_left(token_type tt) {
    return operator_precedence(tt) >= assignment_precedence; // HACK
}

class parser {
public:
    explicit parser(const std::shared_ptr<source_file>& source) : source_(source), lexer_(source_->text) {}

    std::unique_ptr<block_statement> parse() {
        statement_list l;
        while (lexer_.current_token()) {
            try { 
                l.push_back(parse_statement_or_function_declaration());
            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << source_extend{source_, token_start_, lexer_.text_position()} << ": " << e.what();
                throw std::runtime_error(oss.str());
            }
        }
#ifdef PARSER_DEBUG
        std::wcout << "\n\n";
#endif
        return std::make_unique<block_statement>(source_extend{source_, 0, lexer_.text_position()}, std::move(l));
    }

private:
    std::shared_ptr<source_file> source_;
    lexer lexer_;
    uint32_t token_start_ = 0;
    uint32_t expression_start_ = 0;
    uint32_t statement_start_ = 0;
    bool line_break_skipped_ = false;

    template<typename T, typename... Args>
    expression_ptr make_expression(Args&&... args) {
        const auto tp = lexer_.text_position();
        auto e = expression_ptr{new T{source_extend{ source_, expression_start_, tp }, std::forward<Args>(args)...}};
        expression_start_ = tp;
#ifdef PARSER_DEBUG
        std::wcout << e->extend() << " Producting: " << *e << "\n";
#endif
        return e;
    }

    template<typename T, typename... Args>
    statement_ptr make_statement(Args&&... args) {
        const auto tp = lexer_.text_position();
        auto s = statement_ptr{new T{source_extend{ source_, statement_start_, tp }, std::forward<Args>(args)...}};
        statement_start_ = tp;
#ifdef PARSER_DEBUG
        std::wcout << s->extend() << " Producting: " << *s << "\n";
#endif
        return s;
    }

    token_type current_token_type() const {
        return lexer_.current_token().type();
    }

    void skip_whitespace() {
        for (;; lexer_.next_token()) {
            if (current_token_type() == token_type::whitespace) {
            } else if (current_token_type() == token_type::line_terminator) {
                line_break_skipped_ = true;
            } else {
                break;
            }
#ifdef PARSER_DEBUG
            std::wcout << source_extend{source_, token_start_, lexer_.text_position()} << " Consuming token: " << lexer_.current_token() << "\n";
#endif
        }
    }

    token get_token() {
        const auto old_end = lexer_.text_position();
        auto t = lexer_.current_token();
        lexer_.next_token();
        line_break_skipped_ = false;
        skip_whitespace();
#ifdef PARSER_DEBUG
        std::wcout << source_extend{source_, token_start_, old_end} << " Consuming token: " << t << "\n";
#endif

        token_start_ = old_end;
        return t;
    }

    token accept(token_type tt) {
        if (current_token_type() == tt) {
            return get_token();
        }
        return eof_token;
    }

    token expect(token_type tt, const char* func, int line) {
        auto t = accept(tt);
        if (!t) {
            std::ostringstream oss;
            oss << "Expected " << tt << " in " << func << " line " << line << " got " << lexer_.current_token();
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
        } else if (accept(token_type::this_)) {
            return make_expression<identifier_expression>(mjs::string{"this"});
        } else if (accept(token_type::lparen)) {
            auto e = parse_expression();
            EXPECT(token_type::rparen);
            return e;
        } else if (is_literal(current_token_type())) {
            return make_expression<literal_expression>(get_token());
        }
        UNHANDLED();
    }

    expression_ptr parse_postfix_expression() {
        auto lhs = parse_left_hand_side_expression();
        // TODO: no line break before
#ifndef  NDEBUG
        const bool was_line_break_skipped = line_break_skipped_;
#endif // ! NDEBUG
        if (auto t = current_token_type(); accept(token_type::plusplus) || accept(token_type::minusminus)) {
            assert(!was_line_break_skipped);
            return make_expression<postfix_expression>(t, std::move(lhs));
        }
        return lhs;        
    }

    expression_ptr parse_unary_expression() {
        switch (auto t = current_token_type(); t) {
        case token_type::delete_:
        case token_type::void_:
        case token_type::typeof_:
        case token_type::plusplus:
        case token_type::minusminus:
        case token_type::plus:
        case token_type::minus:
        case token_type::tilde:
        case token_type::not_:
            accept(t);
            return make_expression<prefix_expression>(t, parse_unary_expression());
        default:
            return parse_postfix_expression();
        }
    }

    expression_ptr parse_expression1(expression_ptr&& lhs, int outer_precedence) {
        for (;;) {
            const auto op = current_token_type();
            const auto precedence = operator_precedence(op);
            if (precedence > outer_precedence) {
                break;
            }
            get_token();
            if (op == token_type::question) {
                auto l = parse_assignment_expression();
                EXPECT(token_type::colon);
                lhs = make_expression<conditional_expression>(std::move(lhs), std::move(l), parse_assignment_expression());
                continue;
            }
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
        return parse_expression1(parse_unary_expression(), assignment_precedence);
    }

    expression_ptr parse_expression() {
        return parse_expression1(parse_assignment_expression(), comma_precedence);
    }

    expression_list parse_argument_list() {
        EXPECT(token_type::lparen);
        expression_list l;
        if (!accept(token_type::rparen)) {
            do {
                l.push_back(parse_assignment_expression());
            } while (accept(token_type::comma));
            EXPECT(token_type::rparen);
        }
        return l;
    }

    expression_ptr parse_member_expression() {
        // MemberExpression :
        //  PrimaryExpression
        //  MemberExpression [ Expression ]
        //  MemberExpression . Identifier
        //  new MemberExpression Arguments

        expression_ptr me{};
        if (accept(token_type::new_)) {
            auto e = parse_member_expression();
            if (current_token_type() == token_type::lparen) {
                e = make_expression<call_expression>(std::move(e), parse_argument_list());
            }
            me = make_expression<prefix_expression>(token_type::new_, std::move(e));
        } else {
            me = parse_primary_expression();
        }
        for (;;) {
            if (accept(token_type::lbracket)) {
                auto e = parse_expression();
                EXPECT(token_type::rbracket);
                me = make_expression<binary_expression>(token_type::lbracket, std::move(me), std::move(e));
            } else if (accept(token_type::dot)) {
                me = make_expression<binary_expression>(token_type::dot, std::move(me), make_expression<literal_expression>(token{token_type::string_literal, EXPECT(token_type::identifier).text()}));
            } else {
                return me;
            }
        }
    }

    expression_ptr parse_left_hand_side_expression() {
        // LeftHandSideExpression :
        //  NewExpression
        //    MemberExpression
        //    new NewExpression
        //  CallExpression
        //    MemberExpression Arguments
        //    CallExpression Arguments
        //    CallExpression [ Expression ]
        //    CallExpression . Identifier


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

        // MemberExpression:
        //   PrimaryExpression
        //   MemberExpression [ Expression ]
        //   MemberExpression .  Identifier
        //   new MemberExpression Arguments

        auto m = parse_member_expression();
        for (;;) {
            if (current_token_type() == token_type::lparen) {
                m = make_expression<call_expression>(std::move(m), parse_argument_list());
            } else if (accept(token_type::lbracket)) {
                auto e = parse_expression();
                EXPECT(token_type::rbracket);
                m = make_expression<binary_expression>(token_type::lbracket, std::move(m), std::move(e));
            } else if (accept(token_type::dot)) {
                m = make_expression<binary_expression>(token_type::dot, std::move(m), make_expression<literal_expression>(token{token_type::string_literal, EXPECT(token_type::identifier).text()}));
            } else {
                return m;
            }
        }
    }

    statement_ptr parse_block() {
        EXPECT(token_type::lbrace);
        statement_list l;
        while (!accept(token_type::rbrace)) {
            l.push_back(parse_statement_or_function_declaration());
        }
        return make_statement<block_statement>(std::move(l));
    }

    declaration::list parse_variable_declaration_list() {
        declaration::list l;
        do {
            auto id = EXPECT(token_type::identifier).text();
            expression_ptr init{};
            if (accept(token_type::equal)) {
                init = parse_assignment_expression();
            }
            l.push_back(declaration{id, std::move(init)});
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
        } else if (current_token_type() == token_type::semicolon) {
            return make_statement<empty_statement>();
        } else if (accept(token_type::if_)) {
            EXPECT(token_type::lparen);
            auto cond = parse_expression();
            EXPECT(token_type::rparen);
            auto if_s = parse_statement();
            accept(token_type::semicolon);
            auto else_s = accept(token_type::else_) ? parse_statement() : statement_ptr{};
            return make_statement<if_statement>(std::move(cond), std::move(if_s), std::move(else_s));
        } else if (accept(token_type::while_)) {
            EXPECT(token_type::lparen);
            auto cond = parse_expression();
            EXPECT(token_type::rparen);
            return make_statement<while_statement>(std::move(cond), parse_statement());
        } else if (accept(token_type::for_)) {
            statement_ptr init{};
            expression_ptr cond{}, iter{};
            EXPECT(token_type::lparen);
            if (!accept(token_type::semicolon)) {
                if (accept(token_type::var_)) {
                    init = make_statement<variable_statement>(parse_variable_declaration_list());
                } else {
                    init = make_statement<expression_statement>(parse_expression());
                }
                EXPECT(token_type::semicolon);
            }
            if (!accept(token_type::semicolon)) {
                cond = parse_expression();
                EXPECT(token_type::semicolon);
            }
            if (!accept(token_type::rparen)) {
                iter = parse_expression();
                EXPECT(token_type::rparen);
            }
            return make_statement<for_statement>(std::move(init), std::move(cond), std::move(iter), parse_statement());
        } else if (accept(token_type::continue_)) {
            return make_statement<continue_statement>();
        } else if (accept(token_type::break_)) {
            return make_statement<break_statement>();
        } else if (accept(token_type::return_)) {
            assert(!line_break_skipped_);
            // TODO: no line break before
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
        EXPECT(token_type::function_);
        auto id = EXPECT(token_type::identifier).text();
        EXPECT(token_type::lparen);
        std::vector<string> params;
        if (!accept(token_type::rparen)) {
            do {
                params.push_back(EXPECT(token_type::identifier).text());
            } while (accept(token_type::comma));
            EXPECT(token_type::rparen);
        }
        return make_statement<function_definition>(id, std::move(params), parse_block());
    }

    statement_ptr parse_statement_or_function_declaration() {
        skip_whitespace();
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

std::unique_ptr<block_statement> parse(const std::shared_ptr<source_file>& source) {
    return parser{source}.parse();
}

} // namespace mjs
