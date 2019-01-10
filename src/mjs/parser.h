#ifndef MJS_PARSER_H
#define MJS_PARSER_H

#include <ostream>
#include <memory>
#include <vector>
#include <cassert>
#include <algorithm>

#include "lexer.h"
#include "regexp.h"

namespace mjs {

int operator_precedence(token_type tt);
constexpr int assignment_precedence = 15;
constexpr int comma_precedence      = 16;
constexpr int max_precedence        = 17;

struct source_position {
    int line;
    int column;

    template<typename CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const source_position& sp) {
        return os << sp.line << ":" << sp.column;
    }
};

std::pair<source_position, source_position> extend_to_positions(const std::wstring_view& t, uint32_t start, uint32_t end);

class source_file {
public:
    explicit source_file(const std::wstring_view& filename, const std::wstring_view& text, version ver)
        : ver_(ver)
        , filename_(filename)
        , text_(ver == version::es3 ? strip_format_control_characters(text) : text) {
    }

    version language_version() const { return ver_; }
    std::wstring_view filename() const { return filename_; }
    std::wstring_view text() const { return text_; }

private:
    version ver_;
    std::wstring filename_;
    std::wstring text_;
};

struct source_extend {
    std::shared_ptr<source_file> file;
    uint32_t start;
    uint32_t end;

    std::wstring_view source_view() const {
        return std::wstring_view(file->text().data() + start, end - start);
    }

    bool operator==(const source_extend& rhs) const {
        return file == rhs.file && start == rhs.start && end == rhs.end;
    }

    bool operator!=(const source_extend& rhs) const {
        return !(*this == rhs);
    }

    template<typename CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const source_extend& extend) {
        auto [start_pos, end_pos] = extend_to_positions(extend.file->text(), extend.start, extend.end);
        if constexpr (sizeof(CharT) == 1) {
            const auto fn = extend.file->filename();
            os << std::string(fn.begin(), fn.end());
        } else {
            os << extend.file->filename();
        }
        return os << ":" << start_pos << "-" << end_pos;
    }
};


class syntax_node {
public:
    virtual ~syntax_node() {}

    friend std::wostream& operator<<(std::wostream& os, const syntax_node& sn) {
        sn.print(os);
        return os;
    }

    const source_extend& extend() const {
        return extend_;
    }

protected:
    syntax_node(const source_extend& extend) : extend_(extend) {
    }

private:
    source_extend extend_;
    virtual void print(std::wostream& os) const = 0;
};

enum class expression_type {
    identifier,
    literal,
    array_literal,
    object_literal,
    regexp_literal,
    call,
    prefix,
    postfix,
    binary,
    conditional,
    function,
};

class expression : public syntax_node {
public:
    virtual expression_type type() const = 0;
protected:
    using syntax_node::syntax_node;
};

enum class statement_type {
    block,
    variable,
    debugger,
    empty,
    expression,
    if_,
    do_,
    while_,
    for_,
    for_in,
    continue_,
    break_,
    return_,
    with,
    labelled,
    switch_,
    throw_,
    try_,
    function_definition,
};

class statement : public syntax_node {
public:
    virtual statement_type type() const = 0;
protected:
    using syntax_node::syntax_node;
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
    explicit identifier_expression(const source_extend& extend, const std::wstring& id) : expression(extend), id_(id) {}

    expression_type type() const override { return expression_type::identifier; }

    const std::wstring& id() const { return id_; }

private:
    std::wstring id_;

    void print(std::wostream& os) const override {
        os << "identifier_expression{" << id_ << "}";
    }
};

