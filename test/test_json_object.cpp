#include "test.h"
#include "test_spec.h"

using namespace mjs;

void test_main() {
    if (tested_version() < version::es5) {
        RUN_TEST(L"global['JSON']", value::undefined);
        return;
    }

    RUN_TEST_SPEC(R"(
JSON instanceof Object; //$boolean true
Object.isExtensible(JSON); //$boolean true
Object.getOwnPropertyNames(JSON).toString(); //$string 'parse,stringify'
JSON.toString(); //$string '[object JSON]'
try { JSON(); } catch (e) { e.toString(); } //$string 'TypeError: object is not a function'
JSON.parse.length;//$number 2
JSON.stringify.length;//$number 3

    )");
}
