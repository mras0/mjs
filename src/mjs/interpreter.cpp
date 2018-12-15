#include "interpreter.h"
#include "parser.h"
#include "global_object.h"
#include "native_object.h"
#include "array_object.h"
#include "function_object.h"
#include "printer.h"

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
    case completion_type::throw_: return os << "Throw";
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

    void operator()(const do_statement& s) {
        accept(s.s(), *this);
    }

    void operator()(const while_statement& s) {
        accept(s.s(), *this);
    }

    void operator()(const for_statement& s) {
        if (s.init()) accept(*s.init(), *this);
        accept(s.s(), *this);
    }

    void operator()(const for_in_statement& s) {
        accept(s.init(), *this);
        accept(s.s(), *this);
    }

    void operator()(const continue_statement&) {}
    void operator()(const break_statement&) {}
    void operator()(const return_statement&) {}
    void operator()(const with_statement& s) {
        accept(s.s(), *this);
    }

    void operator()(const labelled_statement& s) {
        accept(s.s(), *this);
    }

    void operator()(const switch_statement& s) {
        for (const auto& c: s.cl()) {
            for (const auto& cs: c.sl()) {
                accept(*cs, *this);
            }
        }
    }

    void operator()(const try_statement& s) {
        accept(s.block(), *this);
        if (auto c = s.catch_block()) {
            accept(*c, *this);
        }
        if (auto f = s.finally_block()) {
            accept(*f, *this);
        }
    }

    void operator()(const throw_statement&) {}

    void operator()(const function_definition& f) {
        ids_.push_back(f.id());
    }

    void operator()(const statement& s) {
        std::wostringstream woss;
        print(woss, s);
        NOT_IMPLEMENTED(woss.str());
    }

private:
    explicit hoisting_visitor() {}
    std::vector<std::wstring> ids_;
};

class activation_object : public object {
public:
    object_ptr arguments() const { return arguments_.track(heap()); }

    value get(const std::wstring_view& name) const override {
        if (auto p = find(name)) {
            auto& h = heap();
            return arguments_.dereference(h).get(p->index_string.dereference(h).view());
        }
        return object::get(name);
    }

    void put(const string& name, const value& val, property_attribute attr) override {
        if (auto p = find(name.view())) {
            auto& h = heap();
            arguments_.dereference(h).put(p->index_string.track(h), val, attr);
            return;
        }
        object::put(name, val, attr);
    }

private:
    struct param {
        gc_heap_ptr_untracked<gc_string> key;
        gc_heap_ptr_untracked<gc_string> index_string;

        explicit param(const string& k, const string& is) : key(k.unsafe_raw_get()), index_string(is.unsafe_raw_get()) {}

        void fixup(gc_heap& h) {
            key.fixup(h);
            index_string.fixup(h);
        }
    };

    param* find(const std::wstring_view& s) const {
        if (!params_) {
            return nullptr;
        }
        auto& h = heap();
        for (auto& p: params_.dereference(h)) {
            if (p.key.dereference(h).view() == s) {
                return &p;
            }
        }
        return nullptr;
    }

    friend gc_type_info_registration<activation_object>;
    gc_heap_ptr_untracked<object> arguments_;
    gc_heap_ptr_untracked<gc_vector<param>> params_;

    explicit activation_object(global_object& global, const std::vector<std::wstring>& param_names, const std::vector<value>& args)
        : object(global.common_string("Activation"), global.object_prototype()) {

        if (!param_names.empty()) {
            params_ = gc_vector<param>::make(heap(), static_cast<uint32_t>(param_names.size()));
        }

        // Arguments array
        auto as = global.make_object();
        arguments_ = as;
        as->put(global.common_string("length"), value{static_cast<double>(args.size())}, property_attribute::dont_enum);
        for (uint32_t i = 0; i < args.size(); ++i) {
            string is{heap(), index_string(i)};
            as->put(is, args[i], property_attribute::dont_enum);

            //
            // Handle the (ugly) fact that the arguments array aliases the parameters
            //
            if (i < param_names.size()) {
                params_.dereference(heap()).emplace_back(string{heap(), param_names[i]}, is);
            }
        }

        object::put(global.common_string("arguments"), value{as}, property_attribute::dont_delete);

        // Add placeholders (see note above)
        for (size_t i = 0; i < param_names.size(); ++i) {
            object::put(string{heap(), param_names[i]}, value::undefined, property_attribute::dont_delete);
        }
    }

