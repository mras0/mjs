#include "interpreter.h"
#include "parser.h"
#include "global_object.h"

#include <sstream>
#include <algorithm>
#include <cmath>

#ifndef NDEBUG
#include <iostream>
#endif

namespace mjs {

std::wostream& operator<<(std::wostream& os, const completion_type& t) {
    switch (t) {
    case completion_type::normal: return os << "Normal completion";
    case completion_type::break_: return os << "Break";
    case completion_type::continue_: return os << "Continue";
    case completion_type::return_: return os << "Return";
    }
    NOT_IMPLEMENTED((int)t);
}

std::wostream& operator<<(std::wostream& os, const completion& c) {
    return os << c.type << " value type: " << c.result.type();
}

class hoisting_visitor {
public:
    static std::vector<std::wstring> scan(const block_statement& bs) {
        hoisting_visitor hv{};
        hv(bs);
        return hv.ids_;
    }

    void operator()(const block_statement& s) {
        for (const auto& bs: s.l()) {
            accept(*bs, *this);
        }
    }

    void operator()(const variable_statement& s) {
        for (const auto& d: s.l()) {
            ids_.push_back(d.id());
        }
    }

    void operator()(const empty_statement&) {}

    void operator()(const expression_statement&){}

    void operator()(const if_statement& s) {
        accept(s.if_s(), *this);
        if (auto e = s.else_s()) {
            accept(*e, *this);
        }
    }

    void operator()(const while_statement& s){
        accept(s.s(), *this);
    }

    void operator()(const for_statement& s){
        if (s.init()) accept(*s.init(), *this);
        accept(s.s(), *this);
    }

    void operator()(const for_in_statement& s){
        accept(s.init(), *this);
        accept(s.s(), *this);
    }

    void operator()(const continue_statement&){}
    void operator()(const break_statement&){}
    void operator()(const return_statement&){}
    void operator()(const with_statement&){}

    void operator()(const function_definition& s) {
        assert(!s.id().empty());
        ids_.push_back(s.id());
    }

    void operator()(const statement& s) {
        NOT_IMPLEMENTED(s);
    }

private:
    explicit hoisting_visitor() {}
    std::vector<std::wstring> ids_;
};

static std::string get_eval_exception_repr(const std::vector<source_extend>& stack_trace, const std::wstring_view& msg) {
    std::ostringstream oss;
    oss << std::string(msg.begin(), msg.end());
    for (const auto& e: stack_trace) {
        assert(e.file);
        oss << '\n' << e;
    }
    return oss.str();
}

eval_exception::eval_exception(const std::vector<source_extend>& stack_trace, const std::wstring_view& msg) : std::runtime_error(get_eval_exception_repr(stack_trace, msg)) {
}

class interpreter::impl {
public:
    explicit impl(gc_heap& h, const block_statement& program, const on_statement_executed_type& on_statement_executed) : heap_(h), global_(global_object::make(h)), on_statement_executed_(on_statement_executed) {
        assert(!global_->has_property(L"eval"));

        global_->put_native_function(global_, "eval", [this](const value&, const std::vector<value>& args) {
            if (args.empty()) {
                return value::undefined;
            } else if (args.front().type() != value_type::string) {
                return args.front();
            }
            auto bs = parse(std::make_shared<source_file>(L"eval", args.front().string_value().view()));
            completion ret;
            for (const auto& s: bs->l()) {
                ret = eval(*s);
                if (ret) {
                    return value::undefined;
                }
            }
            assert(!ret);
            return ret.result;
        }, 1);

        auto func_obj = global_->get(L"Function").object_value();
        global_->put_function(func_obj, gc_function::make(h, [this](const value&, const std::vector<value>& args) {
            std::wstring body{}, p{};
            if (args.empty()) {
            } else if (args.size() == 1) {
                body = to_string(heap_, args.front()).view();
            } else {
                p = to_string(heap_, args.front()).view();
                for (size_t k = 1; k < args.size() - 1; ++k) {
                    p += ',';
                    p += to_string(heap_, args[k]).view();
                }
                body = to_string(heap_, args.back()).view();
            }

            auto bs = parse(std::make_shared<source_file>(L"Function definition", L"function anonymous(" + p + L") {\n" + body + L"\n}"));
            if (bs->l().size() != 1 || bs->l().front()->type() != statement_type::function_definition) {
                NOT_IMPLEMENTED("Invalid function definition: " << bs->extend().source_view());
            }

            return value{create_function(static_cast<const function_definition&>(*bs->l().front()), make_scope(global_, nullptr))};
        }), global_object::native_function_body(string{heap_, L"Function"}), 1);
        func_obj->construct_function(func_obj->call_function());


        for (const auto& id: hoisting_visitor::scan(program)) {
            global_->put(string{heap_, id}, value::undefined);
        }

        active_scope_ = make_scope(global_, nullptr);
    }

