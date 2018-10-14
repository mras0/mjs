#ifndef MJS_PARSER_H
#define MJS_PARSER_H

#include <ostream>
#include <memory>
#include <vector>
#include <cassert>

#include "string.h"
#include "lexer.h"

namespace mjs {

int operator_precedence(token_type tt);

class syntax_node {
public:
    virtual ~syntax_node() {}

    friend std::wostream& operator<<(std::wostream& os, const syntax_node& sn) {
        sn.print(os);
        return os;
    }

private:
    virtual void print(std::wostream& os) const = 0;
};

enum class expression_type {
    identifier,
    literal,
    call,
    binary,
};

class expression : public syntax_node {
public:
    virtual expression_type type() const = 0;
};

enum class statement_type {
    block,
    variable,
    empty,
    expression,
    if_,
    iteration,
    continue_,
    break_,
    return_,
    with,
    function_definition,
};

class statement : public syntax_node {
public:
    virtual statement_type type() const = 0;
};

using expression_ptr = std::unique_ptr<expression>;
using statement_ptr = std::unique_ptr<statement>;
using expression_list = std::vector<expression_ptr>;
using statement_list = std::vector<statement_ptr>;

//
// Expressions
//

class identifier_expression : public expression {
public:
    explicit identifier_expression(const string& id) : id_(id) {}

    expression_type type() const override { return expression_type::identifier; }

    const string& id() const { return id_; }

private:
    string id_;

    void print(std::wostream& os) const override {
        os << "identifier_expression{" << id_ << "}";
    }
};

class literal_expression : public expression {
public:
    explicit literal_expression(const token& t) : t_(t) {
        assert(is_literal(t.type()));
    }

    expression_type type() const override { return expression_type::literal; }

    const token& t() const { return t_; }

private:
    token t_;

    void print(std::wostream& os) const override {
        os << "literal_expression{" << t_ << "}";
    }
};

class call_expression : public expression {
public:
    explicit call_expression(expression_ptr&& member, expression_list&& arguments) : member_(std::move(member)), arguments_(std::move(arguments)) {}

    expression_type type() const override { return expression_type::call; }

    const expression& member() const { return *member_; }
    const expression_list& arguments() const { return arguments_; }

private:
    expression_ptr member_;
    expression_list arguments_;

    void print(std::wostream& os) const override {
        os << "call_expression{" << *member_ << ", {";
        bool first = true;
        for (const auto& a: arguments_) {
            if (first) first = false;
            else os << ", ";
            os << *a;
        }
        os << "}}";
    }
};

class binary_expression : public expression {
public:
    explicit binary_expression(token_type op, expression_ptr&& lhs, expression_ptr&& rhs) : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    expression_type type() const override { return expression_type::binary; }

    token_type op() const { return op_; }
    const expression& lhs() const { return *lhs_; }
    const expression& rhs() const { return *rhs_; }

private:
    token_type op_;
    expression_ptr lhs_;
    expression_ptr rhs_;

    void print(std::wostream& os) const override {
        os << "binary_expression{" << op_ << ", " << *lhs_ << ", " << *rhs_ << "}";
    }
};

template<typename Visitor>
auto accept(const expression& e, Visitor& v) {
    switch (e.type()) {
    case expression_type::identifier: return v(static_cast<const identifier_expression&>(e));
    case expression_type::literal:    return v(static_cast<const literal_expression&>(e));
    case expression_type::call:       return v(static_cast<const call_expression&>(e));
    case expression_type::binary:     return v(static_cast<const binary_expression&>(e));
    }
    assert(!"Not implemented");
    return v(e);
}

//
// Statements
//

class block_statement : public statement {
public:
    explicit block_statement(statement_list&& l) : l_(std::move(l)) {}

    statement_type type() const override { return statement_type::block; }

    const statement_list& l() const { return l_; }
private:
    statement_list l_;

    void print(std::wostream& os) const override {
        os << "block_statement{";
        for (size_t i = 0; i < l_.size(); ++i) {
            if (i) os << ", ";
            os << *l_[i];
        }
        os << "}";
    }
};

class variable_statement : public statement {
public:
    class declaration {
    public:
        explicit declaration(const string& id, expression_ptr&& init) : id_(id), init_(std::move(init)) {}

        const string& id() const { return id_;}

        const expression_ptr& init() const { return init_; }

        friend std::wostream& operator<<(std::wostream& os, const declaration& d) {
            os << '{' << d.id_;
            if (d.init_) {
                os << ", " << *d.init_;
            }
            return os << '}';
        }

    private:
        string id_;
        expression_ptr init_;
    };
    using declaration_list = std::vector<declaration>;

    statement_type type() const override { return statement_type::variable; }

    explicit variable_statement(declaration_list&& l) : l_(std::move(l)) {}

    const declaration_list& l() const { return l_; }

private:
    declaration_list l_;

    void print(std::wostream& os) const override {
        os << "variable_statement{[";
        for (size_t i = 0; i < l_.size(); ++i) {
            if (i) os << ", ";
            os << l_[i];
        }
        os << "]}";
    }
};

//empty,

class expression_statement : public statement {
public:
    explicit expression_statement(expression_ptr&& e) : e_(std::move(e)) {}

    statement_type type() const override { return statement_type::expression; }

    const expression& e() const { return *e_; }

private:
    expression_ptr e_;

    void print(std::wostream& os) const override {
        os << "expression_statement{" << *e_ << "}";
    }
};

//if_,
//iteration,
//continue_,
//break_,

class return_statement : public statement {
public:
    explicit return_statement(expression_ptr&& e) : e_(std::move(e)) {}

    statement_type type() const override { return statement_type::return_; }

    const expression_ptr& e() const { return e_; }

private:
    expression_ptr e_;

    void print(std::wostream& os) const override {
        os << "return_statement{";
        if (e_) os << *e_;
        os << "}";
    }
};

//with,

class function_definition : public statement {
public:
    explicit function_definition(const string& id, std::vector<string>&& params, statement_ptr&& block) : id_(id), params_(std::move(params)), block_(std::move(block)) {
        assert(block_->type() == statement_type::block);
    }

    statement_type type() const override { return statement_type::function_definition; }

    const string& id() const { return id_; }
    const std::vector<string>& params() const { return params_; }
    const block_statement& block() const { return static_cast<const block_statement&>(*block_); }

private:
    string id_;
    std::vector<string> params_;
    statement_ptr block_;

    void print(std::wostream& os) const override {
        os << "function_definition{" << id_ << ", [";
        for (size_t i = 0; i < params_.size(); ++i) {
            if (i) os << ", ";
            os << params_[i];
        }
        os << "], " << *block_ << "}";
    }
};

template<typename Visitor>
auto accept(const statement& s, Visitor& v) {
    switch (s.type()) {
    case statement_type::block:                 return v(static_cast<const block_statement&>(s));
    case statement_type::variable:              return v(static_cast<const variable_statement&>(s));
    //case statement_type::empty:
    case statement_type::expression:            return v(static_cast<const expression_statement&>(s));
    //case statement_type::if_:
    //case statement_type::iteration:
    //case statement_type::continue_:
    //case statement_type::break_:
    case statement_type::return_:               return v(static_cast<const return_statement&>(s));
    //case statement_type::with:
    case statement_type::function_definition:   return v(static_cast<const function_definition&>(s));
    }
    assert(!"Not implemented");
    return v(s);
}

//
// Parser
//

statement_ptr parse(const std::wstring_view& str);

} // namespace mjs

#endif
