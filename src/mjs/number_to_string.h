#ifndef MJS_NUMBER_TO_STRING_H
#define MJS_NUMBER_TO_STRING_H

#include <string>

namespace mjs {

std::wstring number_to_string(double x);
std::wstring number_to_radix_string(double x, int radix);
std::wstring number_to_fixed(double x, int f);
std::wstring number_to_exponential(double x, int f);
std::wstring number_to_precision(double x, int p);

} // namespace mjs

#endif

