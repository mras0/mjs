#include "json_object.h"
#include "function_object.h"
#include "array_object.h"
#include "error_object.h"
#include "lexer.h"
#include <sstream>
#include <cmath>

namespace mjs {


//
// parse
//

namespace {

enum class json_token_type {
    whitespace,
    string,
    number,
    null,
    false_,
    true_,
    lbracket,
    rbracket,
    lbrace,
    rbrace,
    comma,
    colon,
    eof,
};

std::wostream& operator<<(std::wostream& os, json_token_type type) {
    switch (type) {
    case json_token_type::whitespace:   return os << "whitespace";
    case json_token_type::string:       return os << "string";
    case json_token_type::number:       return os << "number";
    case json_token_type::null:         return os << "null";
    case json_token_type::false_:       return os << "false";
    case json_token_type::true_:        return os << "true";
    case json_token_type::lbracket:     return os << "lbracket";
    case json_token_type::rbracket:     return os << "rbracket";
    case json_token_type::lbrace:       return os << "lbrace";
    case json_token_type::rbrace:       return os << "rbrace";
    case json_token_type::comma:        return os << "comma";
    case json_token_type::colon:        return os << "colon";
    case json_token_type::eof:          return os << "eof";
    }
    assert(false);
    NOT_IMPLEMENTED(static_cast<int>(type));
}

class json_token {
public:
    explicit json_token(json_token_type type, size_t pos) : type_(type), pos_(pos) {
        assert(type_ != json_token_type::string && type_ != json_token_type::number);
    }
    explicit json_token(json_token_type type, std::wstring&& text, size_t pos) : type_(type), text_(std::move(text)), pos_(pos) {
        assert(type_ == json_token_type::string || type_ == json_token_type::number);
    }

    json_token_type type() const { return type_; }
    const std::wstring& text() const {
        assert(type_ == json_token_type::string || type_ == json_token_type::number);
        return text_;
    }
    size_t pos() const { return pos_; }

private:
    json_token_type type_;
    std::wstring text_;
    size_t pos_;
};

std::wostream& operator<<(std::wostream& os, const json_token& tok) {
    if (tok.type() == json_token_type::string) {
        return os << '"' << cpp_quote(tok.text()) << '"';
    } else if (tok.type() == json_token_type::number) {
        return os << tok.text();
    } else {
        return os << tok.type();
    }
}

constexpr bool json_is_whitespace(char16_t ch) {
    return ch == '\t' || ch == '\r' || ch == '\n' || ch == ' ';
}

constexpr bool json_is_digit(char16_t ch) {
    return ch >= '0' && ch <= '9';
}

class json_lexer {
public:
    explicit json_lexer(const gc_heap_ptr<global_object>& global, std::wstring&& text)
        : global_{global}
        , text_{std::move(text)}
        , pos_{0}
        , tok_{json_token_type::eof, 0} {
        next_token();
    }

    const gc_heap_ptr<global_object>& global() const { return global_; }

    const std::wstring& text() const {
        return text_;
    }

    bool eof() const {
        return tok_.type() == json_token_type::eof;
    }

    const json_token& current_token() const {
        return tok_;
    }

