#ifndef MJS_INTERPRETER_H
#define MJS_INTERPRETER_H

#include "value.h"
#include "version.h"
#include "error_object.h"
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace mjs {

class block_statement;
class statement;
class expression;
struct source_extend;

enum class completion_type {
    normal, break_, continue_, return_, throw_
};
std::wostream& operator<<(std::wostream& os, const completion_type& t);

using label_set = std::vector<std::wstring_view>;
struct completion {
    completion_type type;
    value result;
    std::wstring_view target;

   explicit completion(const value& r = value::undefined, completion_type t = completion_type::normal) : type(t), result(r), target() {
        assert(!has_target());
   }

   explicit completion(completion_type t, std::wstring_view target) : type(t), result(value::undefined), target(target) {
       assert(has_target());
   }

   bool in_set(const label_set& ids) const {
       assert(has_target());
       return target.empty() || std::find(ids.begin(), ids.end(), target) != ids.end();
   }

   explicit operator bool() const { return type != completion_type::normal; }

   bool has_target() const { return type == completion_type::break_ || type == completion_type::continue_; }
};
std::wostream& operator<<(std::wostream& os, const completion& c);

class interpreter {
public:
    using on_statement_executed_type = std::function<void (const statement&, const completion& c)>;

    explicit interpreter(gc_heap& h, version ver, const on_statement_executed_type& on_statement_executed = on_statement_executed_type{});
    ~interpreter();

    gc_heap_ptr<global_object> global() const;

    value eval(const statement& bs);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

} // namespace mjs

#endif
