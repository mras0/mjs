#ifndef MJS_REGEXP_OBEJECT_H
#define MJS_REGEXP_OBEJECT_H

#include "global_object.h"

namespace mjs {

create_result make_regexp_object(global_object& global);
//object_ptr make_array(const object_ptr& array_prototype, const std::vector<value>& args);

} // namespace mjs

#endif