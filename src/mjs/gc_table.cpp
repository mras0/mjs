#include "gc_table.h"

namespace mjs {

static_assert(!gc_type_info_registration<gc_table>::needs_destroy);
static_assert(gc_type_info_registration<gc_table>::needs_fixup);

void gc_table::fixup() {
    auto e = entries();
    auto& h = heap();
    for (uint32_t i = 0, l = length(); i < l; ++i) {
        e[i].key.fixup(h);
        e[i].value.fixup(h);
    }
}

} // namespace mjs