    ~impl() {
        assert(active_scope_ && !active_scope_->get_prev());
    }

    value eval(const expression& e) {
#ifdef MJS_GC_STRESS_TEST
        // Typical causes of crashes when the stress test is enabled:
        //  - `some_ptr->func(eval(...))`: `some_ptr->` can be evaluated before `eval(...)`
        //  - holding pointers to GC objects past calls to eval (be aware of indirect calls like calling a user provided sorting predicate)
        //    Note that `this` is counted for native function implementations (!) see Array.sort for an example
        auto res = accept(e, *this);
        heap_.garbage_collect();
        return res;
#else
        return accept(e, *this);
#endif
    }

    completion eval(const statement& s) {
        if (on_statement_executed_) {
            auto res = accept(s, *this);
            on_statement_executed_(s, res);
            return res;
        } else {
            return accept(s, *this);
        }
    }

    value operator()(const identifier_expression& e) {
        // §10.1.4
        return value{active_scope_->lookup(e.id())};
    }

    value operator()(const literal_expression& e) {
        switch (e.t().type()) {
        case token_type::undefined_:      return value::undefined;
        case token_type::null_:           return value::null;
        case token_type::true_:           return value{true};
        case token_type::false_:          return value{false};
        case token_type::numeric_literal: return value{e.t().dvalue()};
        case token_type::string_literal:  return value{string{heap_, e.t().text()}};
        default: NOT_IMPLEMENTED(e);
        }
    }

    value operator()(const call_expression& e) {
        auto member = eval(e.member());
        auto mval = get_value(member);
        auto args = eval_argument_list(e.arguments());
        if (mval.type() != value_type::object) {
            std::wostringstream woss;
            woss << e.member() << " is not a function";
            throw eval_exception(stack_trace(e.extend()), woss.str());
        }
        auto c = mval.object_value()->call_function();
        if (!c) {
            std::wostringstream woss;
            woss << e.member() << " is not callable";
            throw eval_exception(stack_trace(e.extend()), woss.str());
        }
        auto this_ = value::null;
        if (member.type() == value_type::reference) {
            if (auto o = member.reference_value().base(); o->class_name().view() != L"Activation") {  // TODO: Have better way of checking for Activation object
                this_ = value{o};
            }
        }

        auto_stack_update asu{*this, e.extend()};
        return c->call(this_, args);
    }

