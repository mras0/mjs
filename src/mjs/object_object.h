#ifndef MJS_OBJECT_OBJECT_H
#define MJS_OBJECT_OBJECT_H

#include "global_object.h"

namespace mjs {

global_object_create_result make_object_object(const gc_heap_ptr<global_object>& global);

object_ptr make_accessor_object(const gc_heap_ptr<global_object>& global, const value& get, const value& set);

} // namespace mjs

#endif

