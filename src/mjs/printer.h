#ifndef MJS_PRINTER_H
#define MJS_PRINTER_H

#include <iosfwd>

namespace mjs {

class expression;
class statement;

void print(std::wostream& os, class expression& e);
void print(std::wostream& os, class statement& s);

} // namespace mjs

#endif
