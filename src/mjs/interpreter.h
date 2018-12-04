#ifndef MJS_INTERPRETER_H
#define MJS_INTERPRETER_H

#include "value.h"
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
    normal, break_, continue_, return_
};
std::wostream& operator<<(std::wostream& os, const completion_type& t);

struct completion {
    completion_type type;
    value result;

    explicit completion(completion_type t = completion_type::normal, const value& r = value::undefined) : type(t), result(r) {}

    explicit operator bool() const { return type != completion_type::normal; }
};
std::wostream& operator<<(std::wostream& os, const completion& c);

class eval_exception : public std::runtime_error {
public:
    explicit eval_exception(const std::vector<source_extend>& stack_trace, const std::wstring_view& msg);
};

class interpreter {
public:
    using on_statement_executed_type = std::function<void (const statement&, const completion& c)>;

    explicit interpreter(gc_heap& h, const block_statement& program, const on_statement_executed_type& on_statement_executed = on_statement_executed_type{});
    ~interpreter();

    value eval(const expression& e);
    completion eval(const statement& s);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

} // namespace mjs

#endif