    value operator()(const prefix_expression& e) {
        if (e.op() == token_type::new_) {
            return handle_new_expression(e.e());
        }

        auto u = eval(e.e());
        if (e.op() == token_type::delete_) {
            if (u.type() != value_type::reference) {
                NOT_IMPLEMENTED(u.type());
            }
            const auto& base = u.reference_value().base();
            const auto& prop = u.reference_value().property_name();
            if (!base) {
                return value{true};
            }
            return value{base->delete_property(prop.view())};
        } else if (e.op() == token_type::void_) {
            (void)get_value(u);
            return value::undefined;
        } else if (e.op() == token_type::typeof_) {
            if (u.type() == value_type::reference && !u.reference_value().base()) {
                return value{string{heap_, "undefined"}};
            }
            u = get_value(u);
            switch (u.type()) {
            case value_type::undefined: return value{string{heap_, "undefined"}};
            case value_type::null: return value{string{heap_, "object"}};
            case value_type::boolean: return value{string{heap_, "boolean"}};
            case value_type::number: return value{string{heap_, "number"}};
            case value_type::string: return value{string{heap_, "string"}};
            case value_type::object: return value{string{heap_, u.object_value()->call_function() ? "function" : "object"}};
            default:
                NOT_IMPLEMENTED(u.type());
            }
        } else if (e.op() == token_type::plusplus || e.op() == token_type::minusminus) {
            if (u.type() != value_type::reference) {
                NOT_IMPLEMENTED(to_string(heap_, u));
            }
            auto num = to_number(get_value(u)) + (e.op() == token_type::plusplus ? 1 : -1);
            if (!put_value(u, value{num})) {
                NOT_IMPLEMENTED(to_string(heap_, u));
            }
            return value{num};
        } else if (e.op() == token_type::plus) {
            return value{to_number(get_value(u))};
        } else if (e.op() == token_type::minus) {
            return value{-to_number(get_value(u))};
        } else if (e.op() == token_type::tilde) {
            return value{static_cast<double>(~to_int32(get_value(u)))};
        } else if (e.op() == token_type::not_) {
            return value{!to_boolean(get_value(u))};
        }
        NOT_IMPLEMENTED(e);
    }

    value operator()(const postfix_expression& e) {
        auto member = eval(e.e());
        if (member.type() != value_type::reference) {
            NOT_IMPLEMENTED(e);
        }

        auto orig = to_number(get_value(member));
        auto num = orig;
        switch (e.op()) {
        case token_type::plusplus:   num += 1; break;
        case token_type::minusminus: num -= 1; break;
        default: NOT_IMPLEMENTED(e.op());
        }
        if (!put_value(member, value{num})) {
            NOT_IMPLEMENTED(e);
        }
        return value{orig};
    }

    // 0=false, 1=true, -1=undefined
    static int tri_compare(double l, double r) {
        if (std::isnan(l) || std::isnan(r)) {
            return -1;
        }
        if (l == r || (l == 0.0 && r == 0.0))  {
            return 0;
        }
        if (l == +INFINITY) {
            return 0;
        } else if (r == +INFINITY) {
            return 1;
        } else if (r == -INFINITY) {
            return 0;
        } else if (l == -INFINITY) {
            return 1;
        }
        return l < r;
    }
    static bool compare_equal(const value& l, const value& r) {
        if (l.type() == r.type()) {
            if (l.type() == value_type::undefined || l.type() == value_type::null) {
                return true;
            } else if (l.type() == value_type::number) {
                const double ln = l.number_value();
                const double rn = r.number_value();
                if (std::isnan(ln) || std::isnan(rn)) {
                    return false;
                }
                if ((ln == 0.0 && rn == 0.0) || ln == rn) {
                    return true;
                }
                return false;
            } else if (l.type() == value_type::string) {
                return l.string_value() == r.string_value();
            } else if (l.type() == value_type::boolean) {
                return l.boolean_value() == r.boolean_value();
            }
            assert(l.type() == value_type::object);
            return l.object_value().get() == r.object_value().get();
        }
        // Types are different
        if (l.type() == value_type::null && r.type() == value_type::undefined) {
            return true;
        } else if (l.type() == value_type::undefined && r.type() == value_type::null) {
            return true;
        } else if (l.type() == value_type::number && r.type() == value_type::string) {
            return compare_equal(l, value{to_number(r.string_value())});
        } else if (l.type() == value_type::string && r.type() == value_type::number) {
            return compare_equal(value{to_number(l.string_value())}, r);
        } else if (l.type() == value_type::boolean) {
            return compare_equal(value{static_cast<double>(l.boolean_value())}, r);
        } else if (r.type() == value_type::boolean) {
            return compare_equal(l, value{static_cast<double>(r.boolean_value())});
        } else if ((l.type() == value_type::string || l.type() == value_type::number) && r.type() == value_type::object) {
            return compare_equal(l, to_primitive(r));
        } else if ((r.type() == value_type::string || r.type() == value_type::number) && l.type() == value_type::object) {
            return compare_equal(to_primitive(l), r);
        }
        return false;
    }

