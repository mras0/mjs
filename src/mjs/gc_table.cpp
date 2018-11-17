#include "gc_table.h"

namespace mjs {

static_assert(!gc_type_info_registration<gc_table>::needs_destroy);
static_assert(gc_type_info_registration<gc_table>::needs_fixup);

gc_table::gc_table(gc_table&& other) : heap_(other.heap_), capacity_(other.capacity_), length_(other.length_) {
    static_assert(sizeof(gc_table::entry_representation) == 2*gc_heap::slot_size);
    std::memcpy(entries(), other.entries(), length() * sizeof(entry_representation));
}

void gc_table::fixup() {
    for (uint32_t i = 0; i < length(); ++i) {
        auto& e = entries()[i];
        e.key.fixup(heap_);
        e.value.fixup(heap_);
    }
}

} // namespace mjs
