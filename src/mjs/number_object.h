#ifndef MJS_NUMBER_OBJECT_H
#define MJS_NUMBER_OBJECT_H

#include "global_object.h"

namespace mjs {

global_object_create_result make_number_object(const gc_heap_ptr<global_object>& global);

object_ptr new_number(const object_ptr& prototype, double val);

} // namespace mjs

#endif
