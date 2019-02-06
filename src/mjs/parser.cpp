#include "parser.h"
#include "number_to_string.h"
#include "char_conversions.h"
#include <sstream>
#include <algorithm>

//#define PARSER_DEBUG

#ifdef PARSER_DEBUG
#include <iostream>
#endif

#define UNHANDLED() unhandled(__FUNCTION__, __LINE__)
#define EXPECT(tt) expect(tt, __FUNCTION__, __LINE__)
#define EXPECT_SEMICOLON_ALLOW_INSERTION() expect_semicolon_allow_insertion(__FUNCTION__, __LINE__)
#define SYNTAX_ERROR_AT(expr, pos) do { std::wostringstream _oss; _oss << expr; syntax_error(__FUNCTION__, __LINE__, pos, _oss.str()); } while (0)
#define SYNTAX_ERROR(expr) SYNTAX_ERROR_AT(expr, current_extend())

namespace mjs {

constexpr token_type es1_reserved_tokens[] = { token_type::case_, token_type::catch_, token_type::class_, token_type::const_, token_type::debugger_, token_type::default_, token_type::do_, token_type::enum_, token_type::export_, token_type::extends_, token_type::finally_, token_type::import_, token_type::super_, token_type::switch_, token_type::throw_, token_type::try_ };
constexpr token_type es3_reserved_tokens[] = { token_type::abstract_, token_type::boolean_, token_type::byte_, token_type::char_, token_type::class_, token_type::const_, token_type::debugger_, token_type::double_, token_type::enum_, token_type::export_, token_type::extends_, token_type::final_, token_type::float_, token_type::goto_, token_type::implements_, token_type::import_, token_type::int_, token_type::interface_, token_type::long_, token_type::native_, token_type::package_, token_type::private_, token_type::protected_, token_type::public_, token_type::short_, token_type::static_, token_type::super_, token_type::synchronized_, token_type::throws_, token_type::transient_, token_type::volatile_ };
constexpr token_type es5_reserved_tokens[] = { token_type::class_, token_type::const_, token_type::enum_, token_type::export_, token_type::extends_, token_type::import_, token_type::super_ };
constexpr token_type es5_strict_reserved_tokens[] = { token_type::implements_, token_type::interface_, token_type::let_, token_type::package_, token_type::private_, token_type::protected_, token_type::public_, token_type::static_, token_type::yield_ };

template<size_t Size>
constexpr bool find_token(token_type t, const token_type (&a)[Size]) {
    return std::find(a, a+Size, t) != a+Size;
}

constexpr bool is_reserved(token_type t, version v) {
    return v == version::es1 ? find_token(t, es1_reserved_tokens) :
           v == version::es3 ? find_token(t, es3_reserved_tokens) :
           v == version::es5 ? find_token(t, es5_reserved_tokens) :
           throw std::logic_error{"Invalid version"};
}

std::wstring token_string(token_type t) {
    assert(!is_literal(t));
    std::wostringstream woss;
    woss << t;
    return woss.str();
}

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
    case token_type::in_:
    case token_type::instanceof_:
        return 8;
    case token_type::equalequal:
    case token_type::equalequalequal:
    case token_type::notequal:
    case token_type::notequalequal:
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

function_base::function_base(const source_extend& body_extend, const std::wstring& id, std::vector<std::wstring>&& params, statement_ptr&& block) : body_extend_(body_extend), id_(id), params_(std::move(params)) {
    assert(block && block->type() == statement_type::block);
    block_.reset(static_cast<block_statement*>(block.release()));
}

bool function_base::strict_mode() const {
    return block_->strict_mode();
}

void function_base::base_print(std::wostream& os) const {
    os << "{";
    if (!id_.empty()) os << id_ << ", ";
    os << "[";
    for (size_t i = 0; i < params_.size(); ++i) {
        if (i) os << ", ";
        os << params_[i];
    }
    os << "], " << *block_ << "}";
}

std::wstring property_name_string(const expression& e) {
    assert(is_valid_property_name_expression(e));
    if (e.type() == expression_type::identifier) {
        return static_cast<const identifier_expression&>(e).id();
    }
    const auto& lt = static_cast<const literal_expression&>(e).t();
    if (lt.type() == token_type::string_literal) {
        return lt.text();
    } else {
        assert(lt.type() == token_type::numeric_literal);
        return number_to_string(lt.dvalue());
    }
}

#define RECORD_EXPRESSION_START position_stack_node _expression_position##__LINE__{*this, expression_pos_}
#define RECORD_STATEMENT_START position_stack_node _statement_position##__LINE__{*this, statement_pos_}

// See ES5.1, 10.1.1 and 14.1. The directive can only occur as the first statement in a function/global scope/eval

const wchar_t strict_directive[] = L"use strict";

bool is_directive(const expression& e) {
    if (e.type() != expression_type::literal) {
        return false;
    }
    const auto& le = static_cast<const literal_expression&>(e);
    if (le.t().type() != token_type::string_literal) {
        return false;
    }
    return true;
}

bool is_directive(const statement& s) {
    return s.type() == statement_type::expression && is_directive(static_cast<const expression_statement&>(s).e());
}

bool is_strict_mode_directive(const expression& e) {
    if (!is_directive(e)) {
        return false;
    }
    const auto& le = static_cast<const literal_expression&>(e);
    // Check spelling of directive, "A Use Strict Directive may not contain an EscapeSequence or LineContinuation."
    if (le.extend().source_view().compare(1, sizeof(strict_directive)/sizeof(*strict_directive) -1, strict_directive) != 0) {
        return false;
    }
    assert(le.t().text() == strict_directive);
    return true;
}

constexpr bool is_octal_char(char16_t ch) { return ch >= '0' && ch <= '7'; }

bool is_octal_literal(const std::wstring_view s) {
    assert(s.length() > 0);
    return s.length() > 1 && s[0] == '0' && is_octal_char(s[1]);
}

bool has_octal_escape_sequence(const std::wstring_view s) {
    bool quote = false;
    for (const auto& ch: s) {
        if (quote) {
            if (is_octal_char(ch)) {
                return true;
            }
            quote = false;
        } else if (ch == '\\') {
            quote = true;
        }
    }
    return false;
}

bool is_strict_mode_unassignable_identifier(const std::wstring_view name) {
    return name == L"eval" || name == L"arguments";
}

bool is_strict_mode_unassignable_identifier(const expression& e) {
    if (e.type() != expression_type::identifier) {
        return false;
    }
    return is_strict_mode_unassignable_identifier(static_cast<const identifier_expression&>(e).id());
}

class parser {
public:
    explicit parser(const std::shared_ptr<source_file>& source, parse_mode mode)
        : source_(source)
        , version_(source_->language_version())
        , lexer_(source_->text(), version_)
        , strict_mode_(mode != parse_mode::non_strict)
        , skip_strict_checks_for_first_function_(mode == parse_mode::function_constructor_in_strict_context) {
        check_token();
    }

