#include "gc_table.h"
#include <sstream>
#include <cmath>

namespace mjs {


// TEMPTEMP

class gc_object_ptr {
public:
    const object_ptr& ptr() { return ptr_; }

private:
    friend gc_type_info_registration<gc_object_ptr>;
    object_ptr ptr_;

    explicit gc_object_ptr(const object_ptr& ptr) : ptr_(ptr) {}

    gc_heap_ptr_untyped move(gc_heap& new_heap) const {
        return new_heap.make<gc_object_ptr>(std::move(ptr_));
    }
};


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

uint64_t number_repr(double d) {
    if (std::isnan(d)) {
        return nan_bits | 1;
    }
    uint64_t repr;
    static_assert(sizeof(d) == sizeof(repr));
    std::memcpy(&repr, &d, sizeof(repr));
    return repr;
}

mjs::value gc_table::from_representation(uint64_t repr) const {
    if (!is_special(repr)) {
        double d;
        static_assert(sizeof(d) == sizeof(repr));
        std::memcpy(&d, &repr, sizeof(d));
        return value{d};
    }
    const auto type    = type_from_repr(repr);
    const auto payload = static_cast<uint32_t>(repr);
    switch (type) {
    case value_type::undefined: assert(!payload); return value::undefined;
    case value_type::null:      assert(!payload); return value::null;
    case value_type::boolean:   assert(payload == 0 || payload == 1); return value{!!payload};
    case value_type::number:    /*should be handled above*/ break;
    case value_type::string:    return value{string{heap_->unsafe_create_from_position<gc_string>(payload)}};
    case value_type::object:    return value{heap_->unsafe_create_from_position<gc_object_ptr>(payload)->ptr()};
    default:                    break;
    }
    std::wostringstream woss;
    woss << "Not handled: " << type << " " << repr;
    THROW_RUNTIME_ERROR(woss.str());
    std::abort();
}

uint64_t gc_table::to_representation(const value& v) {
    switch (v.type()) {
    case value_type::undefined: [[fallthrough]];
    case value_type::null:      return make_repr(v.type(), 0);
    case value_type::boolean:   return make_repr(v.type(), v.boolean_value());
    case value_type::number:    return number_repr(v.number_value());
    case value_type::string:    return make_repr(v.type(), v.string_value().unsafe_raw_get().unsafe_get_position());
    case value_type::object:
    {
        auto p = heap_->make<gc_object_ptr>(v.object_value());
        return make_repr(v.type(), p.unsafe_get_position());
    }
    case value_type::reference: break; // Not legal here
    }
    std::wostringstream woss;
    woss << "Not handled: ";
    debug_print(woss, v, 2);
    THROW_RUNTIME_ERROR(woss.str());
}

gc_heap_ptr_untyped gc_table::move(gc_heap& new_heap) const {
    auto p = make(new_heap, capacity());
    p->heap_ = heap_;
    p->length_ = length();
    for (uint32_t i = 0; i < length(); ++i) {
        auto& oe = entries()[i];
        auto& ne = p->entries()[i];
        ne.key_index = heap_->unsafe_gc_move(new_heap, oe.key_index);
        ne.attributes = oe.attributes;
        if (!is_special(oe.value_representation)) {
            ne.value_representation = oe.value_representation;
            continue;
        }
        const auto type = type_from_repr(oe.value_representation);
        switch (type) {
        case value_type::undefined: [[fallthrough]];
        case value_type::null:      [[fallthrough]];
        case value_type::boolean:   [[fallthrough]];
        case value_type::number:
            ne.value_representation = oe.value_representation;
            break;
        case value_type::string:    [[fallthrough]];
        case value_type::object:
            ne.value_representation = make_repr(type, heap_->unsafe_gc_move(new_heap, oe.value_representation & UINT32_MAX));
            break;
        default:
            std::abort();
        } 
    }
    return p;
}


} // namespace mjs
