#include "regexp_object.h"
#include "function_object.h"
#include "array_object.h"
#include "native_object.h"
#include "error_object.h"
#include "regexp.h"
#include <sstream>

namespace mjs {

regexp_flag parse_regexp_flags(global_object& global, const string& s) {
    try {
        return regexp_flags_from_string(s.view());
    } catch (const std::exception& e) {
        throw native_error_exception{native_error_type::syntax, global.stack_trace(), e.what()};
    }
}

namespace {

const char* empty_string_regexp = "(?:)";

string get_source_string(const gc_heap_ptr<global_object>& global, const std::wstring_view s) {
    auto& h = global->heap();

    if (s.empty()) {
        return string{h, empty_string_regexp};
    }

    std::wstring res;
    bool escape = false;
    for (uint32_t i = 0, l = static_cast<uint32_t>(s.length()); i < l; ++i) {
        const auto ch = s[i];
        if (escape) {
            escape = false;
        } else if (ch == '\\') {
            escape = true;
        } else if (ch == '/') {
            res.push_back('\\');
        }
        res.push_back(ch);
    }

    return string{h, res};
}

} // unnamed namespace

// TODO: Could optimize by not creating a new regex object each time,
//       but need to be careful about not using too much (non-GC) memory.
class regexp_object : public native_object {
public:
    static gc_heap_ptr<regexp_object> make(const gc_heap_ptr<global_object>& global, const std::wstring_view& pattern, regexp_flag flags) {
        return global->heap().make<regexp_object>(global, global->regexp_prototype(), get_source_string(global, pattern), flags);
    }

    regexp_flag flags() const {
        return flags_;
    }

    string source() const {
        return source_.track(heap());
    }

    double last_index() const {
        // Could optimize by providing better accessors in value_representation
        return to_number(last_index_.get_value(heap()));
    }

    void last_index(uint32_t val) {
        last_index_ = value_representation{static_cast<double>(val)};
    }

    string to_string() const {
        std::wostringstream woss;
        woss << "/" << source_.track(heap()) << "/" << flags_;
        return string{heap(), woss.str()};
    }

    value exec(const string& str) {
        const bool global = (flags_ & regexp_flag::global) != regexp_flag::none;

        const int32_t start_index = global ? static_cast<int32_t>(to_integer(last_index_.get_value(heap()))) : 0;
        last_index_ = value_representation{value{0.0}};
        if (start_index < 0 || static_cast<size_t>(start_index) > str.view().length()) {
            return value::null;
        }

        const wchar_t* const str_beg = str.view().data();
        const wchar_t* const str_end = str_beg + str.view().length();
        const wchar_t* const search_start = str_beg + start_index;

        const auto match = regexp{source_.dereference(heap()).view(), flags_}.exec(std::wstring_view{search_start, static_cast<size_t>(str_end - search_start)});
        if (match.empty()) {
            return value::null;
        }

        if (global) {
            last_index_ = value_representation{static_cast<double>(match[0].second - str_beg)};
        }

        auto res = make_array(global_.track(heap()), 0);

        res->put(string{heap(), "input"}, value{str});
        res->put(string{heap(), "index"}, value{static_cast<double>(match[0].first - str_beg)});

        for (uint32_t i = 0; i < static_cast<uint32_t>(match.size()); ++i) {
            res->put(string{heap(), index_string(i)}, value{string{heap(), match[i].str()}});
        }

        return value{res};
    }

    double search(const string& str) const {
        const auto index = regexp{source_.dereference(heap()).view(), flags_}.search(str.view());
        return index == regexp::npos ? -1. : static_cast<double>(index);
    }

private:
    friend gc_type_info_registration<regexp_object>;
    gc_heap_ptr_untracked<global_object>    global_;
    gc_heap_ptr_untracked<gc_string>        source_;
    value_representation                    last_index_;
    regexp_flag                             flags_;

    value get_source() const {
        return value{source_.track(heap())};
    }

    value get_global() const {
        return value{(flags_ & regexp_flag::global) != regexp_flag::none};
    }

