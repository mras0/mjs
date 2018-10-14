#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cassert>

#include "value.h"
#include "parser.h"

auto make_runtime_error(const std::wstring_view& s) {
    return std::runtime_error(std::string(s.begin(), s.end()));
}

#define NOT_IMPLEMENTED(o) do { std::wostringstream woss; woss << "Not implemented: " << o << " in " << __FUNCTION__ << " " << __FILE__ << ":" << __LINE__; throw make_runtime_error(woss.str()); } while (0) 

class print_visitor {
public:
    explicit print_visitor(std::wostream& os) : os_(os) {}

    //
    // Expressions
    //

    void operator()(const mjs::identifier_expression& e) {
        std::wcout << e.id();
    }

    void operator()(const mjs::literal_expression& e) {
        switch (e.t().type()) {
        case mjs::token_type::numeric_literal:
            os_ << e.t().dvalue();
            return;
        case mjs::token_type::string_literal:
            os_ << '"' << cpp_quote(e.t().text()) << '"';
            return;
        default:
            NOT_IMPLEMENTED(e);
        }
    }
    void operator()(const mjs::call_expression& e) {
        accept(e.member(), *this);
        os_ << '(';
        const auto& args = e.arguments();
        for (size_t i = 0; i < args.size(); ++i) {
            os_ << (i ? ", " : "");
            accept(*args[i], *this);
        }
        os_ << ')';
    }

    void operator()(const mjs::prefix_expression& e) {
        switch (e.op()) {
        case mjs::token_type::delete_: os_ << "delete "; break;
        case mjs::token_type::typeof_: os_ << "typeof "; break;
        case mjs::token_type::void_:   os_ << "void "; break;
        default:                       os_ << op_text(e.op());
        }
        accept(e.e(), *this);
    }

    void operator()(const mjs::postfix_expression& e) {
        accept(e.e(), *this);
        os_ << op_text(e.op());
    }

    void operator()(const mjs::binary_expression& e) {
        const int precedence = mjs::operator_precedence(e.op());
        print_with_parentheses(e.lhs(), precedence);
        os_ << op_text(e.op());
        print_with_parentheses(e.rhs(), precedence);
    }

    void operator()(const mjs::conditional_expression& e) {
        accept(e.cond(), *this);
        os_ << " ? ";
        accept(e.lhs(), *this);
        os_ << " : ";
        accept(e.rhs(), *this);
    }

    void operator()(const mjs::expression& e) {
        NOT_IMPLEMENTED(e);
    }

    //
    // Statements
    //

    void operator()(const mjs::block_statement& s) {
        os_ << '{';
        for (const auto& bs: s.l()) {
            accept(*bs, *this);
        }
        os_ << '}';
    }

