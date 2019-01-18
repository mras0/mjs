#ifndef MJS_ARRAY_OBJECT_H
#define MJS_ARRAY_OBJECT_H

#include "global_object.h"

namespace mjs {

global_object_create_result make_array_object(const gc_heap_ptr<global_object>& global);
object_ptr make_array(const gc_heap_ptr<global_object>& global, uint32_t length);
object_ptr make_array(const gc_heap_ptr<global_object>& global, const std::vector<value>& args);
bool is_array(const object_ptr& o);

} // namespace mjs

#endif