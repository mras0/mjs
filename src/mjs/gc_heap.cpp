#include "gc_heap.h"
#include "value.h"
#include "object.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cmath>

namespace {

template<typename CharT>
struct save_stream_state {
    save_stream_state(std::basic_ostream<CharT>& os) : os(os), state(nullptr) {
        state.copyfmt(os);
    }
    ~save_stream_state() {
        os.copyfmt(state);
    }
private:
    std::basic_ostream<CharT>& os;
    std::basic_ios<CharT>      state;
};

class number_formatter {
public:
    explicit number_formatter(uint64_t number, int min_width=0, int base=10) : n_(number), w_(min_width), b_(base) {}

    number_formatter& base(int base) { b_ = base; return *this; }
    number_formatter& width(int min_width) { w_ = min_width; return *this; }

    template<typename CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const number_formatter& nf) {
        save_stream_state sss{os};
        if (nf.b_ == 16) os << std::hex;
        else assert(nf.b_ == 10);
        os << std::setfill(static_cast<CharT>('0'));
        if (nf.w_) os << std::setw(nf.w_);
        os << nf.n_;
        return os;
    }

private:
    uint64_t n_;
    int w_;
    int b_;
};

auto fmt(uint64_t n) { return number_formatter{n}; }
template<typename T>
auto hexfmt(T n) { return number_formatter{n}.base(16).width(2*sizeof(T)); }

} // unnamed namespace

