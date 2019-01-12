#ifndef MJS_STRING_OBJECT_H
#define MJS_STRING_OBJECT_H

#include "global_object.h"

namespace mjs {

global_object_create_result make_string_object(global_object& global);

object_ptr new_string(const gc_heap_ptr<global_object>& global, const string& val);

std::wstring_view ltrim(std::wstring_view s, version ver);
std::wstring_view rtrim(std::wstring_view s, version ver);
std::wstring_view trim(std::wstring_view s, version ver);

} // namespace mjs

#endif