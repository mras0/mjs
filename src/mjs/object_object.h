#ifndef MJS_OBJECT_OBJECT_H
#define MJS_OBJECT_OBJECT_H

#include "global_object.h"

namespace mjs {

void add_object_prototype_functions(global_object& global, const object_ptr& prototype);
create_result make_object_object(global_object& global);

} // namespace mjs

#endif

