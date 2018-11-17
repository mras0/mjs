#include "value_representation.h"
#include "value.h"
#include "object.h"
#include <cmath>
#include <sstream>

namespace mjs {

static_assert(sizeof(value_representation) == gc_heap::slot_size);

// sign bit, exponent (11-bits), fraction (52-bits)
// NaNs have exponent 0x7ff and fraction != 0
static constexpr int type_shift         = 52-4;
static constexpr uint64_t nan_bits      = 0x7ffULL << 52;
static constexpr uint64_t type_bits     = 0xfULL << type_shift;

constexpr bool is_special(uint64_t repr) {
    return (repr & nan_bits) == nan_bits && (repr & type_bits) != 0;
}

constexpr value_type type_from_repr(uint64_t repr) {
    return static_cast<value_type>(((repr&type_bits)>>type_shift)-1);
}

constexpr uint64_t make_repr(value_type type, uint32_t payload) {
    return nan_bits | (static_cast<uint64_t>(type)+1)<<type_shift | payload;
}

static uint64_t number_repr(double d) {
    if (std::isnan(d)) {
        // Make sure all NaNs are handled uniformly - In particular don't allow arbitrary NaNs to be turned into the raw representation
        return nan_bits | 1;
    }
    uint64_t repr;
    static_assert(sizeof(d) == sizeof(repr));
    std::memcpy(&repr, &d, sizeof(repr));
    return repr;
}

value_representation::value_representation(const value& v) {
    switch (v.type()) {
    case value_type::undefined: [[fallthrough]];
    case value_type::null:      repr_ = make_repr(v.type(), 0); return;
    case value_type::boolean:   repr_ = make_repr(v.type(), v.boolean_value()); return;
    case value_type::number:    repr_ = number_repr(v.number_value()); return;
    case value_type::string:    repr_ = make_repr(v.type(), v.string_value().unsafe_raw_get().pos_); return;
    case value_type::object:    repr_ = make_repr(v.type(), v.object_value().pos_); return;
    case value_type::reference: break; // Not legal here
    }
    std::wostringstream woss;
    woss << "Not handled: ";
    debug_print(woss, v, 2);
    THROW_RUNTIME_ERROR(woss.str());
}

value value_representation::get_value(gc_heap& heap) const {
    if (!is_special(repr_)) {
        double d;
        static_assert(sizeof(d) == sizeof(repr_));
        std::memcpy(&d, &repr_, sizeof(d));
        return value{d};
    }
    const auto type    = type_from_repr(repr_);
    const auto payload = static_cast<uint32_t>(repr_);
    switch (type) {
    case value_type::undefined: assert(!payload); return value::undefined;
    case value_type::null:      assert(!payload); return value::null;
    case value_type::boolean:   assert(payload == 0 || payload == 1); return value{!!payload};
    case value_type::number:    /*should be handled above*/ break;
    case value_type::string:    return value{string{heap.unsafe_create_from_position<gc_string>(payload)}};
    case value_type::object:    return value{heap.unsafe_create_from_position<object>(payload)};
    default:                    break;
    }
    std::wostringstream woss;
    woss << "Not handled: " << type << " " << repr_;
    THROW_RUNTIME_ERROR(woss.str());
}

void value_representation::fixup(gc_heap& old_heap) {
    if (!is_special(repr_)) {
        return;
    }
    const auto type = type_from_repr(repr_);
    switch (type) {
    case value_type::undefined: [[fallthrough]];
    case value_type::null:      [[fallthrough]];
    case value_type::boolean:   [[fallthrough]];
    case value_type::number:
        return;
    case value_type::string:    [[fallthrough]];
    case value_type::object:
        // TODO: See note in gc_heap_ptr_untracked about adding a (debug) check for whether fixup was done correctly
        // FIXME: This only works on little endian platforms!
        assert(reinterpret_cast<uint32_t*>(&repr_)[1] == (nan_bits | (static_cast<uint64_t>(type)+1)<<type_shift)>>32);
        old_heap.register_fixup(*reinterpret_cast<uint32_t*>(&repr_));
        break;
    default:
        std::abort();
    }
}

} // namespace mjs

