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

JSON.stringify.length;          //$number 3
JSON.stringify();               //$undefined
JSON.stringify(undefined);      //$undefined
JSON.stringify(function(){});   //$undefined
JSON.stringify(null);           //$string 'null'
JSON.stringify(false);          //$string 'false'
JSON.stringify(true);           //$string 'true'
JSON.stringify(42);             //$string '42'
JSON.stringify(-1.232);         //$string '-1.232'
JSON.stringify(NaN);            //$string 'null'
JSON.stringify(-Infinity);      //$string 'null'
JSON.stringify('');             //$string '""'
JSON.stringify('abc');          //$string '"abc"'
JSON.stringify('xb"\u1234\u001F"\b\f\n\r\t');  //$string '"xb\\"\u1234\\u001f\\"\\b\\f\\n\\r\\t"'
JSON.stringify(new Date(686452434213)); //$string '"1991-10-03T01:13:54.213Z"'

function J(x) { this.x = x; }
J.prototype.toJSON = function(key) { return new String(this.x + ',' + key); }
JSON.stringify(new J('xyz')); //$string '"xyz,"'

JSON.stringify(new Number(-60.12)); //$string '-60.12'
JSON.stringify(new Boolean(true)); //$string 'true'

JSON.stringify([]); //$string '[]'
JSON.stringify([1]); //$string '[1]'
JSON.stringify([1,,'x']); //$string '[1,null,"x"]'
JSON.stringify([[1,2],[3,4]]); //$string '[[1,2],[3,4]]'

try { a=[]; a[0]=a; JSON.stringify(a); } catch (e) { e.toString(); } //$string 'TypeError: Object must be acyclic'

try { o={}; o.x=o; JSON.stringify(o); } catch (e) { e.toString(); } //$string 'TypeError: Object must be acyclic'

JSON.stringify({}); //$string '{}'
JSON.stringify({a:1,b:'x'}); //$string '{"a":1,"b":"x"}'

o = {x:42};
Object.defineProperty(o, 'y', {enumerable:true,value:60});
Object.defineProperty(o, 'z', {enumerable:false,value:61});
JSON.stringify(o); //$string '{"x":42,"y":60}'

JSON.stringify({if:{a:42},x:{b:'x'}}); //$string '{"if":{"a":42},"x":{"b":"x"}}'

    )");
}