class literal_expression : public expression {
public:
    explicit literal_expression(const source_extend& extend, const token& t) : expression(extend), t_(t) {
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

class array_literal_expression : public expression {
public:
    explicit array_literal_expression(const source_extend& extend, std::vector<expression_ptr>&& elements) : expression(extend), elements_(std::move(elements)) {
    }

    expression_type type() const override { return expression_type::array_literal; }

    const std::vector<expression_ptr>& elements() const { return elements_; }

private:
    std::vector<expression_ptr> elements_;

    void print(std::wostream& os) const override {
        os << "array_literal_expression{";
        for (size_t i = 0; i < elements_.size(); ++i) {
            if (i) os << ", ";
            if (elements_[i]) os << *elements_[i];
        }
        os << "}";
    }
};

using property_name_and_value = std::pair<expression_ptr, expression_ptr>;
using property_name_and_value_list = std::vector<property_name_and_value>;

class object_literal_expression : public expression {
public:
    explicit object_literal_expression(const source_extend& extend, property_name_and_value_list&& elements) : expression(extend), elements_(std::move(elements)) {
#ifndef  NDEBUG
        for (const auto& p : elements_) {
            assert(p.first && p.second);
            assert(p.first->type() == expression_type::identifier
            || (p.first->type() == expression_type::literal
                && (static_cast<const literal_expression&>(*p.first).t().type() == token_type::string_literal
                    || static_cast<const literal_expression&>(*p.first).t().type() == token_type::numeric_literal)));
        }
#endif
    }

    expression_type type() const override { return expression_type::object_literal; }

    const property_name_and_value_list& elements() const { return elements_; }

private:
    property_name_and_value_list elements_;

    void print(std::wostream& os) const override {
        os << "object_literal_expression{";
        for (size_t i = 0; i < elements_.size(); ++i) {
            if (i) os << ", ";
            os << *elements_[i].first << ": " << *elements_[i].second;
        }
        os << "}";
    }
};

class regexp_literal_expression : public expression {
public:
    explicit regexp_literal_expression(const source_extend& extend, std::wstring_view pattern, std::wstring_view flags) : expression(extend), re_(pattern, flags) {
    }

    expression_type type() const override { return expression_type::regexp_literal; }

    const regexp& re() const { return re_; }

private:
    regexp re_;

    void print(std::wostream& os) const override {
        os << "regexp_literal_expression{" << re_.pattern() << ", " << re_.flags() << "}";
    }
};


class call_expression : public expression {
public:
    explicit call_expression(const source_extend& extend, expression_ptr&& member, expression_list&& arguments) : expression(extend), member_(std::move(member)), arguments_(std::move(arguments)) {
        assert(member_);
    }

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

class prefix_expression : public expression {
public:
    explicit prefix_expression(const source_extend& extend, token_type op, expression_ptr&& e) : expression(extend), op_(op), e_(std::move(e)) {
        assert(e_);
    }

    expression_type type() const override { return expression_type::prefix; }

    token_type op() const { return op_; }
    const expression& e() const { return *e_; }

private:
    token_type op_;
    expression_ptr e_;

    void print(std::wostream& os) const override {
        os << "prefix_expression{" << op_ << ", " << *e_ << "}";
    }
};

class postfix_expression : public expression {
public:
    explicit postfix_expression(const source_extend& extend, token_type op, expression_ptr&& e) : expression(extend), op_(op), e_(std::move(e)) {
        assert(e_);
    }

    expression_type type() const override { return expression_type::postfix; }

    token_type op() const { return op_; }
    const expression& e() const { return *e_; }

private:
    token_type op_;
    expression_ptr e_;

    void print(std::wostream& os) const override {
        os << "postfix_expression{" << op_ << ", " << *e_ << "}";
    }
};

class binary_expression : public expression {
public:
    explicit binary_expression(const source_extend& extend, token_type op, expression_ptr&& lhs, expression_ptr&& rhs) : expression(extend), op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
        assert(lhs_);
        assert(rhs_);
    }

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

class conditional_expression : public expression {
public:
    explicit conditional_expression(const source_extend& extend, expression_ptr&& cond, expression_ptr&& lhs, expression_ptr&& rhs) : expression(extend), cond_(std::move(cond)), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
        assert(lhs_);
        assert(rhs_);
    }

    expression_type type() const override { return expression_type::conditional; }

    const expression& cond() const { return *cond_; }
    const expression& lhs() const { return *lhs_; }
    const expression& rhs() const { return *rhs_; }

private:
    expression_ptr cond_;
    expression_ptr lhs_;
    expression_ptr rhs_;

    void print(std::wostream& os) const override {
        os << "conditional_expression{" << *cond_ << ", " << *lhs_ << ", " << *rhs_ << "}";
    }
};

class block_statement;
class function_base {
public:
    const source_extend& body_extend() const { return body_extend_; }
    const std::wstring& id() const { return id_; }
    const std::vector<std::wstring>& params() const { return params_; }
    const block_statement& block() const { return *block_; }
    const std::shared_ptr<block_statement>& block_ptr() const { return block_; }

protected:
    explicit function_base(const source_extend& body_extend, const std::wstring& id, std::vector<std::wstring>&& params, statement_ptr&& block);

    void base_print(std::wostream& os) const;

private:
    source_extend body_extend_;
    std::wstring id_;
    std::vector<std::wstring> params_;
    std::shared_ptr<block_statement> block_;
};

class function_expression : public expression, public function_base {
public:
    explicit function_expression(const source_extend& extend, const source_extend& body_extend, const std::wstring& id, std::vector<std::wstring>&& params, statement_ptr&& block) : expression(extend), function_base(body_extend, id, std::move(params), std::move(block)) {
    }

    expression_type type() const override { return expression_type::function; }
private:

    void print(std::wostream& os) const override {
        os << "function_expression";
        base_print(os);
    }
};


template<typename Visitor>
auto accept(const expression& e, Visitor& v) {
    switch (e.type()) {
    case expression_type::identifier:       return v(static_cast<const identifier_expression&>(e));
    case expression_type::literal:          return v(static_cast<const literal_expression&>(e));
    case expression_type::array_literal:    return v(static_cast<const array_literal_expression&>(e));
    case expression_type::object_literal:   return v(static_cast<const object_literal_expression&>(e));
    case expression_type::regexp_literal:   return v(static_cast<const regexp_literal_expression&>(e));
    case expression_type::call:             return v(static_cast<const call_expression&>(e));
    case expression_type::prefix:           return v(static_cast<const prefix_expression&>(e));
    case expression_type::postfix:          return v(static_cast<const postfix_expression&>(e));
    case expression_type::binary:           return v(static_cast<const binary_expression&>(e));
    case expression_type::conditional:      return v(static_cast<const conditional_expression&>(e));
    case expression_type::function:         return v(static_cast<const function_expression&>(e));
    }
    assert(!"Not implemented");
    return v(e);
}

//
// Statements
//

class block_statement : public statement {
public:
    explicit block_statement(const source_extend& extend, statement_list&& l) : statement(extend), l_(std::move(l)) {}

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

class declaration {
public:
    using list = std::vector<declaration>;

    explicit declaration(const std::wstring& id, expression_ptr&& init) : id_(id), init_(std::move(init)) {
        assert(!id.empty() || init_);
    }

    const std::wstring& id() const { return id_;}

    const expression* init() const { return init_.get(); }

    friend std::wostream& operator<<(std::wostream& os, const declaration& d) {
        os << '{';
        if (!d.id_.empty()) {
            os << d.id_;
            if (d.init_) {
                os << ", " << *d.init_;
            }
        } else {
            os << *d.init_;
        }
        return os << '}';
    }

private:
    std::wstring id_;
    expression_ptr init_;
};


class variable_statement : public statement {
public:
    statement_type type() const override { return statement_type::variable; }

    explicit variable_statement(const source_extend& extend, declaration::list&& l) : statement(extend), l_(std::move(l)) {}

    const declaration::list& l() const { return l_; }

private:
    declaration::list l_;

    void print(std::wostream& os) const override {
        os << "variable_statement{[";
        for (size_t i = 0; i < l_.size(); ++i) {
            if (i) os << ", ";
            os << l_[i];
        }
        os << "]}";
    }
};

class empty_statement : public statement {
public:
    explicit empty_statement(const source_extend& extend) : statement(extend) {
    }

    statement_type type() const override { return statement_type::empty; }

private:
    void print(std::wostream& os) const override {
        os << "empty_statement{}";
    }
};


class debugger_statement : public statement {
public:
    explicit debugger_statement(const source_extend& extend) : statement(extend) {
    }

    statement_type type() const override { return statement_type::debugger; }

private:
    void print(std::wostream& os) const override {
        os << "debugger_statement{}";
    }
};

class expression_statement : public statement {
public:
    explicit expression_statement(const source_extend& extend, expression_ptr&& e) : statement(extend), e_(std::move(e)) {
        assert(e_);
    }

    statement_type type() const override { return statement_type::expression; }

    const expression& e() const { return *e_; }

private:
    expression_ptr e_;

    void print(std::wostream& os) const override {
        os << "expression_statement{" << *e_ << "}";
    }
};

class if_statement : public statement {
public:
    explicit if_statement(const source_extend& extend, expression_ptr&& cond, statement_ptr&& if_s, statement_ptr&& else_s) : statement(extend), cond_(std::move(cond)), if_s_(std::move(if_s)), else_s_(std::move(else_s)) {
        assert(cond_);
        assert(if_s_);
    }

    statement_type type() const override { return statement_type::if_; }

    const expression& cond() const { return *cond_; }
    const statement& if_s() const { return *if_s_; }
    const statement* else_s() const { return else_s_.get(); }

private:
    expression_ptr cond_;
    statement_ptr if_s_;
    statement_ptr else_s_;

    void print(std::wostream& os) const override {
        os << "if_statement{" << *cond_ << ", " << *if_s_;
        if (else_s_) os << ", " << *else_s_;
        os << "}";
    }
};

class do_statement : public statement {
public:
    explicit do_statement(const source_extend& extend, expression_ptr&& cond, statement_ptr&& s) : statement(extend), cond_(std::move(cond)), s_(std::move(s)) {
        assert(cond_);
        assert(s_);
    }

    statement_type type() const override { return statement_type::do_; }

    const expression& cond() const { return *cond_; }
    const statement& s() const { return *s_; }

private:
    expression_ptr cond_;
    statement_ptr s_;

    void print(std::wostream& os) const override {
        os  << "do_statement{" << *cond_ << ", " << *s_ << "}";
    }
};

class while_statement : public statement {
public:
    explicit while_statement(const source_extend& extend, expression_ptr&& cond, statement_ptr&& s) : statement(extend), cond_(std::move(cond)), s_(std::move(s)) {
        assert(cond_);
        assert(s_);
    }

    statement_type type() const override { return statement_type::while_; }

    const expression& cond() const { return *cond_; }
    const statement& s() const { return *s_; }

private:
    expression_ptr cond_;
    statement_ptr s_;

    void print(std::wostream& os) const override {
        os  << "while_statement{" << *cond_ << ", " << *s_ << "}";
    }
};

class for_statement : public statement {
public:
    explicit for_statement(const source_extend& extend, statement_ptr&& init, expression_ptr&& cond, expression_ptr&& iter, statement_ptr&& s) : statement(extend), init_(std::move(init)), cond_(std::move(cond)), iter_(std::move(iter)), s_(std::move(s)) {
        assert(!init_ || init_->type() == statement_type::expression || init_->type() == statement_type::variable);
        assert(s_);
    }

    statement_type type() const override { return statement_type::for_; }

    const statement* init() const { return init_.get(); }
    const expression* cond() const { return cond_.get(); }
    const expression* iter() const { return iter_.get(); }
    const statement& s() const { return *s_; }

private:
    statement_ptr  init_;
    expression_ptr cond_;
    expression_ptr iter_;
    statement_ptr  s_;

    void print(std::wostream& os) const override {
        os << "for_statement{";
        if (init_) os << *init_; else os << "{}";
        os << ',';
        if (cond_) os << *cond_; else os << "{}";
        os << ',';
        if (iter_) os << *iter_; else os << "{}";
        os << ',' << *s_;
        os << "}";
    }
};

class for_in_statement : public statement {
public:
    explicit for_in_statement(const source_extend& extend, statement_ptr&& init, expression_ptr&& e, statement_ptr&& s) : statement(extend), init_(std::move(init)), e_(std::move(e)), s_(std::move(s)) {
        assert(init_ && (init_->type() == statement_type::expression || (init_->type() == statement_type::variable && static_cast<variable_statement&>(*init_).l().size() == 1)));
        assert(e_);
        assert(s_);
    }

    statement_type type() const override { return statement_type::for_in; }

    const statement& init() const { return *init_; }
    const expression& e() const { return *e_; }
    const statement& s() const { return *s_; }

private:
    statement_ptr  init_;
    expression_ptr e_;
    statement_ptr  s_;

    void print(std::wostream& os) const override {
        os << "for_in_statement{" << *init_ << ",";
        os << ',' << *e_;
        os << ',' << *s_;
        os << "}";
    }
};

class continue_statement : public statement {
public:
    explicit continue_statement(const source_extend& extend, const std::wstring& id) : statement(extend), id_(id) {}
    statement_type type() const override { return statement_type::continue_; }
    const std::wstring& id() const { return id_; }
private:
    std::wstring id_;
    void print(std::wostream& os) const override {
        os << "continue_statement{";
        if (!id_.empty()) {
            os << id_;
        }
        os << "}";
    }
};

class break_statement : public statement {
public:
    explicit break_statement(const source_extend& extend, const std::wstring& id) : statement(extend), id_(id) {}
    statement_type type() const override { return statement_type::break_; }
    const std::wstring& id() const { return id_; }
private:
    std::wstring id_;
    void print(std::wostream& os) const override {
        os << "break_statement{";
        if (!id_.empty()) {
            os << id_;
        }
        os << "}";
    }
};

class return_statement : public statement {
public:
    explicit return_statement(const source_extend& extend, expression_ptr&& e) : statement(extend), e_(std::move(e)) {}

    statement_type type() const override { return statement_type::return_; }

    const expression* e() const { return e_.get(); }

private:
    expression_ptr e_;

    void print(std::wostream& os) const override {
        os << "return_statement{";
        if (e_) os << *e_;
        os << "}";
    }
};

class with_statement : public statement {
public:
    explicit with_statement(const source_extend& extend, expression_ptr&& e, statement_ptr&& s) : statement(extend), e_(std::move(e)), s_(std::move(s)) {
        assert(e_);
        assert(s_);
    }

    statement_type type() const override { return statement_type::with; }

    const expression& e() const { return *e_; }
    const statement& s() const { return *s_; }

private:
    expression_ptr e_;
    statement_ptr s_;

    void print(std::wostream& os) const override {
        os << "with_statement{" << *e_ << ", " << *s_ << "}";
    }
};

class labelled_statement : public statement {
public:
    explicit labelled_statement(const source_extend& extend, const std::wstring& id, statement_ptr&& s) : statement(extend), id_(id), s_(std::move(s)) {
        assert(!id_.empty());
        assert(s_);
    }

    statement_type type() const override { return statement_type::labelled; }

    const std::wstring& id() const { return id_; };
    const statement& s() const { return *s_; };

private:
    std::wstring id_;
    statement_ptr s_;

    void print(std::wostream& os) const override {
        os << "labelled_statement{" << id_<< ", " << *s_ << "}";
    }
};

// The default clause is repesented by case_clause with a null expression
class case_clause {
public:
    explicit case_clause(expression_ptr&& e, statement_list&& sl) : e_(std::move(e)), sl_(std::move(sl)) {
    }

    bool is_default() const { return !e_; }

    const expression_ptr& e() const { return e_; }
    const statement_list& sl() const { return sl_; }

private:
    expression_ptr e_;
    statement_list sl_;
};

using clause_list = std::vector<case_clause>;

class switch_statement : public statement {
public:
    explicit switch_statement(const source_extend& extend, expression_ptr&& e, clause_list&& cl) : statement(extend), e_(std::move(e)), cl_(std::move(cl)) {
        assert(e_);
    }

    statement_type type() const override { return statement_type::switch_; }

    const expression& e() const { return *e_; }
    const clause_list& cl() const { return cl_; }

    auto default_clause() const {
        return std::find_if(cl_.begin(), cl_.end(), [](const auto& c) { return !c.e(); });
    }

private:
    expression_ptr e_;
    clause_list cl_;

    void print(std::wostream& os) const override {
        os << "switch_statement{" << e() << ", [";
        bool first = true;
        for (const auto& c: cl()) {
            if (!first) os << ", ";
            first = false;
            os << "{";
            if (c.e()) {
                os << *c.e() << ", ";
            }
            os << "[";
            for (size_t i = 0; i < c.sl().size(); ++i) {
                if (i) os << ", ";
                os << *c.sl()[i];
            }
            os << "]}";
        }
        os << "]}";
    }
};

class throw_statement : public statement {
public:
    explicit throw_statement(const source_extend& extend, expression_ptr&& e) : statement(extend), e_(std::move(e)) {
        assert(e_);
    }

    statement_type type() const override { return statement_type::throw_; }

    const expression& e() const { return *e_; }
private:
    expression_ptr e_;

    void print(std::wostream& os) const override {
        os << "throw_statement{";
        if (e_) os << *e_;
        os << "}";
    }
};


class try_statement : public statement {
public:
    explicit try_statement(const source_extend& extend, statement_ptr&& block, statement_ptr&& catch_, const std::wstring& catch_id, statement_ptr&& finally_)
        : statement(extend)
        , block_(std::move(block))
        , catch_(std::move(catch_))
        , catch_id_(catch_id)
        , finally_(std::move(finally_)) {
        assert(block_ && block_->type() == statement_type::block);
        assert(!this->catch_ == catch_id_.empty());
        assert(!this->catch_ || this->catch_->type() == statement_type::block);
        assert(!this->finally_ || this->finally_->type() == statement_type::block);
    }

    statement_type type() const override { return statement_type::try_; }

    const block_statement& block() const { return static_cast<const block_statement&>(*block_); }
    const std::wstring& catch_id() const { return catch_id_; }
    const block_statement* catch_block() const { return static_cast<const block_statement*>(catch_.get()); }
    const block_statement* finally_block() const { return static_cast<const block_statement*>(finally_.get()); }

private:
    statement_ptr block_;
    statement_ptr catch_;
    std::wstring catch_id_;
    statement_ptr finally_;

    void print(std::wostream& os) const override {
        os << "try_statement{";
        os << *block_ << ", ";
        if (catch_) os << "catch=" << *catch_;
        if (finally_) os << "catch=" << *finally_;
        os << "}";
    }
};

class function_definition : public statement, public function_base {
public:
    explicit function_definition(const source_extend& extend, const source_extend& body_extend, const std::wstring& id, std::vector<std::wstring>&& params, statement_ptr&& block) : statement(extend), function_base(body_extend, id, std::move(params), std::move(block)) {
        assert(!this->id().empty());
    }

    statement_type type() const override { return statement_type::function_definition; }
private:

    void print(std::wostream& os) const override {
        os << "function_definition";
        base_print(os);
    }
};

template<typename Visitor>
auto accept(const statement& s, Visitor& v) {
    switch (s.type()) {
    case statement_type::block:                 return v(static_cast<const block_statement&>(s));
    case statement_type::variable:              return v(static_cast<const variable_statement&>(s));
    case statement_type::debugger:              return v(static_cast<const debugger_statement&>(s));
    case statement_type::empty:                 return v(static_cast<const empty_statement&>(s));
    case statement_type::expression:            return v(static_cast<const expression_statement&>(s));
    case statement_type::if_:                   return v(static_cast<const if_statement&>(s));
    case statement_type::do_:                   return v(static_cast<const do_statement&>(s));
    case statement_type::while_:                return v(static_cast<const while_statement&>(s));
    case statement_type::for_:                  return v(static_cast<const for_statement&>(s));
    case statement_type::for_in:                return v(static_cast<const for_in_statement&>(s));
    case statement_type::continue_:             return v(static_cast<const continue_statement&>(s));
    case statement_type::break_:                return v(static_cast<const break_statement&>(s));
    case statement_type::return_:               return v(static_cast<const return_statement&>(s));
    case statement_type::with:                  return v(static_cast<const with_statement&>(s));
    case statement_type::labelled:              return v(static_cast<const labelled_statement&>(s));
    case statement_type::switch_:               return v(static_cast<const switch_statement&>(s));
    case statement_type::throw_:                return v(static_cast<const throw_statement&>(s));
    case statement_type::try_:                  return v(static_cast<const try_statement&>(s));
    case statement_type::function_definition:   return v(static_cast<const function_definition&>(s));
    }
    assert(!"Not implemented");
    return v(s);
}

//
// Parser
//

std::unique_ptr<block_statement> parse(const std::shared_ptr<source_file>& source);

} // namespace mjs

#endif
