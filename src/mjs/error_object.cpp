#include "error_object.h"
#include "native_object.h"
#include "function_object.h"
#include <sstream>

namespace mjs {

constexpr const char* type_string(native_error_type e) {
    switch (e) {
    case native_error_type::generic:
        return "Error";
    case native_error_type::eval:
        return "EvalError";
    case native_error_type::range:
        return "RangeError";
    case native_error_type::reference:
        return "ReferenceError";
    case native_error_type::syntax:
        return "SyntaxError";
    case native_error_type::type:
        return "TypeError";
    case native_error_type::uri:
        return "URIError";
    case native_error_type::assertion:
        return "AssertionError";
    default:
        assert(false);
        throw std::runtime_error("Invalid error type " + std::to_string(static_cast<int>(e)));
    }
}

std::wostream& operator<<(std::wostream& os, native_error_type type) {
    return os << type_string(type);
}

class error_object : public native_object {
public:

    string to_string() const {
        auto& h = heap();
        std::wostringstream woss;
        woss << mjs::to_string(h, get_name()).view() << ": " << mjs::to_string(h, get(L"message")).view();
        return string{h, woss.str()};
    }

    [[noreturn]] void rethrow() const {
        auto& h = heap();
        throw native_error_exception{type_, stack_trace_.dereference(h).view(), mjs::to_string(h, get(L"message")).view()};
    }

private:
    friend gc_type_info_registration<error_object>;
    native_error_type                type_;
    value_representation             name_;
    gc_heap_ptr_untracked<gc_string> stack_trace_;

    void fixup() {
        auto& h = heap();
        name_.fixup(h);
        stack_trace_.fixup(h);
        native_object::fixup();
    }

    value get_name() const {
        return name_.get_value(heap());
    }

    void put_name(const value& v) {
        name_ = value_representation{v};
    }

    explicit error_object(native_error_type type, const string& class_name, const object_ptr& prototype, const string& stack_trace)
        : native_object{class_name, prototype}
        , type_(type)
        , name_(value_representation{value{class_name}})
        , stack_trace_(stack_trace.unsafe_raw_get()) {
        DEFINE_NATIVE_PROPERTY(error_object, name);
    }
};
static_assert(!gc_type_info_registration<error_object>::needs_destroy);
static_assert(gc_type_info_registration<error_object>::needs_fixup);

namespace {

std::string get_eval_exception_repr(native_error_type type, const std::wstring_view& stack_trace, const std::wstring_view& msg) {
    std::ostringstream oss;
    oss << type_string(type) << ": "<< std::string(msg.begin(), msg.end());
    if (!stack_trace.empty()) {
        oss << '\n' << std::string(stack_trace.begin(), stack_trace.end());
    }
    return oss.str();
}

constexpr auto message_attributes = property_attribute::dont_delete | property_attribute::dont_enum;

} // unnamed namespace


global_object_create_result make_error_object(const gc_heap_ptr<global_object>& global) {
    auto err_str = global->common_string("Error");

    object_ptr error_prototype = global->heap().make<error_object>(native_error_type::generic, err_str, global->object_prototype(), string{global->heap(), ""});

    gc_heap_ptr<function_object> error_constructor;
    for (const auto error_type: native_error_types) {
        auto n = global->common_string(type_string(error_type));
        auto prototype = error_type == native_error_type::generic ? error_prototype : global->heap().make<error_object>(error_type, n, error_prototype, string{global->heap(), ""});
        auto constructor = make_function(global, [error_type, n, prototype, global](const value&, const std::vector<value>& args) {
            auto& h = global->heap();
            auto eo = h.make<error_object>(error_type, n, prototype, string{h, global->stack_trace()});
            if (!args.empty() && args.front().type() != value_type::undefined) {
                eo->put(global->common_string("message"), value{to_string(h, args.front())}, message_attributes);
            }
            return value{eo};
        }, prototype->class_name().unsafe_raw_get(), 1);
        constructor->default_construct_function();

        constructor->put_prototype_with_attributes(prototype, global_object::prototype_attributes);

        prototype->redefine_own_property(global->common_string("constructor"), value{constructor}, global_object::prototype_attributes);
        prototype->put(string{global->heap(), "message"}, value{string{global->heap(), ""}}, message_attributes);

        if (!error_constructor) {
            assert(n.view() == L"Error");
            error_constructor = constructor;
        } else {
            global->put(n, value{constructor}, property_attribute::dont_enum);
        }
    }

    put_native_function(global, error_prototype, "toString", [global](const value& this_, const std::vector<value>&) {
        if (this_.type() == value_type::object) {
            auto& o = this_.object_value();
            if (o.has_type<error_object>()) {
                return value{static_cast<const error_object&>(*o).to_string()};
            }
        }
        return value{global->common_string("Error")};
    }, 0);

    return { error_constructor, nullptr };
}

native_error_exception::native_error_exception(native_error_type type, const std::wstring_view& stack_trace, const std::wstring_view& msg)
    : eval_exception{get_eval_exception_repr(type, stack_trace, msg)}
    , type_{type}
    , msg_{msg}
    , stack_trace_{stack_trace} {
}

native_error_exception::native_error_exception(native_error_type type, const std::wstring_view& stack_trace, const std::string_view& msg)
    : native_error_exception{type, stack_trace, std::wstring{msg.begin(), msg.end()}} {
}

object_ptr native_error_exception::make_error_object(const gc_heap_ptr<global_object>& global) const {
    auto& h = global->heap();
    auto eo = h.make<error_object>(type_, global->common_string(type_string(type_)), global->error_prototype(type_), string{h, stack_trace_});
    eo->put(global->common_string("message"), value{string{h, msg_}}, message_attributes);
    return eo;
}

[[noreturn]] void rethrow_error(const object_ptr& error) {
    assert(error.has_type<error_object>());
    static_cast<const error_object&>(*error).rethrow();
}

} // namespace mjs