    void fixup() {
        auto& h = heap();
        arguments_.fixup(h);
        params_.fixup(h);
        object::fixup();
    }
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
    explicit impl(gc_heap& h, version ver, const block_statement& program, const on_statement_executed_type& on_statement_executed) : heap_(h), program_(program), global_(global_object::make(h, ver)), on_statement_executed_(on_statement_executed) {
        assert(!global_->has_property(L"eval"));

        put_native_function(*global_, global_, "eval", [this](const value&, const std::vector<value>& args) {
            if (args.empty()) {
                return value::undefined;
            } else if (args.front().type() != value_type::string) {
                return args.front();
            }
            auto bs = parse(std::make_shared<source_file>(L"eval", args.front().string_value().view()), global_->language_version());
            return top_level_eval(*bs);
        }, 1);

        auto func_obj = global_->get(L"Function").object_value();
        assert(func_obj.has_type<function_object>());
        static_cast<function_object&>(*func_obj).put_function([this](const value&, const std::vector<value>& args) {
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

            auto bs = parse(std::make_shared<source_file>(L"Function definition", L"function anonymous(" + p + L") {\n" + body + L"\n}"), global_->language_version());
            if (bs->l().size() != 1 || bs->l().front()->type() != statement_type::function_definition) {
                NOT_IMPLEMENTED("Invalid function definition: " << bs->extend().source_view());
            }

            return value{create_function(static_cast<const function_definition&>(*bs->l().front()), make_scope(global_, nullptr))};
        }, func_obj->class_name().unsafe_raw_get(), nullptr, 1);
        static_cast<function_object&>(*func_obj).default_construct_function();


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
        if (!gc_cooldown_) {
            if (heap_.use_percentage() > 90) {
                heap_.garbage_collect();
                gc_cooldown_ = 1000; // Arbitrary, but avoid collecting all the time when close to full
            }
        } else {
            --gc_cooldown_;
        }

        if (on_statement_executed_) {
            auto res = accept(s, *this);
            on_statement_executed_(s, res);
            return res;
        } else {
            return accept(s, *this);
        }
    }

    value top_level_eval(const statement& s, bool allow_return = true) {
        auto c = eval(s);
        if (!c) {
            return c.result;
        }
        if (allow_return && c.type == completion_type::return_) {
            return c.result;
        }
        std::wostringstream woss;
        woss << "Top level evaluation resulted in abrupt termination: " << c;
        throw eval_exception(stack_trace(s.extend()), woss.str());
    }

