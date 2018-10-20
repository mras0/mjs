#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cmath>

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
        case mjs::token_type::undefined_:       os_ << "undefined"; return;
        case mjs::token_type::null_:            os_ << "null"; return;
        case mjs::token_type::true_:            os_ << "true"; return;
        case mjs::token_type::false_:           os_ << "false"; return;
        case mjs::token_type::numeric_literal:  os_ << e.t().dvalue(); return;
        case mjs::token_type::string_literal:   os_ << '"' << cpp_quote(e.t().text()) << '"'; return;
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
        case mjs::token_type::new_:    os_ << "new "; break;
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
        if (e.op() == mjs::token_type::lbracket) {
            os_ << '[';
            accept(e.rhs(), *this);
            os_ << ']';
        } else if (e.op() == mjs::token_type::dot) {
            os_ << '.';
            if (e.rhs().type() == mjs::expression_type::literal) {
                if (const auto& t = static_cast<const mjs::literal_expression&>(e.rhs()).t(); t.type() == mjs::token_type::string_literal) {
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

mjs::string index_string(uint32_t index) {
    return mjs::string{std::to_wstring(index)};
}

class array_object : public mjs::object {
public:
    static std::shared_ptr<array_object> make(const mjs::object_ptr& prototype, uint32_t length) {
        struct make_shared_helper : array_object {
            make_shared_helper(const mjs::object_ptr& prototype, uint32_t length) : array_object(prototype, length) {}
        };
        return std::make_shared<make_shared_helper>(prototype, length);
    }

    void put(const mjs::string& name, const mjs::value& val, mjs::property_attribute attr) override {
        if (!can_put(name)) {
            return;
        }

        if (name.view() == L"length") {
            const uint32_t length = to_uint32(mjs::object::get(mjs::string{"length"}));
            const uint32_t new_length = to_uint32(val);
            if (new_length < length) {
                for (uint32_t i = new_length; i < length; ++i) {
                    const bool res = mjs::object::delete_property(index_string(i));
                    assert(res); (void)res;
                }
            }
            mjs::object::put(mjs::string{"length"}, mjs::value{static_cast<double>(new_length)});
        } else {
            mjs::object::put(name, val, attr);
            uint32_t index = to_uint32(mjs::value{name});
            if (name.view() == mjs::to_string(index).view() && index != UINT32_MAX && index >= to_uint32(mjs::object::get(mjs::string{"length"}))) {
                mjs::object::put(mjs::string{"length"}, mjs::value{static_cast<double>(index+1)});
            }
        }
    }

    void unchecked_put(uint32_t index, const mjs::value& val) {
        const auto name = index_string(index);
        assert(index < to_uint32(get(mjs::string{"length"})) && can_put(name));
        mjs::object::put(name, val);
    }

private:
    explicit array_object(const mjs::object_ptr& prototype, uint32_t length) : object{mjs::string{"Array"}, prototype} {
        mjs::object::put(mjs::string{"length"}, mjs::value{static_cast<double>(length)}, mjs::property_attribute::dont_enum | mjs::property_attribute::dont_delete);
    }
};

mjs::string join(const mjs::object_ptr& o, const std::wstring_view& sep) {
    const uint32_t l = to_uint32(o->get(mjs::string{"length"}));
    std::wstring s;
    for (uint32_t i = 0; i < l; ++i) {
        if (i) s += sep;
        const auto& oi = o->get(mjs::string{std::to_wstring(i)});
        if (oi.type() != mjs::value_type::undefined && oi.type() != mjs::value_type::null) {
            s += to_string(oi).view();
        }
    }
    return mjs::string{s};
}

class global_object : public mjs::object {
public:
    static std::shared_ptr<global_object> make(const mjs::block_statement& bs) {
        struct make_shared_helper : global_object {
            make_shared_helper(){}
        };

        auto global = std::make_shared<make_shared_helper>();
        for (const auto& id: hoisting_visitor::scan(bs)) {
            global->put(id, mjs::value::undefined);
        }

        return global;
    }

    auto make_raw_function() {
        auto o = mjs::object::make(mjs::string{"Function"}, function_prototype_);
        o->put(mjs::string{"prototype"}, mjs::value{function_prototype_});
        return o;
    }

    static auto put_function(const mjs::object_ptr& o, const mjs::native_function_type& f, int named_args) {
        assert(o->class_name().str() == L"Function");
        assert(!o->call_function());
        o->put(mjs::string{"length"}, mjs::value{static_cast<double>(named_args)}, mjs::property_attribute::read_only | mjs::property_attribute::dont_delete | mjs::property_attribute::dont_enum);
        o->call_function(f);
        o->construct_function(f);
    }

    auto make_function(const mjs::native_function_type& f, int named_args) {
        auto o = make_raw_function();
        put_function(o, f, named_args);
        return o;
    }

    auto make_arguments_array(const std::vector<mjs::value>& args, const mjs::object_ptr& callee) {
        assert(callee->class_name().str() == L"Function");
        auto as = mjs::object::make(mjs::string{"Object"}, object_prototype_); // TODO: See §10.1.8
        as->put(mjs::string{"callee"}, mjs::value{callee}, mjs::property_attribute::dont_enum);
        as->put(mjs::string{"length"}, mjs::value{static_cast<double>(args.size())}, mjs::property_attribute::dont_enum);
        for (size_t i = 0; i < args.size(); ++i) {
            as->put(mjs::to_string(mjs::value{static_cast<double>(i)}), args[i], mjs::property_attribute::dont_enum);
        }
        return as;
    }

    mjs::object_ptr to_object(const mjs::value& v) {
        switch (v.type()) {
            // undefined and null give runtime errors
        case mjs::value_type::undefined:
        case mjs::value_type::null:
            {
                std::ostringstream oss;
                oss << "Cannot convert " << v.type() << " to object in " << __FUNCTION__;
                throw std::runtime_error(oss.str());
            }
        case mjs::value_type::boolean: return new_boolean(v.boolean_value());
        case mjs::value_type::number:  return new_number(v.number_value());
        case mjs::value_type::string:  return new_string(v.string_value());
        case mjs::value_type::object:  return v.object_value();
        default:
            NOT_IMPLEMENTED(v);
        }
    }

private:
    explicit global_object() : mjs::object(mjs::string{"Global"}, mjs::object_ptr{}) {
        object_prototype_   = mjs::object::make(mjs::string{"ObjectPrototype"}, nullptr);
        function_prototype_ = mjs::object::make(mjs::string{"Function"}, object_prototype_);
        popuplate_global();
    }

    mjs::object_ptr object_prototype_;
    mjs::object_ptr function_prototype_;
    mjs::object_ptr array_prototype_;
    mjs::object_ptr string_prototype_;
    mjs::object_ptr boolean_prototype_;
    mjs::object_ptr number_prototype_;

    static constexpr auto prototype_attributes = mjs::property_attribute::dont_enum | mjs::property_attribute::dont_delete | mjs::property_attribute::read_only;

    static void validate_type(const mjs::value& v, const wchar_t* expected_type) {
        if (v.type() == mjs::value_type::object && v.object_value()->class_name().view() == expected_type) {
            return;
        }
        std::wostringstream woss;
        woss << v << " is not a " << expected_type;
        throw make_runtime_error(woss.str());
    }

    //
    // Function
    //

    mjs::value function_constructor(const mjs::value&, const std::vector<mjs::value>&) {
        NOT_IMPLEMENTED("function constructor not implemented");
    }

    mjs::object_ptr make_function_object() {
        auto o = make_function(std::bind(&global_object::function_constructor, this, std::placeholders::_1, std::placeholders::_2), 1);
        o->put(mjs::string{"prototype"}, mjs::value{function_prototype_}, prototype_attributes);
        o->put(mjs::string{"length"}, mjs::value{1.0});

        // §15.3.4
        function_prototype_->call_function([](const mjs::value&, const std::vector<mjs::value>&) {
            return mjs::value::undefined;
        });
        function_prototype_->put(mjs::string{"constructor"}, mjs::value{o});
        // TODO: toString()
        return o;
    }

    //
    // Object
    //

    mjs::value object_constructor(const mjs::value&, const std::vector<mjs::value>& args) {
        if (args.empty() || args.front().type() == mjs::value_type::undefined || args.front().type() == mjs::value_type::null) {
            auto o = mjs::object::make(mjs::string{"Object"}, object_prototype_);
            return mjs::value{o};
        }
        return mjs::value{to_object(args.front())};
    }

    mjs::object_ptr make_object_object() {
        auto o = make_function(std::bind(&global_object::object_constructor, this, std::placeholders::_1, std::placeholders::_2), 1);
        o->put(mjs::string{"prototype"}, mjs::value{object_prototype_}, prototype_attributes);

        // §15.2.4
        object_prototype_->put(mjs::string{"constructor"}, mjs::value{o});
        object_prototype_->put(mjs::string{"toString"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>&){
            return mjs::value{mjs::string{"[object "} + this_.object_value()->class_name() + mjs::string{"]"}};
        }, 0)});
        object_prototype_->put(mjs::string{"valueOf"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>&){
            return this_;
        }, 0)});
        return o;
    }

    //
    // Boolean
    //

    mjs::object_ptr new_boolean(bool value) {
        auto o = mjs::object::make(mjs::string{"Boolean"}, boolean_prototype_);
        o->internal_value(mjs::value{value});
        return o;
    }

    mjs::object_ptr make_boolean_object() {
        boolean_prototype_ = mjs::object::make(mjs::string{"Boolean"}, object_prototype_);
        boolean_prototype_->internal_value(mjs::value{false});

        auto c = make_function([this](const mjs::value&, const std::vector<mjs::value>& args) {
            return mjs::value{new_boolean(!args.empty() && to_boolean(args.front()))};
        }, 1);
        c->call_function([](const mjs::value&, const std::vector<mjs::value>& args) {
            return mjs::value{!args.empty() && to_boolean(args.front())};
        });
        c->put(mjs::string{"prototype"}, mjs::value{boolean_prototype_}, prototype_attributes);

        boolean_prototype_->put(mjs::string{"constructor"}, mjs::value{c});
        boolean_prototype_->put(mjs::string{"toString"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>&){
            validate_type(this_, L"Boolean");
            return mjs::value{mjs::string{this_.object_value()->internal_value().boolean_value() ? L"true" : L"false"}};
        }, 0)});
        boolean_prototype_->put(mjs::string{"valueOf"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>&){
            validate_type(this_, L"Boolean");
            return this_.object_value()->internal_value();
        }, 0)});

        return c;
    }

    //
    // Number
    //

    mjs::object_ptr new_number(double value) {
        auto o = mjs::object::make(mjs::string{"Number"}, number_prototype_);
        o->internal_value(mjs::value{value});
        return o;
    }

    mjs::object_ptr make_number_object() {
        number_prototype_ = mjs::object::make(mjs::string{"Number"}, object_prototype_);
        number_prototype_->internal_value(mjs::value{0.});

        auto c = make_function([this](const mjs::value&, const std::vector<mjs::value>& args) {
            return mjs::value{new_number(args.empty() ? 0.0 : to_number(args.front()))};
        }, 1);
        c->call_function([](const mjs::value&, const std::vector<mjs::value>& args) {
            return mjs::value{args.empty() ? 0.0 : to_number(args.front())};
        });
        c->put(mjs::string{L"prototype"}, mjs::value{number_prototype_}, prototype_attributes);
        c->put(mjs::string{"MAX_VALUE"}, mjs::value{1.7976931348623157e308});
        c->put(mjs::string{"MIN_VALUE"}, mjs::value{5e-324});
        c->put(mjs::string{"NaN"}, mjs::value{NAN});
        c->put(mjs::string{"NEGATIVE_INFINITY"}, mjs::value{-INFINITY});
        c->put(mjs::string{"POSITIVE_INFINITY"}, mjs::value{INFINITY});

        number_prototype_->put(mjs::string{L"constructor"}, mjs::value{c});
        number_prototype_->put(mjs::string{"toString"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>& args){
            validate_type(this_, L"Number");
            const int radix = args.empty() ? 10 : to_int32(args.front());
            if (radix < 2 || radix > 36) {
                std::wostringstream woss;
                woss << "Invalid radix in Number.toString: " << args.front();
                throw make_runtime_error(woss.str());
            }
            if (radix != 10) {
                NOT_IMPLEMENTED(radix);
            }
            return mjs::value{to_string(this_.object_value()->internal_value())};
        }, 1)});
        number_prototype_->put(mjs::string{"valueOf"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>&){
            validate_type(this_, L"Number");
            return this_.object_value()->internal_value();
        }, 0)});

        return c;
    }

    //
    // String
    //


    mjs::object_ptr new_string(const mjs::string& value) {
        auto o = mjs::object::make(mjs::string{"String"}, string_prototype_);
        o->internal_value(mjs::value{value});
        o->put(mjs::string{"length"}, mjs::value{static_cast<double>(value.view().length())}, prototype_attributes);
        return o;
    }

    static mjs::value get_arg(const std::vector<mjs::value>& args, int index) {
        return index < static_cast<int>(args.size()) ? args[index] : mjs::value::undefined;
    }

    mjs::object_ptr make_string_object() {
        string_prototype_ = mjs::object::make(mjs::string{"String"}, object_prototype_);
        string_prototype_->internal_value(mjs::value{mjs::string{""}});

        auto c = make_function([this](const mjs::value&, const std::vector<mjs::value>& args) {
            return mjs::value{new_string(args.empty() ? mjs::string{""} : to_string(args.front()))};
        }, 1);
        c->call_function([](const mjs::value&, const std::vector<mjs::value>& args) {
            return mjs::value{args.empty() ? mjs::string{""} : to_string(args.front())};
        });
        c->put(mjs::string{"prototype"}, mjs::value{string_prototype_}, prototype_attributes);
        c->put(mjs::string{"fromCharCode"}, mjs::value{make_function([](const mjs::value&, const std::vector<mjs::value>& args){
            std::wstring s;
            for (const auto& a: args) {
                s.push_back(to_uint16(a));
            }
            return mjs::value{mjs::string{s}};
        }, 0)});
        
        string_prototype_->put(mjs::string{"constructor"}, mjs::value{c});
        string_prototype_->put(mjs::string{"toString"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>&){
            validate_type(this_, L"String");
            return this_.object_value()->internal_value();
        }, 0)});
        string_prototype_->put(mjs::string{"valueOf"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>&){
            validate_type(this_, L"String");
            return this_.object_value()->internal_value();
        }, 0)});


        auto make_string_function = [this](const char* name, int num_args, auto f) {
            string_prototype_->put(mjs::string{name}, mjs::value{make_function([f](const mjs::value& this_, const std::vector<mjs::value>& args){
                return mjs::value{f(to_string(this_).view(), args)};
            }, num_args)});
        };

        make_string_function("charAt", 1, [](const std::wstring_view& s, const std::vector<mjs::value>& args){
            const int position = to_int32(get_arg(args, 0));
            if (position < 0 || position >= static_cast<int>(s.length())) {
                return mjs::string{""};
            }
            return mjs::string{s.substr(position, 1)};
        });

        make_string_function("charCodeAt", 1, [](const std::wstring_view& s, const std::vector<mjs::value>& args){
            const int position = to_int32(get_arg(args, 0));
            if (position < 0 || position >= static_cast<int>(s.length())) {
                return static_cast<double>(NAN);
            }
            return static_cast<double>(s[position]);
        });

        make_string_function("indexOf", 2, [](const std::wstring_view& s, const std::vector<mjs::value>& args){
            const auto& search_string = to_string(get_arg(args, 0));
            const int position = to_int32(get_arg(args, 1));
            auto index = s.find(search_string.view(), position);
            return index == std::wstring_view::npos ? -1. : static_cast<double>(index);
        });

        make_string_function("lastIndexOf", 2, [](const std::wstring_view& s, const std::vector<mjs::value>& args){
            const auto& search_string = to_string(get_arg(args, 0));
            double position = to_number(get_arg(args, 1));
            const int ipos = std::isnan(position) ? INT_MAX : mjs::to_int32(position);
            auto index = s.rfind(search_string.view(), ipos);
            return index == std::wstring_view::npos ? -1. : static_cast<double>(index);
        });

        make_string_function("split", 1, [this](const std::wstring_view& s, const std::vector<mjs::value>& args){
            auto a = array_constructor(mjs::value::null, {}).object_value();
            if (args.empty()) {
                a->put(index_string(0), mjs::value{mjs::string{s}});
            } else {
                const auto sep = to_string(args.front());
                if (sep.view().empty()) {
                    for (uint32_t i = 0; i < s.length(); ++i) {
                        a->put(index_string(i), mjs::value{mjs::string{ s.substr(i,1) }});
                    }
                } else {
                    size_t pos = 0;
                    uint32_t i = 0;
                    for (; pos < s.length(); ++i) {
                        const auto next_pos = s.find(sep.view(), pos);
                        if (next_pos == std::wstring_view::npos) {
                            break;
                        }
                        a->put(index_string(i), mjs::value{mjs::string{ s.substr(pos, next_pos-pos) }});
                        pos = next_pos + 1;
                    }
                    if (pos < s.length()) {
                        a->put(index_string(i), mjs::value{mjs::string{ s.substr(pos) }});
                    }
                }
            }
            return a;
        });

        make_string_function("substring", 1, [](const std::wstring_view& s, const std::vector<mjs::value>& args){
            int start = std::min(std::max(to_int32(get_arg(args, 0)), 0), static_cast<int>(s.length()));
            if (args.size() < 2) {
                return mjs::string{s.substr(start)};
            }
            int end = std::min(std::max(to_int32(get_arg(args, 1)), 0), static_cast<int>(s.length()));
            if (start > end) {
                std::swap(start, end);
            }
            return mjs::string{s.substr(start, end-start)};
        });

        make_string_function("toLowerCase", 0, [](const std::wstring_view& s, const std::vector<mjs::value>&){
            std::wstring res;
            for (auto c: s) {
                res.push_back(towlower(c));
            }
            return mjs::string{res};
        });

        make_string_function("toUpperCase", 0, [](const std::wstring_view& s, const std::vector<mjs::value>&){
            std::wstring res;
            for (auto c: s) {
                res.push_back(towupper(c));
            }
            return mjs::string{res};
        });

        return mjs::object_ptr{c};
    }

    //
    // Array
    //

    mjs::value array_constructor(const mjs::value&, const std::vector<mjs::value>& args) {
        if (args.size() == 1 && args[0].type() == mjs::value_type::number) {
            return mjs::value{array_object::make(array_prototype_, mjs::to_uint32(args[0].number_value()))};
        }
        auto arr = array_object::make(array_prototype_, static_cast<uint32_t>(args.size()));
        for (uint32_t i = 0; i < args.size(); ++i) {
            arr->unchecked_put(i, args[i]);
        }
        return mjs::value{arr};
    }

    mjs::object_ptr make_array_object() {
        array_prototype_ = array_object::make(object_prototype_, 0);

        auto o = make_function(std::bind(&global_object::array_constructor, this, std::placeholders::_1, std::placeholders::_2), 1);
        o->put(mjs::string{"prototype"}, mjs::value{array_prototype_}, prototype_attributes);

        array_prototype_->put(mjs::string{"constructor"}, mjs::value{o});
        array_prototype_->put(mjs::string{"toString"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>&) {
            assert(this_.type() == mjs::value_type::object);
            return mjs::value{join(this_.object_value(), L",")};
        }, 0)});
        array_prototype_->put(mjs::string{"join"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>& args) {
            assert(this_.type() == mjs::value_type::object);
            mjs::string sep{L","};
            if (!args.empty()) {
                sep = to_string(args.front());
            }
            return mjs::value{join(this_.object_value(), sep.view())};
        }, 1)});
        array_prototype_->put(mjs::string{"reverse"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>&) {
            assert(this_.type() == mjs::value_type::object);
            const auto& o = this_.object_value();
            const uint32_t length = to_uint32(o->get(mjs::string{L"length"}));
            for (uint32_t k = 0; k != length / 2; ++k) {
                const auto i1 = index_string(k);
                const auto i2 = index_string(length - k - 1);
                auto v1 = o->get(i1);
                auto v2 = o->get(i2);
                o->put(i1, v2);
                o->put(i2, v1);
            }
            return this_;
        }, 0)});
        array_prototype_->put(mjs::string{"sort"}, mjs::value{make_function([](const mjs::value& this_, const std::vector<mjs::value>& args) {
            assert(this_.type() == mjs::value_type::object);
            const auto& o = this_.object_value();
            const uint32_t length = to_uint32(o->get(mjs::string{L"length"}));

            mjs::native_function_type comparefn{};
            if (!args.empty()) {
                if (args.front().type() == mjs::value_type::object) {
                    comparefn = args.front().object_value()->call_function();
                }
                if (!comparefn) {
                    NOT_IMPLEMENTED("Invalid compare function given to sort");
                }
            }

            auto sort_compare = [comparefn](const mjs::value& x, const mjs::value& y) {
                if (x.type() == mjs::value_type::undefined && y.type() == mjs::value_type::undefined) {
                    return 0;
                } else if (x.type() == mjs::value_type::undefined) {
                    return 1;
                } else if (y.type() == mjs::value_type::undefined) {
                    return -1;
                }
                if (comparefn) {
                    const auto r = to_number(comparefn(mjs::value::null, {x,y}));
                    if (r < 0) return -1;
                    if (r > 0) return 1;
                    return 0;
                } else {
                    const auto xs = to_string(x);
                    const auto ys = to_string(y);
                    if (xs.view() < ys.view()) return -1;
                    if (xs.view() > ys.view()) return 1;
                    return 0;
                }
            };

            std::vector<mjs::value> values(length);
            for (uint32_t i = 0; i < length; ++i) {
                values[i] = o->get(index_string(i));
            }
            std::stable_sort(values.begin(), values.end(), [&](const mjs::value& x, const mjs::value& y) {
                return sort_compare(x, y) < 0;
            });
            for (uint32_t i = 0; i < length; ++i) {
                o->put(index_string(i), values[i]);
            }
            return this_;
        }, 1)});
        return o;
    }


    //
    // Global
    //
    void popuplate_global() {
        // §15.1
        const auto attr = mjs::property_attribute::dont_enum;
        put(mjs::string{"Object"}, mjs::value{make_object_object()}, attr);
        put(mjs::string{"Function"}, mjs::value{make_function_object()}, attr);
        put(mjs::string{"Array"}, mjs::value{make_array_object()}, attr);
        put(mjs::string{"String"}, mjs::value{make_string_object()}, attr);
        put(mjs::string{"Boolean"}, mjs::value{make_boolean_object()}, attr);
        put(mjs::string{"Number"}, mjs::value{make_number_object()}, attr);

        put(mjs::string{"NaN"}, mjs::value{NAN}, attr);
        put(mjs::string{"Infinity"}, mjs::value{INFINITY}, attr);
        put(mjs::string{"eval"}, mjs::value{make_function(
            [](const mjs::value&, const std::vector<mjs::value>& args) {
            if (args.empty()) {
                return mjs::value::undefined; 
            } else if (args.front().type() != mjs::value_type::string) {
                return args.front();
            }
            auto bs = mjs::parse(args.front().string_value().view());
            (void)bs;
            throw std::runtime_error("TODO: Handle eval");
        }, 1)}, attr);
        put(mjs::string{"isNaN"}, mjs::value{make_function(
            [](const mjs::value&, const std::vector<mjs::value>& args) {
            return mjs::value(std::isnan(to_number(args.empty() ? mjs::value::undefined : args.front())));
        }, 1)}, attr);
        put(mjs::string{"isFinite"}, mjs::value{make_function(
            [](const mjs::value&, const std::vector<mjs::value>& args) {
            return mjs::value(std::isfinite(to_number(args.empty() ? mjs::value::undefined : args.front())));
        }, 1)}, attr);
        put(mjs::string{"alert"}, mjs::value{make_function(
            [](const mjs::value&, const std::vector<mjs::value>& args) {
            std::wcout << "ALERT";
            if (!args.empty()) std::wcout << ": " << args[0];
            std::wcout << "\n";
            return mjs::value::undefined; 
        }, 1)}, attr);
    }
};

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
    explicit eval_visitor(const std::shared_ptr<global_object>& global) : global_(global) {
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
         case mjs::token_type::undefined_:      return mjs::value::undefined;
         case mjs::token_type::null_:           return mjs::value::null;
         case mjs::token_type::true_:           return mjs::value{true};
         case mjs::token_type::false_:          return mjs::value{false};
         case mjs::token_type::numeric_literal: return mjs::value{e.t().dvalue()};
         case mjs::token_type::string_literal:  return mjs::value{e.t().text()};
         default: NOT_IMPLEMENTED(e);
         }
    }

    mjs::value operator()(const mjs::call_expression& e) {
        auto member = accept(e.member(), *this);
        auto mval = get_value(member);
        auto args = eval_argument_list(e.arguments());
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
        auto this_ = mjs::value::null;
        if (member.type() == mjs::value_type::reference) {
            if (auto o = member.reference_value().base(); o->class_name().view() != L"Activation") {
                this_ = mjs::value{o};
            }
        }

        // TODO: scope etc.
        return c(this_, args);
    }

    mjs::value operator()(const mjs::prefix_expression& e) {
        if (e.op() == mjs::token_type::new_) {
            return handle_new_expression(e.e());
        }

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
        NOT_IMPLEMENTED(e);
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
        if (l.type() == mjs::value_type::null && r.type() == mjs::value_type::undefined) {
            return true;
        } else if (l.type() == mjs::value_type::undefined && r.type() == mjs::value_type::null) {
            return true;
        } else if (l.type() == mjs::value_type::number && r.type() == mjs::value_type::string) {
            return compare_equal(l, mjs::value{to_number(r.string_value())});
        } else if (l.type() == mjs::value_type::string && r.type() == mjs::value_type::number) {
            return compare_equal(mjs::value{to_number(l.string_value())}, r);
        } else if (l.type() == mjs::value_type::boolean) {
            return compare_equal(mjs::value{static_cast<double>(l.boolean_value())}, r);
        } else if (r.type() == mjs::value_type::boolean) {
            return compare_equal(l, mjs::value{static_cast<double>(r.boolean_value())});
        } else if ((l.type() == mjs::value_type::string || l.type() == mjs::value_type::number) && r.type() == mjs::value_type::object) {
            return compare_equal(l, to_primitive(r));
        } else if ((r.type() == mjs::value_type::string || r.type() == mjs::value_type::number) && l.type() == mjs::value_type::object) {
            return compare_equal(to_primitive(l), r);
        }
        return false;
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
        if (e.op() == mjs::token_type::dot || e.op() == mjs::token_type::lbracket) {
            return mjs::value{mjs::reference{global_->to_object(l), to_string(r)}};
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
        auto callee = global_->make_raw_function();
        auto func = [&s, this, prev_scope, callee, ids = hoisting_visitor::scan(s.block())](const mjs::value& this_, const std::vector<mjs::value>& args) {
            auto_scope as{*this, prev_scope};
            auto& scope = scopes_->activation;
            scope->put(mjs::string{"this"}, this_, mjs::property_attribute::dont_delete | mjs::property_attribute::dont_enum | mjs::property_attribute::read_only);
            scope->put(mjs::string{"arguments"}, mjs::value{global_->make_arguments_array(args, callee)}, mjs::property_attribute::dont_delete);
            for (size_t i = 0; i < std::min(args.size(), s.params().size()); ++i) {
                scope->put(s.params()[i], args[i]);
            }
            for (const auto& id: ids) {
                assert(!scope->has_property(id)); // TODO: Handle this..
                scope->put(id, mjs::value::undefined);
            }
            return accept(s.block(), *this).result;
        };
        global_->put_function(callee, func, static_cast<int>(s.params().size()));
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
    scope_ptr                      scopes_;
    std::shared_ptr<global_object> global_;

    std::vector<mjs::value> eval_argument_list(const mjs::expression_list& es) {
        std::vector<mjs::value> args;
        for (const auto& e: es) {
            args.push_back(get_value(accept(*e, *this)));
        }
        return args;
    }

    mjs::value handle_new_expression(const mjs::expression& e) {
        mjs::value o;
        std::vector<mjs::value> args;
        if (e.type() == mjs::expression_type::call) {
            const auto& ce = static_cast<const mjs::call_expression&>(e);
            o = accept(ce.member(), *this);
            args = eval_argument_list(ce.arguments());
        } else {
            o = accept(e, *this);
        }
        o = get_value(o);
        if (o.type() != mjs::value_type::object) {
            std::wostringstream woss;
            woss << e << " is not an object";
            throw make_runtime_error(woss.str());
        }
        auto c = o.object_value()->construct_function();
        if (!c) {
            std::wostringstream woss;
            woss << e << " is not constructable";
            throw make_runtime_error(woss.str());
        }
        return c(mjs::value{}, args);
    }
};

void test(const std::wstring_view& text, const mjs::value& expected) {
    decltype(mjs::parse(text)) bs;
    try {
        bs = mjs::parse(text);
    } catch (const std::exception& e) {
        std::wcout << "Parse failed for \"" << text << "\": " << e.what() <<  "\n";
        assert(false);
        return;
    }
    auto pb = [&bs]() {
        print_visitor pv{std::wcout}; accept(*bs, pv); std::wcout << '\n';
        for (const auto& s: bs->l()) {
            std::wcout << *s << "\n";
        }
    };
    try {
        auto global = global_object::make(*bs);
        eval_visitor ev{global};
        mjs::value res{};
        for (const auto& s: bs->l()) {
            res = accept(*s, ev).result;
        }
        if (res != expected) {
            std::wcout << "Test failed: " << text << " expecting " << expected.type() << " " << expected << " got " << res.type() << " " << res << "\n";
            pb();
            assert(false);
        }
    } catch (const std::exception& e) {
        std::wcout << "Test failed: " << text << " uexpected exception throw: " << e.what() << "\n";
        pb();
        assert(false);
    }

#ifndef NDEBUG
    mjs::object::garbage_collect({});
    if (mjs::object::object_count()) {
        std::wcout << "Total number of objects left: " << mjs::object::object_count() <<  " while testing '" << text << "'\n";
        assert(!"Leaks");
    }
#endif

}

void eval_tests() {
    using namespace mjs;
    test(L"undefined", value::undefined);
    test(L"null", value::null);
    test(L"false", value{false});
    test(L"true", value{true});
    test(L"'te\"st'", value{string{"te\"st"}});
    test(L"\"te'st\"", value{string{"te'st"}});
    test(L"-7.5 % 2", value{-1.5});
    test(L"1+2*3", value{7.});
    test(L"x = 42; 'test ' + 2 * (6 - 4 + 1) + ' ' + x", value{string{"test 6 42"}});
    test(L"y=1/2; z='string'; y+z", value{string{"0.5string"}});
    test(L"var x=2; x++;", value{2.0});
    test(L"var x=2; x++; x", value{3.0});
    test(L"var x=2; x--;", value{2.0});
    test(L"var x=2; x--; x", value{1.0});
    test(L"var x = 42; delete x; x", value::undefined);
    test(L"void(2+2)", value::undefined);
    test(L"typeof(2)", value{string{"number"}});
    test(L"x=4.5; ++x", value{5.5});
    test(L"x=4.5; --x", value{3.5});
    test(L"x=42; +x;", value{42.0});
    test(L"x=42; -x;", value{-42.0});
    test(L"x=42; !x;", value{false});
    test(L"x=42; ~x;", value{(double)~42});
    test(L"1<<2", value{4.0});
    test(L"-5>>2", value{-2.0});
    test(L"-5>>>2", value{1073741822.0});
    test(L"1 < 2", value{true});
    test(L"1 > 2", value{false});
    test(L"1 <= 2", value{true});
    test(L"1 >= 2", value{false});
    test(L"1 == 2", value{false});
    test(L"1 != 2", value{true});
    test(L"255 & 128", value{128.0});
    test(L"255 ^ 128", value{127.0});
    test(L"64 | 128", value{192.0});
    test(L"42 || 13", value{42.0});
    test(L"42 && 13", value{13.0});
    test(L"1 ? 2 : 3", value{2.0});
    test(L"0 ? 2 : 1+2", value{3.0});
    test(L"x=2.5; x+=4; x", value{6.5});
    test(L"function f(x,y) { return x*x+y; } f(2, 3)", value{7.0});
    test(L"function f(){ i = 42; }; f(); i", value{42.0});
    test(L"i = 1; function f(){ var i = 42; }; f(); i", value{1.0});
    test(L";", value::undefined);
    test(L"if (1) 2;", value{2.0});
    test(L"if (0) 2;", value::undefined);
    test(L"if (0) 2; else ;", value::undefined);
    test(L"if (0) 2; else 3;", value{3.0});
    test(L"1,2", value{2.0});
    test(L"x=5; while(x-3) { x = x - 1; } x", value{3.0});
    test(L"x=2; y=0; while(1) { if(x) {x = x - 1; y = y + 2; continue; y = y + 1000; } else break; y = y + 1;} y", value{4.0});
    test(L"var x = 0; for(var i = 10, dec = 1; i; i = i - dec) x = x + i; x", value{55.0});
    test(L"var x=0; for (i=2; i; i=i-1) x=x+i; x+i", value{3.0});
    // TODO: for .. in, with
    test(L"function sum() {  var s = 0; for (var i = 0; i < arguments.length; ++i) s += arguments[i]; return s; } sum(1,2,3)", value{6.0});
    // Object
    test(L"''+Object(null)", value{string{"[object Object]"}});
    test(L"o=Object(null); o.x=42; o.y=60; o.x+o['y']", value{102.0});
    test(L"a=Object(null);b=Object(null);a.x=b;a.x.y=42;a['x']['y']", value{42.0});
    test(L"'' + new Object", value{string{L"[object Object]"}});
    test(L"'' + new Object()", value{string{L"[object Object]"}});
    test(L"'' + new Object(null)", value{string{L"[object Object]"}});
    test(L"'' + new Object(undefined)", value{string{L"[object Object]"}});
    test(L"o = new Object;o.x=42; new Object(o).x", value{42.0});
    // TODO: Function
    // Array
    test(L"Array.length", value{1.0});
    test(L"Array.prototype.length", value{0.0});
    test(L"new Array().length", value{0.0});
    test(L"new Array(60).length", value{60.0});
    test(L"new Array(1,2,3,4).length", value{4.0});
    test(L"new Array(1,2,3,4)[3]", value{4.0});
    test(L"new Array(1,2,3,4)[4]", value::undefined);
    test(L"a=new Array(); a[5]=42; a.length", value{6.0});
    test(L"a=new Array(); a[5]=42; a[3]=2; a.length", value{6.0});
    test(L"a=new Array(1,2,3,4); a.length=2; a.length", value{2.0});
    test(L"a=new Array(1,2,3,4); a.length=2; a[3]", value::undefined);
    test(L"''+Array()", value{string{""}});
    test(L"a=Array();a[0]=1;''+a", value{string{"1"}});
    test(L"''+Array(1,2,3,4)", value{string{"1,2,3,4"}});
    test(L"Array(1,2,3,4).join()", value{string{"1,2,3,4"}});
    test(L"Array(1,2,3,4).join('')", value{string{"1234"}});
    test(L"Array(4).join()", value{string{",,,"}});
    test(L"Array(1,2,3,4).reverse()+''", value{string{"4,3,2,1"}});
    test(L"''+Array('March', 'Jan', 'Feb', 'Dec').sort()", value{string{"Dec,Feb,Jan,March"}});
    test(L"''+Array(1,30,4,21).sort()", value{string{"1,21,30,4"}});
    test(L"function c(x,y) { return x-y; }; ''+Array(1,30,4,21).sort(c)", value{string{"1,4,21,30"}});
    // String
    test(L"String()", value{string{""}});
    test(L"String('test')", value{string{"test"}});
    test(L"''+new String()", value{string{""}});
    test(L"''+new String('test')", value{string{"test"}});
    test(L"Object('testXX').valueOf()", value{string{"testXX"}});
    test(L"String.fromCharCode()", value{string{""}});
    test(L"String.fromCharCode(65,66,67+32)", value{string{"ABc"}});
    test(L"'test'.charAt(0)", value{string{"t"}});
    test(L"'test'.charAt(2)", value{string{"s"}});
    test(L"'test'.charAt(5)", value{string{""}});
    test(L"'test'.charCodeAt(0)", value{(double)'t'});
    test(L"'test'.charCodeAt(2)", value{(double)'s'});
    test(L"'test'.charCodeAt(5)", value{NAN});
    test(L"''.indexOf()", value{-1.});
    test(L"'11 undefined'.indexOf()", value{3.});
    test(L"'testfesthest'.indexOf('XX')", value{-1.});
    test(L"'testfesthest'.indexOf('est')", value{1.});
    test(L"'testfesthest'.indexOf('est',3)", value{5.});
    test(L"'testfesthest'.indexOf('est',7)", value{9.});
    test(L"'testfesthest'.indexOf('est',11)", value{-1.});
    test(L"'testfesthest'.lastIndexOf('estX')", value{-1.});
    test(L"'testfesthest'.lastIndexOf('est')", value{9.});
    test(L"'testfesthest'.lastIndexOf('est',1)", value{1.});
    test(L"'testfesthest'.lastIndexOf('est',3)", value{1.});
    test(L"'testfesthest'.lastIndexOf('est',7)", value{5.});
    test(L"'testfesthest'.lastIndexOf('est', 22)", value{9.});
    test(L"''.split()+''", value{string{}});
    test(L"'1 2 3'.split()+''", value{string{"1 2 3"}});
    test(L"'abcd'.split('')+''", value{string{"a,b,c,d"}});
    test(L"'1 2 3'.split('not found')+''", value{string{"1 2 3"}});
    test(L"'1 2 3'.split(' ')+''", value{string{"1,2,3"}});
    test(L"'foo bar'.substring()", value{string{"foo bar"}});
    test(L"'foo bar'.substring(-1)", value{string{"foo bar"}});
    test(L"'foo bar'.substring(42)", value{string{""}});
    test(L"'foo bar'.substring(3)", value{string{" bar"}});
    test(L"'foo bar'.substring(0, 1)", value{string{"f"}});
    test(L"'foo bar'.substring(1, 0)", value{string{"f"}});
    test(L"'foo bar'.substring(1000, -1)", value{string{"foo bar"}});
    test(L"'foo bar'.substring(1, 4)", value{string{"oo "}});
    test(L"'ABc'.toLowerCase()", value{string{"abc"}});
    test(L"'ABc'.toUpperCase()", value{string{"ABC"}});
    // Boolean
    test(L"Boolean()", value{false});
    test(L"Boolean(true)", value{true});
    test(L"Boolean(42)", value{true});
    test(L"Boolean(0)", value{false});
    test(L"Boolean('')", value{false});
    test(L"Boolean('x')", value{true});
    test(L"new Boolean('x').valueOf()", value{true});
    test(L"0 + new Boolean()", value{0.});
    test(L"0 + new Boolean(1)", value{1.});
    test(L"'' + new Boolean(0)", value{string{"false"}});
    test(L"'' + new Boolean(1)", value{string{"true"}});
    test(L"Object(true).toString()", value{string{"true"}});
    // Number
    test(L"Number()", value{0.});
    test(L"Number(42.42)", value{42.42});
    test(L"Number.MIN_VALUE", value{5e-324});
    test(L"new Number(42.42).toString()", value{string{"42.42"}});
    test(L"''+new Number(60)", value{string{"60"}});
    test(L"new Number(123).valueOf()", value{123.0});
    test(L"Object(42).toString(10)", value{string{"42"}}); 
    // TODO: Math
    // TODO: Date

    // wat
    test(L"!!('')", value{false});
    test(L"\"\" == false", value{true});
    test(L"null == false", value{false});
    test(L"+true", value{1.0});
    test(L"true + true", value{2.0});
    test(L"!!('0' && Object(null))", value{true});

}

int main() {
    try {
        eval_tests();
        auto bs = mjs::parse(L"new Array(1,2,3,4).valueOf()");
        auto global = global_object::make(*bs);
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
#ifndef NDEBUG
    mjs::object::garbage_collect({});
    assert(!mjs::object::object_count());
#endif
}
