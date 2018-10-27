#ifndef MJS_PRINTER_H
#define MJS_PRINTER_H

#include <iosfwd>

namespace mjs {

class expression;
class statement;

void print(std::wostream& os, const class expression& e);
void print(std::wostream& os, const class statement& s);

} // namespace mjs

#endif
