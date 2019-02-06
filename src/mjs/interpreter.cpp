#include "interpreter.h"
#include "parser.h"
#include "global_object.h"
#include "native_object.h"
#include "array_object.h"
#include "function_object.h"
#include "regexp_object.h"
#include "object_object.h"
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
    case completion_type::normal: return os << "normal completion";
    case completion_type::break_: return os << "break";
    case completion_type::continue_: return os << "continue";
    case completion_type::return_: return os << "return";
    case completion_type::throw_: return os << "throw";
    }
    NOT_IMPLEMENTED((int)t);
}

std::wostream& operator<<(std::wostream& os, const completion& c) {
    return os << c.type << " value type: " << c.result.type();
}

class hoisting_visitor {
public:
    using scan_result = std::tuple<std::vector<std::wstring>, std::vector<const function_definition*>>;

    static scan_result scan(const statement& s) {
        hoisting_visitor hv{};
        accept(s, hv);
        return std::tuple(std::move(hv.ids_), std::move(hv.funcs_));
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

    void operator()(const debugger_statement&) {}

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
        assert(!f.id().empty());
        // For functions it's important that we start out by registering the first encutered definition
        if (std::find(ids_.begin(), ids_.end(), f.id()) == ids_.end()) {
            ids_.push_back(f.id());
            funcs_.push_back(&f);
        }
    }

    void operator()(const statement& s) {
        std::wostringstream woss;
        print(woss, s);
        NOT_IMPLEMENTED(woss.str());
    }

private:
    explicit hoisting_visitor() {}
    std::vector<std::wstring> ids_;
    std::vector<const function_definition*> funcs_;
};

