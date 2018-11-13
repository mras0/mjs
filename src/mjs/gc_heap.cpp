#include "gc_heap.h"
#include <algorithm>
#include <iomanip>
#include <cstdlib>

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

std::vector<const gc_type_info*> gc_type_info::types_;

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
            save_stream_state sss{os};
            os << std::left << std::setw(tt_width) << a.type_info().name();
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
    gc_heap new_heap{capacity_}; // TODO: Allow resize

    std::vector<const gc_heap_ptr_untyped*> roots;

    std::copy_if(pointers_.begin(), pointers_.end(), std::back_inserter(roots), [this](const gc_heap_ptr_untyped* p) { return !is_internal(p); });

    for (auto p: roots) {
        // If the assert fires a GC allocated object used e.g. a raw std::function, that captured gc_heap_ptr's
        assert(pointers_.find(p) != pointers_.end() && "Internal pointer was accidently classified as root!");
        const_cast<gc_heap_ptr_untyped*>(p)->pos_ = gc_move(new_heap, p->pos_);
    }

    std::swap(storage_, new_heap.storage_);
    std::swap(next_free_, new_heap.next_free_);
}

uint32_t gc_heap::gc_move(gc_heap& new_heap, uint32_t pos) {
    auto& a = storage_[pos-1].allocation;
    assert(a.type != unallocated_type_index);
    assert(a.size > 1);

    if (a.type == gc_moved_type_index) {
        return storage_[pos].new_position;
    }

    const auto& type_info = a.type_info();
    void* const p = &storage_[pos];

    const auto new_pos = type_info.move(new_heap, p).pos_;
    type_info.destroy(p);

    a.type = gc_moved_type_index;
    storage_[pos].new_position = new_pos;


    // Can now avoid infinite recursion when copying the internal pointers (after changing the allocation header)
    // Copy any internal pointers of the just moved object
    auto l = reinterpret_cast<gc_heap_ptr_untyped*>(&new_heap.storage_[new_pos]);
    auto u = reinterpret_cast<gc_heap_ptr_untyped*>(&new_heap.storage_[new_pos] + new_heap.storage_[new_pos-1].allocation.size - 1);
    for (auto it = pointers_.lower_bound(l); it != pointers_.end() && (*it) < u; ++it) {
        const_cast<gc_heap_ptr_untyped*>(*it)->pos_ = gc_move(new_heap, (*it)->pos_);
    }

    return new_pos;
}

uint32_t gc_heap::unsafe_gc_move(gc_heap& new_heap, uint32_t pos) {
    return gc_move(new_heap, pos);
}

gc_heap_ptr_untyped gc_heap::allocate(size_t num_bytes) {
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
    return gc_heap_ptr_untyped{*this, pos + 1};
}

void gc_heap::attach(const gc_heap_ptr_untyped& p) {
    assert(p.pos_ > 0);
    assert(p.pos_ < next_free_);

    [[maybe_unused]] const auto inserted = pointers_.insert(&p).second;
    assert(inserted);
}

void gc_heap::detach(const gc_heap_ptr_untyped& p) {
    [[maybe_unused]] const auto deleted = pointers_.erase(&p);
    assert(deleted);
}

gc_heap::slot_allocation_header& gc_heap::allocation_header(const gc_heap_ptr_untyped& p) {
    assert(p.heap_ && p.pos_ > 0 && p.pos_ < p.heap_->next_free_);
    return p.heap().storage_[p.pos_ - 1].allocation;
}

} // namespace mjs
