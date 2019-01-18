#ifndef MJS_BOOLEAN_OBJECT_H
#define MJS_BOOLEAN_OBJECT_H

#include "global_object.h"

namespace mjs {

global_object_create_result make_boolean_object(const gc_heap_ptr<global_object>& global);

object_ptr new_boolean(const object_ptr& prototype, bool val);

} // namespace mjs

#endif
