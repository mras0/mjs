#include "json_object.h"
#include "function_object.h"
#include "array_object.h"
#include "error_object.h"
#include <sstream>
#include <cmath>

namespace mjs {

namespace {

[[noreturn]] void throw_not_implement(const char* name, const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    std::wostringstream woss;
    woss << name << " is not yet implemented for arguments: {";
    for (const auto& a: args) {
        woss << " ";
        debug_print(woss, a, 4);
    }
    woss << " }";
    throw native_error_exception{native_error_type::assertion, global->stack_trace(), woss.str()};
}

std::wstring json_quote(const std::wstring_view& s) {
    std::wstring res;
    res.push_back('"');
    for (const auto ch: s) {
        if (ch == '"' || ch == '\\') {
            res.push_back('\\');
            res.push_back(ch);
        } else if (ch < ' ') {
            res.push_back('\\');
            switch (ch) {
            case  8: res.push_back('b'); break;
            case  9: res.push_back('t'); break;
            case 10: res.push_back('n'); break;
            case 12: res.push_back('f'); break;
            case 13: res.push_back('r'); break;
            default:
                res.push_back('u');
                res.push_back('0');
                res.push_back('0');
                res.push_back((ch&0x10)?'1':'0');
                res.push_back("0123456789abcdef"[ch&0xf]);
            }
        } else {
            res.push_back(ch);
        }
    }
    res.push_back('"');
    return res;
}

class stringify_state {
public:
    explicit stringify_state(const gc_heap_ptr<global_object>& global, const std::vector<value>& args) : global_(global) {
        assert(!args.empty());
        if (args.size() > 1 && args[1].type() == value_type::object) {
            // TODO: Handle PropertyList and ReplacerFunction
            throw_not_implement("stringify", global, args);
        }
        if (args.size() > 2) {
            auto space = args[2];
            if (space.type() == value_type::object) {
                // FIXME: As below this is a bad way to determine the type...
                auto o = space.object_value();
                const auto class_name = o->class_name();
                if (class_name.view() == L"Number") {
                    space = value{to_number(space)};
                } else if (class_name.view() == L"String") {
                    space = value{to_string(heap(), space)};
                }
            }

            if (space.type() == value_type::number) {
                gap_ = std::wstring(std::min(10, static_cast<int>(to_integer(space.number_value()))), ' ');
            } else if (space.type() == value_type::string) {
                auto s = space.string_value().view(); 
                gap_ = s.substr(0, std::min(size_t{10}, s.length()));
            }
        }
    }

    ~stringify_state() {
        assert(!stack_ || stack_->length() == 0);
    }


    gc_heap& heap() const { return global_.heap(); }
    const gc_heap_ptr<global_object>& global() { return global_; }

    string null_str() const { return global_->common_string("null"); }
    string bool_str(bool val) const { return global_->common_string(val ? "true" : "false"); }

    class nest {
    public:
        explicit nest(stringify_state& state, const object_ptr& o) : state_(state) {
            state_.push(o);
            step_back_ = state.indent_;
            state.indent_ += state_.gap_;
        }
        ~nest() {
            state_.pop();
            state_.indent_ = step_back_;
        }

        std::wstring start_gap() const {
            return state_.gap_.empty() ? L"" : L"\n" + state_.indent_;
        }

        std::wstring end_gap() const {
            return state_.gap_.empty() ? L"" : L"\n" + step_back_;
        }

        std::wstring sep() const {
            return state_.gap_.empty() ? L"," : L",\n" + state_.indent_;
        }

        std::wstring spacing() const {
            return state_.gap_.empty() ? L"" : L" ";
        }

    private:
        stringify_state& state_;
        std::wstring step_back_;
    };

private:
    gc_heap_ptr<global_object> global_;
    gc_heap_ptr<gc_vector<gc_heap_ptr_untracked<object>>> stack_ = nullptr;
    std::wstring indent_;
    std::wstring gap_;

    void push(const object_ptr& obj) {
        if (!stack_) {
            stack_ = gc_vector<gc_heap_ptr_untracked<object>>::make(heap(), 4);
        }
        for (const auto& o: *stack_) {
            if (&o.dereference(heap()) == obj.get()) {
                throw native_error_exception{native_error_type::type, global()->stack_trace(),  "Object must be acyclic"};
            }
        }
        stack_->push_back(obj);
    }

