#include "string_object.h"
#include "native_object.h"
#include "function_object.h"
#include "array_object.h"
#include "regexp_object.h"
#include <cmath>

namespace mjs {

namespace {

inline const value& get_arg(const std::vector<value>& args, int index) {
    return index < static_cast<int>(args.size()) ? args[index] : value::undefined;
}

} // unnamed namespace


class string_object : public native_object {
public:
    value get(const std::wstring_view& name) const override {
        if (const auto s = handle_array_like_access(name); !s.empty()) {
            return value{string{heap(), s}};
        }
        return native_object::get(name);
    }

    // can_put needed?

    bool has_property(const std::wstring_view& name) const override {
        if (const auto s = handle_array_like_access(name); !s.empty()) {
            return true;
        }
        return native_object::has_property(name);
    }

    bool delete_property(const std::wstring_view& name) override {
        if (const auto s = handle_array_like_access(name); !s.empty()) {
            return false;
        }
        return native_object::delete_property(name);
    }

    bool check_own_property_attribute(const std::wstring_view& name, property_attribute mask, property_attribute expected) const override {
        if (const auto s = handle_array_like_access(name); !s.empty()) {
            return expected == property_attribute::none;
        }
        return native_object::check_own_property_attribute(name, mask, expected);
    }

private:
    friend gc_type_info_registration<string_object>;
    bool is_v5_or_later_;

    std::wstring_view handle_array_like_access(const std::wstring_view property_name) const {
        if (is_v5_or_later_) {
            if (const uint32_t index = index_value_from_string(property_name); index != invalid_index_value) {
                auto v = view();
                if (index < v.length()) {
                    return v.substr(index, 1);
                }
            }
        }
        return {};
    }

    std::wstring_view view() const {
        return internal_value().string_value().view();
    }

    value get_length() const {
        return value{static_cast<double>(view().length())};
    }

    explicit string_object(const object_ptr& prototype, const string& val, bool is_v5_or_later) : native_object(prototype->class_name(), prototype), is_v5_or_later_(is_v5_or_later) {
        DEFINE_NATIVE_PROPERTY_READONLY(string_object, length);
        internal_value(value{val});
    }

