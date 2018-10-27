#ifndef MJS_TEST_SPEC_H
#define MJS_TEST_SPEC_H

#include <string_view>

namespace mjs {

void run_test_spec(const std::string_view& test_spec, const std::string_view& name = "run_test_spec");
#define RUN_TEST_SPEC(text) ::mjs::run_test_spec(text, std::string("testspec@")+__FILE__+"@"+__FUNCTION__+"@"+std::to_string(__LINE__))

} // namespace mjs

#endif
