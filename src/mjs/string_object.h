#ifndef MJS_STRING_OBJECT_H
#define MJS_STRING_OBJECT_H

#include "global_object.h"

namespace mjs {

create_result make_string_object(global_object& global);

object_ptr new_string(const gc_heap_ptr<global_object>& global, const string& val);

} // namespace mjs

#endif