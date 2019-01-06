#ifndef MJS_GLOBAL_OBJECT_H
#define MJS_GLOBAL_OBJECT_H

#include "value.h"
#include "object.h"
#include "version.h"
#include <functional>

namespace mjs {

// TODO: Clean up this mess

class function_object;

class global_object : public object {
public:
    static gc_heap_ptr<global_object> make(gc_heap& h, version ver);
    virtual ~global_object() {}

    virtual object_ptr object_prototype() const = 0;
    virtual object_ptr function_prototype() const = 0;
    virtual object_ptr array_prototype() const = 0;
    virtual object_ptr regexp_prototype() const = 0;
    virtual object_ptr error_prototype() const = 0;
    virtual object_ptr to_object(const value& v) = 0;    

    static constexpr auto prototype_attributes = property_attribute::dont_enum | property_attribute::dont_delete | property_attribute::read_only;
    static constexpr auto default_attributes = property_attribute::dont_enum;

    virtual gc_heap_ptr<global_object> self_ptr() const = 0; // FIXME: Remove need for this

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

    // Throw TypeError if 'v' is not an object
    void validate_object(const value& v) const;

    // Throw TypeError if 'v' isn't an object with prototype equal to 'expected_prototype'
    void validate_type(const value& v, const object_ptr& expected_prototype, const char* expected_type) const;

    object_ptr make_arguments_array();
    bool is_arguments_array(const object_ptr& o);

protected:
    version version_;
    using object::object;
    global_object(global_object&&) = default;

private:
    std::function<std::wstring()> stack_trace_;
};

extern std::wstring index_string(uint32_t index);

// TODO: Need better name
struct create_result {
    object_ptr obj;
    object_ptr prototype;
};

} // namespace mjs

#endif
