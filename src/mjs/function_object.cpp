#include "function_object.h"
#include "error_object.h"
#include <sstream>

namespace mjs {

namespace {
static value get_this_arg(const gc_heap_ptr<global_object>& global, const value& this_arg) {
    // TODO: Check ES5.1, 10.4.3
    if (this_arg.type() == value_type::undefined || this_arg.type() == value_type::null) {
        return value{global};
    } else {
        return value{global->to_object(this_arg)};
    }
}

} // unnamed namespace

class bound_function_args {
public:
    value bound_this() const {
        return bound_this_.get_value(heap_);
    }

    uint32_t bound_args_len() const {
        return bound_args_ ? bound_args_.dereference(heap_).length() : 0;
    }

    value bound_arg(uint32_t index) const {
        assert(index < bound_args_len());
        return bound_args_.dereference(heap_)[index].get_value(heap_);
    }

    std::vector<value> build_args(const std::vector<value>& extra_args) const {
        if (!bound_args_) {
            return extra_args;
        }
        std::vector<value> res;
        auto& ba = bound_args_.dereference(heap_);
        for (uint32_t i = 0; i < ba.length(); ++i) {
            res.push_back(ba[i].get_value(heap_));
        }
        res.insert(res.end(), extra_args.begin(), extra_args.end());
        return res;
    }

private:
    friend class gc_type_info_registration<bound_function_args>;
    gc_heap&                                               heap_;
    value_representation                                   bound_this_;
    gc_heap_ptr_untracked<gc_vector<value_representation>> bound_args_;

    explicit bound_function_args(gc_heap& h, const std::vector<value>& args)
        : heap_{h}
        , bound_this_{value::undefined}
        , bound_args_{nullptr} {
        if (!args.empty()) {
            bound_this_ = value_representation{args[0]};
            if (args.size() > 1) {
                bound_args_ =  gc_vector<value_representation>::make(heap_, static_cast<uint32_t>(args.size() - 1));
                auto a = bound_args_.dereference(heap_);
                for (uint32_t i = 1; i < static_cast<uint32_t>(args.size()); ++i) {
                    a.push_back(value_representation{args[i]});
                }
            }
        }
    }