namespace mjs {

//
// gc_type_info
//

uint32_t gc_type_info::num_types_;
const gc_type_info* gc_type_info::types_[gc_type_info::max_types];

//
// value_representation
//

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

//
// gc_heap
//

gc_heap::gc_heap(uint32_t capacity) : storage_(static_cast<slot*>(std::malloc(capacity * sizeof(slot)))), capacity_(capacity) {
    if (!storage_) {
        THROW_RUNTIME_ERROR("Could not allocate heap for " + std::to_string(capacity) + " slots");
    }
}

gc_heap::~gc_heap() {
    assert(gc_state_.initial_state());
    run_destructors();
    std::free(storage_);
}

void gc_heap::run_destructors() {
    for (uint32_t pos = 0; pos < next_free_;) {
        const auto a = storage_[pos].allocation;
        if (a.active()) {
            a.type_info().destroy(&storage_[pos+1]);
        }
        pos += a.size;
    }
    assert(pointers_.empty());
}

void gc_heap::debug_print(std::wostream& os) const {
    const int size_w = 4;
    const int pos_w = 8;
    const int tt_width = 25;
    os << "Heap:\n";
    for (uint32_t pos = 0; pos < next_free_;) {
        const auto a = storage_[pos].allocation;
        os << fmt(pos+1).width(pos_w) << " size: " << fmt(a.size).width(size_w) << " type: " << fmt(a.type).width(2) << " ";
        if (a.active()) {
            {
                save_stream_state sss{os};
                os << std::left << std::setw(tt_width) << a.type_info().name();
            }
            if (a.type == gc_type_info_registration<gc_string>::index()) {
                os << " '" << reinterpret_cast<const gc_string*>(&storage_[pos+1])->view() << "'";
            }
        }
        os << "\n";
        pos += a.size;
    }
    os << "Pointers:\n";
    for (auto p: pointers_) {
        assert(p->heap_ == this);
        os << fmt(p->pos_).width(pos_w);
        const auto a = storage_[p->pos_-1].allocation;
        os << " [size: " << fmt(a.size).width(size_w) << " type: " << fmt(a.type).width(2);
        if (a.active()) {
            save_stream_state sss{os};
            os << " " << std::left << std::setw(tt_width) << a.type_info().name();
        } else {
            assert(false);
        }
        os << "] " << p << " ";
        
        if (is_internal(p)) {
            os << " (internal at " << fmt(reinterpret_cast<const uint64_t*>(p) - &storage_[0].representation).width(pos_w) << ")";
        }
        os << "\n";
    }
}

uint32_t gc_heap::calc_used() const {
    uint32_t used = 0;
    for (uint32_t pos = 0; pos < next_free_;) {
        const auto a = storage_[pos].allocation;
        if (a.active()) {
            used += a.size;
        }
        pos += a.size;
    }
    return used;
}

void gc_heap::garbage_collect() {
    assert(gc_state_.initial_state());

    // Determine roots and add their positions as pending fixups
    // TODO: Used to move the roots lower in the pointers_ array (since we know they won't be destroyed this time around). That still might be an optimization.
    for (auto p: pointers_) {
        if (!is_internal(p)) {
            register_fixup(p->pos_);
        }
    }

    if (!gc_state_.pending_fixups.empty()) {
        gc_heap new_heap{capacity_}; // TODO: Allow resize

        gc_state_.new_heap = &new_heap;
        gc_state_.level = 0;

        // Keep going while there are still fixups to be processed (note: the array changes between loop iterations)
        while (!gc_state_.pending_fixups.empty()) {
            auto ppos = gc_state_.pending_fixups.back();
            gc_state_.pending_fixups.pop_back();
            *ppos = gc_move(*ppos);
        }

        std::swap(storage_, new_heap.storage_);
        std::swap(next_free_, new_heap.next_free_);
        gc_state_.new_heap = nullptr;
        // new_heap's destructor checks that it doesn't contain pointers
    } else {
        run_destructors();
        next_free_ = 0;
    }

    assert(gc_state_.initial_state());
}

uint32_t gc_heap::gc_move(const uint32_t pos) {
    struct auto_level {
        auto_level(uint32_t& l) : l(l) { ++l; assert(l < 4 && "Arbitrary recursion level reached"); }
        ~auto_level() { --l; }
        uint32_t& l;
    } al{gc_state_.level};

    assert(pos > 0 && pos < next_free_);

    auto& a = storage_[pos-1].allocation;
    assert(a.type != uninitialized_type_index);
    assert(a.size > 1 && a.size <= next_free_ - (pos - 1));

    if (a.type == gc_moved_type_index) {
        return storage_[pos].new_position;
    }

    assert(a.type < gc_type_info::num_types());

    // Allocate memory block in new_heap of the same size
    auto& new_heap = *gc_state_.new_heap;
    const auto new_pos = new_heap.allocate((a.size-1)*slot_size) + 1;
    auto& new_a = new_heap.storage_[new_pos - 1].allocation;
    auto* const new_p = &new_heap.storage_[new_pos];
    assert(new_a.type == uninitialized_type_index && new_a.size == a.size);

    // Record number of pointers that exist before constructing the new object
    const auto num_pointers_initially = pointers_.size();

    // Move the object to its new position
    const auto& type_info = a.type_info();
    void* const p = &storage_[pos];
    type_info.move(new_p, p);
    new_a.type = a.type;

    // Register fixups for the position of all internal pointers that were created by the move (construction)
    // The new pointers will be at the end of the pointer set since they were just added
    // Note this obviously makes assumption about the pointer_set implementation!
    // TODO: Used to move the pointers lower in the pointers_ array (since we know they won't be destroyed this time around). That still might be an optimization.
    if (const auto num_internal_pointers  = pointers_.size() - num_pointers_initially) {
        auto ps = pointers_.data();
        for (uint32_t i = 0; i < num_internal_pointers; ++i) {
            assert(reinterpret_cast<uintptr_t>(ps[num_pointers_initially+i]) >= reinterpret_cast<uintptr_t>(new_p) && reinterpret_cast<uintptr_t>(ps[num_pointers_initially+i]) < reinterpret_cast<uintptr_t>(&new_heap.storage_[new_pos] + a.size - 1));
            register_fixup(ps[num_pointers_initially+i]->pos_);
        }
    }

    // And destroy it at the old position
    type_info.destroy(p);

    // There should now be the same amount of pointers (otherwise something went wrong with moving/destroying the object)
    assert(pointers_.size() == num_pointers_initially);

    // Record the object's new position at the old position
    a.type = gc_moved_type_index;
    storage_[pos].new_position = new_pos;

    // After changing the allocation header infinite recursion can now be avoided when copying the internal pointers.

    // Let the object register its fixup
    type_info.fixup(new_p);

    return new_pos;
}

void gc_heap::register_fixup(uint32_t& pos) {
    gc_state_.pending_fixups.push_back(&pos);
}

uint32_t gc_heap::allocate(size_t num_bytes) {
    if (!num_bytes || num_bytes >= UINT32_MAX) {
        assert(!"Invalid allocation size");
        std::abort();
    }

    const auto num_slots = 1 + bytes_to_slots(num_bytes);
    if (num_slots > capacity_ || next_free_ > capacity_ - num_slots) {
        assert(!"Not implemented: Ran out of heap");
        std::abort();
    }
    const auto pos = next_free_;
    next_free_ += num_slots;
    storage_[pos].allocation.size = num_slots;
    storage_[pos].allocation.type = uninitialized_type_index;
    return pos;
}

void gc_heap::attach(gc_heap_ptr_untyped& p) {
    assert(p.heap_ == this && p.pos_ > 0 && p.pos_ < next_free_);
    pointers_.insert(p);
}

void gc_heap::detach(gc_heap_ptr_untyped& p) {
    assert(p.heap_ == this);
    pointers_.erase(p);
}

} // namespace mjs
