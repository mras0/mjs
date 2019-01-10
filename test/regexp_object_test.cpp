#include "test.h"
#include "test_spec.h"
#include <mjs/array_object.h>
#include <iostream>

using namespace mjs;

void test_main() {
    if (tested_version() == version::es1) {
        expect_eval_exception(L"new RegExp();");
        // That RegExp literals aren't supported is tested elsewhere
        return;
    }

    // TODO: There will be looooooots of cases missing here...

    RUN_TEST_SPEC(R"(
r=new RegExp();

r.key=42;
keys = '';
for (var k in r) { keys += k+','; }
keys; //$string 'key,'

delete r.source;
r.source; //$string '(?:)'
r.source=42;
r.source; //$string '(?:)'

delete r.global;
r.global; //$boolean false
r.global = 42;
r.global; //$boolean false

delete r.ignoreCase;
r.ignoreCase; //$boolean false
r.ignoreCase = 42;
r.ignoreCase; //$boolean false

delete r.multiline;
r.multiline; //$boolean false
r.multiline = false;
r.multiline; //$boolean false

delete r.lastIndex;
r.lastIndex; //$number 0
r.lastIndex = 42;
r.lastIndex; //$number 42
)");

    RUN_TEST_SPEC(R"(
RegExp.prototype.constructor.length;//$number 2
r=RegExp.prototype.constructor('a*b', 'gim');

r.source; //$string 'a*b'
r.global; //$boolean true
r.ignoreCase; //$boolean true
r.multiline; //$boolean true
r.lastIndex; //$number 0

r.toString(); //$string '/a*b/gim'
)");


    //
    // RegExp literal
    //

    RUN_TEST_SPEC(R"(
r = / test \/ foo /gi;

r instanceof RegExp; //$boolean true
r.source; //$string ' test \\/ foo '
r.global; //$boolean true
r.ignoreCase; //$boolean true
r.multiline; //$boolean false
r.lastIndex; //$number 0

r.toString(); //$string '/ test \\/ foo /gi'

x = /a/gi; x === x; //$boolean true
/a/gi == /a/gi; //$boolean false
)");

    //
    // Check basic matching
    //
    RUN_TEST_SPEC(R"(
r = /(a*)b/;

r.exec('xx');       //$null
r.test('xx');       //$boolean false

/undef/.test();     //$boolean true

x = r.exec('aabb');
x instanceof Array; //$boolean true
x.length;           //$number 2
x[0];               //$string 'aab'
x[1];               //$string 'aa'
x.index;            //$number 0
x.input;            //$string 'aabb'
r.lastIndex;        //$number 0

x = r.exec('aabb');
x.length;           //$number 2
x[0];               //$string 'aab'
x[1];               //$string 'aa'
x.index;            //$number 0
x.input;            //$string 'aabb'
r.lastIndex;        //$number 0

r.test('aaaaaabb'); //$boolean true

y = r.exec('x ab');
y.length;           //$number 2
y[0];               //$string 'ab'
y[1];               //$string 'a'
y.index;            //$number 2
y.input;            //$string 'x ab'

r = /\w+/g;
s = 'xx yy zz';
w = r.exec(s);

w.length;           //$number 1
w[0];               //$string 'xx'
r.lastIndex;        //$number 2

w = r.exec(s);
w.length;           //$number 1
w[0];               //$string 'yy'
r.lastIndex;        //$number 5

w = r.exec(s);
w.length;           //$number 1
w[0];               //$string 'zz'
r.lastIndex;        //$number 8
)");

    //TODO:
    // - Invalid flags (no duplicates, case sensitive, invalid characters not allowed) => SyntaxError
    // - Invalid pattern => SyntaxError
    // - Matching (basic)
    //    - RegExp.prototype.exec(string)
    //    - RegExp.prototype.test(string) `RegExp.prototype.exec(string) != null`
    // - Multiline regex
    // - RegExp use with String
    // - Constructor corner cases
    // - etc...
    //
    // SyntaxError
    //
    // ES3, 15.10.2.5, 15.10.2.9, 15.10.2.15, 15.10.2.19
    //

    RUN_TEST_SPEC(R"(
// ES3, 15.10.3.1
r =  /xyz/g;
RegExp(r) === r; //$boolean true
RegExp(r,undefined) === r; //$boolean true
new RegExp(r) === r; //$boolean false

// ES3, 15.10.4.1
re = new RegExp(/ab/gi, undefined);
re.source;//$string 'ab'
re.global;//$boolean true
re.ignoreCase;//$boolean true
try { RegExp(/ab/,'gi'); } catch (e) { e.toString(); } //$string 'TypeError: Invalid flags argument to RegExp constructor'
try { new RegExp(/ab/,'gi'); } catch (e) { e.toString(); } //$string 'TypeError: Invalid flags argument to RegExp constructor'
try { new RegExp('123','x'); } catch (e) { e.toString(); } //$string 'SyntaxError: Invalid flag \'x\' given to RegExp constructor'
try { new RegExp('123','gig'); } catch (e) { e.toString(); } //$string 'SyntaxError: Duplicate flag \'g\' given to RegExp constructor'
// ES3, 15.10.6
function e(func) { try { RegExp.prototype[func].call({}); } catch (e) { return e.toString(); } };
e('toString'); //$string 'TypeError: Object is not a RegExp'
e('exec'); //$string 'TypeError: Object is not a RegExp'
e('test'); //$string 'TypeError: Object is not a RegExp'
)");

    // From ES5 RegExp.prototype is a RegExp object
    gc_heap h{8192};
    RUN_TEST(L"RegExp.prototype.source", tested_version() >= version::es5 ? value{string{h,"(?:)"}} : value::undefined);

    if (tested_version() >= version::es5) {
        RUN_TEST_SPEC(R"(
r = /[/]/;
r.source; //$string '[\\/]'
r.global; //$boolean false
r.ignoreCase; //$boolean false
r.multiline; //$boolean false
)");
    }
}