    value eval_program() {
        return top_level_eval(program_, false);
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

    value operator()(const array_literal_expression& e) {
        auto a = make_array(global_->array_prototype(), {});
        const auto& es = e.elements();
        for (unsigned i = 0; i < es.size(); ++i) {
            if (es[i]) {
                auto val = get_value(eval(*es[i]));
                a->put(string{heap_, index_string(i)}, val);
            } else {
                a->put(string{heap_, index_string(i)}, value::undefined);
            }
        }
        return value{a};
    }

    value operator()(const object_literal_expression& e) {
        auto o = global_->make_object();
        for (const auto& i : e.elements()) {
            auto v = get_value(eval(*i.second));
            switch (i.first->type()) {
            case expression_type::identifier:
                o->put(string{heap_, static_cast<const identifier_expression&>(*i.first).id()}, v);
                break;
            case expression_type::literal:
                {
                    const auto& le = static_cast<const literal_expression&>(*i.first);
                    if (le.t().type() == token_type::string_literal) {
                        o->put(string{heap_, le.t().text()}, v);
                        break;
                    } else if (le.t().type() == token_type::numeric_literal) {
                        o->put(to_string(heap_, le.t().dvalue()), v);
                        break;
                    }
                }
                [[fallthrough]];
            default:
                NOT_IMPLEMENTED(*i.first);
            }
        }
        return value{o};
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
            if (auto o = member.reference_value().base(); !o.has_type<activation_object>()) {
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

    static bool compare_strict_equal(const value& l, const value& r) {
        if (l.type() != r.type()) return false;
        if (l.type() == value_type::undefined) return true;
        if (l.type() == value_type::null) return true;
        if (l.type() == value_type::number) {
            const auto x = l.number_value(), y = r.number_value();
            if (std::isnan(x) || std::isnan(y)) return false;
            return x==y || (x==0.0 && y == 0.0);
        } else if (l.type() == value_type::string) {
            return l.string_value() == r.string_value();
        } else if (l.type() == value_type::boolean) {
            return l.boolean_value() == r.boolean_value();
        }
        assert(l.type() == value_type::object);
        if (l.object_value().get() == r.object_value().get()) {
            return true;
        }
        // TODO: Return true if they refer to objects joined to each other (see §13.1.2).
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
        } else if (op == token_type::equalequalequal || op == token_type::notequalequal) {
            const bool eq = compare_strict_equal(l ,r);
            return value{op == token_type::equalequalequal ? eq : !eq};
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
        } else if (e.op() == token_type::dot || e.op() == token_type::lbracket) {
            return value{reference{global_->to_object(l), to_string(heap_, r)}};
        } else if (e.op() == token_type::in_) {
            if (r.type() != value_type::object) {
                // TODO: Throw TypeError exception
                NOT_IMPLEMENTED(to_string(heap_, r));
            }
            return value{r.object_value()->has_property(to_string(heap_, l).view())};
        } else if (e.op() == token_type::instanceof_) {
            if (r.type() != value_type::object || r.object_value()->prototype().get() != global_->function_prototype().get()) {
                // TODO: Throw TypeError exception
                NOT_IMPLEMENTED(to_string(heap_, r));
            }
            // ES3, 15.3.5.3 [[HasInstance]] (only implemented for Function objects)
            if (l.type() != value_type::object) {
                return value{false};
            }
            auto o = r.object_value()->get(L"prototype");
            if (o.type() != value_type::object) {
                // TODO: Throw TypeError exception
                NOT_IMPLEMENTED(to_string(heap_, o));
            }
            auto v = l.object_value()->prototype();
            if (!v) {
                return value{false};
            }
            return value{v.get() == o.object_value().get()}; // TODO: handle joined objects (ES3, 13.1.2)

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

    value operator()(const function_expression& e) {
        // string{heap_, s.id()}
        return value{create_function(e, active_scope_)};
    }

    value operator()(const expression& e) {
        std::wostringstream woss;
        print(woss, e);
        NOT_IMPLEMENTED(woss.str());
    }

    //
    // Statements
    //

    label_set get_labels(const statement& current_statement) {
        label_set ls;
        if (&current_statement == labels_valid_for_) {
            std::swap(ls, label_set_);
        } else {
            label_set_.clear();
        }
        labels_valid_for_ = nullptr;
        return ls;
    }

    static bool handle_completion(completion& c, const label_set& labels) {
        if (c.type == completion_type::break_) {
            assert(c.has_target());
            if (c.in_set(labels)) {
                c = completion{};
            }
            return true;
        } else if (c.type == completion_type::return_) {
            return true;
        } else if (c.type == completion_type::continue_) {
            assert(c.has_target());
            if (!c.in_set(labels)) {
                return true;
            }
            c = completion{};
        } else {
            assert(c.type == completion_type::normal);
        }
        return false;
    }

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
        return completion{get_value(eval(s.e()))};
    }

    completion operator()(const if_statement& s) {
        if (to_boolean(get_value(eval(s.cond())))) {
            return eval(s.if_s());
        } else if (auto e = s.else_s()) {
            return eval(*e);
        }
        return completion{};
    }

    completion operator()(const do_statement& s) {
        const auto ls = get_labels(s);
        completion c{};
        do {
            c = eval(s.s());
            if (handle_completion(c, ls)) {
                return c;
            }
        } while (to_boolean(get_value(eval(s.cond()))));
        assert(!c);
        return c;//completion{c.result};
    }

    completion operator()(const while_statement& s) {
        const auto ls = get_labels(s);
        while (to_boolean(get_value(eval(s.cond())))) {
            auto c = eval(s.s());
            if (handle_completion(c, ls)) {
                return c;
            }
        }
        return completion{};
    }

    // TODO: Reduce code duplication in handling of for/for in statements

    completion operator()(const for_statement& s) {
        const auto ls = get_labels(s);
        if (auto is = s.init()) {
            auto c = eval(*is);
            assert(!c); // Expect normal completion
            (void)get_value(c.result);
        }
        completion c{};
        while (!s.cond() || to_boolean(get_value(eval(*s.cond())))) {
            c = eval(s.s());

            if (handle_completion(c, ls)) {
                return c;
            }

            if (s.iter()) {
                (void)get_value(eval(*s.iter()));
            }
        }
        return c;
    }

    completion operator()(const for_in_statement& s) {
        const auto ls = get_labels(s);
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
                if (handle_completion(c, ls)) {
                    return c;
                }
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
                if (handle_completion(c, ls)) {
                    return c;
                }
            }
        }
        return c;
    }

    completion operator()(const continue_statement& s) {
        return completion{completion_type::continue_, s.id()};
    }

    completion operator()(const break_statement& s) {
        return completion{completion_type::break_, s.id()};
    }

    completion operator()(const return_statement& s) {
        value res{};
        if (s.e()) {
            res = get_value(eval(*s.e()));
        }
        return completion{res, completion_type::return_};
    }

    completion operator()(const with_statement& s) {
        auto val = get_value(eval(s.e()));
        auto_scope with_scope{*this, global_->to_object(val), active_scope_};
        return eval(s.s());
    }

    completion operator()(const labelled_statement& s) {
        if (labels_valid_for_ != &s) {
            label_set_.clear();
        }
        if (std::find(label_set_.begin(), label_set_.end(), s.id()) != label_set_.end()) {
            std::wostringstream woss;
            woss << "Duplicate label " << s.id();
            throw eval_exception(stack_trace(s.extend()), woss.str());
        }
        label_set_.push_back(s.id());
        labels_valid_for_ = &s.s();
        return eval(s.s());
    }

    completion operator()(const switch_statement& s) {
        const auto ls = get_labels(s);
        auto to_run = s.default_clause(); // Unless we find a match, we'll run the default caluse (if it exists)
        const auto switch_val = get_value(eval(s.e())); // Evaluate the switch value
        for (auto it = s.cl().begin(), e = s.cl().end(); it != e; ++it) {
            const auto& clause_e = it->e();
            if (!clause_e) {
                // This is the default clause, but we're not ready to run it yet
                continue;
            }
            const auto clause_val = get_value(eval(*clause_e));
            if (!compare_strict_equal(switch_val, clause_val)) {
                continue;
            }
            // We found a match, run it
            to_run = it;
            break;
        }

        // We've decided which cases need to be run (if any), process them
        completion c{};
        for (auto end = s.cl().end(); to_run != end; ++to_run) {
            for (const auto& cs: to_run->sl()) {
                c = eval(*cs);
                if (c) {
                    if (c.type == completion_type::break_ && c.in_set(ls)) {
                        return completion{c.result};
                    }
                    return c;
                }
            }
        }
        return c;
    }

    completion operator()(const throw_statement& s) {
        return completion{get_value(eval(s.e())), completion_type::throw_};
    }

    completion operator()(const try_statement& s) {
        auto c = eval(s.block());
        if (c.type != completion_type::throw_) {
            // Nothing to do
        } else if (auto catch_ = s.catch_block()) {
            auto op = global_->object_prototype();
            auto o = heap_.make<object>(op->class_name(), op);
            o->put(string{heap_, s.catch_id()}, c.result, property_attribute::dont_delete);
            auto_scope catch_scope{*this, o, active_scope_};
            c = eval(*catch_);
        }
        if (auto finally_ = s.finally_block()) {
            auto fc = eval(*finally_);
            return fc ? fc : c;
        }
        return c;
    }

    completion operator()(const function_definition& s) {
        active_scope_->put(string{heap_, s.id()}, value{create_function(s, active_scope_)});
        return completion{};
    }

    completion operator()(const statement& s) {
        std::wostringstream woss;
        print(woss, s);
        NOT_IMPLEMENTED(woss.str());
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
    const block_statement&         program_;
    scope_ptr                      active_scope_;
    gc_heap_ptr<global_object>     global_;
    on_statement_executed_type     on_statement_executed_;
    std::vector<source_extend>     stack_trace_;
    int                            gc_cooldown_ = 0;
    label_set                      label_set_;
    const statement*               labels_valid_for_ = nullptr;

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
        auto callee = make_raw_function(*global_);
        auto func = [this, block, param_names, prev_scope, callee, id, ids = hoisting_visitor::scan(*block)](const value& this_, const std::vector<value>& args) {
            // Scope
            auto activation = heap_.make<activation_object>(*global_, param_names, args);
            activation->put(global_->common_string("this"), this_, property_attribute::dont_delete | property_attribute::dont_enum | property_attribute::read_only);
            activation->arguments()->put(global_->common_string("callee"), value{callee}, property_attribute::dont_enum);
            // Variables
            for (const auto& var_id: ids) {
                assert(!activation->has_property(var_id)); // TODO: Handle this..
                activation->put(string{heap_, var_id}, value::undefined, property_attribute::dont_delete);
            }
            if (!id.view().empty()) {
                // Add name of function to activation record even if it's not necessary for function_definition
                // It's needed for function expressions since `a=function x() { x(...); }` is legal, but x isn't added to the containing scope
                // TODO: Should actually be done a separate scope object (See ES3, 13)
                assert(!activation->has_property(id.view())); // TODO: Handle this..
                activation->put(id, value{callee}, property_attribute::dont_enum);
            }
            auto_scope auto_scope_{*this, activation, prev_scope};
            return top_level_eval(*block);
        };
        callee->put_function(func, nullptr, string{heap_, L"function " + std::wstring{id.view()} + body_text}.unsafe_raw_get(), static_cast<int>(param_names.size()));

        callee->construct_function(gc_function::make(heap_, [global = global_, callee, id](const value& this_, const std::vector<value>& args) {
            assert(this_.type() == value_type::undefined); (void)this_; // [[maybe_unused]] not working with MSVC here?
            assert(!id.view().empty());
            auto p = callee->get(L"prototype");
            auto o = value{global->heap().make<object>(id, p.type() == value_type::object ? p.object_value() : global->object_prototype())};
            auto r = callee->call_function()->call(o, args);
            return r.type() == value_type::object ? r : value{o};
        }));

        return callee;
    }

    object_ptr create_function(const function_base& f, const scope_ptr& prev_scope) {
        return create_function(string{heap_, f.id()}, f.block_ptr(), f.params(), std::wstring{f.body_extend().source_view()}, prev_scope);
    }
};

interpreter::interpreter(gc_heap& h, version ver, const block_statement& program, const on_statement_executed_type& on_statement_executed) : impl_(new impl{h, ver, program, on_statement_executed}) {
}

interpreter::~interpreter() = default;

value interpreter::eval(const expression& e) {
    return impl_->eval(e);

}

completion interpreter::eval(const statement& s) {
    return impl_->eval(s);
}

value interpreter::eval_program() {
    return impl_->eval_program();
}

} // namespace mjs
