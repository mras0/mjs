#ifndef MJS_GC_FUNCTION_H
#define MJS_GC_FUNCTION_H

#include "gc_heap.h"
#include "value.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4324) // Structure was padded due to alignment specifier
#endif

namespace mjs {

// Alignment needed so impl<F> will be construct with proper alignment
class alignas(uint64_t) gc_function {
public:
    template<typename F>
    static gc_heap_ptr<gc_function> make(gc_heap& h, const F& f) {
        auto p = gc_heap::construct<gc_function>(h.allocate(sizeof(gc_function) + sizeof(impl<F>)));
        new (p->get_model()) impl<F>{f};
        return p;
    }

    value call(const value& this_, const std::vector<value>& args) const {
        return get_model()->call(this_, args);
    }

private:
    friend gc_type_info_registration<gc_function>;

    class model {
    public:
        virtual void destroy() = 0;
        virtual value call(const value& this_, const std::vector<value>& args) = 0;
        virtual gc_heap_ptr<gc_function> move(gc_heap& new_heap) = 0;
    };
    template<typename F>
    class impl : public model {
    public:
        explicit impl(const F& f) : f(f) {}
        void destroy() override { f.~F(); }
        value call(const value& this_, const std::vector<value>& args) override { return f(this_, args); }
        gc_heap_ptr<gc_function> move(gc_heap& new_heap) override { return make(new_heap, f); }
    private:
        F f;
    };

    model* get_model() const {
        return reinterpret_cast<model*>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) + sizeof(*this));
    }

    explicit gc_function() {
    }

    ~gc_function() {
        get_model()->destroy();
    }

    gc_heap_ptr_untyped move(gc_heap& new_heap) const {
        return get_model()->move(new_heap);
    }
};

} // namespace mjs


#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif