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

std::vector<const gc_type_info*> gc_type_info::types_;
const gc_type_info::fixup_function gc_type_info::no_fixup_needed = reinterpret_cast<gc_type_info::fixup_function>(1);

//
// value_representation
//
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

uint64_t value_representation::to_representation(const value& v) {
    switch (v.type()) {
    case value_type::undefined: [[fallthrough]];
    case value_type::null:      return make_repr(v.type(), 0);
    case value_type::boolean:   return make_repr(v.type(), v.boolean_value());
    case value_type::number:    return number_repr(v.number_value());
    case value_type::string:    return make_repr(v.type(), v.string_value().unsafe_raw_get().pos_);
    case value_type::object:    return make_repr(v.type(), v.object_value().pos_);
    case value_type::reference: break; // Not legal here
    }
    std::wostringstream woss;
    woss << "Not handled: ";
    debug_print(woss, v, 2);
    THROW_RUNTIME_ERROR(woss.str());
}

void value_representation::fixup_after_move(gc_heap& new_heap, gc_heap& old_heap) {
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
        repr_ = make_repr(type, old_heap.gc_move(new_heap, repr_ & UINT32_MAX));
        break;
    default:
        std::abort();
    } 
}

//
// gc_heap
//

thread_local gc_heap* gc_heap::local_heap_ = nullptr;

gc_heap::gc_heap(uint32_t capacity) : storage_(static_cast<slot*>(std::malloc(capacity * sizeof(slot)))), capacity_(capacity) {
}

gc_heap::~gc_heap() {
    // Run destructors
    for (uint32_t pos = 0; pos < next_free_;) {
        const auto a = storage_[pos].allocation;
        if (a.active()) {
            a.type_info().destroy(&storage_[pos+1]);
        }
        pos += a.size;
    }
    assert(pointers_.empty());
    std::free(storage_);
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
            if (a.type == gc_type_info_registration<gc_string>::get().get_index()) {
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
    {
        gc_heap new_heap{capacity_}; // TODO: Allow resize

        std::vector<const gc_heap_ptr_untyped*> roots;

        std::copy_if(pointers_.begin(), pointers_.end(), std::back_inserter(roots), [this](const gc_heap_ptr_untyped* p) { return !is_internal(p); });

        for (auto p: roots) {
            // If the assert fires a GC allocated object used something that captured gc_heap_ptr's (e.g. a raw std::function)
            assert(pointers_.find(p) != pointers_.end() && "Internal pointer was accidently classified as root!");
            const_cast<gc_heap_ptr_untyped*>(p)->pos_ = gc_move(new_heap, p->pos_);
        }

        std::swap(storage_, new_heap.storage_);
        std::swap(next_free_, new_heap.next_free_);
    }
}

uint32_t gc_heap::gc_move(gc_heap& new_heap, const uint32_t pos) {
    assert(pos > 0 && pos < next_free_);

    auto& a = storage_[pos-1].allocation;
    assert(a.type != unallocated_type_index);
    assert(a.size > 1);

    if (a.type == gc_moved_type_index) {
        return storage_[pos].new_position;
    }

    // Allocate memory block in new_heap of the same size
    const auto new_pos = new_heap.allocate((a.size-1)*sizeof(uint64_t)) + 1;
    auto& new_a = new_heap.storage_[new_pos - 1].allocation;
    auto* const new_p = &new_heap.storage_[new_pos];
    assert(new_a.size == a.size);

    // Move the object to its new position
    const auto& type_info = a.type_info();
    void* const p = &storage_[pos];
    type_info.move(new_p, p);
    new_a.type = a.type;

    // And destroy it at the old position
    type_info.destroy(p);

    // Record the object's new position at the old position
    a.type = gc_moved_type_index;
    storage_[pos].new_position = new_pos;

    // After changing the allocation header, infinite recursion can now be avoided when copying the internal pointers.
    auto l = reinterpret_cast<gc_heap_ptr_untyped*>(new_p);
    auto u = reinterpret_cast<gc_heap_ptr_untyped*>(new_p + a.size - 1);

    // Let the object do fixup on its own if it wants to
    if (!type_info.fixup(new_heap, new_p)) {
        // Handle it otherwise
        for (auto it = pointers_.lower_bound(l); it != pointers_.end() && (*it) < u; ++it) {
            const_cast<gc_heap_ptr_untyped*>(*it)->pos_ = gc_move(new_heap, (*it)->pos_);
        }
    } else {
        assert((pointers_.lower_bound(l) == pointers_.end() || *pointers_.lower_bound(l) >= u) && "Object lied about its fixup capabilities!");
    }

    return new_pos;
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
    storage_[pos].allocation.type = unallocated_type_index;
    return pos;
}

void gc_heap::attach(const gc_heap_ptr_untyped& p) {
    assert(p.pos_ > 0 && p.pos_ < next_free_);
    [[maybe_unused]] const auto inserted = pointers_.insert(&p).second;
    assert(inserted);
}

void gc_heap::detach(const gc_heap_ptr_untyped& p) {
    [[maybe_unused]] const auto deleted = pointers_.erase(&p);
    assert(deleted);
}

} // namespace mjs
