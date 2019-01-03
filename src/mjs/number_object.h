#ifndef MJS_NUMBER_OBJECT_H
#define MJS_NUMBER_OBJECT_H

#include "global_object.h"

namespace mjs {

create_result make_number_object(global_object& global);

object_ptr new_number(const object_ptr& prototype, double val);

} // namespace mjs

#endif