    value do_binary_op(const token_type op, value& l, value& r) {
        if (op == token_type::plus) {
            l = to_primitive(l);
            r = to_primitive(r);
            if (l.type() == value_type::string || r.type() == value_type::string) {
                auto ls = to_string(heap_, l);
                auto rs = to_string(heap_, r);
                return value{ls + rs};
            }
            // Otherwise handle like the other operators
        } else if (is_relational(op)) {
            l = to_primitive(l, value_type::number);
            r = to_primitive(r, value_type::number);
            if (l.type() == value_type::string && r.type() == value_type::string) {
                // TODO: See §11.8.5 step 16-21
                NOT_IMPLEMENTED(op);
            }
            const auto ln = to_number(l);
            const auto rn = to_number(r);
            int res;
            switch (op) {
            case token_type::lt:
                res = tri_compare(ln, rn);
                return value{res == -1 ? false : static_cast<bool>(res)};
            case token_type::ltequal:
                res = tri_compare(rn, ln);
                return value{res == -1 || res == 1 ? false : true};
            case token_type::gt:
                res = tri_compare(rn, ln);
                return value{res == -1 ? false : static_cast<bool>(res)};
            case token_type::gtequal:
                res = tri_compare(ln, rn);
                return value{res == -1 || res == 1 ? false : true};
            default: NOT_IMPLEMENTED(op);
            }
        } else if (op == token_type::equalequal || op == token_type::notequal) {
            const bool eq = compare_equal(l ,r);
            return value{op == token_type::equalequal ? eq : !eq};
        }

        const auto ln = to_number(l);
        const auto rn = to_number(r);
        switch (op) {
        case token_type::plus:         return value{ln + rn};
        case token_type::minus:        return value{ln - rn};
        case token_type::multiply:     return value{ln * rn};
        case token_type::divide:       return value{ln / rn};
        case token_type::mod:          return value{std::fmod(ln, rn)};
        case token_type::lshift:       return value{static_cast<double>(to_int32(ln) << (to_uint32(rn) & 0x1f))};
        case token_type::rshift:       return value{static_cast<double>(to_int32(ln) >> (to_uint32(rn) & 0x1f))};
        case token_type::rshiftshift:  return value{static_cast<double>(to_uint32(ln) >> (to_uint32(rn) & 0x1f))};
        case token_type::and_:         return value{static_cast<double>(to_int32(ln) & to_int32(rn))};
        case token_type::xor_:         return value{static_cast<double>(to_int32(ln) ^ to_int32(rn))};
        case token_type::or_:          return value{static_cast<double>(to_int32(ln) | to_int32(rn))};
        default: NOT_IMPLEMENTED(op);
        }
    }

    value operator()(const binary_expression& e) {
        if (e.op() == token_type::comma) {
            (void)get_value(eval(e.lhs()));;
            return get_value(eval(e.rhs()));
        }
        if (operator_precedence(e.op()) == assignment_precedence) {
            auto l = eval(e.lhs());
            auto r = get_value(eval(e.rhs()));
            if (e.op() != token_type::equal) {
                auto lval = get_value(l);
                r = do_binary_op(without_assignment(e.op()), lval, r);
            }
            if (!put_value(l, r)) {
                NOT_IMPLEMENTED(e);
            }
            return r;
        }

        auto l = get_value(eval(e.lhs()));
        if ((e.op() == token_type::andand && !to_boolean(l)) || (e.op() == token_type::oror && to_boolean(l))) {
            return l;
        }
        auto r = get_value(eval(e.rhs()));
        if (e.op() == token_type::andand || e.op() == token_type::oror) {
            return r;
        }
        if (e.op() == token_type::dot || e.op() == token_type::lbracket) {
            return value{reference{global_->to_object(l), to_string(heap_, r)}};
        }
        return do_binary_op(e.op(), l, r);
    }