    void operator()(const mjs::variable_statement& s) {
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

    void operator()(const mjs::empty_statement&) {
        os_ << ';';
    }

    void operator()(const mjs::if_statement& s) {
        os_ << "if ("; 
        accept(s.cond(), *this);
        os_ << ") ";
        accept(s.if_s(), *this);
        if (auto e = s.else_s()) {
            os_ << " else ";
            accept(*e, *this);
        }
    }
    
    void operator()(const mjs::while_statement& s) {
        os_ << "while (";
        accept(s.cond(), *this);
        os_ << ") ";
        accept(s.s(), *this);
    }
    
    void operator()(const mjs::for_statement& s) {
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

    //void operator()(const mjs::for_in_statement&){}

    void operator()(const mjs::expression_statement& s) {
        accept(s.e(), *this);
        os_ << ';';
    }

    void operator()(const mjs::continue_statement&) {
        os_ << "continue;";
    }

    void operator()(const mjs::break_statement&) {
        os_ << "break;";
    }

    void operator()(const mjs::return_statement& s) {
        os_ << "return";
        if (s.e()) {
            os_ << " ";
            accept(*s.e(), *this);
        }
        os_ << ';';
    }

    void operator()(const mjs::function_definition& s) {
        os_ << "function " << s.id() << "(";
        for (size_t i = 0; i < s.params().size(); ++i) {
            os_ << (i?", ":"") << s.params()[i];
        }
        os_ << ")";
        (*this)(s.block());
    }

    void operator()(const mjs::statement& s) {
        NOT_IMPLEMENTED(s);
    }

private:
    std::wostream& os_;

    void print_with_parentheses(const mjs::expression& e, int outer_precedence) {
        const int inner_precedence = e.type() == mjs::expression_type::binary ? mjs::operator_precedence(static_cast<const mjs::binary_expression&>(e).op()) : 0;
        if (inner_precedence > outer_precedence) os_ << '(';
        accept(e, *this);
        if (inner_precedence > outer_precedence) os_ << ')';
    }
};

class hoisting_visitor {
public:
    static std::vector<mjs::string> scan(const mjs::block_statement& bs) {
        hoisting_visitor hv{};
        hv(bs);
        return hv.ids_;
    }

    void operator()(const mjs::block_statement& s) {
        for (const auto& bs: s.l()) {
            accept(*bs, *this);
        }
    }
 
    void operator()(const mjs::variable_statement& s) {
        for (const auto& d: s.l()) {
            ids_.push_back(d.id());
        }
    }
 
    void operator()(const mjs::empty_statement&) {}
    
    void operator()(const mjs::expression_statement&){}
    
    void operator()(const mjs::if_statement& s) {
        accept(s.if_s(), *this);
        if (auto e = s.else_s()) {
            accept(*e, *this);
        }
    }

    void operator()(const mjs::while_statement& s){
        accept(s.s(), *this);
    }

    void operator()(const mjs::for_statement& s){
        if (s.init()) accept(*s.init(), *this);
        accept(s.s(), *this);
    }

    //void operator()(const mjs::for_in_statement&){}
    void operator()(const mjs::continue_statement&){}
    void operator()(const mjs::break_statement&){}
    void operator()(const mjs::return_statement&){}
    //void operator()(const mjs::with_statement&){}

    void operator()(const mjs::function_definition& s) {
        assert(!s.id().view().empty());
        ids_.push_back(s.id());
    }

    void operator()(const mjs::statement& s) {
        NOT_IMPLEMENTED(s);
    }

private:
    explicit hoisting_visitor() {}
    std::vector<mjs::string> ids_;
};

auto make_function_object() {
    return mjs::object::make(mjs::string{"Function"}, nullptr); // TODO
}

auto make_function(const mjs::native_function_type& f) {
    auto o = make_function_object();
    o->call_function(f);
    return o;
}

template<typename F>
void set_function(const mjs::object_ptr& o, const mjs::string& name, const F& f) {
    o->put(name, mjs::value{make_function(f)});
}

auto make_arguments_array(const std::vector<mjs::value>& args, const mjs::object_ptr& callee) {
    assert(callee->class_name().str() == L"Function");
    auto as = mjs::object::make(mjs::string{"Object"}, nullptr /*Object prototype*/); // TODO: See §10.1.8
    as->put(mjs::string{"callee"}, mjs::value{callee});
    as->put(mjs::string{"length"}, mjs::value{static_cast<double>(args.size())}, mjs::property_attribute::dont_enum);
    for (size_t i = 0; i < args.size(); ++i) {
        as->put(mjs::to_string(mjs::value{static_cast<double>(i)}), args[i], mjs::property_attribute::dont_enum);
    }
    return as;
}

enum class completion_type {
    normal, break_, continue_, return_
};
std::wostream& operator<<(std::wostream& os, const completion_type& t) {
    switch (t) {
    case completion_type::normal: return os << "Normal completion";
    case completion_type::break_: return os << "Break";
    case completion_type::continue_: return os << "Continue";
    case completion_type::return_: return os << "Return";
    }
    NOT_IMPLEMENTED((int)t);
}

struct completion {
    completion_type type;
    mjs::value result;

    explicit completion(completion_type t = completion_type::normal, const mjs::value& r = mjs::value::undefined) : type(t), result(r) {}

    explicit operator bool() const { return type != completion_type::normal; }
};

std::wostream& operator<<(std::wostream& os, const completion& c) {
    return os << c.type << " " << c.result;
}


class eval_visitor {
public:
    explicit eval_visitor(const mjs::object_ptr& global) {
        scopes_.reset(new scope{global, nullptr});
    }

    ~eval_visitor() {
        assert(scopes_ && !scopes_->prev);
    }

    mjs::value operator()(const mjs::identifier_expression& e) {
        // §10.1.4 
        return mjs::value{scopes_->lookup(e.id())};
    }

    mjs::value operator()(const mjs::literal_expression& e) {
         switch (e.t().type()) {
         case mjs::token_type::numeric_literal: return mjs::value{e.t().dvalue()};
         case mjs::token_type::string_literal:  return mjs::value{e.t().text()};
         default: NOT_IMPLEMENTED(e);
         }
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
        return c(mjs::value{}, args);
    }

    mjs::value operator()(const mjs::prefix_expression& e) {
        auto u = accept(e.e(), *this);
        if (e.op() == mjs::token_type::delete_) {
            if (u.type() != mjs::value_type::reference) {
                NOT_IMPLEMENTED(u);
            }
            const auto& base = u.reference_value().base();
            const auto& prop = u.reference_value().property_name();
            if (!base) {
                return mjs::value{true};
            }
            return mjs::value{base->delete_property(prop)};
        } else if (e.op() == mjs::token_type::void_) {
            (void)get_value(u);
            return mjs::value::undefined;
        } else if (e.op() == mjs::token_type::typeof_) {
            if (u.type() == mjs::value_type::reference && !u.reference_value().base()) {
                return mjs::value{mjs::string{"undefined"}};
            }
            u = get_value(u);
            switch (u.type()) {
            case mjs::value_type::undefined: return mjs::value{mjs::string{"undefined"}}; 
            case mjs::value_type::null: return mjs::value{mjs::string{"object"}}; 
            case mjs::value_type::boolean: return mjs::value{mjs::string{"boolean"}}; 
            case mjs::value_type::number: return mjs::value{mjs::string{"number"}}; 
            case mjs::value_type::string: return mjs::value{mjs::string{"string"}}; 
            case mjs::value_type::object: return mjs::value{mjs::string{u.object_value()->call_function() ? "function" : "object"}}; 
            default:
                NOT_IMPLEMENTED(u.type());
            }
        } else if (e.op() == mjs::token_type::plusplus || e.op() == mjs::token_type::minusminus) {
            if (u.type() != mjs::value_type::reference) {
                NOT_IMPLEMENTED(u);
            }
            auto num = to_number(get_value(u)) + (e.op() == mjs::token_type::plusplus ? 1 : -1);
            if (!put_value(u, mjs::value{num})) {
                NOT_IMPLEMENTED(u);
            }
            return mjs::value{num};
        } else if (e.op() == mjs::token_type::plus) {
            return mjs::value{to_number(get_value(u))};
        } else if (e.op() == mjs::token_type::minus) {
            return mjs::value{-to_number(get_value(u))};
        } else if (e.op() == mjs::token_type::tilde) {
            return mjs::value{static_cast<double>(~to_int32(get_value(u)))};
        } else if (e.op() == mjs::token_type::not_) {
            return mjs::value{!to_boolean(get_value(u))};
        }
        NOT_IMPLEMENTED(e.op());
    }

    mjs::value operator()(const mjs::postfix_expression& e) {
        auto member = accept(e.e(), *this);
        if (member.type() != mjs::value_type::reference) {
            NOT_IMPLEMENTED(e);
        }

        auto orig = to_number(get_value(member));
        auto num = orig;
        switch (e.op()) {
        case mjs::token_type::plusplus:   num += 1; break;
        case mjs::token_type::minusminus: num -= 1; break;
        default: NOT_IMPLEMENTED(e.op());
        }
        if (!put_value(member, mjs::value{num})) {
            NOT_IMPLEMENTED(e);
        }
        return mjs::value{orig};
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
    static bool compare_equal(const mjs::value& l, const mjs::value& r) {
        if (l.type() == r.type()) {
            if (l.type() == mjs::value_type::undefined || l.type() == mjs::value_type::null) {
                return true;
            } else if (l.type() == mjs::value_type::number) {
                const double ln = l.number_value();
                const double rn = r.number_value();
                if (std::isnan(ln) || std::isnan(rn)) {
                    return false;
                }
                if ((ln == 0.0 && rn == 0.0) || ln == rn) {
                    return true;
                }
                return false;
            } else if (l.type() == mjs::value_type::string) {
                return l.string_value() == r.string_value();
            } else if (l.type() == mjs::value_type::boolean) {
                return l.boolean_value() == r.boolean_value();
            }
            assert(l.type() == mjs::value_type::object);
            return l.object_value() == r.object_value();
        }
        // Types are different
        NOT_IMPLEMENTED("compare_equal");
    }

    static mjs::value do_binary_op(const mjs::token_type op, mjs::value& l, mjs::value& r) {
        if (op == mjs::token_type::plus) {
            l = to_primitive(l);
            r = to_primitive(r);
            if (l.type() == mjs::value_type::string || r.type() == mjs::value_type::string) {
                auto ls = to_string(l);
                auto rs = to_string(r);
                return mjs::value{ls + rs};
            }
            // Otherwise handle like the other operators
        } else if (is_relational(op)) {
            l = to_primitive(l, mjs::value_type::number);
            r = to_primitive(r, mjs::value_type::number);
            if (l.type() == mjs::value_type::string && r.type() == mjs::value_type::string) {
                // TODO: See §11.8.5 step 16-21
                NOT_IMPLEMENTED(op);
            }
            const auto ln = to_number(l);
            const auto rn = to_number(r);
            int res;
            switch (op) {
            case mjs::token_type::lt:
                res = tri_compare(ln, rn);
                return mjs::value{res == -1 ? false : static_cast<bool>(res)};
            case mjs::token_type::ltequal:
                res = tri_compare(rn, ln);
                return mjs::value{res == -1 || res == 1 ? false : true};
            case mjs::token_type::gt:
                res = tri_compare(rn, ln);
                return mjs::value{res == -1 ? false : static_cast<bool>(res)};
            case mjs::token_type::gtequal:
                res = tri_compare(ln, rn);
                return mjs::value{res == -1 || res == 1 ? false : true};
            default: NOT_IMPLEMENTED(op);
            }
        } else if (op == mjs::token_type::equalequal || op == mjs::token_type::notequal) {
            const bool eq = compare_equal(l ,r);
            return mjs::value{op == mjs::token_type::equalequal ? eq : !eq};
        }

        const auto ln = to_number(l);
        const auto rn = to_number(r);
        switch (op) {
        case mjs::token_type::plus:         return mjs::value{ln + rn};
        case mjs::token_type::minus:        return mjs::value{ln - rn};
        case mjs::token_type::multiply:     return mjs::value{ln * rn};
        case mjs::token_type::divide:       return mjs::value{ln / rn};
        case mjs::token_type::mod:          return mjs::value{std::fmod(ln, rn)};
        case mjs::token_type::lshift:       return mjs::value{static_cast<double>(mjs::to_int32(ln) << (mjs::to_uint32(rn) & 0x1f))};
        case mjs::token_type::rshift:       return mjs::value{static_cast<double>(mjs::to_int32(ln) >> (mjs::to_uint32(rn) & 0x1f))};
        case mjs::token_type::rshiftshift:  return mjs::value{static_cast<double>(mjs::to_uint32(ln) >> (mjs::to_uint32(rn) & 0x1f))};
        case mjs::token_type::and_:         return mjs::value{static_cast<double>(mjs::to_int32(ln) & mjs::to_int32(rn))};
        case mjs::token_type::xor_:         return mjs::value{static_cast<double>(mjs::to_int32(ln) ^ mjs::to_int32(rn))};
        case mjs::token_type::or_:          return mjs::value{static_cast<double>(mjs::to_int32(ln) | mjs::to_int32(rn))};
        default: NOT_IMPLEMENTED(op);
        }
    }

    mjs::value operator()(const mjs::binary_expression& e) {
        if (e.op() == mjs::token_type::comma) {
            (void)get_value(accept(e.lhs(), *this));;
            return get_value(accept(e.rhs(), *this));
        }
        if (operator_precedence(e.op()) == mjs::assignment_precedence) {
            auto l = accept(e.lhs(), *this);
            auto r = get_value(accept(e.rhs(), *this));
            if (e.op() != mjs::token_type::equal) {
                auto lval = get_value(l);
                r = do_binary_op(without_assignment(e.op()), lval, r);
            }
            if (!put_value(l, r)) {
                NOT_IMPLEMENTED(e);
            }
            return r;
        }

        auto l = get_value(accept(e.lhs(), *this));
        if ((e.op() == mjs::token_type::andand && !to_boolean(l)) || (e.op() == mjs::token_type::oror && to_boolean(l))) {
            return l;
        }
        auto r = get_value(accept(e.rhs(), *this));
        if (e.op() == mjs::token_type::andand || e.op() == mjs::token_type::oror) {
            return r;
        }
        return do_binary_op(e.op(), l, r);
    }

    mjs::value operator()(const mjs::conditional_expression& e) {
        if (to_boolean(get_value(accept(e.cond(), *this)))) {
            return get_value(accept(e.lhs(), *this));
        } else {
            return get_value(accept(e.rhs(), *this));
        }
    }

    mjs::value operator()(const mjs::expression& e) {
        NOT_IMPLEMENTED(e);
    }

    //
    // Statements
    //

    completion operator()(const mjs::block_statement& s) {
        for (const auto& bs: s.l()) {
            if (auto c = accept(*bs, *this)) {
                return c;
            }
        }
        return completion{};
    }

    completion operator()(const mjs::variable_statement& s) {
        for (const auto& d: s.l()) {
            assert(scopes_->activation->has_property(d.id()));
            if (d.init()) {
                scopes_->activation->put(d.id(), accept(*d.init(), *this));
            }
        }
        return completion{};
    }

    completion operator()(const mjs::empty_statement&) {
        return completion{};
    }

    completion operator()(const mjs::expression_statement& s) {
        return completion{completion_type::normal, get_value(accept(s.e(), *this))};
    }

    completion operator()(const mjs::if_statement& s) {
        if (to_boolean(get_value(accept(s.cond(), *this)))) {
            return accept(s.if_s(), *this);
        } else if (auto e = s.else_s()) {
            return accept(*e, *this);
        }
        return completion{};
    }

    completion operator()(const mjs::while_statement& s) {
        while (to_boolean(get_value(accept(s.cond(), *this)))) {
            auto c = accept(s.s(), *this);
            if (c.type == completion_type::break_) {
                return completion{};
            } else if (c.type == completion_type::return_) {
                return c;
            }
            assert(c.type == completion_type::normal || c.type == completion_type::continue_);
        }
        return completion{};
    }
    
    completion operator()(const mjs::for_statement& s) {
        if (auto is = s.init()) {
            auto c = accept(*is, *this);
            assert(!c); // Expect normal completion
            (void)get_value(c.result);
        }
        while (!s.cond() || to_boolean(get_value(accept(*s.cond(), *this)))) {
            auto c = accept(s.s(), *this);
            if (c.type == completion_type::break_) {
                return completion{};
            } else if (c.type == completion_type::return_) {
                return c;
            }
            assert(c.type == completion_type::normal || c.type == completion_type::continue_);

            if (s.iter()) {
                (void)get_value(accept(*s.iter(), *this));
            }
        }
        return completion{};
    }

    //completion operator()(const mjs::for_in_statement&) {}

    completion operator()(const mjs::continue_statement&) {
        return completion{completion_type::continue_};
    }

    completion operator()(const mjs::break_statement&) {
        return completion{completion_type::break_};
    }

    completion operator()(const mjs::return_statement& s) {
        mjs::value res{};
        if (s.e()) {
            res = get_value(accept(*s.e(), *this));
        }
        return completion{completion_type::return_, res};
    }

    //completion operator()(const mjs::with_statement&) {}

    completion operator()(const mjs::function_definition& s) {
        auto prev_scope = scopes_;
        auto callee = make_function_object();
        auto func = [&s, this, prev_scope, callee, ids = hoisting_visitor::scan(s.block())](const mjs::value& this_, const std::vector<mjs::value>& args) {
            auto_scope as{*this, prev_scope};
            auto& scope = scopes_->activation;
            //scope->put(mjs::string{"this"}, this_);//TODO:dont delete? dont enum?
            assert(this_.type() == mjs::value_type::undefined);
            scope->put(mjs::string{"arguments"}, mjs::value{make_arguments_array(args, callee)}, mjs::property_attribute::dont_delete);
            for (size_t i = 0; i < std::min(args.size(), s.params().size()); ++i) {
                scope->put(s.params()[i], args[i]);
            }
            for (const auto& id: ids) {
                assert(!scope->has_property(id)); // TODO: Handle this..
                scope->put(id, mjs::value::undefined);
            }
            return accept(s.block(), *this).result;
        };
        callee->call_function(func);
        prev_scope->activation->put(s.id(), mjs::value{callee});
        return completion{};
    }

    completion operator()(const mjs::statement& s) {
        NOT_IMPLEMENTED(s);
    }

private:
    class scope;
    using scope_ptr = std::shared_ptr<scope>;
    class scope {
    public:
        explicit scope(const mjs::object_ptr& act, const scope_ptr& prev) : activation(act), prev(prev) {}

        mjs::reference lookup(const mjs::string& id) const {
            if (!prev || activation->has_property(id)) {
                return mjs::reference{activation, id};
            }
            return prev->lookup(id);
        }

        mjs::object_ptr activation;
        scope_ptr prev;
    };
    class auto_scope {
    public:
        explicit auto_scope(eval_visitor& parent, const scope_ptr& prev) : parent(parent), old_scopes(parent.scopes_) {
            auto activation = mjs::object::make(mjs::string{"Activation"}, nullptr); // TODO
            parent.scopes_.reset(new scope{activation, prev});
        }
        ~auto_scope() {
            parent.scopes_ = old_scopes;
        }

        eval_visitor& parent;
        scope_ptr old_scopes;
    };
    scope_ptr scopes_;
};

auto make_global(const mjs::block_statement& bs) {
    auto global = mjs::object::make(mjs::string{"Object"}, nullptr); // TODO
    set_function(global, mjs::string{"alert"}, [](const mjs::value&, const std::vector<mjs::value>& args) {
        std::wcout << "ALERT";
        if (!args.empty()) std::wcout << ": " << args[0];
        std::wcout << "\n";
        return mjs::value::undefined; 
    });
    for (const auto& id: hoisting_visitor::scan(bs)) {
        global->put(id, mjs::value::undefined);
    }
    return global;
}

void test(const std::wstring_view& text, const mjs::value& expected) {
    std::cout << "Parsing \"" << std::string(text.begin(), text.end()) << "\"..." << std::flush;
    auto bs = mjs::parse(text);
    print_visitor pv{std::wcout}; accept(*bs, pv); std::wcout << '\n';
    auto global = make_global(*bs); eval_visitor ev{global};
    mjs::value res{};
    for (const auto& s: bs->l()) {
        res = accept(*s, ev).result;
    }
    if (res != expected) {
        std::wcout << "Test failed: " << text << " expecting " << expected << " got " << res << "\n";
        assert(false);
    }
}

int main() {
    try {
        test(L"-7.5 % 2", mjs::value{-1.5});
        test(L"1+2*3", mjs::value{7.});
        test(L"x = 42; 'test ' + 2 * (6 - 4 + 1) + ' ' + x", mjs::value{mjs::string{"test 6 42"}});
        test(L"y=1/2; z='string'; y+z", mjs::value{mjs::string{"0.5string"}});
        test(L"var x=2; x++;", mjs::value{2.0});
        test(L"var x=2; x++; x", mjs::value{3.0});
        test(L"var x=2; x--;", mjs::value{2.0});
        test(L"var x=2; x--; x", mjs::value{1.0});
        test(L"var x = 42; delete x; x", mjs::value::undefined);
        test(L"void(2+2)", mjs::value::undefined);
        test(L"typeof(2)", mjs::value{mjs::string{"number"}});
        test(L"x=4.5; ++x", mjs::value{5.5});
        test(L"x=4.5; --x", mjs::value{3.5});
        test(L"x=42; +x;", mjs::value{42.0});
        test(L"x=42; -x;", mjs::value{-42.0});
        test(L"x=42; !x;", mjs::value{false});
        test(L"x=42; ~x;", mjs::value{(double)~42});
        test(L"1<<2", mjs::value{4.0});
        test(L"-5>>2", mjs::value{-2.0});
        test(L"-5>>>2", mjs::value{1073741822.0});
        test(L"1 < 2", mjs::value{true});
        test(L"1 > 2", mjs::value{false});
        test(L"1 <= 2", mjs::value{true});
        test(L"1 >= 2", mjs::value{false});
        test(L"1 == 2", mjs::value{false});
        test(L"1 != 2", mjs::value{true});
        test(L"255 & 128", mjs::value{128.0});
        test(L"255 ^ 128", mjs::value{127.0});
        test(L"64 | 128", mjs::value{192.0});
        test(L"42 || 13", mjs::value{42.0});
        test(L"42 && 13", mjs::value{13.0});
        test(L"1 ? 2 : 3", mjs::value{2.0});
        test(L"0 ? 2 : 1+2", mjs::value{3.0});
        test(L"x=2.5; x+=4; x", mjs::value{6.5});
        test(L"function f(x,y) { return x*x+y; } f(2, 3)", mjs::value{7.0});
        test(L"function f(){ i = 42; }; f(); i", mjs::value{42.0});
        test(L"i = 1; function f(){ var i = 42; }; f(); i", mjs::value{1.0});
        test(L";", mjs::value::undefined);
        test(L"if (1) 2;", mjs::value{2.0});
        test(L"if (0) 2;", mjs::value::undefined);
        test(L"if (0) 2; else ;", mjs::value::undefined);
        test(L"if (0) 2; else 3;", mjs::value{3.0});
        test(L"1,2", mjs::value{2.0});
        test(L"x=5; while(x-3) { x = x - 1; } x", mjs::value{3.0});
        test(L"x=2; y=0; while(1) { if(x) {x = x - 1; y = y + 2; continue; y = y + 1000; } else break; y = y + 1;} y", mjs::value{4.0});
        test(L"var x = 0; for(var i = 10, dec = 1; i; i = i - dec) x = x + i; x", mjs::value{55.0});
        test(L"var x=0; for (i=2; i; i=i-1) x=x+i; x+i", mjs::value{3.0});
        // TODO: for .. in, with
        auto bs = mjs::parse(L";");
        auto global = make_global(*bs);
        eval_visitor e{global};
        print_visitor p{std::wcout};
        for (const auto& s: bs->l()) {
            std::wcout << "> ";
            accept(*s, p);
            std::wcout << "\n";
            std::wcout << accept(*s, e).result << "\n";
        }
    } catch (const std::exception& e) {
        std::wcout << e.what() << "\n";
        return 1;
    }
}
