#ifndef MJS_ARRAY_OBJECT_H
#define MJS_ARRAY_OBJECT_H

#include "global_object.h"

namespace mjs {

create_result make_array_object(global_object& global);
object_ptr make_array(const object_ptr& array_prototype, const std::vector<value>& args);

} // namespace mjs

#endif