    value operator()(const conditional_expression& e) {
        if (to_boolean(get_value(eval(e.cond())))) {
            return get_value(eval(e.lhs()));
        } else {
            return get_value(eval(e.rhs()));
        }
    }

    value operator()(const expression& e) {
        NOT_IMPLEMENTED(e);
    }

    //
    // Statements
    //

    completion operator()(const block_statement& s) {
        completion c{};
        for (const auto& bs: s.l()) {
            c = eval(*bs);
            if (c) break;
        }
        return c;
    }

    completion operator()(const variable_statement& s) {
        for (const auto& d: s.l()) {
            assert(active_scope_->has_property(d.id()));
            if (d.init()) {
                // Evaulate in two steps to avoid using stale activation object pointer in case the evaulation forces a garbage collection
                auto init_val = eval(*d.init());
                active_scope_->put(string{heap_, d.id()}, init_val);
            }
        }
        return completion{};
    }

    completion operator()(const empty_statement&) {
        return completion{};
    }

    completion operator()(const expression_statement& s) {
        return completion{completion_type::normal, get_value(eval(s.e()))};
    }

    completion operator()(const if_statement& s) {
        if (to_boolean(get_value(eval(s.cond())))) {
            return eval(s.if_s());
        } else if (auto e = s.else_s()) {
            return eval(*e);
        }
        return completion{};
    }

    completion operator()(const while_statement& s) {
        while (to_boolean(get_value(eval(s.cond())))) {
            auto c = eval(s.s());
            if (c.type == completion_type::break_) {
                return completion{};
            } else if (c.type == completion_type::return_) {
                return c;
            }
            assert(c.type == completion_type::normal || c.type == completion_type::continue_);
        }
        return completion{};
    }

    // TODO: Reduce code duplication in handling of for/for in statements

    completion operator()(const for_statement& s) {
        if (auto is = s.init()) {
            auto c = eval(*is);
            assert(!c); // Expect normal completion
            (void)get_value(c.result);
        }
        completion c{};
        while (!s.cond() || to_boolean(get_value(eval(*s.cond())))) {
            c = eval(s.s());
            if (c.type == completion_type::break_) {
                break;
            } else if (c.type == completion_type::return_) {
                return c;
            }
            assert(c.type == completion_type::normal || c.type == completion_type::continue_);

            if (s.iter()) {
                (void)get_value(eval(*s.iter()));
            }
        }
        return c;
    }

    completion operator()(const for_in_statement& s) {
        completion c{};
        if (s.init().type() == statement_type::expression) {
            auto ev = get_value(eval(s.e()));
            auto o = global_->to_object(ev);
            const auto& lhs_expression = static_cast<const expression_statement&>(s.init()).e();
            for (const auto& n: o->property_names()) {
                if (!put_value(eval(lhs_expression), value{n})) {
                    std::wostringstream woss;
                    woss << lhs_expression << " is not an valid left hand side expression in for in loop";
                    throw eval_exception(stack_trace(lhs_expression.extend()), woss.str());
                }
                c = eval(s.s());
                if (c.type == completion_type::break_) {
                    break;
                } else if (c.type == completion_type::return_) {
                    return c;
                }
                assert(c.type == completion_type::normal || c.type == completion_type::continue_);
            }
        } else {
            assert(s.init().type() == statement_type::variable);
            const auto& var_statement = static_cast<const variable_statement&>(s.init());
            assert(var_statement.l().size() == 1);
            const auto& init = var_statement.l()[0];

            auto assign = [&](const value& val) {
                if (!put_value(value{active_scope_->lookup(init.id())}, val)) {
                    // Shouldn't happen (?)
                    NOT_IMPLEMENTED(s);
                }
            };

            assign(init.init() ? get_value(eval(*init.init())) : value::undefined);
            
            // Happens after the initial assignment
            auto ev = get_value(eval(s.e()));
            auto o = global_->to_object(ev);

            for (const auto& n: o->property_names()) {
                assign(value{n});
                c = eval(s.s());
                if (c.type == completion_type::break_) {
                    break;
                } else if (c.type == completion_type::return_) {
                    return c;
                }
                assert(c.type == completion_type::normal || c.type == completion_type::continue_);
            }
        }
        return c;
    }