    void fixup() {
        bound_this_.fixup(heap_);
        bound_args_.fixup(heap_);
    }
};
static_assert(!gc_type_info_registration<bound_function_args>::needs_destroy);
static_assert(gc_type_info_registration<bound_function_args>::needs_fixup);

function_object::function_object(const gc_heap_ptr<global_object>& global, const string& class_name, const object_ptr& prototype)
    : native_object(class_name, prototype)
    , global_(global)
    , prototype_prop_(value_representation{value::null}) {
    DEFINE_NATIVE_PROPERTY_READONLY(function_object, length);
    DEFINE_NATIVE_PROPERTY(function_object, prototype);
    if (global->language_version() < version::es5 || !global->strict_mode()) {
        object::redefine_own_property(global->common_string("arguments"), value::null, property_attribute::dont_delete|property_attribute::dont_enum|property_attribute::read_only);
    } else {
        global->define_thrower_accessor(*this, "arguments");
        global->define_thrower_accessor(*this, "caller");
    }

    static_assert(!gc_type_info_registration<function_object>::needs_destroy);
    static_assert(gc_type_info_registration<function_object>::needs_fixup);
}

void function_object::fixup() {
    auto& h = heap();
    global_.fixup(h);
    construct_.fixup(h);
    call_.fixup(h);
    text_.fixup(h);
    prototype_prop_.fixup(h);
    native_object::fixup();
}

void function_object::put_function(const gc_heap_ptr<gc_function>& f, const gc_heap_ptr<gc_string>& name, const gc_heap_ptr<gc_string>& body_text, int named_args) {
    assert(!call_ && !text_ && !named_args_ && !is_native_);
    call_ = f;
    if (!body_text) {
        text_ = name;
        is_native_ = true;
    } else {
        assert(!name);
        text_ = body_text;
    }
    named_args_ = named_args;
}


string function_object::to_string() const {
    auto& h = heap();
    if (is_native_) {
        std::wostringstream oss;
        oss << "function ";
        if (text_) {
            oss << text_.dereference(h).view();
        }
        oss << "() { [native code] }";
        return string{h, oss.str()};
    } else {
        return text_.track(h);
    }
}

value function_object::call(const value& this_, const std::vector<value>& args) const {
    if (!call_) throw not_callable_exception{};
    return call_.dereference(heap()).call(get_this_arg(global_.track(heap()), this_), args);
}

value function_object::construct(const value& this_, const std::vector<value>& args) const {
    if (!construct_) throw not_callable_exception{};
    return construct_.dereference(heap()).call(this_, args);
}

object_ptr function_object::bind(const gc_heap_ptr<function_object>& f, const std::vector<value>& args) {
    auto& h = f.heap();
    auto global = f->global_.track(h);
    auto bound_args = h.make<bound_function_args>(h, args);

    // Actually need to create object to store "args" (thisArg and the extra (needed?) arguments)
    // Put it in a special pointer in function? (will help with the type checks)

    // ES5.1, 15.3.4.5
    auto res = make_function(global, [f, bound_args](const value&, const std::vector<value>& args) {
        return f->call(bound_args->bound_this(), bound_args->build_args(args));
    }, string{h,""}.unsafe_raw_get(), std::max(0,  f->named_args_ - static_cast<int>(bound_args->bound_args_len())));

    make_constructable(global, res, [f, bound_args](const value&, const std::vector<value>& args) {
        return f->construct(value::undefined, bound_args->build_args(args));
    });
    
    return res;
}

gc_heap_ptr<function_object> make_raw_function(const gc_heap_ptr<global_object>& global) {
    auto fp = global->function_prototype();
    auto o = global.heap().make<function_object>(global, fp->class_name(), fp);
    const auto ver = global->language_version();
    const auto attrs =
        (ver != version::es3 ? property_attribute::dont_enum : property_attribute::none)
        | (ver >= version::es3 ? property_attribute::dont_delete : property_attribute::none);
    o->put_prototype_with_attributes(global->make_object(),  attrs);
    return o;
}

global_object_create_result make_function_object(const gc_heap_ptr<global_object>& global) {
    auto prototype = global->function_prototype();

    // §15.3.4
    assert(prototype.has_type<function_object>());
    static_cast<function_object&>(*prototype).put_function([](const value&, const std::vector<value>&) {
        return value::undefined;
    }, nullptr, nullptr, 0);

    put_native_function(global, prototype, "toString", [global, prototype](const value& this_, const std::vector<value>&) {
        // HACK to make Function.prototype.toString() work..
        if (this_.type() != value_type::object || this_.object_value().get() != prototype.get()) {
            global->validate_type(this_, prototype, "function");
        }
        assert(this_.object_value().has_type<function_object>());
        return value{static_cast<function_object&>(*this_.object_value()).to_string()};
    }, 0);

    if (global->language_version() >= version::es3) {
        put_native_function(global, prototype, "call", [global](const value& this_, const std::vector<value>& args) {
            global->validate_type(this_, global->function_prototype(), "function");
            std::vector<value> new_args;
            if (args.size() > 1) {
                new_args.insert(new_args.end(), args.cbegin() + 1, args.cend());
            }
            return static_cast<const function_object&>(*this_.object_value()).call(!args.empty() ? args.front() : value::undefined, new_args);
        }, 1);

        put_native_function(global, prototype, "apply", [global](const value& this_, const std::vector<value>& args) {
            global->validate_type(this_, global->function_prototype(), "function");
            std::vector<value> new_args;

            if (args.size() > 1 && args[1].type() != value_type::undefined && args[1].type() != value_type::null) {
                std::wostringstream woss;
                if (args[1].type() == value_type::object) {
                    auto a = args[1].object_value();
                    auto p = a->prototype();
                    if (global->language_version() >= version::es5
                    || (p && p.get() == global->array_prototype().get()) 
                        || global->is_arguments_array(a)) {
                        const uint32_t len = to_uint32(a->get(L"length"));
                        new_args.resize(len);
                        for (uint32_t i = 0; i < len; ++i) {
                            new_args[i] = a->get(index_string(i));
                        }
                        goto do_call;
                    }
                    woss << a->class_name();
                } else {
                    debug_print(woss, args[1], 4);
                }
                woss << " is not an (arguments) array";
                throw native_error_exception(native_error_type::type, global->stack_trace(), woss.str());
            }
do_call:
            return static_cast<const function_object&>(*this_.object_value()).call(!args.empty() ? args.front() : value::undefined, new_args);
        }, 2);
    }

    if (global->language_version() >= version::es5) {
        put_native_function(global, prototype, "bind", [global](const value& this_, const std::vector<value>& args) {
            global->validate_type(this_, global->function_prototype(), "function");
            return value{function_object::bind(gc_heap_ptr<function_object>{this_.object_value()}, args)};
        }, 1);
    }

    auto obj = make_raw_function(global); // Note: function constructor is added by interpreter
    return { obj, prototype };
}

function_object& get_function_object(const value& v) {
    if (v.type() == value_type::object) {
        if (auto o = v.object_value(); o.has_type<function_object>()) {
            return static_cast<function_object&>(*o);
        }
    }
    throw not_callable_exception{v.type()};
}

value call_function(const value& v, const value& this_, const std::vector<value>& args) {
    return get_function_object(v).call(this_, args);
}

value construct_function(const value& v, const value& this_, const std::vector<value>& args) {
    return get_function_object(v).construct(this_, args);
}

} // namespace mjs