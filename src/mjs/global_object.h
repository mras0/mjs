#ifndef MJS_GLOBAL_OBJECT_H
#define MJS_GLOBAL_OBJECT_H

#include "value.h"
#include "object.h"
#include "version.h"

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

protected:
    version version_;
    using object::object;
    global_object(global_object&&) = default;
};

extern std::wstring index_string(uint32_t index);
extern void validate_type(const value& v, const object_ptr& expected_prototype, const char* expected_type);

// TODO: Need better name
struct create_result {
    object_ptr obj;
    object_ptr prototype;
};

} // namespace mjs

#endif