    json_token next_token() {
        const auto len = text_.length();
        if (pos_ >= len) {
            tok_ = json_token{json_token_type::eof, pos_};
            return tok_;
        }

        const auto start = pos_;
        const char16_t ch = text_[pos_];
        auto end = ++pos_;

        if (json_is_whitespace(ch)) {
            while (end < len && json_is_whitespace(text_[end])) {
                ++end;
            }
            tok_ = json_token{json_token_type::whitespace, start};
        } else if (ch == '[') {
            tok_ = json_token{json_token_type::lbracket, start};
        } else if (ch == ']') {
            tok_ = json_token{json_token_type::rbracket, start};
        } else if (ch == '{') {
            tok_ = json_token{json_token_type::lbrace, start};
        } else if (ch == '}') {
            tok_ = json_token{json_token_type::rbrace, start};
        } else if (ch == ',') {
            tok_ = json_token{json_token_type::comma, start};
        } else if (ch == ':') {
            tok_ = json_token{json_token_type::colon, start};
        } else if (ch == 'n' && text_.compare(end, 3, L"ull") == 0) {
            end += 3;
            tok_ = json_token{json_token_type::null, start};
        } else if (ch == 'f' && text_.compare(end, 4, L"alse") == 0) {
            end += 4;
            tok_ = json_token{json_token_type::false_, start};
        } else if (ch == 't' && text_.compare(end, 3, L"rue") == 0) {
            end += 3;
            tok_ = json_token{json_token_type::true_, start};
        } else if (ch == '-' || json_is_digit(ch)) {
            auto invalid = [&]() {
                std::wostringstream woss;
                woss << "Invalid number in JSON at position " << start;
                throw native_error_exception{native_error_type::syntax, global_->stack_trace(), woss.str()};
            };
            auto decimal_digits = [&]() {
                bool any = false;
                while (end < len && isdigit(text_[end])) {
                    any = true;
                    ++end;
                }
                return any;
            };
            if (!decimal_digits() && ch == '-') {
                invalid();
            }
            if (end < len && text_[end] == '.') {
                ++end;
                if (!decimal_digits()) {
                    invalid();
                }
            }
            if (end < len && (text_[end] == 'e' || text_[end] == 'E')) {
                ++end;
                if (end == len) {
                    invalid();
                }
                if (text_[end] == '-' || text_[end] == '+') {
                    ++end;
                }
                if (!decimal_digits()) {
                    invalid();
                }
            }
            tok_ = json_token{json_token_type::number, text_.substr(start, end-start), start};
        } else if (ch == '"') {
            std::wstring s;
            for (bool escape = false;;) {
                if (end == len) {
                    std::wostringstream woss;
                    woss << "Unterminated string in JSON at position " << start;
                    throw native_error_exception{native_error_type::syntax, global_->stack_trace(), woss.str()};
                }
                const char16_t sch = text_[end++];
                if (escape) {
                    if (end == len) {
                        std::wostringstream woss;
                        woss << "Unterminated escape sequence in JSON at position " << end-1;
                        throw native_error_exception{native_error_type::syntax, global_->stack_trace(), woss.str()};
                    }
                    switch (sch) {
                    case '"': [[fallthrough]];
                    case '/': [[fallthrough]];
                    case '\\':
                        s.push_back(sch);
                        break;
                    case 'b': s.push_back(8); break;
                    case 'f': s.push_back(12); break;
                    case 'n': s.push_back(10); break;
                    case 'r': s.push_back(13); break;
                    case 't': s.push_back(9); break;
                    case 'u':
                        if (end + 5 > len) {
                            std::wostringstream woss;
                            woss << "Unterminated escape sequence in JSON at position " << end-1;
                            throw native_error_exception{native_error_type::syntax, global_->stack_trace(), woss.str()};
                        }
                        try {
                            s.push_back(static_cast<char16_t>(get_hex_value4(&text_[end])));
                        } catch (const std::runtime_error&) {
                            std::wostringstream woss;
                            woss << "Invalid escape sequence in JSON at position " << end-1;
                            throw native_error_exception{native_error_type::syntax, global_->stack_trace(), woss.str()};
                        }
                        end+=4;
                        break;
                    default:
                        {
                            std::wostringstream woss;
                            woss << "Invalid escape sequence \"\\" << (wchar_t)sch << "\" in JSON at position " << end-1;
                            throw native_error_exception{native_error_type::syntax, global_->stack_trace(), woss.str()};
                        }
                    }
                    escape = false;
                } else if (sch == '"') {
                    break;
                } else if (sch == '\\') {
                    escape = true;
                } else {
                    s.push_back(sch);
                }
            }
            tok_ = json_token{json_token_type::string, std::move(s), start};
        } else {
            std::wostringstream woss;
            woss << "Unexpected token \"" << cpp_quote(text_.substr(start, 1)) << "\" in JSON at position " << start;
            throw native_error_exception{native_error_type::syntax, global_->stack_trace(), woss.str()};
        }

        pos_ = end;
        return tok_;
    }

    void skip_whitespace() {
        while (current_token().type() == json_token_type::whitespace) {
            next_token();
        }
    }