    void add_property_names(std::vector<string>& names) const override {
        native_object::add_property_names(names);
        if (is_v5_or_later_) {
            const auto s = view();
            auto& h = heap();
            for (auto i = 0U, l = static_cast<uint32_t>(s.length()); i < l; ++i) {
                names.push_back(string{h, index_string(i)});
            }
        }
    }

};

create_result make_string_object(global_object& global) {
    auto& h = global.heap();
    auto String_str_ = global.common_string("String");
    auto prototype = h.make<object>(String_str_, global.object_prototype());
    prototype->internal_value(value{string{h, ""}});

    auto c = make_function(global, [&h](const value&, const std::vector<value>& args) {
        return value{args.empty() ? string{h, ""} : to_string(h, args.front())};
    }, String_str_.unsafe_raw_get(), 1);
    make_constructable(global, c, [global = global.self_ptr()](const value&, const std::vector<value>& args) {
        auto& h = global.heap();
        return value{new_string(global, args.empty() ? string{h, ""} : to_string(h, args.front()))};
    });

    put_native_function(global, c, string{h, "fromCharCode"}, [&h](const value&, const std::vector<value>& args){
        std::wstring s;
        for (const auto& a: args) {
            s.push_back(to_uint16(a));
        }
        return value{string{h, s}};
    }, 0);

    auto check_type = [global = global.self_ptr(), prototype](const value& this_) {
        global->validate_type(this_, prototype, "String");
    };

    put_native_function(global, prototype, global.common_string("toString"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return this_.object_value()->internal_value();
    }, 0);
    put_native_function(global, prototype, global.common_string("valueOf"), [check_type](const value& this_, const std::vector<value>&){
        check_type(this_);
        return this_.object_value()->internal_value();
    }, 0);


    auto make_string_function = [&](const char* name, int num_args, auto f) {
        put_native_function(global, prototype, string{h, name}, [&h, f](const value& this_, const std::vector<value>& args){
            return value{f(to_string(h, this_), args)};
        }, num_args);
    };

    make_string_function("charAt", 1, [&h](const string& s, const std::vector<value>& args){
        const int position = to_int32(get_arg(args, 0));
        if (position < 0 || position >= static_cast<int>(s.view().length())) {
            return string{h, ""};
        }
        return string{h, s.view().substr(position, 1)};
    });

    make_string_function("charCodeAt", 1, [](const string& s, const std::vector<value>& args){
        const int position = to_int32(get_arg(args, 0));
        if (position < 0 || position >= static_cast<int>(s.view().length())) {
            return static_cast<double>(NAN);
        }
        return static_cast<double>(s.view()[position]);
    });

    make_string_function("indexOf", 2, [&h](const string& s, const std::vector<value>& args){
        const auto& search_string = to_string(h, get_arg(args, 0));
        const int position = to_int32(get_arg(args, 1));
        auto index = s.view().find(search_string.view(), position);
        return index == std::wstring_view::npos ? -1. : static_cast<double>(index);
    });

    make_string_function("lastIndexOf", 2, [&h](const string& s, const std::vector<value>& args){
        const auto& search_string = to_string(h, get_arg(args, 0));
        double position = to_number(get_arg(args, 1));
        const int ipos = std::isnan(position) ? INT_MAX : to_int32(position);
        auto index = s.view().rfind(search_string.view(), ipos);
        return index == std::wstring_view::npos ? -1. : static_cast<double>(index);
    });

    make_string_function("split", 1, [global = global.self_ptr()](const string& str, const std::vector<value>& args){
        const auto s = str.view();
        auto& h = global->heap();
        auto a = make_array(global, 0);
        if (args.empty()) {
            a->put(string{h, index_string(0)}, value{string{h, s}});
        } else {
            const auto sep = to_string(h, args.front());
            if (sep.view().empty()) {
                for (uint32_t i = 0; i < s.length(); ++i) {
                    a->put(string{h, index_string(i)}, value{string{ h, s.substr(i,1) }});
                }
            } else {
                size_t pos = 0;
                uint32_t i = 0;
                for (; pos < s.length(); ++i) {
                    const auto next_pos = s.find(sep.view(), pos);
                    if (next_pos == std::wstring_view::npos) {
                        break;
                    }
                    a->put(string{h, index_string(i)}, value{string{ h, s.substr(pos, next_pos-pos) }});
                    pos = next_pos + 1;
                }
                if (pos < s.length()) {
                    a->put(string{h, index_string(i)}, value{string{ h, s.substr(pos) }});
                }
            }
        }
        return a;
    });

    make_string_function("substring", 1, [&h](const string& str, const std::vector<value>& args) {
        const auto s = str.view();
        int start = std::min(std::max(to_int32(get_arg(args, 0)), 0), static_cast<int>(s.length()));
        if (args.size() < 2) {
            return string{h, s.substr(start)};
        }
        int end = std::min(std::max(to_int32(get_arg(args, 1)), 0), static_cast<int>(s.length()));
        if (start > end) {
            std::swap(start, end);
        }
        return string{h, s.substr(start, end-start)};
    });

    auto to_lower = [&h](const string& s, const std::vector<value>&){
        std::wstring res;
        for (auto c: s.view()) {
            res.push_back(towlower(c));
        }
        return string{h, res};
    };

    auto to_upper = [&h](const string& s, const std::vector<value>&){
        std::wstring res;
        for (auto c: s.view()) {
            res.push_back(towupper(c));
        }
        return string{h, res};
    };

    make_string_function("toLowerCase", 0, to_lower);
    make_string_function("toUpperCase", 0, to_upper);

    if (global.language_version() >= version::es3) {
        make_string_function("toLocaleLowerCase", 0, to_lower);
        make_string_function("toLocaleUpperCase", 0, to_upper);
        make_string_function("localeCompare", 1, [&h](const string& s, const std::vector<value>& args){
            const auto res = s.view().compare(to_string(h, get_arg(args, 0)).view());
            return value{ static_cast<double>(res < 0 ? 1 : res > 0 ? -1 : 0) };
        });
        make_string_function("concat", 1, [&h](const string& s, const std::vector<value>& args) {
            std::wstring res{s.view()};
            for (const auto& a: args) {
                res += to_string(h, a).view();
            }
            return value{string{h, res}};
        });
        make_string_function("slice", 2, [&h](const string& str, const std::vector<value>& args) {
            const auto s = str.view();
            const auto l = static_cast<uint32_t>(s.length());
            const auto ns = to_integer(get_arg(args, 0));
            const auto& end_arg = get_arg(args, 1);
            const auto ne = end_arg.type() == value_type::undefined ? l : to_integer(end_arg);
            const auto start = static_cast<uint32_t>(ns < 0 ? std::max(l + ns, 0.0) : std::min(ns, 0.0+l));
            const auto end   = static_cast<uint32_t>(ne < 0 ? std::max(l + ne, 0.0) : std::min(ne, 0.0+l));
            return value{string{h, s.substr(start, start < end ? end - start : 0)}};
        });
        make_string_function("match", 1, [global = global.self_ptr()](const string& s, const std::vector<value>& args) {
            return string_match(global, s, get_arg(args, 0));
        });
        make_string_function("search", 1, [global = global.self_ptr()](const string& s, const std::vector<value>& args) {
            return string_search(global, s, get_arg(args, 0));
        });
        make_string_function("replace", 2, [global = global.self_ptr()](const string& s, const std::vector<value>& args) {
            return string_replace(global, s, get_arg(args, 0), get_arg(args, 1));
        });
    }

    return {c, prototype};
}

object_ptr new_string(const gc_heap_ptr<global_object>& global, const string& val) {
    return global.heap().make<string_object>(global->string_prototype(), val, global->language_version() >= version::es5);
}

} // namespace mjs
