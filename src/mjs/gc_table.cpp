#include "gc_table.h"

namespace mjs {

static_assert(std::is_trivially_destructible_v<gc_table>);

gc_heap_ptr_untyped gc_table::move(gc_heap& new_heap) const {
    auto p = make(new_heap, capacity());
    p->heap_ = heap_;
    p->length_ = length();
    std::memcpy(p->entries(), entries(), length() * sizeof(entry_representation));
    return p;
}

bool gc_table::fixup(gc_heap& new_heap) {
    for (uint32_t i = 0; i < length(); ++i) {
        auto& e = entries()[i];
        e.key.fixup_after_move(new_heap, *heap_);
        e.value.fixup_after_move(new_heap, *heap_);
    }
    return true;
}

} // namespace mjs
