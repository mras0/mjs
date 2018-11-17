#ifndef MJS_GC_FUNCTION_H
#define MJS_GC_FUNCTION_H

#include "gc_heap.h"
#include "value.h"

namespace mjs {

// Alignment needed so impl<F> will be construct with proper alignment
class alignas(uint64_t) gc_function {
public:
    template<typename F>
    static gc_heap_ptr<gc_function> make(gc_heap& h, const F& f) {
        auto p = h.allocate_and_construct<gc_function>(sizeof(gc_function) + sizeof(impl<F>));
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
        virtual void move(model* to) = 0;
    };
    template<typename F>
    class impl : public model {
    public:
        explicit impl(const F& f) : f(f) {}
        explicit impl(F&& f) : f(std::move(f)) {}
        void destroy() override { f.~F(); }
        value call(const value& this_, const std::vector<value>& args) override { return f(this_, args); }
        void move(model* to) override { new (to) impl<F>(std::move(*this)); }
    private:
        F f;
    };

    model* get_model() const {
        return reinterpret_cast<model*>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) + sizeof(*this));
    }

    explicit gc_function() {
    }

    gc_function(gc_function&& from) {
        from.get_model()->move(get_model());
    }

    ~gc_function() {
        get_model()->destroy();
    }
};

} // namespace mjs

#endif