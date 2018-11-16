#include "gc_table.h"

namespace mjs {

static_assert(std::is_trivially_destructible_v<gc_table>);

gc_table::gc_table(gc_table&& other) : heap_(other.heap_), capacity_(other.capacity_), length_(other.length_) {
    std::memcpy(entries(), other.entries(), length() * sizeof(entry_representation));
}

bool gc_table::fixup(gc_heap& new_heap) {
    for (uint32_t i = 0; i < length(); ++i) {
        auto& e = entries()[i];
        e.key.fixup_after_move(new_heap, heap_);
        e.value.fixup_after_move(new_heap, heap_);
    }
    return true;
}

} // namespace mjs
