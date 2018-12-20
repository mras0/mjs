#include "regexp_object.h"
#include "function_object.h"
#include "array_object.h"
#include "native_object.h"
#include <sstream>
#include <regex>

namespace mjs {

enum class regexp_flag {
    none        = 0,
    global      = 1<<0,
    ignore_case = 1<<1,
    multiline   = 1<<2,
};

constexpr inline regexp_flag operator|(regexp_flag l, regexp_flag r) {
    return static_cast<regexp_flag>(static_cast<int>(l) | static_cast<int>(r));
}

constexpr inline regexp_flag operator&(regexp_flag l, regexp_flag r) {
    return static_cast<regexp_flag>(static_cast<int>(l) & static_cast<int>(r));
}

regexp_flag parse_regexp_flags(const std::wstring_view& s) {
    auto f = regexp_flag::none;
    for (const auto ch: s) {
        if (ch == 'g' && (f & regexp_flag::global) == regexp_flag::none) {
            f = f | regexp_flag::global;
        } else if (ch == 'i' && (f & regexp_flag::ignore_case) == regexp_flag::none) {
            f = f | regexp_flag::ignore_case;
        } else if (ch == 'm' && (f & regexp_flag::multiline) == regexp_flag::none) {
            f = f | regexp_flag::multiline;
        } else {
            NOT_IMPLEMENTED("throw SyntaxError");
        }
    }
    return f;
}

class regexp_object : public native_object {
public:
    static gc_heap_ptr<regexp_object> make(const gc_heap_ptr<global_object>& global, const string& pattern, const string& flags) {
        // TODO: ES3, 15.10.4.1
        // - "If pattern is an object R whose [[Class]] property is "RegExp" and flags is undefined , then return R unchanged."
        // - ..
        return global.heap().make<regexp_object>(global, pattern.view().empty() ? string{global.heap(), "(?:)"} : pattern, parse_regexp_flags(flags.view()));
    }

    static gc_heap_ptr<regexp_object> check_type(const value& v) {
        // TODO: See ES3, 15.10.6 on thowing TypeError
        if (v.type() != value_type::object) {
            NOT_IMPLEMENTED("throw TypeError");
        }
        auto o = v.object_value();
        if (!o.has_type<regexp_object>()) {
            NOT_IMPLEMENTED("throw TypeError");
        }
        return gc_heap_ptr<regexp_object>{o};
    }

    string to_string() const {
        std::wostringstream woss;
        woss << "/" << source_.track(heap()) << "/";
        if ((flags_ & regexp_flag::global) != regexp_flag::none) woss << "g";
        if ((flags_ & regexp_flag::ignore_case) != regexp_flag::none) woss << "i";
        if ((flags_ & regexp_flag::multiline) != regexp_flag::none) woss << "m";
        return string{heap(), woss.str()};
    }

    value exec(const string& str) {
        // TODO: Could optimize by not creating a new regex object each time,
        //       but need to be careful about not using too much (non-GC) memory.

        const bool global = (flags_ & regexp_flag::global) != regexp_flag::none;

        auto flags = std::regex_constants::ECMAScript;
        if ((flags_ & regexp_flag::ignore_case) != regexp_flag::none) flags |= std::regex_constants::icase;
        // TODO: Not implemented in MSVC yet
        //if ((flags_ & regexp_flag::multiline) != regexp_flag::none) flags |= std::regex_constants::multiline;


        const int32_t start_index = global ? static_cast<int32_t>(to_integer(last_index_.get_value(heap()))) : 0;
        last_index_ = value_representation{value{0.0}};
        if (start_index < 0 || static_cast<size_t>(start_index) > str.view().length()) {
            return value::null;
        }

        std::wregex regex{std::wstring{source_.dereference(heap()).view()}, flags};
        std::wsmatch match;
        std::wstring wstr{str.view()};
        if (!std::regex_search(wstr.cbegin() + start_index, wstr.cend(), match, regex)) {
            return value::null;
        }

        if (global) {
            last_index_ = value_representation{static_cast<double>(match[0].second - wstr.cbegin())};
        }

        auto res = make_array(global_.dereference(heap()).array_prototype(), {});

        res->put(string{heap(), "input"}, value{str});
        res->put(string{heap(), "index"}, value{static_cast<double>(match[0].first - wstr.cbegin())});

        for (uint32_t i = 0; i < static_cast<uint32_t>(match.size()); ++i) {
            res->put(string{heap(), index_string(i)}, value{string{heap(), match[i].str()}});
        }

        return value{res};
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

    explicit regexp_object(const gc_heap_ptr<global_object>& global, const string& source, regexp_flag flags)
        : native_object(global->common_string("RegExp"), global->regexp_prototype())
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

create_result make_regexp_object(global_object& global) {
//    auto& h = global.heap();
    auto prototype = global.make_object();
    auto regexp_str = global.common_string("RegExp");
    auto constructor = make_function(global, [global_ = global.self_ptr()](const value&, const std::vector<value>& args) {
        // TODO: ES3, 15.10.3.1, almost same as 15.10.4.1 -- hence the same function...
        auto& h = global_.heap();
        auto p = args.size() > 0 && args[0].type() != value_type::undefined? to_string(h, args[0]) : string{h, ""};
        auto f = args.size() > 1 && args[1].type() != value_type::undefined? to_string(h, args[1]) : string{h, ""};
        return value{regexp_object::make(global_, p, f)};
    }, regexp_str.unsafe_raw_get(), 2);
    constructor->default_construct_function();

    put_native_function(global, prototype, "toString", [](const value& this_, const std::vector<value>&) {
        return value{regexp_object::check_type(this_)->to_string()};
    }, 0);
    put_native_function(global, prototype, "exec", [](const value& this_, const std::vector<value>& args) {
        auto ro = regexp_object::check_type(this_);
        return ro->exec(to_string(ro.heap(), !args.empty()?args[0]:value::undefined));
    }, 0);
    put_native_function(global, prototype, "test", [](const value& this_, const std::vector<value>& args) {
        auto ro = regexp_object::check_type(this_);
        return value{ro->exec(to_string(ro.heap(), !args.empty()?args[0]:value::undefined)) != value::null};
    }, 0);

    prototype->put(global.common_string("constructor"), value{constructor}, global_object::prototype_attributes);

    return { constructor, prototype };
}

object_ptr make_regexp(const gc_heap_ptr<global_object>& global, const string& pattern, const string& flags) {
    return regexp_object::make(global, pattern, flags);
}

} // namespace mjs
