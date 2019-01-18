#ifndef MJS_DATE_OBJECT_H
#define MJS_DATE_OBJECT_H

#include "global_object.h"

namespace mjs {

global_object_create_result make_date_object(const gc_heap_ptr<global_object>& global);

} // namespace mjs

#endif