    completion operator()(const continue_statement&) {
        return completion{completion_type::continue_};
    }

    completion operator()(const break_statement&) {
        return completion{completion_type::break_};
    }

    completion operator()(const return_statement& s) {
        value res{};
        if (s.e()) {
            res = get_value(eval(*s.e()));
        }
        return completion{completion_type::return_, res};
    }

    completion operator()(const with_statement& s) {
        auto val = get_value(eval(s.e()));
        auto_scope with_scope{*this, global_->to_object(val), active_scope_};
        return eval(s.s());
    }

    completion operator()(const function_definition& s) {
        active_scope_->put(string{heap_, s.id()}, value{create_function(s, active_scope_)});
        return completion{};
    }

    completion operator()(const statement& s) {
        NOT_IMPLEMENTED(s);
    }

private:
    class scope;
    using scope_ptr = gc_heap_ptr<scope>;
    class scope {
    public:
        friend gc_type_info_registration<scope>;

#ifndef NDBEUG
        bool has_property(const std::wstring& id) const {
            return activation_.dereference(heap_).has_property(id);
        }
#endif

        reference lookup(const string& id) const {
            if (!prev_ || activation_.dereference(heap_).has_property(id.view())) {
                return reference{activation_.track(heap_), id};
            }
            return prev_.dereference(heap_).lookup(id);
        }

        reference lookup(const std::wstring& id) const {
            return lookup(string{heap_, id});
        }

        void put(const string& key, const value& val) {
            activation_.dereference(heap_).put(key, val);
        }

        const scope* get_prev() const {
            return prev_ ? &prev_.dereference(heap_) : nullptr;
        }

    private:
        explicit scope(const object_ptr& act, const scope_ptr& prev) : heap_(act.heap()), activation_(act), prev_(prev) {}
        scope(scope&&) = default;

        void fixup() {
            activation_.fixup(heap_);
            prev_.fixup(heap_);
        }

        gc_heap& heap_;
        gc_heap_ptr_untracked<object> activation_;
        gc_heap_ptr_untracked<scope>  prev_;
    };
    static_assert(!gc_type_info_registration<scope>::needs_destroy);
    class auto_scope {
    public:
        explicit auto_scope(impl& parent, const object_ptr& act, const scope_ptr& prev) : parent(parent), old_scopes(parent.active_scope_) {
            parent.active_scope_ = act.heap().make<scope>(act, prev);
        }
        ~auto_scope() {
            parent.active_scope_ = old_scopes;
        }

        impl& parent;
        scope_ptr old_scopes;
    };
    class auto_stack_update {
    public:
        explicit auto_stack_update(impl& parent, const source_extend& pos) : parent_(parent)
#ifndef NDEBUG
            , pos_(pos)
#endif
        {
            parent_.stack_trace_.push_back(pos);
        }

        ~auto_stack_update() {
            assert(!parent_.stack_trace_.empty() && parent_.stack_trace_.back() == pos_);
            parent_.stack_trace_.pop_back();
        }
    private:
        impl& parent_;
#ifndef NDEBUG
        source_extend pos_;
#endif
    };

    gc_heap&                       heap_;
    scope_ptr                      active_scope_;
    gc_heap_ptr<global_object>     global_;
    on_statement_executed_type     on_statement_executed_;
    std::vector<source_extend>     stack_trace_;

    static scope_ptr make_scope(const object_ptr& act, const scope_ptr& prev) {
        return act.heap().make<scope>(act, prev);
    }

    std::vector<source_extend> stack_trace(const source_extend& current_extend) const {
        std::vector<source_extend> t;
        t.push_back(current_extend);
        t.insert(t.end(), stack_trace_.rbegin(), stack_trace_.rend());
        return t;
    }

