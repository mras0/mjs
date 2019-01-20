#ifndef MJS_ERROR_OBEJECT_H
#define MJS_ERROR_OBEJECT_H

#include "global_object.h"

namespace mjs {

enum class native_error_type {
    generic, eval, range, reference, syntax, type, uri, assertion
};
constexpr native_error_type native_error_types[] = {native_error_type::generic, native_error_type::eval, native_error_type::range, native_error_type::reference, native_error_type::syntax, native_error_type::type, native_error_type::uri, native_error_type::assertion};
constexpr uint32_t num_native_error_types = static_cast<uint32_t>(sizeof(native_error_types)/sizeof(*native_error_types));

std::wostream& operator<<(std::wostream& os, native_error_type type);

global_object_create_result make_error_object(const gc_heap_ptr<global_object>& global);

class eval_exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class native_error_exception : public eval_exception {
public:
    explicit native_error_exception(native_error_type type, const std::wstring_view& stack_trace, const std::wstring_view& msg);
    explicit native_error_exception(native_error_type type, const std::wstring_view& stack_trace, const std::string_view& msg);

    object_ptr make_error_object(const gc_heap_ptr<global_object>& global) const;
private:
    native_error_type type_;
    std::wstring msg_;
    std::wstring stack_trace_;
};

[[noreturn]] void rethrow_error(const object_ptr& error);

} // namespace mjs

#endif
