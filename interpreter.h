#ifndef MJS_INTERPRETER_H
#define MJS_INTERPRETER_H

#include "value.h"

namespace mjs {

class block_statement;
class statement;
class expression;

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

class interpreter {
public:
    explicit interpreter(const block_statement& program);
    ~interpreter();

    value eval(const expression& e);
    completion eval(const statement& s);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

} // namespace mjs

#endif