    [[noreturn]] void throw_unexpected(const json_token& token) {
        std::wostringstream woss;
        woss << "Unexpected ";
        if (token.type() == json_token_type::eof) {
            woss << "end of input";
        } else {
            woss << "token " << token << " in JSON";
        }
        woss << " at position " << token.pos();
        throw native_error_exception{native_error_type::syntax, global()->stack_trace(), woss.str()};
    }

    [[noreturn]] void throw_unexpected() {
        throw_unexpected(current_token());
    }

private:
    gc_heap_ptr<global_object> global_;
    std::wstring               text_;
    size_t                     pos_;
    json_token                 tok_;
};

value json_parse_value(json_lexer& lex);

value json_parse_array(json_lexer& lex) {
    auto a = make_array(lex.global(), 0);
    auto& h = lex.global().heap();
    for (uint32_t i = 0;; ++i) {
        lex.skip_whitespace();
        if (lex.current_token().type() == json_token_type::rbracket) {
            lex.next_token();
            break;
        }
        if (i != 0) {
            if (lex.current_token().type() != json_token_type::comma) {
                lex.throw_unexpected();
            }
            lex.next_token();
            lex.skip_whitespace();
        }
        a->put(string{h, index_string(i)}, json_parse_value(lex));
    }

    return value{a};
}

value json_parse_object(json_lexer& lex) {
    auto o = lex.global()->make_object();
    auto& h = lex.global().heap();
    for (uint32_t i = 0;; ++i) {
        lex.skip_whitespace();
        if (lex.current_token().type() == json_token_type::rbrace) {
            lex.next_token();
            break;
        }
        if (i != 0) {
            if (lex.current_token().type() != json_token_type::comma) {
                lex.throw_unexpected();
            }
            lex.next_token();
            lex.skip_whitespace();
        }
        if (lex.current_token().type() != json_token_type::string) {
            lex.throw_unexpected();
        }
        const auto key = lex.current_token().text();
        lex.next_token();
        lex.skip_whitespace();
        if (lex.current_token().type() != json_token_type::colon) {
            lex.throw_unexpected();
        }
        lex.next_token();
        lex.skip_whitespace();

        o->put(string{h, key}, json_parse_value(lex));
    }

    return value{o};
}

value json_parse_value(json_lexer& lex) {
    lex.skip_whitespace();
    auto token = lex.current_token();
    lex.next_token();
    switch (token.type()) {
    case json_token_type::string:       return value{string{lex.global().heap(), token.text()}};
    case json_token_type::number:       return value{to_number(token.text())};
    case json_token_type::null:         return value::null;
    case json_token_type::false_:       return value{false};
    case json_token_type::true_:        return value{true};
    case json_token_type::lbracket:     return json_parse_array(lex);
    case json_token_type::lbrace:       return json_parse_object(lex);
    default:
        break;
    }
    lex.throw_unexpected(token);
}

value json_walk(const gc_heap_ptr<global_object>& global, const value& reviver, const object_ptr& holder, const std::wstring_view name) {
    auto& h = global.heap();

    const auto val = holder->get(name);
    if (val.type() == value_type::object) {
        const auto& o = val.object_value();
        if (is_array(o)) {
            const uint32_t len = to_uint32(o->get(L"length"));
            for (uint32_t i = 0; i < len; ++i) {
                const auto is = index_string(i);
                auto new_element = json_walk(global, reviver, o, is);
                if (new_element.type() == value_type::undefined) {
                    o->delete_property(is);
                } else {
                    o->put(string{h, is}, new_element);
                }
            }
        } else {
            const auto keys = o->own_property_names(true);
            for (const auto& p: keys) {
                const std::wstring key{p.view()}; // Keep local copy in case p gets moved during the walk
                auto new_element = json_walk(global, reviver, o, key);
                if (new_element.type() == value_type::undefined) {
                    o->delete_property(p.view());
                } else {
                    o->put(p, new_element);
                }
            }
        }
    }
    return call_function(reviver, value{holder}, { value{string{h, name}}, val });
}

} // unnamed namespace

value json_parse(const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    json_lexer lex{global, std::wstring{args.empty() ? L"undefined" : to_string(global.heap(), args.front()).view()}};
    value val = json_parse_value(lex);
    lex.skip_whitespace();
    if (!lex.eof()) {
        lex.throw_unexpected();
    }
    if (args.size() > 1 && args[1].type() == value_type::object && args[1].object_value().has_type<function_object>()) {
        auto holder = global->make_object();
        holder->put(global->common_string(""), val);
        return json_walk(global, args[1], holder, L"");
    }
    return val;
}

//
// stringify
//

namespace {

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
            const auto& o = args[1].object_value();
            if (o.has_type<function_object>()) {
                replacer_ = o;
            } else if (is_array(o)) {
                auto& h = heap();
                const uint32_t len = to_uint32(o->get(L"length"));
                property_names_ = gc_vector<gc_heap_ptr_untracked<gc_string>>::make(h, len);
                auto insert_item = [&](const string& item) {
                    // Check if the item is already in the list
                    auto n = property_names_->data();
                    for (uint32_t i = 0, l = property_names_->length(); i < l; ++i) {
                        if (n[i].dereference(h).view() == item.view()) {
                            return;
                        }
                    }
                    property_names_->push_back(item.unsafe_raw_get());
                };
                for (uint32_t i = 0; i < len; ++i) {
                    auto item = o->get(index_string(i));
                    if (item.type() == value_type::string) {
                        insert_item(item.string_value());
                    } else if (item.type() == value_type::number) {
                        insert_item(to_string(h, item.number_value()));
                    } else if (item.type() == value_type::object) {
                        auto io = item.object_value();
                        if (io->class_name().view() == L"String" || io->class_name().view() == L"Number") {
                            insert_item(to_string(h, item));
                        }
                    }
                }
            }
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

