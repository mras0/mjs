#ifndef MJS_REGEXP_OBEJECT_H
#define MJS_REGEXP_OBEJECT_H

#include "global_object.h"

namespace mjs {

create_result make_regexp_object(global_object& global);
object_ptr make_regexp(const gc_heap_ptr<global_object>& global, const string& pattern, const string& flags);

} // namespace mjs

#endif