    ~parser() {
        assert(!expression_pos_);
        assert(!statement_pos_);
    }

    std::unique_ptr<block_statement> parse() {

#ifdef PARSER_DEBUG
        std::wcout << "\nParsing '" << source_->text() << "'\n\n";
#endif

        statement_list l;

        try {
            skip_whitespace();
            l = parse_statement_list(version_ >= version::es5);
            EXPECT_SEMICOLON_ALLOW_INSERTION();
        } catch (const std::exception& e) {
            std::ostringstream oss;
            oss << source_extend{source_, token_start_, lexer_.text_position()} << ": " << e.what();
            throw std::runtime_error(oss.str());
        }

#ifdef PARSER_DEBUG
        std::wcout << "\n\n";
#endif
        return std::make_unique<block_statement>(source_extend{source_, 0, lexer_.text_position()}, std::move(l), strict_mode_);
    }

private:
    class position_stack_node {
    public:
        explicit position_stack_node(parser& parent, position_stack_node*& stack) : parent_(parent), prev_(stack), stack_(stack) {
            stack_ = this;
            pos_ = parent_.token_start_;
        }
        ~position_stack_node() {
            assert(stack_ == this);
            stack_ = prev_;
        }
        source_extend extend() const {
            return source_extend{parent_.source_, pos_, parent_.token_start_};
        }
    private:
        parser&                 parent_;
        position_stack_node*    prev_;
        position_stack_node*&   stack_;
        uint32_t                pos_;
    };

    class scoped_strict_mode {
    public:
        explicit scoped_strict_mode(parser& p) : parser_(p), strict_before_(p.strict_mode_) {
        }
        ~scoped_strict_mode() {
            parser_.strict_mode_ = strict_before_;
        }
    private:
        parser& parser_;
        bool    strict_before_;
    };