class activation_object : public object {
public:
    static auto make(const gc_heap_ptr<global_object>& global, const std::vector<std::wstring>& param_names, const std::vector<value>& args) {
        return global.heap().make<activation_object>(*global, param_names, args);
    }

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
        const auto ps = params_.dereference(h);
        const auto pdat = ps.data();
        for (uint32_t index = ps.length(); index--;) {
            auto& p = pdat[index];
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

        const auto ver    = global.language_version();
        const bool strict = ver >= version::es5 && global.strict_mode();

        // Arguments array
        auto as = global.make_arguments_array();
        arguments_ = as;
        as->put(global.common_string("length"), value{static_cast<double>(args.size())}, property_attribute::dont_enum);
        for (uint32_t i = 0; i < args.size(); ++i) {
            string is{heap(), index_string(i)};
            as->put(is, args[i], ver >= version::es5 ? property_attribute::none : property_attribute::dont_enum);

            if (i < param_names.size()) {
                if (!strict) {
                    // Handle the (ugly) fact that the arguments array aliases the parameters
                    params_.dereference(heap()).emplace_back(string{heap(), param_names[i]}, is);
                } else {
                    object::put(string{heap(), param_names[i]}, args[i], property_attribute::dont_delete);
                }
            }
        }

        object::put(global.common_string("arguments"), value{as}, property_attribute::dont_delete);

        // Add placeholders (see note above)
        for (size_t i = strict ? args.size() : 0; i < param_names.size(); ++i) {
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

constexpr bool is_reference_op(token_type t) {
    return t == token_type::dot || t == token_type::lbracket;
}

value get_this_arg(const gc_heap_ptr<global_object>& global, const value& this_arg) {
    if (this_arg.type() == value_type::undefined || this_arg.type() == value_type::null) {
        return value{global};
    } else {
        return value{global->to_object(this_arg)};
    }
}

class interpreter::impl {
public:
    explicit impl(gc_heap& h, version ver, const on_statement_executed_type& on_statement_executed)
        : heap_(h)
        , global_(global_object::make(h, ver, strict_mode_))
        , on_statement_executed_(on_statement_executed) {

        global_->set_stack_trace_function([this]() {
            return stack_trace();
        });

        assert(!global_->has_property(L"eval"));
        put_native_function(global_, global_, "eval", [this](const value&, const std::vector<value>& args) {
            // ES3, 15.1.2.1
            if (args.empty()) {
                return value::undefined;
            } else if (args.front().type() != value_type::string) {
                return args.front();
            }
            std::unique_ptr<block_statement> bs;

            try {
                bs = parse(std::make_shared<source_file>(L"eval", args.front().string_value().view(), global_->language_version()), strict_mode_ ? parse_mode::strict : parse_mode::non_strict);
            } catch (const std::exception&) {
                throw native_error_exception{native_error_type::syntax, stack_trace(), L"Invalid argument to eval"};
            }

            std::unique_ptr<auto_scope> eval_scope;
            if (bs->strict_mode()) {
                eval_scope.reset(new auto_scope{*this, activation_object::make(global_, {}, {}), active_scope_});
            }

            const std::unique_ptr<force_global_scope> fgs{!was_direct_call_to_eval_ ? new force_global_scope{*this} : nullptr};
            hoist(*bs);
            auto c = eval(*bs);
            if (!c) {
                return c.result;
            } else if (c.type == completion_type::throw_) {
                assert(c.result.type() == value_type::object); // FIXME should probably support any value type?
                rethrow_error(c.result.object_value());
            }

            std::wostringstream woss;
            woss << "Illegal " << c.type << " statement";
            throw native_error_exception{native_error_type::syntax, stack_trace(), woss.str()};
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

            std::unique_ptr<block_statement> bs;
            try {
                bs = parse(std::make_shared<source_file>(L"Function definition", L"function anonymous(" + p + L") {\n" + body + L"\n}", global_->language_version()), strict_mode_ ? parse_mode::function_constructor_in_strict_context : parse_mode::non_strict);
            } catch (const std::exception&) {
                throw native_error_exception{native_error_type::syntax, stack_trace(), L"Invalid argument to function constructor"};
            }
            if (bs->l().size() != 1 || bs->l().front()->type() != statement_type::function_definition) {
                NOT_IMPLEMENTED("Invalid function definition: " << bs->extend().source_view());
            }

            return value{create_function(static_cast<const function_definition&>(*bs->l().front()), make_scope(global_, nullptr))};
        }, func_obj->class_name().unsafe_raw_get(), nullptr, 1);
        static_cast<function_object&>(*func_obj).default_construct_function();

        global_->function_prototype()->put(global_->common_string("constructor"), value{func_obj}, global_object::default_attributes);

        active_scope_ = make_scope(global_, nullptr);
    }

    ~impl() {
        assert(active_scope_ && !active_scope_->get_prev());
    }

    gc_heap_ptr<global_object> global() const {
        return global_;
    }

    void current_extend(const source_extend& e) {
        current_extend_ = e;
    }

    void hoist(const hoisting_visitor::scan_result& sr) {
        const auto& [ids, funcs] = sr;
        for (const auto& var_id: ids) {
            if (!active_scope_->has_property(var_id)) {
                active_scope_->put_local(string{heap_, var_id}, value::undefined);
            }
        }
        for (const auto f: funcs) {
            active_scope_->put_local_function(*this, *f);
        }
    }

    void hoist(const statement& s) {
        hoist(hoisting_visitor::scan(s));
    }

    value eval(const expression& e) {
        try {
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
        } catch (const not_supported_exception& e) {
            throw native_error_exception{native_error_type::assertion, stack_trace(), e.what()};
        } catch (const to_primitive_failed_error& e) {
            throw native_error_exception{native_error_type::type, stack_trace(), e.what()};
        } catch (const no_internal_value& e) {
            throw native_error_exception{native_error_type::type, stack_trace(), e.what()};
        } catch (const not_callable_exception& e) {
            throw native_error_exception{native_error_type::type, stack_trace(), e.what()};
#if 0
        } catch (std::exception& e) {
            std::wcerr << e.what() << " at\n" << stack_trace() << "\n\n";
            throw;
#endif
        }
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

        completion res{};

        current_extend(s.extend());

        if (global_->language_version() >= version::es3) {
            try {
                res = accept(s, *this);
            } catch (const native_error_exception& e) {
                res = completion{value{e.make_error_object(global_)}, completion_type::throw_};
            }
        } else {
            res = accept(s, *this);
        }
        if (on_statement_executed_) {
            on_statement_executed_(s, res);
        }
        return res;
    }

    value top_level_eval(const statement& s, bool allow_return = true) {
        auto c = eval(s);
        // ES3, 13.2.1 [[Call]]
        if (c.type == completion_type::throw_) {
            assert(c.result.type() == value_type::object);
            rethrow_error(c.result.object_value());
        } else if  (allow_return && c.type == completion_type::return_) {
            return c.result;
        } else if (!c) {
            // Don't let normal completion values escape
            return value::undefined;
        }
        std::wostringstream woss;
        woss << "Top level evaluation resulted in abrupt termination: " << c;
        throw native_error_exception(native_error_type::eval, stack_trace(), woss.str());
    }

    value operator()(const identifier_expression& e) {
        // §10.1.4
        return value{active_scope_->lookup(e.id())};
    }

    value operator()(const this_expression&) {
        return value{active_scope_->lookup(L"this")};
    }

    value operator()(const literal_expression& e) {
        switch (e.t().type()) {
        case token_type::null_:           return value::null;
        case token_type::true_:           return value{true};
        case token_type::false_:          return value{false};
        case token_type::numeric_literal: return value{e.t().dvalue()};
        case token_type::string_literal:  return value{string{heap_, e.t().text()}};
        default: NOT_IMPLEMENTED(e);
        }
    }

    value operator()(const array_literal_expression& e) {
        auto a = make_array(global_, 0);
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
            const auto& name = i.name_str();
            const auto prev_attr = o->own_property_attributes(name);
            auto v = get_value(eval(i.value()));
            if (i.type() == property_assignment_type::normal) {
                o->put(string{heap_, name}, v);
            } else {
                // getter/setter
                assert(is_function(v));
                const bool is_get = i.type() == property_assignment_type::get;
                if (!is_valid(prev_attr) || !has_attributes(prev_attr, property_attribute::accessor)) {
                    // Define new property with get or set
                    o->define_accessor_property(string{heap_, name}, make_accessor_object(global_, is_get ? v : value::undefined, !is_get ? v : value::undefined), property_attribute::none);
                } else {
                    // Modifying existing property
                    o->modify_accessor_object(name, v, is_get);
                }
            }
        }
        return value{o};
    }

    value operator()(const regexp_literal_expression& e) {
        return value{make_regexp(global_, e.re())};
    }

    auto eval_call_member(const expression& member) {
        // For strict mode we need to check if the MemberExpresson results in a primtive type
        // to be able to pass it on to the called function (even if the expression occurs in
        // a non-strict context)
        if (member.type() == expression_type::binary) {
            const auto& be = static_cast<const binary_expression&>(member);
            if (is_reference_op(be.op())) {
                // Commit to handling the evaluation
                auto l = get_value(eval(be.lhs()));
                auto r = eval(be.rhs());
                return std::tuple(make_reference(l, r), l);
            }
        }
        return std::tuple(eval(member), value::undefined);
    }

    value operator()(const call_expression& e) {
        // 11.2.3 The order of these two steps are actually reversed prior to ES5, but it's unlikely to be observable
        auto [member, this_] = eval_call_member(e.member());
        auto mval = get_value(member);
        auto args = eval_argument_list(e.arguments());
        if (mval.type() != value_type::object) {
            std::wostringstream woss;
            woss << to_string(heap_, mval).view() << " is not a function";
            throw native_error_exception(native_error_type::type, stack_trace(), woss.str());
        }
        // See ES5.1, 10.4.2 and 15.1.2.1.1
        const bool is_es5_or_later = global_->language_version() >= version::es5;
        was_direct_call_to_eval_ = !is_es5_or_later;
        if (this_.type() == value_type::undefined && member.type() == value_type::reference) {
            const auto& ref = member.reference_value();
            if (auto o = ref.base(); o && !o.has_type<activation_object>() && !is_global_object(o)) {
                this_ = value{o};
            }
            if (is_es5_or_later && ref.property_name().view() == L"eval") {
                // Meh, may need extra/better check
                was_direct_call_to_eval_ = true;
            }
        }

        auto_stack_update asu{*this, e.extend()};
        return call_function(mval, this_, args);
    }

    value operator()(const prefix_expression& e) {
        if (e.op() == token_type::new_) {
            return handle_new_expression(e.e());
        }

        auto u = eval(e.e());
        if (e.op() == token_type::delete_) {
            // ES1-5.1, 11.4.1: The delete Pperator
            if (u.type() != value_type::reference) {
                return value{true};
            }
            const auto& base = u.reference_value().base();
            const auto& prop = u.reference_value().property_name();
            if (!base) {
                assert(!strict_mode_);
                return value{true};
            }
            if (strict_mode_) {
                auto a = base->own_property_attributes(prop.view());
                if (is_valid(a) && has_attributes(a, property_attribute::dont_delete)) {
                    std::wostringstream woss;
                    woss << L"may not delete non-configurable property \"" << prop.view() << "\" in strict mode";
                    throw native_error_exception{native_error_type::type, stack_trace(), woss.str()};
                }
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
            case value_type::object: return value{string{heap_, u.object_value().has_type<function_object>() ? "function" : "object"}};
            default:
                NOT_IMPLEMENTED(u.type());
            }
        } else if (e.op() == token_type::plusplus || e.op() == token_type::minusminus) {
            auto num = to_number(get_value(u)) + (e.op() == token_type::plusplus ? 1 : -1);
            put_value(u, value{num});
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
        auto orig = to_number(get_value(member));
        auto num = orig;
        switch (e.op()) {
        case token_type::plusplus:   num += 1; break;
        case token_type::minusminus: num -= 1; break;
        default: NOT_IMPLEMENTED(e.op());
        }
        put_value(member, value{num});
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

    value make_reference(const value& obj, const value& prop) {
        return value{reference{global_->to_object(obj), to_string(heap_, prop)}};
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
            put_value(l, r);
            return r;
        }

        auto l = get_value(eval(e.lhs()));
        if ((e.op() == token_type::andand && !to_boolean(l)) || (e.op() == token_type::oror && to_boolean(l))) {
            return l;
        }
        auto r = get_value(eval(e.rhs()));
        if (e.op() == token_type::andand || e.op() == token_type::oror) {
            return r;
        } else if (is_reference_op(e.op())) {
            return make_reference(l, r);
        } else if (e.op() == token_type::in_) {
            if (r.type() != value_type::object) {
                std::wostringstream woss;
                woss << r.type() << " is not an object";
                throw native_error_exception{native_error_type::type, stack_trace(), woss.str()};
            }
            return value{r.object_value()->has_property(to_string(heap_, l).view())};
        } else if (e.op() == token_type::instanceof_) {
            if (r.type() != value_type::object || r.object_value()->prototype().get() != global_->function_prototype().get()) {
                std::wostringstream woss;
                woss << r.type() << " is not an object";
                throw native_error_exception{native_error_type::type, stack_trace(), woss.str()};
            }
            // ES3, 15.3.5.3 [[HasInstance]] (only implemented for Function objects)
            if (l.type() != value_type::object) {
                return value{false};
            }
            auto o = r.object_value()->get(L"prototype");
            if (o.type() != value_type::object) {
                std::wostringstream woss;
                woss << "Function has non-object prototype of type " << o.type() << " in instanceof check";
                throw native_error_exception{native_error_type::type, stack_trace(), woss.str()};
            }
            auto v = l.object_value()->prototype();
            for (;;) {
                if (!v) {
                    return value{false};
                }
                if (v.get() == o.object_value().get()) { // TODO: handle joined objects (ES3, 13.1.2)
                    return value{true};
                }
                v = v->prototype();
            }

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
        } else if (c.type == completion_type::return_ || c.type == completion_type::throw_) {
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
        strict_mode_scope sms{*this, s.strict_mode()};
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
                auto init_val = get_value(eval(*d.init()));
                active_scope_->put_local(string{heap_, d.id()}, init_val);
            }
        }
        return completion{};
    }

    completion operator()(const debugger_statement&) {
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

        // In ES5.1 for (?? in null/undefined) is just a no-op
        auto skip = [ver = global_->language_version()](value_type t) { return ver >= version::es5 && (t == value_type::undefined || t == value_type::null); };

        completion c{};
        if (s.init().type() == statement_type::expression) {
            auto ev = get_value(eval(s.e()));
            if (skip(ev.type())) {
                return completion{};
            }
            auto o = global_->to_object(ev);
            const auto& lhs_expression = static_cast<const expression_statement&>(s.init()).e();
            for (const auto& n: o->enumerable_property_names()) {
                put_value(eval(lhs_expression), value{n});
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
                put_value(value{active_scope_->lookup(init.id())}, val);
            };

            assign(init.init() ? get_value(eval(*init.init())) : value::undefined);

            // Happens after the initial assignment
            auto ev = get_value(eval(s.e()));
            if (skip(ev.type())) {
                return completion{};
            }
            auto o = global_->to_object(ev);

            for (const auto& n: o->enumerable_property_names()) {
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
            throw native_error_exception(native_error_type::syntax, stack_trace(), woss.str());
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
            auto o = global_->make_object();
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
        active_scope_->put_local_function(*this, s);
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

        bool has_property(const std::wstring& id) const {
            if (!activation_) {
                return false;
            }
            return activation_.dereference(heap_).has_property(id) || (prev_ && prev_.dereference(heap_).has_property(id));
        }

        reference lookup(const string& id) const {
            if (activation_.dereference(heap_).has_property(id.view())) {
                return reference{activation_.track(heap_), id};
            }
            if (prev_) {
                return prev_.dereference(heap_).lookup(id);
            } else {
                return reference{nullptr, id};
            }
        }

        reference lookup(const std::wstring& id) const {
            return lookup(string{heap_, id});
        }

        void put_local(const string& key, const value& val) {
            auto act = activation_.track(heap_);
            // Hack-ish, but variable/function definitions end up in e.g. in the "with" object otherwise
            // The final check if is in the case of a variabl declaration 'shadowing' the catch identifier
            if (act.has_type<activation_object>() || is_global_object(act) || is_valid(act->own_property_attributes(key.view()))) {
                act->redefine_own_property(key, val, property_attribute::dont_delete);
            } else {
                assert(prev_);
                prev_.dereference(heap_).put_local(key, val);
            }
        }

        void put_local_function(impl& i, const function_definition& func) {
            // We only want the function defined once, but want to allow re-definitions
            auto& funcs     = active_functions_.dereference(heap_);
            auto& func_vals = active_function_values_.dereference(heap_);

            // If this function has already been registered in this scope
            if (auto it = std::find_if(funcs.begin(), funcs.end(), [&func](const function_definition* f) { return f == &func; }); it != funcs.end()) {
                // And the original value hasn't expired
                const auto index = static_cast<uint32_t>(it - funcs.begin());
                const auto expected = func_vals[index];
                if (expected) {
                    // And still matches
                    auto current_val = activation_.dereference(heap_).get(func.id());
                    if (current_val.type() == value_type::object && current_val.object_value().get() == &expected.dereference(heap_)) {
                        // Then don't (re-)create the function
                        return;
                    }
                }
                // There's some kind of mismatch, remove the old references
                funcs.erase(it);
                func_vals.erase(func_vals.begin() + index);
            }

            // Create the function object
            auto f = i.create_function(func, heap_.unsafe_track(*this));

            // And record its info
            funcs.push_back(&func);
            func_vals.push_back(f);

            // Update the value in the activation object
            put_local(string{heap_, func.id()}, value{f});
        }

        const scope* get_prev() const {
            return prev_ ? &prev_.dereference(heap_) : nullptr;
        }

    private:
        using weak_object_ptr = gc_heap_weak_ptr_untracked<object>;

        explicit scope(const object_ptr& act, const scope_ptr& prev)
            : heap_(act.heap())
            , activation_(act)
            , prev_(prev)
            , active_functions_(gc_vector<const function_definition*>::make(heap_, 4))
            , active_function_values_(gc_vector<weak_object_ptr>::make(heap_, 4)) {
        }
        scope(scope&&) = default;

        void fixup() {
            activation_.fixup(heap_);
            prev_.fixup(heap_);
            active_functions_.fixup(heap_);
            active_function_values_.fixup(heap_);
        }

        gc_heap&                                                     heap_;
        gc_heap_ptr_untracked<object>                                activation_;
        gc_heap_ptr_untracked<scope>                                 prev_;
        gc_heap_ptr_untracked<gc_vector<const function_definition*>> active_functions_;
        gc_heap_ptr_untracked<gc_vector<weak_object_ptr>>            active_function_values_;

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
        explicit auto_stack_update(impl& parent, const source_extend& pos)
            : parent_(parent)
            , old_extend_(parent.current_extend_)
#ifndef NDEBUG
            , pos_(pos)
#endif
        {
            parent_.stack_trace_.push_back(pos);
        }

        ~auto_stack_update() {
            assert(!parent_.stack_trace_.empty() && parent_.stack_trace_.back() == pos_);
            parent_.stack_trace_.pop_back();
            parent_.current_extend_ = old_extend_;
        }
    private:
        impl& parent_;
        source_extend old_extend_;
#ifndef NDEBUG
        source_extend pos_;
#endif
    };
    class force_global_scope {
    public:
        explicit force_global_scope(impl& i) : parent_(i), old_scope_(parent_.active_scope_) {
            parent_.active_scope_ = make_scope(i.global_, nullptr);
        }
        ~force_global_scope() {
            parent_.active_scope_ = old_scope_;
        }
    private:
        impl&     parent_;
        scope_ptr old_scope_;
    };
    class strict_mode_scope {
    public:
        explicit strict_mode_scope(impl& i, bool next_) : impl_(i), prev_(i.strict_mode_) {
            assert(!i.strict_mode_ || next_);
            i.strict_mode_ = next_;
        }
        ~strict_mode_scope() {
            impl_.strict_mode_ = prev_;
        }
    private:
        impl& impl_;
        bool  prev_;
    };

    gc_heap&                       heap_;
    bool                           strict_mode_ = false; // Must be before global
    scope_ptr                      active_scope_;
    gc_heap_ptr<global_object>     global_;
    on_statement_executed_type     on_statement_executed_;
    std::vector<source_extend>     stack_trace_;
    int                            gc_cooldown_ = 0;
    label_set                      label_set_;
    const statement*               labels_valid_for_ = nullptr;
    source_extend                  current_extend_;
    bool                           was_direct_call_to_eval_ = false; // To support ES5.1, 15.1.2.1.1 Direct Call to Eval (TODO: Do this smarter...)

    static scope_ptr make_scope(const object_ptr& act, const scope_ptr& prev) {
        return act.heap().make<scope>(act, prev);
    }

    std::wstring stack_trace() const {
        std::wostringstream woss;
        assert(current_extend_.file);
        woss << current_extend_;
        for (auto it =  stack_trace_.crbegin(), end = stack_trace_.crend(); it != end; ++it) {
            assert(it->file);
            woss << "\n" << *it;
        }
        return woss.str();
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
        try {
            auto_stack_update asu{*this, e.extend()};
            return construct_function(o, value::undefined, args);
        } catch (const not_callable_exception&) {
            std::wostringstream woss;
            if (o.type() != value_type::object) {
                woss << to_string(heap_, o).view() << " is not an object";
            } else {
                // TODO: Could get function name here at some point
                woss << o.object_value()->class_name().view() << " is not constructable";
            }            
            throw native_error_exception(native_error_type::type, stack_trace(), woss.str());
        }
    }

    object_ptr create_function(const string& id, const std::shared_ptr<block_statement>& block, const std::vector<std::wstring>& param_names, const std::wstring& body_text, const scope_ptr& prev_scope) {
        // §15.3.2.1
        auto callee = make_raw_function(global_);
        auto func = [this, block, param_names, prev_scope, callee, id, hv_result = hoisting_visitor::scan(*block)](const value& this_, const std::vector<value>& args) {
            strict_mode_scope sms{*this, block->strict_mode()};
            // Scope
            auto activation = activation_object::make(global_, param_names, args);
            activation->put(global_->common_string("this"), block->strict_mode() ? this_ : get_this_arg(global_, this_), property_attribute::dont_delete | property_attribute::dont_enum | property_attribute::read_only);
            if (!strict_mode_) {
                activation->arguments()->put(global_->common_string("callee"), value{callee}, property_attribute::dont_enum);
            } else {
                global_->define_thrower_accessor(*activation->arguments(), "callee");
                global_->define_thrower_accessor(*activation->arguments(), "caller");
            }
            auto_scope auto_scope_{*this, activation, prev_scope};
            // Variables
            hoist(hv_result);
            if (!id.view().empty()) {
                // Add name of function to activation record even if it's not necessary for function_definition
                // It's needed for function expressions since `a=function x() { x(...); }` is legal, but x isn't added to the containing scope
                // TODO: Should actually be done a separate scope object (See ES3, 13)
                assert(!activation->has_property(id.view())); // TODO: Handle this..
                activation->put(id, value{callee}, property_attribute::dont_delete|property_attribute::read_only);
            }
            return top_level_eval(*block);
        };
        callee->put_function(func, nullptr, string{heap_, L"function " + std::wstring{id.view()} + body_text}.unsafe_raw_get(), static_cast<int>(param_names.size()));

        callee->construct_function([global = global_, callee, id](const value& this_, const std::vector<value>& args) {
            assert(this_.type() == value_type::undefined); (void)this_; // [[maybe_unused]] not working with MSVC here?
            assert(!id.view().empty());
            auto p = callee->get(L"prototype");
            auto o = value{global->heap().make<object>(id, p.type() == value_type::object ? p.object_value() : global->object_prototype())};
            auto r = callee->call(o, args);
            return r.type() == value_type::object ? r : value{o};
        });

        return callee;
    }

    object_ptr create_function(const function_base& f, const scope_ptr& prev_scope) {
        return create_function(string{heap_, f.id()}, f.block_ptr(), f.params(), std::wstring{f.body_extend().source_view()}, prev_scope);
    }

    // ES3, 8.7.1
    value get_value(const value& v) const {
        if (v.type() != value_type::reference) {
            return v;
        }
        auto& r = v.reference_value();
        auto b = r.base();
        if (!b) {
            std::wostringstream woss;
            woss << cpp_quote(r.property_name().view()) << " is not defined";
            throw native_error_exception{native_error_type::reference, stack_trace(), woss.str()};
        }
        return b->get(r.property_name().view());
    }

    void check_strict_mode_put(const object_ptr& o, const std::wstring_view p) const {
        if (!o) {
            std::wostringstream woss;
            woss << cpp_quote(p) << " is not defined";
            throw native_error_exception{native_error_type::reference, stack_trace(), woss.str()};
        }

        // ES5.1, 11.13.1
        if (const auto a = o->own_property_attributes(p); is_valid(a)) {
            if (!has_attributes(a, property_attribute::read_only)) {
                // OK property is writable
                return;
            }
            std::wostringstream woss;
            woss << "Cannot assign to read only property " << cpp_quote(p) << " in strict mode";
            throw native_error_exception{native_error_type::type, stack_trace(), woss.str()};
        }

        if (o->is_extensible()) {
            // OK property doesn't exist, but object is extensible
            return;
        }

        std::wostringstream woss;
        woss << "Cannot add property " << cpp_quote(p) << " to non-extensible object in strict mode";
        throw native_error_exception{native_error_type::type, stack_trace(), woss.str()};
    }

    // ES3, 8.7.2
    void put_value(const value& v, const value& w) const {
        if (v.type() != value_type::reference) {
            std::wostringstream woss;
            debug_print(woss, v, 4);
            woss << " is not a reference";
            throw native_error_exception{native_error_type::reference, stack_trace(), woss.str()};
        }
        auto& r = v.reference_value();
        auto b = r.base();
        if (strict_mode_) {
            check_strict_mode_put(b, r.property_name().view());
        }
        if (!b) {
            b = global_;
        }
        b->put(r.property_name(), w);
    }
};

interpreter::interpreter(gc_heap& h, version ver, const on_statement_executed_type& on_statement_executed) : impl_(new impl{h, ver, on_statement_executed}) {
}

interpreter::~interpreter() = default;

gc_heap_ptr<global_object> interpreter::global() const {
    return impl_->global();
}

value interpreter::eval(const statement& s) {
    impl_->hoist(s);
    auto c = impl_->eval(s);
    if (!c) {
        return c.result;
    } else if (c.type == completion_type::throw_) {
        rethrow_error(c.result.object_value());
    }
    std::wostringstream woss;
    woss << "Eval resulted in unexpected completion type " << c;
    throw native_error_exception{native_error_type::eval, impl_->global()->stack_trace(), woss.str()};
}

} // namespace mjs