    std::vector<value> eval_argument_list(const expression_list& es) {
        std::vector<value> args;
        for (const auto& e: es) {
            args.push_back(get_value(eval(*e)));
        }
        return args;
    }

    value handle_new_expression(const expression& e) {
        value o;
        std::vector<value> args;
        if (e.type() == expression_type::call) {
            const auto& ce = static_cast<const call_expression&>(e);
            o = eval(ce.member());
            args = eval_argument_list(ce.arguments());
        } else {
            o = eval(e);
        }
        o = get_value(o);
        if (o.type() != value_type::object) {
            std::wostringstream woss;
            woss << e << " is not an object";
            throw eval_exception(stack_trace(e.extend()), woss.str());
        }
        auto c = o.object_value()->construct_function();
        if (!c) {
            std::wostringstream woss;
            woss << e << " is not constructable";
            throw eval_exception(stack_trace(e.extend()), woss.str());
        }

        auto_stack_update asu{*this, e.extend()};
        return c->call(value::undefined, args);
    }

    object_ptr create_function(const string& id, const std::shared_ptr<block_statement>& block, const std::vector<std::wstring>& param_names, const std::wstring& body_text, const scope_ptr& prev_scope) {
        // §15.3.2.1
        auto callee = global_->make_raw_function();
        auto func = [this, block, param_names, prev_scope, callee, ids = hoisting_visitor::scan(*block)](const value& this_, const std::vector<value>& args) {
            // Arguments array
            auto as = object::make(heap_, string{heap_, "Object"}, global_->object_prototype());
            as->put(string{heap_, "callee"}, value{callee}, property_attribute::dont_enum);
            as->put(string{heap_, "length"}, value{static_cast<double>(args.size())}, property_attribute::dont_enum);
            for (uint32_t i = 0; i < args.size(); ++i) {
                as->put(string{heap_, index_string(i)}, args[i], property_attribute::dont_enum);
            }

            // Scope
            auto activation = object::make(heap_, string{heap_, "Activation"}, nullptr);
            auto_scope auto_scope_{*this, activation, prev_scope};
            activation->put(string{heap_, "this"}, this_, property_attribute::dont_delete | property_attribute::dont_enum | property_attribute::read_only);
            activation->put(string{heap_, "arguments"}, value{as}, property_attribute::dont_delete);
            for (size_t i = 0; i < param_names.size(); ++i) {
                activation->put(string{heap_, param_names[i]}, i < args.size() ? args[i] : value::undefined);
            }
            // Variables
            for (const auto& id: ids) {
                assert(!activation->has_property(id)); // TODO: Handle this..
                activation->put(string{heap_, id}, value::undefined);
            }
            return eval(*block).result;
        };
        global_->put_function(callee, gc_function::make(heap_, func), string{heap_, L"function " + std::wstring{id.view()} + body_text}, static_cast<int>(param_names.size()));

        callee->construct_function(gc_function::make(heap_, [global = global_, callee, id](const value& this_, const std::vector<value>& args) {
            assert(this_.type() == value_type::undefined); (void)this_; // [[maybe_unused]] not working with MSVC here?
            assert(!id.view().empty());
            auto p = callee->get(L"prototype");
            auto o = value{object::make(global->heap(), id, p.type() == value_type::object ? p.object_value() : global->object_prototype())};
            auto r = callee->call_function()->call(o, args);
            return r.type() == value_type::object ? r : value{o};
        }));

        return callee;
    }

    object_ptr create_function(const function_definition& s, const scope_ptr& prev_scope) {
        return create_function(string{heap_, s.id()}, s.block_ptr(), s.params(), std::wstring{s.body_extend().source_view()}, prev_scope);
    }
};

interpreter::interpreter(gc_heap& h, const block_statement& program, const on_statement_executed_type& on_statement_executed) : impl_(new impl{h, program, on_statement_executed}) {
}

interpreter::~interpreter() = default;

value interpreter::eval(const expression& e) {
    return impl_->eval(e);

}

completion interpreter::eval(const statement& s) {
    return impl_->eval(s);
}

} // namespace mjs
