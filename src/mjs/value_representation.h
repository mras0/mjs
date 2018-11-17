#ifndef MJS_VALUE_REPRESENTATION_H
#define MJS_VALUE_REPRESENTATION_H

#include <stdint.h>

namespace mjs {

class value;
class gc_heap;

class value_representation {
public:
    value_representation() = default;
    explicit value_representation(const value& v);
    value get_value(gc_heap& heap) const;
    void fixup(gc_heap& old_heap);
private:
    uint64_t repr_;
};

} // namespace mjs

#endif