    void call_replacer(value& v, const std::wstring_view key, const object_ptr& holder) {
        if (!replacer_) {
            return;
        }
        assert(replacer_.has_type<function_object>());
        v = call_function(value{replacer_}, value{holder}, {value{string{heap(), key}}, v});
    }

    std::vector<string> property_list(const object_ptr& o) {
        if (!property_names_) {
            return o->own_property_names(true);
        } else {
            std::vector<string> k;
            auto n = property_names_->data();
            auto& h = heap();
            for (uint32_t i = 0, l = property_names_->length(); i < l; ++i) {
                k.push_back(n[i].track(h));
            }
            return k;
        }
    }

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
    gc_heap_ptr<global_object>                                  global_;
    gc_heap_ptr<gc_vector<gc_heap_ptr_untracked<object>>>       stack_ = nullptr;
    object_ptr                                                  replacer_ = nullptr;
    gc_heap_ptr<gc_vector<gc_heap_ptr_untracked<gc_string>>>    property_names_ = nullptr;
    std::wstring                                                indent_;
    std::wstring                                                gap_;

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
    auto k = state.property_list(o);

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
    state.call_replacer(v, key, holder);

    // 4. If Type(value) is Object then, 
    if (v.type() == value_type::object) {        
        if (auto o = v.object_value(); is_primitive_object(*o)) {
            v = o->internal_value();
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

value json_stringify(const gc_heap_ptr<global_object>& global, const std::vector<value>& args) {
    if (args.empty()) {
        return value::undefined;
    }

    auto wrapper = global->make_object();
    wrapper->put(global->common_string(""), args.front());

    stringify_state s{global, args};

    return json_str(s, L"", wrapper);
}

global_object_create_result make_json_object(const gc_heap_ptr<global_object>& global) {
    assert(global->language_version() >= version::es5);
    auto& h = global.heap();
    auto json = h.make<object>(string{h,"JSON"}, global->object_prototype());

    put_native_function(global, json, "parse", [global](const value&, const std::vector<value>& args) {
        auto g = global; // Keep local copy in case the function gets GC'ed
        return json_parse(g, args);
    }, 2);
    
    put_native_function(global, json, "stringify", [global](const value&, const std::vector<value>& args) {
        auto g = global; // Keep local copy in case the function gets GC'ed
        return json_stringify(g, args);
    }, 3);

    return { json, nullptr };
}

} // namespace mjs

