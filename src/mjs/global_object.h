#ifndef MJS_GLOBAL_OBJECT_H
#define MJS_GLOBAL_OBJECT_H

#include "value.h"
#include "object.h"
#include "version.h"
#include <functional>

namespace mjs {

// TODO: Clean up this mess

class function_object;

enum class native_error_type;

class global_object : public object {
public:
    static gc_heap_ptr<global_object> make(gc_heap& h, version ver, bool& strict_mode);
    virtual ~global_object() {}

    virtual object_ptr object_prototype() const = 0;
    virtual object_ptr function_prototype() const = 0;
    virtual object_ptr string_prototype() const = 0;
    virtual object_ptr array_prototype() const = 0;
    virtual object_ptr regexp_prototype() const = 0;
    virtual object_ptr error_prototype(native_error_type type) const = 0;
    virtual object_ptr to_object(const value& v) = 0;
    virtual void define_thrower_accessor(object& o, const char* property_name) = 0;

    static constexpr auto prototype_attributes = property_attribute::dont_enum | property_attribute::dont_delete | property_attribute::read_only;
    static constexpr auto default_attributes = property_attribute::dont_enum;

    // Make a string (to make it possible to use a cache)
    virtual string common_string(const char* str) = 0;

    version language_version() const { return version_; }

    object_ptr make_object();

    std::wstring stack_trace() const {
        assert(stack_trace_);
        return stack_trace_();
    }

    void set_stack_trace_function(const std::function<std::wstring()>& f) {
        assert(!stack_trace_);
        stack_trace_ = f;
    }

    bool strict_mode() const {
        return *strict_mode_;
    }

    // Throw TypeError if 'v' is not an object
    object_ptr validate_object(const value& v) const;

    // Throw TypeError if 'v' isn't an object with prototype equal to 'expected_prototype'
    void validate_type(const value& v, const object_ptr& expected_prototype, const char* expected_type) const;

    object_ptr make_arguments_array();
    bool is_arguments_array(const object_ptr& o);

protected:
    version version_;
    bool* strict_mode_;
    using object::object;
    global_object(global_object&&) = default;

private:
    std::function<std::wstring()> stack_trace_;
    std::function<bool()>         strict_mode_func_;
};

extern std::wstring index_string(uint32_t index);
extern bool is_global_object(const object_ptr& o);

struct global_object_create_result {
    object_ptr obj;
    object_ptr prototype;
};

class not_supported_exception : public std::runtime_error {
public:
    explicit not_supported_exception(const std::string& s) : std::runtime_error(s) {}
};

} // namespace mjs

#endif
