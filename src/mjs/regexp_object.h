#ifndef MJS_REGEXP_OBEJECT_H
#define MJS_REGEXP_OBEJECT_H

#include "global_object.h"

namespace mjs {

class regexp;

global_object_create_result make_regexp_object(const gc_heap_ptr<global_object>& global);

object_ptr make_regexp(const gc_heap_ptr<global_object>& global, const regexp& re);

object_ptr make_regexp(const gc_heap_ptr<global_object>& global, const string& pattern, const string& flags);

// ES3, 15.5.4.10 String.prototype.match(regexp)
value string_match(const gc_heap_ptr<global_object>& global, const string& str, const value& regexp);

// ES3 15.5.4.11 String.prototype.replace(searchValue, replaceValue)
value string_replace(const gc_heap_ptr<global_object>& global, const string& str, const value& search_value, const value& replace_value);

// ES3, 15.5.4.12 String.prototype.search(regexp)
value string_search(const gc_heap_ptr<global_object>& global, const string& str, const value& regexp);

} // namespace mjs

#endif