    void pop() {
        stack_->pop_back();
    }
};

value json_str(stringify_state& state, const std::wstring_view key, const object_ptr& holder);

string json_str_object(stringify_state& state, const object_ptr& o) {
    stringify_state::nest nest{state, o};
    // TODO: Check PrpoertyList
    auto k = o->own_property_names(true);

    std::vector<std::wstring> partial;
    for (const auto& p: k) {
        auto str_p = json_str(state, p.view(), o);
        if (str_p.type() != value_type::undefined) {
            assert(str_p.type() == value_type::string);
            partial.push_back(json_quote(p.view()) + L":" + nest.spacing() + std::wstring{str_p.string_value().view()});
        }
    }

    if (partial.empty()) {
        return state.global()->common_string("{}");
    }

    std::wstring res;
    res.push_back('{');
    res += nest.start_gap();
    bool first = true;
    for (const auto& s: partial) {
        if (!first) {
            res += nest.sep();
        }
        res += s;
        first = false;
    }
    res += nest.end_gap();
    res.push_back('}');
    return string{state.heap(), res};
}

string json_str_array(stringify_state& state, const object_ptr& a) {
    stringify_state::nest nest{state, a};

    std::vector<string> partial;
    const auto len = to_uint32(a->get(L"length"));
    for (uint32_t index = 0; index < len; ++index) {
        auto str_p = json_str(state, index_string(index), a);
        if (str_p.type() == value_type::undefined) {
            partial.push_back(state.null_str());
        } else {
            assert(str_p.type() == value_type::string);
            partial.push_back(str_p.string_value());
        }
    }

    if (partial.empty()) {
        return state.global()->common_string("[]");
    }

    std::wstring res;
    res.push_back('[');
    res += nest.start_gap();
    bool first = true;
    for (const auto& s: partial) {
        if (!first) {
            res += nest.sep();
        }
        res += s.view();
        first = false;
    }
    res += nest.end_gap();
    res.push_back(']');
    return string{state.heap(), res};
}

value json_str(stringify_state& state, const std::wstring_view key, const object_ptr& holder) {
    // ES5.1, 15.12.3 Str(key, holder)
    // May only return a string or undefined

    auto& h = state.heap();
    // 1. Let value be the result of calling the [[Get]] internal method of holder with argument key.
    auto v = holder->get(key);

    // 2. If Type(value) is Object, then
    if (v.type() == value_type::object) {        
        auto o = v.object_value();
        auto to_json_value = o->get(L"toJSON");
        if (to_json_value.type() == value_type::object) {
            if (auto to_json = to_json_value.object_value(); to_json.has_type<function_object>()) {
                v = call_function(to_json_value, v, {value{string{h,key}}});
            }
        }
    }

    // 3. If ReplacerFunction is not undefined, then
    // TODO

    // 4. If Type(value) is Object then, 
    if (v.type() == value_type::object) {
        auto o = v.object_value();
        // FIXME: Check actual class or handle differently
        const auto class_name = o->class_name();
        if (class_name.view() == L"Number") {
            v = value{to_number(v)};
        } else if (class_name.view() == L"String") {
            v = value{to_string(h, v)};
        } else if (class_name.view() == L"Boolean") {
            v = value{static_cast<bool>(to_number(v))};
        }
    }

    switch (v.type()) {
    case value_type::undefined:
        return value::undefined;
    case value_type::null:
        return value{state.null_str()};
    case value_type::boolean:
        return value{state.bool_str(v.boolean_value())};
    case value_type::number:
        if (!std::isfinite(v.number_value())) {
            return value{state.null_str()};
        } else {
            return value{to_string(h, v.number_value())};
        }
    case value_type::string:
        return value{string{h, json_quote(v.string_value().view())}};
    default:
        break;
    }
    assert(v.type() == value_type::object);
    const auto o = v.object_value();
    if (o.has_type<function_object>()) {
        return value::undefined;
    } else {
        return value{is_array(o) ? json_str_array(state, o) : json_str_object(state, o) };
    }
}

} // unnamed namespace


value json_parse(const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    throw_not_implement("parse", global, args);
}

value json_stringify(const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    if (args.empty()) {
        return value::undefined;
    }

    auto wrapper = global->make_object();
    wrapper->put(global->common_string(""), args.front());

    stringify_state s{global, args};

    return json_str(s, L"", wrapper);
}

global_object_create_result make_json_object(global_object& global) {
    assert(global.language_version() >= version::es5);
    auto& h = global.heap();
    auto json = h.make<object>(string{h,"JSON"}, global.object_prototype());

    put_native_function(global, json, "parse", [global=global.self_ptr()](const value&, const std::vector<value>& args) {
        return json_parse(global, args);
    }, 2);
    
    put_native_function(global, json, "stringify", [global=global.self_ptr()](const value&, const std::vector<value>& args) {
        return json_stringify(global, args);
    }, 3);

    return { json, nullptr };
}

} // namespace mjs