    std::shared_ptr<source_file> source_;
    const version version_;
    lexer lexer_;
    bool strict_mode_;
    bool skip_strict_checks_for_first_function_;
    uint32_t token_start_ = 0;
    position_stack_node* expression_pos_ = nullptr;
    position_stack_node* statement_pos_ = nullptr;
    bool line_break_skipped_ = false;
    bool supress_in_ = false;
    token current_token_{token_type::eof};

    template<typename T, typename... Args>
    expression_ptr make_expression(Args&&... args) {
        assert(expression_pos_);
        auto e = expression_ptr{new T{expression_pos_->extend(), std::forward<Args>(args)...}};
#ifdef PARSER_DEBUG
        std::wcout << e->extend() << " Producting: " << *e << "\n";
#endif
        return e;
    }

    template<typename T, typename... Args>
    statement_ptr make_statement(Args&&... args) {
        assert(statement_pos_);
        auto s = statement_ptr{new T{statement_pos_->extend(), std::forward<Args>(args)...}};
#ifdef PARSER_DEBUG
        std::wcout << s->extend() << " Producting: " << *s << "\n";
#endif
        return s;
    }

    const token& current_token() const {
        return current_token_;
    }

    token_type current_token_type() const {
        return current_token().type();
    }

    //
    // The need for this function is a bit of a hack. Before the parser gets to look
    // at a token, the token needs to be validated or transformed depending on the
    // language version and strict mode setting. This is handled here in coordination
    // with parse_statement() which makes sure strict mode is enabled at the right time.
    //
    void check_token() {
        current_token_ = lexer_.current_token();
        if (is_reserved(current_token_type(), version_)) {
            SYNTAX_ERROR(current_token_type() << " is reserved in " << version_);
        } else if (version_ >= version::es5 && find_token(current_token_type(), es5_strict_reserved_tokens)) {
            if (strict_mode_) {
                SYNTAX_ERROR(current_token_type() << " is reserved in " << version_ << " strict mode");
            }
            current_token_ = token{token_type::identifier, token_string(current_token_type())};
        }
    }

    void next_token() {
        lexer_.next_token();
        check_token();
    }

    void skip_whitespace() {
        for (;; next_token()) {
            if (current_token_type() == token_type::whitespace) {
#ifdef PARSER_DEBUG
                std::wcout << source_extend{source_, token_start_, lexer_.text_position()} << " Consuming token: " << current_token() << "\n";
#endif
            } else if (current_token_type() == token_type::line_terminator) {
                line_break_skipped_ = true;
            } else {
                break;
            }
            token_start_ = lexer_.text_position();
        }
    }

    token get_token() {
        const auto old_end = lexer_.text_position();
        auto t = current_token();
        next_token();
        line_break_skipped_ = false;
#ifdef PARSER_DEBUG
        std::wcout << source_extend{source_, token_start_, old_end} << " Consuming token: " << t << "\n";
#endif
        token_start_ = old_end;
        skip_whitespace();
        return t;
    }

    source_extend current_extend() const {
        return source_extend{source_, token_start_, lexer_.text_position()};
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
            oss << "Expected " << tt << " in " << func << " line " << line << " got " << current_token();
            throw std::runtime_error(oss.str());
        }
        return t;
    }

    void check_string_literal(const source_extend& extend) {
        if (has_octal_escape_sequence(extend.source_view())) {
            SYNTAX_ERROR_AT("Octal escape sequences may not be used in strict mode", extend);
        }
    }

    void check_literal() {
        const auto tt = current_token_type();
        assert(is_literal(tt));
        if (!strict_mode_) {
            return;
        }
        if (tt == token_type::numeric_literal && is_octal_literal(current_extend().source_view())) {
            SYNTAX_ERROR("Octal literals may not be used in strict mode");
        } else if (tt == token_type::string_literal) {
            check_string_literal(current_extend());
        }
    }

    void expect_semicolon_allow_insertion(const char* func, int line) {
        //
        // Automatic semi-colon insertion
        //
        if (!line_break_skipped_ && current_token_type() != token_type::rbrace && current_token_type() != token_type::eof) {
            expect(token_type::semicolon, func, line);
        } else {
            accept(token_type::semicolon);
        }
    }

    std::wstring get_identifier_name(const char* func, int line);
    expression_ptr parse_identifier_name(const char* func, int line);
    expression_ptr parse_property_name();
    property_name_and_value parse_property_name_and_value();

