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

class error_object : public native_object {
public:

    string to_string() const {
        auto& h = heap();
        std::wostringstream woss;
        woss << type_string(type_) << ": " << mjs::to_string(h, message_.get_value(h)).view();
        return string{h, woss.str()};
    }

    [[noreturn]] void rethrow() const {
        auto& h = heap();
        throw native_error_exception{type_, stack_trace_.dereference(h).view(), mjs::to_string(h, message_.get_value(h)).view()};
    }

private:
    friend gc_type_info_registration<error_object>;
    native_error_type                type_;
    value_representation             name_;
    value_representation             message_;
    gc_heap_ptr_untracked<gc_string> stack_trace_;

    void fixup() {
        auto& h = heap();
        name_.fixup(h);
        message_.fixup(h);
        stack_trace_.fixup(h);
        native_object::fixup();
    }

    value get_name() const {
        return name_.get_value(heap());
    }

    void put_name(const value& v) {
        name_ = value_representation{v};
    }
    
    value get_message() const {
        return message_.get_value(heap());
    }

    void put_message(const value& v) {
        message_ = value_representation{v};
    }

    explicit error_object(native_error_type type, const string& class_name, const object_ptr& prototype, const value& message, const string& stack_trace)
        : native_object{class_name, prototype}
        , type_(type)
        , name_(value_representation{value{class_name}})
        , message_(value_representation{message})
        , stack_trace_(stack_trace.unsafe_raw_get()) {
        DEFINE_NATIVE_PROPERTY(error_object, name);
        DEFINE_NATIVE_PROPERTY(error_object, message);
    }
};
static_assert(!gc_type_info_registration<error_object>::needs_destroy);
static_assert(gc_type_info_registration<error_object>::needs_fixup);

create_result make_error_object(global_object& global) {
    auto prototype = global.heap().make<error_object>(native_error_type::generic, global.common_string("Error"), global.object_prototype(), value::undefined, string{global.heap(), ""});

    gc_heap_ptr<function_object> error_constructor;
    for (const auto error_type: {native_error_type::generic, native_error_type::eval, native_error_type::range, native_error_type::reference, native_error_type::syntax, native_error_type::type, native_error_type::uri}) {
        auto n = global.common_string(type_string(error_type));
        auto constructor = make_function(global, [error_type, n, global = global.self_ptr()](const value&, const std::vector<value>& args) {
            auto& h = global.heap();
            auto prototype = global->error_prototype();
            string stack_trace{h, "TODO: Stack trace in error constructor"};
            return value{h.make<error_object>(error_type, n, prototype, value{args.empty() || args[0].type() == value_type::undefined ? string{h, ""} : to_string(h, args[0])}, stack_trace)};
        }, prototype->class_name().unsafe_raw_get(), 1);
        constructor->default_construct_function();
        if (!error_constructor) {
            assert(n.view() == L"Error");
            error_constructor = constructor;
        } else {
            global.put(n, value{constructor}, property_attribute::dont_enum);
        }
    }

    put_native_function(global, prototype, "toString", [global = global.self_ptr()](const value& this_, const std::vector<value>&) {
        if (this_.type() == value_type::object) {
            auto& o = this_.object_value();
            if (o.has_type<error_object>()) {
                return value{static_cast<const error_object&>(*o).to_string()};
            }
        }
        return value{global->common_string("Error")};
    }, 0);

    prototype->put(global.common_string("constructor"), value{error_constructor}, global_object::prototype_attributes);

    return { error_constructor, prototype };
}

namespace {

std::string get_eval_exception_repr(native_error_type type, const std::wstring_view& stack_trace, const std::wstring_view& msg) {
    std::ostringstream oss;
    oss << type_string(type) << ": "<< std::string(msg.begin(), msg.end());
    if (!stack_trace.empty()) {
        oss << '\n' << std::string(stack_trace.begin(), stack_trace.end());
    }
    return oss.str();
}

} // unnamed namespace

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
    auto& h = global.heap();
    return h.make<error_object>(type_, global->common_string(type_string(type_)), global->error_prototype(), value{string{h, msg_}}, string{h, stack_trace_});
}

[[noreturn]] void rethrow_error(const object_ptr& error) {
    assert(error.has_type<error_object>());
    static_cast<const error_object&>(*error).rethrow();
}

} // namespace mjs