    value get_ignoreCase() const {
        return value{(flags_ & regexp_flag::ignore_case) != regexp_flag::none};
    }

    value get_multiline() const {
        return value{(flags_ & regexp_flag::multiline) != regexp_flag::none};
    }

    value get_lastIndex() const {
        return last_index_.get_value(heap());
    }

    void put_lastIndex(const value& v) {
        last_index_ = value_representation{v};
    }

    explicit regexp_object(const gc_heap_ptr<global_object>& global, const object_ptr& prototype, const string& source, regexp_flag flags)
        : native_object(global->common_string("RegExp"), prototype)
        , global_(global)
        , source_(source.unsafe_raw_get())
        , last_index_(value_representation{0.0})
        , flags_(flags) {
        DEFINE_NATIVE_PROPERTY_READONLY(regexp_object, source);
        DEFINE_NATIVE_PROPERTY_READONLY(regexp_object, global);
        DEFINE_NATIVE_PROPERTY_READONLY(regexp_object, ignoreCase);
        DEFINE_NATIVE_PROPERTY_READONLY(regexp_object, multiline);
        DEFINE_NATIVE_PROPERTY(regexp_object, lastIndex);
    }

    void fixup() {
        auto& h = heap();
        global_.fixup(h);
        source_.fixup(h);
        last_index_.fixup(h);
        native_object::fixup();
    }
};
static_assert(!gc_type_info_registration<regexp_object>::needs_destroy);
static_assert(gc_type_info_registration<regexp_object>::needs_fixup);

namespace {

gc_heap_ptr<regexp_object> cast_to_regexp(const value& v) {
    if (v.type() != value_type::object) {
        return nullptr;
    }
    auto o = v.object_value();
    if (!o.has_type<regexp_object>()) {
        return nullptr;
    }
    return gc_heap_ptr<regexp_object>{o};
}

gc_heap_ptr<regexp_object> check_type(const gc_heap_ptr<global_object>& global, const value& v) {
    if (auto r = cast_to_regexp(v)) {
        return r;
    }
    std::wostringstream woss;
    if (v.type() == value_type::object) {
        woss << v.object_value()->class_name();
    } else {
        debug_print(woss, v, 4);
    }
    woss << " is not a RegExp";
    throw native_error_exception{native_error_type::type, global->stack_trace(), woss.str()};
}

} // unnamed namespace

global_object_create_result make_regexp_object(const gc_heap_ptr<global_object>& global) {
    // In ES5+ the prototype object is a RegExp object
    auto prototype = global->language_version() < version::es5 ? global->make_object() : global->heap().make<regexp_object>(global, global->object_prototype(), string{global->heap(), empty_string_regexp}, regexp_flag::none);
    auto regexp_str = global->common_string("RegExp");

    auto construct_regexp = [global](const value&, const std::vector<value>& args) {
        auto& h = global.heap();

        string pattern{h, ""};
        regexp_flag flags{};
        if (!args.empty()) {
            const bool has_flags_argument = args.size() > 1 && args[1].type() != value_type::undefined;

            if (auto r = cast_to_regexp(args[0])) {
                if (has_flags_argument) {
                    throw native_error_exception{native_error_type::type, global->stack_trace(), L"Invalid flags argument to RegExp constructor"};
                }
                pattern = r->source();
                flags = r->flags();
            } else {
                if (args[0].type() != value_type::undefined) {
                    pattern = to_string(h, args[0]);
                }
                if (has_flags_argument) {
                    flags = parse_regexp_flags(*global, to_string(h, args[1]));
                }
            }
        }

        return value{regexp_object::make(global, pattern.view(), flags)};
    };

    auto constructor = make_function(global, [construct_regexp](const value& this_, const std::vector<value>& args) {
        // ES3, 15.10.3.1 if pattern is a RegExp and flags is undefined, return it unchanged
        if (args.size() >= 1 && cast_to_regexp(args[0]) && (args.size() == 1 || args[1].type() == value_type::undefined)) {
            return args[0];
        }
        return construct_regexp(this_, args);
    }, regexp_str.unsafe_raw_get(), 2);
    constructor->construct_function(construct_regexp);

    put_native_function(global, prototype, "toString", [global](const value& this_, const std::vector<value>&) {
        return value{check_type(global, this_)->to_string()};
    }, 0);
    put_native_function(global, prototype, "exec", [global](const value& this_, const std::vector<value>& args) {
        return check_type(global, this_)->exec(to_string(global->heap(), !args.empty()?args[0]:value::undefined));
    }, 0);
    put_native_function(global, prototype, "test", [global](const value& this_, const std::vector<value>& args) {
        return value{check_type(global, this_)->exec(to_string(global->heap(), !args.empty()?args[0]:value::undefined)) != value::null};
    }, 0);

    prototype->put(global->common_string("constructor"), value{constructor}, global_object::prototype_attributes);

    return { constructor, prototype };
}

object_ptr make_regexp(const gc_heap_ptr<global_object>& global, const regexp& re) {
    return regexp_object::make(global, re.pattern(), re.flags());
}

object_ptr make_regexp(const gc_heap_ptr<global_object>& global, const string& pattern, const string& flags) {
    return regexp_object::make(global, pattern.view(), parse_regexp_flags(*global, flags));
}

namespace {

gc_heap_ptr<regexp_object> to_regexp_object(const gc_heap_ptr<global_object>& global, const value& regexp) {
    if (regexp.type() == value_type::object) {
        const auto& o = regexp.object_value();
        if (o.has_type<regexp_object>()) {
            return gc_heap_ptr<regexp_object>{o};
        }
    }
    auto& h = global->heap();
    return regexp_object::make(global, to_string(h, regexp).view(), regexp_flag::none);
}

string do_get_replacement_string(const string& str, const object_ptr& match, const value& replace_value) {
    if (replace_value.type() == value_type::string) {
        const auto rep_str = replace_value.string_value();
        if (rep_str.view().find_first_of(L'$') == std::wstring_view::npos) {
            // Fast and easy
            return rep_str;
        }

        std::wstring res;
        const auto r = rep_str.view();
        for (uint32_t i = 0, l = static_cast<uint32_t>(r.length()); i < l;) {
            if (r[i] != L'$') {
                res += r[i];
                ++i;
                continue;
            }
            ++i;
            if (i >= l) {
                NOT_IMPLEMENTED("$ at end of string");
            }
            if (r[i] == '$') {
                res += L'$';
                ++i;
            } else if (r[i] == L'&') {
                // The matched substring
                res += match->get(L"0").string_value().view();
                ++i;
            } else if (r[i] == L'`') {
                //The portion of string that precedes the matched substring.
                ++i;
                res += str.view().substr(0, to_uint32(match->get(L"index")));
            } else if (r[i] == L'\'') {
                //The portion of string that follows the matched substring.
                ++i;
                res += str.view().substr(to_uint32(match->get(L"index")) + match->get(L"0").string_value().view().length());
            } else if (isdigit(r[i])) {
                uint32_t idx = r[i]-L'0';
                ++i;
                if (i < l && isdigit(r[i])) {
                    idx = idx*10+r[i]-L'0';
                    ++i;
                }
                if (!idx) {
                    NOT_IMPLEMENTED("$0 used");
                }
                auto cval = match->get(index_string(idx));
                if (cval.type() == value_type::string) {
                    res += cval.string_value().view();
                } else {
                    // Using empty string
                }
            } else {
                NOT_IMPLEMENTED("Invalid replacement characters");
            }
        }

        return string{match.heap(), res};
    }

    std::vector<value> args;
    args.push_back(match->get(L"0"));
    const auto m = to_uint32(match->get(L"length"));
    for (uint32_t i = 1; i < m; ++i) {
        args.push_back(match->get(index_string(i)));
    }
    args.push_back(match->get(L"index"));
    args.push_back(value{str});

    return to_string(match.heap(), call_function(replace_value, value::undefined, args));
}

string do_replace(const string& str, const value& match_val, const value& replace_value) {
    if (match_val.type() == value_type::null) {
        return str;
    }
    assert(match_val.type() == value_type::object);
    const auto match           = match_val.object_value();
    const auto s               = std::wstring{str.view()};
    const auto match_index_val = match->get(L"index");
    const auto match_str       = match->get(L"0").string_value();
    const auto match_index     = static_cast<uint32_t>(match_index_val.number_value());
    const auto match_length    = static_cast<uint32_t>(match_str.view().length());

    auto& h = match.heap();

    std::wstring res;
    res = s.substr(0, match_index);
    res += do_get_replacement_string(str, match, replace_value).view();
    res += s.substr(match_index + match_length);
    return string{h, res};
}

string do_global_replace(const string& str, const gc_heap_ptr<regexp_object>& re, const value& replace_value) {
    auto& h = re.heap();
    const auto s = std::wstring{str.view()};

    uint32_t last_index = 0;
    re->last_index(last_index);

    std::wstring res;
    for (uint32_t i=0;; ++i) {
        auto match = re->exec(str);
        if (match.type() == value_type::null) {
            break;
        }
        assert(match.type() == value_type::object);
        const auto& match_object = match.object_value();
        const auto match_index = static_cast<uint32_t>(match_object->get(L"index").number_value());

        res += str.view().substr(last_index, match_index-last_index);
        res += do_get_replacement_string(str, match_object, replace_value).view();

        const uint32_t new_last_index = static_cast<uint32_t>(re->last_index());
        if (last_index == re->last_index()) {
            // Empty match
            last_index = new_last_index + 1;
            re->last_index(last_index);
        } else {
            last_index = new_last_index;
        }
    }

    if (!last_index) {
        return str;
    }

    if (last_index < s.length()) {
        res += s.substr(last_index);
    }

    return string{h, res};
}

} // unnamed namespace

value string_match(const gc_heap_ptr<global_object>& global, const string& str, const value& regexp) {
    auto re = to_regexp_object(global, regexp);
    if ((re->flags() & regexp_flag::global) == regexp_flag::none) {
        return re->exec(str);
    }
    // Global regexp
    re->last_index(0);

    auto res = make_array(global, 0);
    auto& h = global->heap();
    for (uint32_t i=0;; ++i) {
        const auto last_index_before = re->last_index();
        auto match = re->exec(str);
        if (match.type() == value_type::null) {
            break;
        }
        assert(match.type() == value_type::object);
        const auto& match_object = match.object_value();
        res->put(string{h, index_string(i)}, match_object->get(L"0"));
        if (last_index_before == re->last_index()) {
            // Empty match
            re->last_index(static_cast<uint32_t>(last_index_before + 1));
        }
    }
    return value{res};
}

value string_search(const gc_heap_ptr<global_object>& global, const string& str, const value& regexp) {
    return value{to_regexp_object(global, regexp)->search(str)};
}

value string_replace(const gc_heap_ptr<global_object>& global, const string& str, const value& search_value, const value& replace_value) {
    auto& h = global->heap();

    value replace_val = replace_value;
    if (replace_val.type() != value_type::object || !replace_val.object_value().has_type<function_object>()) {
        replace_val = value{to_string(h, replace_val)};
    }

    if (search_value.type() == value_type::object) {
        auto o = search_value.object_value();
        if (o.has_type<regexp_object>()) {
            gc_heap_ptr<regexp_object> re{o};
            if ((re->flags() & regexp_flag::global) == regexp_flag::none) {
                auto match = re->exec(str);
                return value{do_replace(str, match, replace_val)};
            }
            return value{do_global_replace(str, re, replace_val)};
        }
    }

    auto search_string = to_string(h, search_value);
    auto idx = str.view().find(search_string.view());
    if (idx == std::wstring_view::npos) {
        return value{str};
    }
    // Fake up a match object
    auto match = make_array(global, 0);
    match->put(string{h, L"0"}, value{search_string});
    match->put(global->common_string("index"), value{static_cast<double>(idx)});

    return value{do_replace(str, value{match}, replace_val)};
}


} // namespace mjs