    expression_ptr parse_primary_expression() {
        // PrimaryExpression :
        //  this
        //  Identifier
        //  Literal
        //  ArrayLiteral
        //  ObjectLiteral
        //  ( Expression )
        if (auto id = accept(token_type::identifier)) {
            return make_expression<identifier_expression>(id.text());
        } else if (accept(token_type::this_)) {
            return make_expression<this_expression>();
        } else if (version_ >= version::es3 && accept(token_type::lbracket)) {
            // ArrayLiteral
            std::vector<expression_ptr> elements;
            bool last_was_assignment_expression = false;
            while (!accept(token_type::rbracket)) {
                if (accept(token_type::comma)) {
                    if (!last_was_assignment_expression) {
                        elements.push_back(nullptr);
                    }
                    last_was_assignment_expression = false;
                } else {
                    if (last_was_assignment_expression) {
                        EXPECT(token_type::comma);
                    }
                    elements.push_back(parse_assignment_expression());
                    last_was_assignment_expression = true;
                }
            }
            return make_expression<array_literal_expression>(std::move(elements));
        } else if (version_ >= version::es3 && accept(token_type::lbrace)) {
            // ObjectLiteral
            property_name_and_value_list elements;
            while (!accept(token_type::rbrace)) {
                if (!elements.empty()) {
                    EXPECT(token_type::comma);
                    if (version_ >= version::es5 && accept(token_type::rbrace)) {
                        // Trailing comma allowed in ES5+
                        break;
                    }
                }
                elements.push_back(parse_property_name_and_value());
                if (version_ >= version::es5) {
                    const auto& new_item = elements.back();
                    const auto new_item_str = new_item.name_str();
                    auto e = elements.end() - 1;
                    auto it = std::find_if(elements.begin(), e, [&](const property_name_and_value& v) {
                        if (v.name_str() != new_item_str) {
                            // Item doesn't match
                            return false;
                        }
                        // Repeated definitions are not allowed for data properties in strict mode
                        if (new_item.type() == property_assignment_type::normal && v.type() == property_assignment_type::normal) {
                            return strict_mode_;
                        }
                        // May not change a data property to an accessor property or vice versa
                        if (new_item.type() == property_assignment_type::normal && v.type() != property_assignment_type::normal) {
                            return true;
                        }
                        if (new_item.type() != property_assignment_type::normal && v.type() == property_assignment_type::normal) {
                            return true;
                        }
                        // May only define getter/setter once
                        assert(new_item.type() == property_assignment_type::get || new_item.type() == property_assignment_type::set);
                        assert(v.type() == property_assignment_type::get || v.type() == property_assignment_type::set);
                        return new_item.type() == v.type();
                    });
                    if (it != e) {
                        SYNTAX_ERROR_AT("Invalid redefinition of property: \"" << cpp_quote(new_item_str) << "\"", elements.back().name().extend());
                    }
                }
            }
            return make_expression<object_literal_expression>(std::move(elements));
        } else if (version_ >= version::es3 && (current_token_type() == token_type::divide || current_token_type() == token_type::divideequal)) {
            // RegularExpressionLiteral
            const auto lit = lexer_.get_regex_literal();
            check_token();
            skip_whitespace();
            assert(lit.size() >= 3);
            assert(lit[0] == '/');
            const auto lit_end = lit.find_last_of('/');
            assert(lit_end > 1 && lit_end != std::wstring_view::npos);
            return make_expression<regexp_literal_expression>(lit.substr(1, lit_end-1), lit.substr(lit_end+1));
        } else if (accept(token_type::lparen)) {
            auto e = parse_expression();
            EXPECT(token_type::rparen);
            return e;
        } else if (is_literal(current_token_type())) {
            check_literal();
            return make_expression<literal_expression>(get_token());
        }
        UNHANDLED();
    }

    expression_ptr parse_postfix_expression() {
        auto lhs = parse_left_hand_side_expression();
        // no line break before
        if (line_break_skipped_) {
            return lhs;
        }
        if (auto t = current_token_type(); accept(token_type::plusplus) || accept(token_type::minusminus)) {
            if (strict_mode_ && is_strict_mode_unassignable_identifier(*lhs)) {
                SYNTAX_ERROR("\"" << cpp_quote(static_cast<const identifier_expression&>(*lhs).id()) << "\" may not be modified in strict mode");
            }
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
            {
                accept(t);
                auto e = parse_unary_expression();
                if (strict_mode_) {
                    if (t == token_type::delete_ && e->type() == expression_type::identifier) {
                        SYNTAX_ERROR_AT("May not delete unqualified identifier \"" << cpp_quote(static_cast<const identifier_expression&>(*e).id()) << "\" in strict mode", e->extend());
                    }
                    if (is_strict_mode_unassignable_identifier(*e)) {
                        SYNTAX_ERROR_AT("\"" << cpp_quote(static_cast<const identifier_expression&>(*e).id()) << "\" may not be modified in strict mode", e->extend());
                    }
                }
                return make_expression<prefix_expression>(t, std::move(e));
            }
        default:
            return parse_postfix_expression();
        }
    }

    expression_ptr parse_expression1(expression_ptr&& lhs, int outer_precedence) {
        auto prec = [no_in=supress_in_ || version_ < version::es3](token_type t) { return !no_in || t != token_type::in_ ? operator_precedence(t) : 100; };

        for (;;) {
            const auto op = current_token_type();
            const auto precedence = prec(op);
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
            if (strict_mode_ && is_assignment_op(op) && is_strict_mode_unassignable_identifier(*lhs)) {
                SYNTAX_ERROR("\"" << cpp_quote(static_cast<const identifier_expression&>(*lhs).id()) << "\" may not be assigned in strict mode");
            }

            auto rhs = parse_unary_expression();
            for (;;) {
                const auto look_ahead = current_token_type();
                const auto look_ahead_precedence = prec(look_ahead);
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
        RECORD_EXPRESSION_START;
        return parse_expression1(parse_unary_expression(), assignment_precedence);
    }

    expression_ptr parse_expression() {
        RECORD_EXPRESSION_START;
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

    auto parse_function() {
        const auto body_start = lexer_.text_position() - 1;
        EXPECT(token_type::lparen);
        std::vector<std::wstring> params;
        std::vector<source_extend> param_extends;
        if (!accept(token_type::rparen)) {
            do {
                param_extends.push_back(current_extend());
                params.push_back(EXPECT(token_type::identifier).text());
            } while (accept(token_type::comma));
            EXPECT(token_type::rparen);
        }
        scoped_strict_mode ssm{*this};   // Make sure state is restored afterwards
        auto block = parse_block(version_ >= version::es5);
        const auto body_end = block->extend().end;

        assert(block->type() == statement_type::block);
        if (static_cast<const block_statement&>(*block).strict_mode() && !skip_strict_checks_for_first_function_) {
            for (size_t i = 0; i < params.size(); ++i) {
                auto n = params[i];
                if (is_strict_mode_unassignable_identifier(n)) {
                    SYNTAX_ERROR_AT("\"" << cpp_quote(n) << "\" may not be used as a parameter name in strict mode", param_extends[i]);
                }
                if (std::find(params.begin() + i + 1, params.end(), n) != params.end()) {
                    SYNTAX_ERROR_AT("Parameter names may not be repeated in strict mode", param_extends[i]);
                }
            }
        }
        skip_strict_checks_for_first_function_ = false;

        return std::make_tuple(source_extend{source_, body_start, body_end}, std::move(params), std::move(block));
    }

    void check_function_name(const std::wstring_view id, const source_extend& extend) {
        if (is_strict_mode_unassignable_identifier(id)) {
            SYNTAX_ERROR_AT("\"" << cpp_quote(id) << "\" may not be used as a function name in strict mode", extend);
        }
    }

    expression_ptr parse_member_expression() {
        // MemberExpression :
        //  PrimaryExpression
        //  FunctionExpression
        //  MemberExpression [ Expression ]
        //  MemberExpression . Identifier (IdentifierName in ES5+)
        //  new MemberExpression Arguments

        expression_ptr me{};
        if (accept(token_type::new_)) {
            auto e = parse_member_expression();
            if (current_token_type() == token_type::lparen) {
                e = make_expression<call_expression>(std::move(e), parse_argument_list());
            }
            me = make_expression<prefix_expression>(token_type::new_, std::move(e));
        } else if (version_ >= version::es3 && accept(token_type::function_)) {
            std::wstring id{};
            const auto id_extend = current_extend();
            if (auto id_token = accept(token_type::identifier)) {
                id = id_token.text();
            }
            auto [extend, params, block] = parse_function();
            assert(block->type() == statement_type::block);
            if (static_cast<const block_statement&>(*block).strict_mode()) {
                check_function_name(id, id_extend);
            }
            return make_expression<function_expression>(extend, id, std::move(params), std::move(block));
        } else {
            me = parse_primary_expression();
        }
        for (;;) {
            if (accept(token_type::lbracket)) {
                auto e = parse_expression();
                EXPECT(token_type::rbracket);
                me = make_expression<binary_expression>(token_type::lbracket, std::move(me), std::move(e));
            } else if (accept(token_type::dot)) {
                me = make_expression<binary_expression>(token_type::dot, std::move(me), make_expression<literal_expression>(token{token_type::string_literal, get_identifier_name(__func__, __LINE__)}));
            } else {
                return me;
            }
        }
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
        //  CallExpression . Identifier (IdentifierName in ES5+)

        // MemberExpression:
        //   PrimaryExpression
        //   MemberExpression [ Expression ]
        //   MemberExpression .  Identifier (IdentifierName in ES5+)
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
                m = make_expression<binary_expression>(token_type::dot, std::move(m), make_expression<literal_expression>(token{token_type::string_literal, get_identifier_name(__func__, __LINE__)}));
            } else {
                return m;
            }
        }
    }

    statement_list parse_statement_list(bool check_for_strict_mode) {
        statement_list l;
        while (current_token() && current_token_type() != token_type::rbrace) {
            const bool strict_mode_active_before = strict_mode_;
            l.push_back(parse_statement(check_for_strict_mode));
            if (!strict_mode_active_before && strict_mode_) {
                // HACK: Strict mode was activated. Now go back and check the string literals before for octal escape sequences.
                for (auto it = l.begin(); it != l.end() - 1; ++it) {
                    assert(is_directive(**it));
                    check_string_literal(static_cast<const expression_statement&>(**it).e().extend());
                }
            }

            // Check if we're still in the directive prologue (ES5.1, 14.1)
            // which is an unbroken sequence of expression statements consisting of just a string literal
            if (check_for_strict_mode && !is_directive(*l.back())) {
                check_for_strict_mode = false;
            }
        }
        return l;
    }

    statement_ptr parse_block(bool check_for_strict_mode = false) {
        EXPECT(token_type::lbrace);
        statement_list l = parse_statement_list(check_for_strict_mode);
        EXPECT(token_type::rbrace);
        return make_statement<block_statement>(std::move(l), strict_mode_);
    }

    declaration::list parse_variable_declaration_list() {
        declaration::list l;
        do {
            if (strict_mode_ && current_token_type() == token_type::identifier) {
                if (auto n = current_token().text(); is_strict_mode_unassignable_identifier(n)) {
                    // "args" and "arguments" may not be used as variable names
                    SYNTAX_ERROR("\"" << cpp_quote(n) << "\" may not appear in a variable declaration in strict mode");
                }
            }

            auto id = EXPECT(token_type::identifier).text();

            expression_ptr init{};
            if (accept(token_type::equal)) {
                RECORD_EXPRESSION_START;
                init = parse_assignment_expression();
            }
            l.push_back(declaration{id, std::move(init)});
        } while (accept(token_type::comma));
        return l;
    }

    std::wstring get_label() {
        // no line break before
        if (version_ >= version::es3 && !line_break_skipped_) {
            if (auto t = accept(token_type::identifier)) {
                return t.text();
            }
        }
        return L"";
    }

    statement_ptr parse_statement(bool check_for_strict_mode = false) {
        RECORD_STATEMENT_START;
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
        //  WithStatement
        //  LabelledStatement
        //  SwitchStatement
        //  ThrowStatement
        //  TryStatement

        if (current_token_type() == token_type::lbrace) {
            return parse_block();
        } else if (accept(token_type::function_)) {
            const auto id_extend = current_extend();
            auto id = EXPECT(token_type::identifier).text();
            auto [extend, params, block] = parse_function();
            assert(block->type() == statement_type::block);
            if (static_cast<const block_statement&>(*block).strict_mode()) {
                check_function_name(id, id_extend);
            }
            return make_statement<function_definition>(extend, id, std::move(params), std::move(block));
        } else if (accept(token_type::var_)) {
            auto dl = parse_variable_declaration_list();
            EXPECT_SEMICOLON_ALLOW_INSERTION();
            return make_statement<variable_statement>(std::move(dl));
        } else if (current_token_type() == token_type::semicolon) {
            get_token();
            return make_statement<empty_statement>();
        } else if (accept(token_type::if_)) {
            EXPECT(token_type::lparen);
            auto cond = parse_expression();
            EXPECT(token_type::rparen);
            auto if_s = parse_statement();
            accept(token_type::semicolon);
            auto else_s = accept(token_type::else_) ? parse_statement() : statement_ptr{};
            return make_statement<if_statement>(std::move(cond), std::move(if_s), std::move(else_s));
        } else if (/*version_ >= version::es3 && */accept(token_type::do_)) {
            auto s = parse_statement();
            EXPECT(token_type::while_);
            EXPECT(token_type::lparen);
            auto cond = parse_expression();
            EXPECT(token_type::rparen);
            EXPECT_SEMICOLON_ALLOW_INSERTION();
            return make_statement<do_statement>(std::move(cond), std::move(s));
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
                // HACK: to support "NoIn" grammar :(
                supress_in_ = true;
                if (accept(token_type::var_)) {
                    init = make_statement<variable_statement>(parse_variable_declaration_list());
                } else {
                    init = make_statement<expression_statement>(parse_expression());
                }
                supress_in_ = false;
                if (accept(token_type::in_)) {
                    if (init->type() == statement_type::variable && static_cast<variable_statement&>(*init).l().size() != 1) {
                        // Only a single variable assigment is legal
                        UNHANDLED();
                    }

                    auto e = parse_expression();
                    EXPECT(token_type::rparen);
                    return make_statement<for_in_statement>(std::move(init), std::move(e), parse_statement());
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
            auto id = get_label();
            EXPECT_SEMICOLON_ALLOW_INSERTION();
            return make_statement<continue_statement>(std::move(id));
        } else if (accept(token_type::break_)) {
            auto id = get_label();
            EXPECT_SEMICOLON_ALLOW_INSERTION();
            return make_statement<break_statement>(std::move(id));
        } else if (accept(token_type::return_)) {
            // no line break before
            expression_ptr e{};
            if (!line_break_skipped_ && current_token_type() != token_type::semicolon) {
                e = parse_expression();
            }
            EXPECT_SEMICOLON_ALLOW_INSERTION();
            return make_statement<return_statement>(std::move(e));
        } else if (current_token_type() == token_type::with_) {
            if (strict_mode_) {
                SYNTAX_ERROR("Strict mode code may not include a WithStatement");
            }
            get_token(); // Consume now that any syntax errors have been reported
            EXPECT(token_type::lparen);
            auto e = parse_expression();
            EXPECT(token_type::rparen);
            return make_statement<with_statement>(std::move(e), parse_statement());
        } else if (/*version_ >= version::es3 && */accept(token_type::switch_)) {
            EXPECT(token_type::lparen);
            auto switch_e = parse_expression();
            EXPECT(token_type::rparen);
            EXPECT(token_type::lbrace);
            clause_list cl;
            bool has_default = false;
            while (!accept(token_type::rbrace)) {
                // CaseClause
                expression_ptr e{};
                if (!accept(token_type::default_)) {
                    EXPECT(token_type::case_);
                    e = parse_expression();
                } else {
                    if (has_default) {
                        // TODO: Better error message when a switch has multiple default clauses
                        UNHANDLED();
                    }
                    has_default = true;
                }
                EXPECT(token_type::colon);
                statement_list sl;
                for (;;) {
                    const auto tt = current_token_type();
                    if (tt == token_type::rbrace
                        || tt == token_type::case_
                        || tt ==  token_type::default_
                        || tt == token_type::eof) {
                        break;
                    }
                    sl.emplace_back(parse_statement());
                }
                cl.push_back(case_clause{std::move(e), std::move(sl)});
            }
            return make_statement<switch_statement>(std::move(switch_e), std::move(cl));
        } else if (/*version_ >= version::es3 && */accept(token_type::throw_)) {
            // no line break before
            if (line_break_skipped_) {
                // TODO: Give better error message
                UNHANDLED();
            }
            auto e = parse_expression();
            EXPECT_SEMICOLON_ALLOW_INSERTION();
            return make_statement<throw_statement>(std::move(e));
        } else if (/*version_ >= version::es3 && */accept(token_type::try_)) {
            auto block = parse_block();
            statement_ptr catch_{}, finally_{};
            std::wstring catch_id;
            if (accept(token_type::catch_)) {
                EXPECT(token_type::lparen);
                if (strict_mode_ && current_token_type() == token_type::identifier) {
                    auto n = current_token().text();
                    if (is_strict_mode_unassignable_identifier(n)) {
                        SYNTAX_ERROR("\"" << cpp_quote(n) << "\" may not be used as an identifier in strict mode");
                    }
                }
                catch_id = EXPECT(token_type::identifier).text();
                EXPECT(token_type::rparen);
                catch_ = parse_block();
            }
            if (accept(token_type::finally_)) {
                finally_ = parse_block();
            }
            return make_statement<try_statement>(std::move(block), std::move(catch_), catch_id, std::move(finally_));
        } else if (/*version_ >= version::es5 && */accept(token_type::debugger_)) {
            auto s = make_statement<debugger_statement>();;
            EXPECT_SEMICOLON_ALLOW_INSERTION();
            return s;
        } else {
            auto e = parse_expression();
            if (version_ >= version::es3 && accept(token_type::colon)) {
                if (e->type() != expression_type::identifier) {
                    // TODO: Better error message when encountering an invalid label (or other invalid construct)
                    UNHANDLED();
                }
                return make_statement<labelled_statement>(static_cast<const identifier_expression&>(*e).id(), parse_statement());
            }
            if (check_for_strict_mode && is_strict_mode_directive(*e)) {
                strict_mode_ = true;
#ifdef PARSER_DEBUG
                std::wcout << e->extend() << ": Strict mode enabled\n";
#endif
            }
            EXPECT_SEMICOLON_ALLOW_INSERTION();
            return make_statement<expression_statement>(std::move(e));
        }
    }

    [[noreturn]] void unhandled(const char* function, int line) {
        std::ostringstream oss;
        oss << "Unhandled token in " << function  << " line " << line << " " << current_token();
        throw std::runtime_error(oss.str());
    }

    [[noreturn]] static void syntax_error(const char* function, int line, const source_extend& extend, const std::wstring_view message) {
        std::wostringstream oss;
        oss << "Syntax error in " << function  << " line " << line << " at \"" << cpp_quote(extend.source_view()) << "\": " << message;
        throw std::runtime_error(unicode::utf16_to_utf8(oss.str()));
    }
};

std::wstring parser::get_identifier_name(const char* func, int line) {
    if (auto id = accept(token_type::identifier)) {
        return id.text();
    } else if (version_ >= version::es5) {
        // In ES5+ IdentifierName is used in various grammars
        // This means reserved words can be used as identifiers in certain contexts (e.g. Left-Hand-Side and Object Initialiser Expressions)
        switch (const auto t = current_token_type()) {
#define CASE_KEYWORD(tt, ...) case token_type::tt##_:
            MJS_KEYWORDS(CASE_KEYWORD);
#undef CASE_KEYWORD
            {
                get_token(); // Advance to next token
                return token_string(t);
            }
        default:
            break;
        }
    }

    std::ostringstream oss;
    oss << "Expected identifier name in " << func << " line " << line << " got " << current_token();
    throw std::runtime_error(oss.str());
}

expression_ptr parser::parse_identifier_name(const char* func, int line) {
    return make_expression<identifier_expression>(get_identifier_name(func, line));
}

expression_ptr parser::parse_property_name() {
    RECORD_EXPRESSION_START;
    if (current_token_type() == token_type::string_literal || current_token_type() == token_type::numeric_literal) {
        check_literal();
        return make_expression<literal_expression>(get_token());
    } else {
        auto p = parse_identifier_name(__func__, __LINE__);
        assert(p && p->type() == expression_type::identifier);
        return p;
    }
}

property_name_and_value parser::parse_property_name_and_value() {
    expression_ptr p;
    {
        p = parse_property_name();
        // get/set i.e. accessor properties
        if (version_ >= version::es5 && current_token_type() != token_type::colon && p->type() == expression_type::identifier) {
            const auto& p_id = static_cast<const identifier_expression&>(*p).id();
            const bool is_get = p_id == L"get";
            if (is_get || p_id == L"set") {
                auto new_p = parse_property_name();
                const auto id = p_id + L" " + property_name_string(*new_p);
                auto [extend, params, block] = parse_function();
                const size_t expected_args = is_get ? 0 : 1;
                if (expected_args != params.size()) {
                    SYNTAX_ERROR("Wrong number of arguments to " << p_id << " " << params.size() << " expected " << expected_args);
                }

                auto f = make_expression<function_expression>(extend, id, std::move(params), std::move(block));
                return property_name_and_value{is_get ? property_assignment_type::get : property_assignment_type::set, std::move(new_p), std::move(f)};
            }
        }
    }
    EXPECT(token_type::colon);
    return property_name_and_value{property_assignment_type::normal, std::move(p), parse_assignment_expression()};
}

std::unique_ptr<block_statement> parse(const std::shared_ptr<source_file>& source, parse_mode mode) {
    return parser{source, mode}.parse();
}

} // namespace mjs
