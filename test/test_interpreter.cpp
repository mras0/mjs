#include <iostream>
#include <cmath>
#include <cstring>
#include <sstream>

#include <mjs/interpreter.h>
#include <mjs/parser.h>
#include <mjs/printer.h>
#include <mjs/object.h>
#include <mjs/object.h>

#include "test_spec.h"
#include "test.h"

using namespace mjs;

void eval_tests() {
    gc_heap h{1<<20};

    RUN_TEST(L"undefined", value::undefined);
    RUN_TEST(L"null", value::null);
    RUN_TEST(L"false", value{false});
    RUN_TEST(L"true", value{true});
    RUN_TEST(L"123", value{123.});
    RUN_TEST(L"0123", value{83.});
    RUN_TEST(L"0x123", value{291.});
    RUN_TEST(L"'te\"st'", value{string{h, "te\"st"}});
    RUN_TEST(L"\"te'st\"", value{string{h, "te'st"}});
    RUN_TEST(L"/*42*/60", value{60.0});
    RUN_TEST(L"x=12//\n+34;x", value{46.0});
    RUN_TEST(L"-7.5 % 2", value{-1.5});
    RUN_TEST(L"1+2*3", value{7.});
    RUN_TEST(L"''+(0.1+0.2)", value{string{h, "0.30000000000000004"}});
    RUN_TEST(L"x = 42; 'test ' + 2 * (6 - 4 + 1) + ' ' + x", value{string{h, "test 6 42"}});
    RUN_TEST(L"y=1/2; z='string'; y+z", value{string{h, "0.5string"}});
    RUN_TEST(L"var x=2; x++;", value{2.0});
    RUN_TEST(L"var x=2; x++; x", value{3.0});
    RUN_TEST(L"var x=2; x--;", value{2.0});
    RUN_TEST(L"var x=2; x--; x", value{1.0});
    RUN_TEST(L"var x = 42; delete x; x", value{42.}); // Variables aren't deleted
    RUN_TEST(L"delete Object.prototype;", value{false});
    RUN_TEST(L"delete !global", value{true});
    RUN_TEST(L"this==global", value{true});
    if (tested_version() >= version::es5) {
        RUN_TEST(L"'use strict';var ge;try{delete Object.prototype;} catch(e) {ge=e;}; ge.toString()", value{string{h, "TypeError: may not delete non-configurable property \"prototype\" in strict mode"}});
    }
    RUN_TEST(L"var i=42; var x=i; x;", value{42.0});
    RUN_TEST(L"void(2+2)", value::undefined);
    RUN_TEST(L"typeof(2)", value{string{h, "number"}});
    RUN_TEST(L"x=4.5; ++x", value{5.5});
    RUN_TEST(L"x=4.5; --x", value{3.5});
    RUN_TEST(L"x=42; +x;", value{42.0});
    RUN_TEST(L"x=42; -x;", value{-42.0});
    RUN_TEST(L"x=42; !x;", value{false});
    RUN_TEST(L"x=42; ~x;", value{(double)~42});
    RUN_TEST(L"1<<2", value{4.0});
    RUN_TEST(L"-5>>2", value{-2.0});
    RUN_TEST(L"-5>>>2", value{1073741822.0});
    RUN_TEST(L"1 < 2", value{true});
    RUN_TEST(L"1 > 2", value{false});
    RUN_TEST(L"1 <= 2", value{true});
    RUN_TEST(L"1 >= 2", value{false});
    RUN_TEST(L"1 == 2", value{false});
    RUN_TEST(L"1 != 2", value{true});
    RUN_TEST(L"1 == '1'", value{true});
    RUN_TEST(L"NaN == NaN", value{false});
    if (tested_version() >= version::es3) {
        RUN_TEST(L"1 === '1'", value{false});
        RUN_TEST(L"1 !== '1'", value{true});
        RUN_TEST(L"undefined === undefined", value{true});
        RUN_TEST(L"undefined === null", value{false});
        RUN_TEST(L"undefined !== null", value{true});
        RUN_TEST(L"null === null", value{true});
        RUN_TEST(L"NaN !== NaN", value{true});
        RUN_TEST(L"NaN === NaN", value{false});
        RUN_TEST(L"+0 === -0", value{true});
        RUN_TEST(L"-0 === +0", value{true});
        RUN_TEST(L"true === false", value{false});
        RUN_TEST(L"true === true", value{true});
        RUN_TEST(L"'1234' === '1234'", value{true});
        RUN_TEST(L"'1234' === '1234 '", value{false});
        RUN_TEST(L"Object === Object", value{true});
        RUN_TEST(L"Object !== Object", value{false});
        RUN_TEST(L"Object === Function", value{false});
        RUN_TEST(L"Object !== Function", value{true});
        RUN_TEST(L"a={};1 in a", value{false});
        RUN_TEST(L"a={1:1};1 in a", value{true});
        RUN_TEST(L"a={'x':1};'x' in a", value{true});
        RUN_TEST(L"a={1000:1};1e3 in a", value{true});
        // These tests of `instanceof` don't really belong here as they rely on global object, but eh
        RUN_TEST(L"o={};o instanceof Object", value{true});
        RUN_TEST(L"o={};o instanceof Array", value{false});
        RUN_TEST(L"4 instanceof Number", value{false});
        RUN_TEST(L"'' instanceof String", value{false});
        RUN_TEST(L"new String() instanceof String", value{true});

        RUN_TEST(L"function o(){};function p(){};p.prototype=new o();i=new p(); i instanceof o && i instanceof p;", value{true});
    }

    // Check access to properties in the prototype
    RUN_TEST_SPEC(R"(
function C(){}
C.prototype = new Array('a','b');
ci = new C();
ci.join.toString();//$string 'function join() { [native code] }'
ci.length; //$number 2
ci[0]; //$string 'a'
ci[1]; //$string 'b'
ci[2]; //$undefined
)");

    RUN_TEST(L"255 & 128", value{128.0});
    RUN_TEST(L"255 ^ 128", value{127.0});
    RUN_TEST(L"64 | 128", value{192.0});
    RUN_TEST(L"42 || 13", value{42.0});
    RUN_TEST(L"42 && 13", value{13.0});
    RUN_TEST(L"1 ? 2 : 3", value{2.0});
    RUN_TEST(L"0 ? 2 : 1+2", value{3.0});
    RUN_TEST(L"x=2.5; x+=4; x", value{6.5});
    RUN_TEST(L"function f(x,y) { return x*x+y; } f(2, 3)", value{7.0});
    RUN_TEST(L"function f(){ i = 42; }; f(); i", value{42.0});
    RUN_TEST(L"i = 1; function f(){ var i = 42; }; f(); i", value{1.0});
    RUN_TEST(L"x=42; function f(x) { return x; } f()", value::undefined);
    RUN_TEST(L"function f(){if(1){function a(){return 42;} return a(); }}; f()", value{42.});
    RUN_TEST(L"{if(1) function a(){return 42;}} a(42);", value{42.});
    RUN_TEST(L"function a(){return 1;}; a()", value{1.});

    // functions are defined at the start of scope
    RUN_TEST(L"a=f.length; function f(x,y) {} a", value{2.0});
    RUN_TEST_SPEC(R"(
function g() { return ''+this.x; }
f.prototype.toString = g;
function f(x)  {this.x=x; }

(''+new f('test'));//$string 'test'

)");
    //TODO: Test more advanced cases

    RUN_TEST(L";", value::undefined);
    RUN_TEST(L"if (1) 2;", value{2.0});
    RUN_TEST(L"if (0) 2;", value::undefined);
    RUN_TEST(L"if (0) 2; else ;", value::undefined);
    RUN_TEST(L"if (0) 2; else 3;", value{3.0});
    RUN_TEST(L"1,2", value{2.0});
    RUN_TEST(L"x=5; while(x-3) { x = x - 1; } x", value{3.0});
    RUN_TEST(L"x=2; y=0; while(1) { if(x) {x = x - 1; y = y + 2; continue; y = y + 1000; } else break; y = y + 1;} y", value{4.0});
    RUN_TEST(L"var x = 0; for(var i = 10, dec = 1; i; i = i - dec) x = x + i; x", value{55.0});
    RUN_TEST(L"var x=0; for (i=2; i; i=i-1) x=x+i; x+i", value{3.0});
    RUN_TEST(L"for (var i=0;!i;) { i=1 }", value{1.});

    // Check break in nested for loop
    RUN_TEST_SPEC(R"(
s='';
for (var i=0;i<2;++i)for (var j=0;j<2;++j) { s+=i+'-'+j; break; }
s; //$string '0-01-0'
)");

    // for in statement
    // FIXME: The order of the objects are unspecified in earlier revisions of ECMAScript..
    RUN_TEST_SPEC(R"(
function r(o) {
    var s = '';
    for (k in o) {
        if(s) s+=',';
        s += k;
    }
    return s;
}
r(42); //$ string ''
global['k']; //$ undefined
var o = new Object(); o.x=42; o.y=60;
r(o); //$ string 'x,y'
global['k']; //$ string 'y'
)");

    RUN_TEST_SPEC(R"(
function r(o) {
    var s = '';
    for (var k in o) {
        if(s) s+=',';
        s += k;
    }
    return s;
}
r(42); //$ string ''
global['k']; //$ undefined
var o = new Object(); o['test test'] = 60; o.prop = 12;
r(o); //$ string 'test test,prop'
global['k']; //$ undefined

for (var y = 11 in 60){}
y; //$ number 11
)");

    // for..in  on undefined/null is a NO-op in ES5+ (12.6.4)
    if (tested_version() < version::es5) {
        expect_eval_exception(L"for(k in undefined){}");
        expect_eval_exception(L"for(k in null){}");
    } else {
        RUN_TEST_SPEC(R"(
s= ''; for (k in undefined) { s+=k; }; s; //$string ''
s= ''; for (k in null) { s+=k; }; s; //$string ''
s= ''; for (k in {a:32}) { s+=k; }; s; //$string 'a'
s= ''; for (var k in undefined) { s+=k; }; s; //$string ''
s= ''; for (var k in null) { s+=k; }; s; //$string ''
)");
    }

    // with statement
    RUN_TEST_SPEC(R"(
var a = new Object();
a.x = 42; //$ number 42
with (a) {
    x; //$ number 42
    x = 60;
    y = true;
    var z = 60;
}
global['x']; //$ undefined
global['y']; //$ boolean true
global['z']; //$ number 60
a.x; //$ number 60
a.y; //$ undefined

function f() {
    a.y=0;
    with(a) {
        x=123;
        y=1;
        return;
    }
};
f();
global['x']; //$ undefined
global['y']; //$ boolean true
a.x; //$ number 123
a.y; //$ number 1

with(42) {
    toString(); //$ string '42'
}
)");


    RUN_TEST(L"function sum() {  var s = 0; for (var i = 0; i < arguments.length; ++i) s += arguments[i]; return s; } sum(1,2,3)", value{6.0});

    // In ES5 the [[Class]] of the arguments array changed from Object to Arguments and the indexed properties are enumerable
    RUN_TEST(L" function f() { var a=''; for (k in arguments) {a+=k+',';} arguments.s=Object.prototype.toString; return a+arguments.s(); }; f(23,45);", value{string{h, tested_version() >= version::es5 ? L"0,1,[object Arguments]" : L"[object Object]"}});

    // ES3, 15.1: The [[Prototype]] and [[Class]] properties of the global object are implementation-dependent
    RUN_TEST(L"global.toString()", value{string{h, "[object Global]"}});

    // Object
    RUN_TEST(L"''+Object(null)", value{string{h, "[object Object]"}});
    RUN_TEST(L"o=Object(null); o.x=42; o.y=60; o.x+o['y']", value{102.0});
    RUN_TEST(L"a=Object(null);b=Object(null);a.x=b;a.x.y=42;a['x']['y']", value{42.0});
    RUN_TEST(L"'' + new Object", value{string{h, "[object Object]"}});
    RUN_TEST(L"'' + new Object()", value{string{h, "[object Object]"}});
    RUN_TEST(L"'' + new Object(null)", value{string{h, "[object Object]"}});
    RUN_TEST(L"'' + new Object(undefined)", value{string{h, "[object Object]"}});
    RUN_TEST(L"o = new Object;o.x=42; new Object(o).x", value{42.0});

    // Function. Note: toString() rendering isn't specified (exactly)
    RUN_TEST_SPEC(R"(
f=new Function();
f.toString(); //$ string 'function anonymous() {\n\n}'
f(); //$ undefined

g=Function('return 42');
g.toString(); //$ string 'function anonymous() {\nreturn 42\n}'
g(); //$ number 42

g=Function('x','return x+1;');
g.toString(); //$ string 'function anonymous(x) {\nreturn x+1;\n}'
g(2); //$ number 3

var o = new Object();
o.x = 42;
Function('return o.x')();
)");
    RUN_TEST_SPEC(R"(f=Function('a','b','c','return a+b+c');
        f(1,2,3); //$ number 6
        f.toString(); //$ string 'function anonymous(a,b,c) {\nreturn a+b+c\n}'
)");
    RUN_TEST_SPEC(R"(f=new Function('a','b','c','return a+b+c');
        f(1,2,3); //$ number 6
        f.toString(); //$ string 'function anonymous(a,b,c) {\nreturn a+b+c\n}'
)");
    RUN_TEST_SPEC(R"(
g = 0;

function f() {
    var o = new Object();
    var g = 12;
    g; //$ number 12
    o.foo = Function('y','++g; return g+y');
    return o;
}

var o = f();
o.foo(42); //$number 43
o.foo(42); //$number 44

A = new Function('x','this.x=x');
a = new A(42);
a.x; //$ number 42

a=Function('a', 'b', 'c', 'return a+b+c');
a.toString(); //$ string 'function anonymous(a,b,c) {\nreturn a+b+c\n}'
a.length; //$ number 3
b=Function('a, b, c', 'return a+b+c');
b.toString(); //$ string 'function anonymous(a, b, c) {\nreturn a+b+c\n}'
b.length; //$ number 3
c=Function('a,b', ' c', ' return     a+b+ c ');
c.toString(); //$ string 'function anonymous(a,b, c) {\n return     a+b+ c \n}'
c.length; //$ number 3

f=Function('x','return x?arguments.callee(x-1)*x:1');
f(5) //$ number 120
)");
    RUN_TEST_SPEC(R"(
var y = 0;

function print() {
    return ''+this.x+','+this.y+','+y;
}

function f(x) {
    function g() {
        this.x = x;
        this.y = y++;
    }

    g.prototype.toString = print;
    return new g();
}

''+f(42); //$ string '42,0,1'
''+f(43); //$ string '43,1,2'
)");
    RUN_TEST_SPEC(R"(
function f() { 42; }
f(); //$undefined

function f(y) { this.x=y; }
a = new f(parseInt);
a.x == parseInt; //$boolean true

function f(x) { this.x=x; }
a = new f(new f(42));
a.x.x ; //$number 42

function t(l,r) { this.l=l; this.r=r; }
function make(d) { return d > 0 ? new t(make(d-1),make(d-1)) : new t(null, null); }

x = make(1);
x.l == null; //$boolean false
x.r == null; //$boolean false
x.l.l == null; //$boolean true
x.l.r == null; //$boolean true
x.r.l == null; //$boolean true
x.r.r == null; //$boolean true

new t(42,60).l; //$number 42
new t(42,60).r; //$number 60
var x = new t(1,2);
x.l; //$number 1
x.r; //$number 2

function count(t) { return 1 + (t.l ? count(t.l) + count(t.r) : 0); }
count(make(1)); //$number 3
count(make(2)); //$number 7
count(make(3)); //$number 15
count(make(4)); //$number 31
)");

    RUN_TEST(L"function  f ( x   ,\ny )  { return x + y;  }; f.toString()", value{string{h, "function f( x   ,\ny )  { return x + y;  }"}});
    RUN_TEST(L"a=parseInt;a.toString()", value{string{h, "function parseInt() { [native code] }"}});

    // wat
    RUN_TEST(L"!!('')", value{false});
    RUN_TEST(L"\"\" == false", value{true});
    RUN_TEST(L"null == false", value{false});
    RUN_TEST(L"+true", value{1.0});
    RUN_TEST(L"true + true", value{2.0});
    RUN_TEST(L"!!('0' && Object(null))", value{true});

    RUN_TEST(L"function X() { this.val = 42; }; new X().val", value{42.0});
    RUN_TEST(L"function A() { this.n=1; }; function B() { this.n=2;} function g() { return this.n; } A.prototype.foo = g; new A().foo()", value{1.});
    RUN_TEST(L"function A() { this.n=1; }; function B() { this.n=2;} function g() { return this.n; } A.prototype.foo = g; new B().foo", value::undefined);
    RUN_TEST(L"function f() { this.a = 1; this.b = 2; } var o = new f(); f.prototype.b = 3; f.prototype.c = 4; '' + new Array(o.a,o.b,o.c,o.d)", value{string{h, "1,2,4,"}});
    RUN_TEST(L"var o = new Object(); o.a = 2; function m(){return this.a+1; }; o.m = m; o.m()", value{3.});

    RUN_TEST(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; si.prop", value{string{h, "some value"}});
    RUN_TEST(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; si.foo", value{string{h, "bar"}});
    RUN_TEST(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.prop", value::undefined);
    RUN_TEST(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.foo", value::undefined);
    RUN_TEST(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.prototype.prop", value::undefined);
    RUN_TEST(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.prototype.foo", value{string{h, "bar"}});

    // Test arguments alias
    RUN_TEST_SPEC(R"(
function evil(x, y) {
    arguments.length; //$ number 2
    arguments[0]; //$number 12
    arguments[1]; //$number 34
    x; //$number 12
    y; //$number 34
    arguments[0] = 56;
    y = 78;
    arguments[0]; //$number 56
    arguments[1]; //$number 78
    x; //$number 56
    y; //$number 78
}
evil(12, 34);
)");

    if (tested_version() >= version::es5) {
        RUN_TEST(L"debugger", value::undefined);

        // No argument aliasing in strict mode
        RUN_TEST_SPEC(R"(
function evil(x, y) {
    'use strict';
    arguments.length; //$ number 2
    arguments[0]; //$number 12
    arguments[1]; //$number 34
    x; //$number 12
    y; //$number 34
    arguments[0] = 56;
    y = 78;
    arguments[0]; //$number 56
    arguments[1]; //$number 34
    x;            //$number 12
    y;            //$number 78
}
evil(12, 34);
)");
    }

    RUN_TEST_SPEC(R"(
// 11.8.5 Order of evaluation is left to right
function X(n){this.n=n;}
function xval(){s+=this.n;return this.n;};
X.prototype.valueOf=xval;
s=''; new X(0) < new X(1); s; //$string '01'
s=''; new X(2) <= new X(3); s; //$string '23'
s=''; new X(8) >= new X(9); s; //$string '89'
s=''; new X(10) > new X(11); s; //$string '1011'
)");

    RUN_TEST_SPEC(R"(
if (1) { var x = 42; }
x; //$number 42
function f() { var y = 42; }
f();
global['y']; //$undefined
)");
}

void test_global_functions() {  
    // Values
    RUN_TEST_SPEC(R"(
NaN //$ number NaN
Infinity //$ number Infinity
)");

    // eval
    RUN_TEST_SPEC(R"(
eval() //$ undefined
eval(42) //$ number 42
eval(true) //$ boolean true
eval(new String('123')).length //$ number 3
eval('1+2*3') //$ number 7
x42=50; eval('x'+42+'=13'); x42 //$ number 13
eval('var x;'); //$undefined
)");

    if (tested_version() < version::es5) {
        RUN_TEST_SPEC(R"(
eval('"use strict"; var let;'); //$undefined
global['x']; //$undefined
eval('"use strict";var x=42');
global['x']; //$number 42
)");
    } else {
        RUN_TEST_SPEC(R"(
try { eval('"use strict"; var let;'); } catch (e) { e.toString(); } //$string 'SyntaxError: Invalid argument to eval'
try { eval('(function(){"use strict";var let;})()'); } catch (e) { e.toString(); } //$string 'SyntaxError: Invalid argument to eval'
try { eval('(function(){"use strict";var let;})()'); } catch (e) { e instanceof SyntaxError; } //$boolean true

global['x']; //$undefined
eval('"use strict"; var x=42');
global['x']; //$undefined


({ x: function() { var f = eval; f("var arguments;"); }, }).x(); //$undefined

)");

    }

    // ES5.1, 10.4.2 the global context is used for indirect calls to eval
    RUN_TEST(L"var s=1; function t() { var s=2; return eval('s'); }; t();", value{2.});
    RUN_TEST(L"var s=1; function t() { var s=2; return (0,eval)('s'); }; t();", value{tested_version() >= version::es5 ? 1. : 2.});

    // parseInt
    RUN_TEST_SPEC(R"(
parseInt('0'); //$ number 0 
parseInt(42); //$ number 42
parseInt('10', 2); //$ number 2
parseInt('123'); //$ number 123
parseInt(' \t123', 10); //$ number 123
parseInt(' 123', 8); //$ number 83
parseInt(' 0x123'); //$ number 291
parseInt(' 123', 16); //$ number 291
parseInt(' 123', 37); //$ number NaN
parseInt(' 123', 1); //$ number NaN
parseInt(' x', 1); //$ number NaN
)");
    // Octal numbers (when radix == 0) are mandatory in ES1, implemention-defined in ES3 and disallowed in ES5+
    RUN_TEST(L"parseInt(' \\t\\r 0123')", value{tested_version() >= version::es5 ? 123. : 83.});

    // parseFloat
    RUN_TEST_SPEC(R"(
parseFloat('0'); //$ number 0 
parseFloat(42); //$ number 42
parseFloat(' 42.25x'); //$ number 42.25
parseFloat('h'); //$ number NaN
)");

    // escape / unescape
    RUN_TEST_SPEC(R"(
escape() //$ string 'undefined'
escape('') //$ string ''
escape('test') //$ string 'test'
escape('42') //$ string '42'
escape('(hel lo)') //$ string '%28hel%20lo%29'
escape('\u1234') //$ string '%u1234'
escape('\u0029') //$ string '%29'
escape('\u00FF') //$ string '%FF'

unescape('') //$ string ''
unescape('test') //$ string 'test'
unescape('%28%29') //$ string '()'
unescape('%u1234').charCodeAt(0) //$ number 4660
unescape('%FF').charCodeAt(0) //$ number 255

)");

    // isNaN / isFinite
    RUN_TEST_SPEC(R"(
isNaN(NaN) //$ boolean true
isNaN(Infinity) //$ boolean false
isNaN(-Infinity) //$ boolean false
isNaN(42) //$ boolean false
isFinite(NaN) //$ boolean false
isFinite(Infinity) //$ boolean false
isFinite(-Infinity) //$ boolean false
isFinite(42) //$ boolean true
isFinite(Number.MAX_VALUE) //$ boolean true

// properties on the number object are immutable
Number.MAX_VALUE=42;
Number.MAX_VALUE;//$number 1.7976931348623157e+308
)");

    // undefined, NaN and Infinity are immutable in ES5+
    RUN_TEST(L"undefined=42;undefined", tested_version() >= version::es5 ? value::undefined : value{42.});
    RUN_TEST(L"NaN=42;NaN", tested_version() >= version::es5 ? value{NAN} : value{42.});
    RUN_TEST(L"Infinity=42;Infinity", tested_version() >= version::es5 ? value{INFINITY} : value{42.});

    // ES3, 15.1.3, URI Handling Functions

    if (tested_version() < version::es3) {
        RUN_TEST_SPEC(R"(
delete undefined; //$boolean true
delete NaN;       //$boolean true
delete Infinity;  //$boolean true
)");
        RUN_TEST(L"global.decodeURI || global.decodeURIComponent || global.encodeURI || global.encodeURIComponent", value::undefined);
        return;
    }

    RUN_TEST_SPEC(R"(
delete undefined; //$boolean false
delete NaN;       //$boolean false
delete Infinity;  //$boolean false
)");

    // Tests adapted from https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURI
    RUN_TEST_SPEC(R"x(
encodeURI(); //$string 'undefined'
encodeURIComponent(); //$string 'undefined'

var set1 = ";,/?:@&=+$#"; // Reserved Characters
var set2 = "-_.!~*'()";   // Unreserved Marks
var set3 = "ABC abc 123"; // Alphanumeric Characters + Space

encodeURI(set1); //$string ';,/?:@&=+$#'
encodeURI(set2); //$string '-_.!~*\'()'
encodeURI(set3); //$string 'ABC%20abc%20123'
encodeURI('ABC\x80\u0800\ud7ff\ue000\u1234EF'); //$string 'ABC%C2%80%E0%A0%80%ED%9F%BF%EE%80%80%E1%88%B4EF'
encodeURI('\ud7ff'); //$string '%ED%9F%BF'
encodeURI('\uD800\uDFFF'); //$string '%F0%90%8F%BF'

encodeURIComponent(set1); //$string '%3B%2C%2F%3F%3A%40%26%3D%2B%24%23'
encodeURIComponent(set2); //$string '-_.!~*\'()'
encodeURIComponent(set3); //$string 'ABC%20abc%20123'
encodeURIComponent('ABC\x7F\xAD\u1234EF'); //$string 'ABC%7F%C2%AD%E1%88%B4EF'
encodeURIComponent('\uDBFF\uDFFF'); //$string '%F4%8F%BF%BF'

try { encodeURI('\ud800'); } catch (e) { e.toString(); } //$ string 'URIError: URI malformed'
try { encodeURI('\udc00'); } catch (e) { e.toString(); } //$ string 'URIError: URI malformed'
try { encodeURIComponent('\ud800'); } catch (e) { e.toString(); } //$ string 'URIError: URI malformed'
try { encodeURIComponent('\udc00'); } catch (e) { e.toString(); } //$ string 'URIError: URI malformed'

decodeURI(); //$string 'undefined'
decodeURIComponent(); //$string 'undefined'

decodeURI(set1); //$string ';,/?:@&=+$#'
decodeURI(set2); //$string '-_.!~*\'()'
decodeURI(set3); //$string 'ABC abc 123'

decodeURI('ABC%7f'); //$string 'ABC\u007f'
decodeURI('%E0%A0%80'); //$string '\u0800'
decodeURI('%ED%9F%BF'); //$string '\ud7ff'
decodeURI('%EE%80%80'); //$string '\ue000'
decodeURI('ABC%C2%80'); //$string 'ABC\u0080'
decodeURI('%f0%90%8f%bf'); //$string '\ud800\udfff'

decodeURI('ABC%C2%80%E0%A0%80%ED%9F%BF%EE%80%80%E1%88%B4EF'); //$string 'ABC\u0080\u0800\ud7ff\ue000\u1234EF'
decodeURIComponent('%3B%2C%2F%3F%3A%40%26%3D%2B%24%23'); //$string ';,/?:@&=+$#'
decodeURIComponent(set2); //$string '-_.!~*\'()'
decodeURIComponent('ABC%20abc%20123'); //$string 'ABC abc 123'
decodeURIComponent('ABC%7F%C2%AD%E1%88%B4EF'); //$string 'ABC\u007F\u00ad\u1234EF'
decodeURIComponent('%F4%8F%BF%BF'); //$string '\uDBFF\uDFFF'

try { decodeURI('%E0%A4%A'); } catch (e) { e.toString(); } //$ string 'URIError: URI malformed'
try { decodeURIComponent('%E0%A4%A'); } catch (e) { e.toString(); } //$ string 'URIError: URI malformed'
)x");

}

void test_math_functions() {
    RUN_TEST_SPEC(R"(
Math.E;                 //$ number 2.7182818284590452354
Math.LN10;              //$ number 2.302585092994046
Math.LN2;               //$ number 0.6931471805599453
Math.LOG2E;             //$ number 1.4426950408889634
Math.LOG10E;            //$ number 0.4342944819032518
Math.PI;                //$ number 3.14159265358979323846
Math.SQRT1_2;           //$ number 0.7071067811865476
Math.SQRT2;             //$ number 1.4142135623730951

// Properties are read-only
Math.E = 42;
Math.E;                 //$ number 2.7182818284590452354

Math.abs(NaN);          //$ number NaN
Math.abs(-0);           //$ number 0
Math.abs(-Infinity);    //$ number Infinity
Math.abs(Infinity);     //$ number Infinity
Math.abs(42);           //$ number 42
Math.abs(-42);          //$ number 42

Math.acos(1);           //$ number 0
Math.asin(0);           //$ number 0
Math.atan(0);           //$ number 0
Math.atan2(0,1)         //$ number 0
Math.ceil(42.32)        //$ number 43
Math.ceil(-42.32)       //$ number -42
Math.cos(0);            //$ number 1
Math.exp(0);            //$ number 1
Math.floor(42.32);      //$ number 42
Math.floor(-42.32);     //$ number -43
Math.log(1);            //$ number 0
Math.max(1,2);          //$ number 2
Math.min(1,2);          //$ number 1
Math.pow(2,3);          //$ number 8
Math.round(0.4);        //$ number 0
Math.round(0.5);        //$ number 1
Math.round(-0.5);       //$ number 0
Math.round(-0.6);       //$ number -1
Math.round(3.5);        //$ number 4
Math.round(-3.5);       //$ number -3
Math.sin(0);            //$ number 0
Math.sqrt(4);           //$ number 2
Math.tan(0);            //$ number 0
)");

    // random not tested

    // TODO: Test many of the functions more thouroughly, they are (probably) not following the specification in corner cases
}

void test_long_object_chain() {
    RUN_TEST(LR"(
var l = null;
for (var i = 0; i < )"
#ifdef MJS_GC_STRESS_TEST
    L"100" // Long enough when stress testing
#else
    L"10000"
#endif
LR"(; ++i) {
    n = new Object();
    n.prev = l;
    l = n;
}
null
)", value::null);
}

#define EX_EQUAL(expected, actual) do { const auto _e = (expected); const auto _a = (actual); if (_e != _a) { std::ostringstream _woss; _woss << "Expected\n\"" << _e << "\" got\n\"" << _a << "\"\n"; THROW_RUNTIME_ERROR(_woss.str()); } } while (0)

void test_eval_exception() {
    EX_EQUAL("ReferenceError: not_callable is not defined\ntest:1:2-1:18", expect_eval_exception(LR"( not_callable(); )"));
    EX_EQUAL("TypeError: 42 is not a function\ntest:2:16-2:21\ntest:3:19-3:22\ntest:4:17-4:20\ntest:5:40-5:43", expect_eval_exception(LR"( x = 42;
function a() { x(); }
   function b() { a(); }
 function c() { b(); }
                                       c();
)"));

    EX_EQUAL("ReferenceError: bar is not defined\ntest:2:17-2:52\ntest:2:32-2:39\ntest:2:32-2:39\ntest:3:1-3:5", expect_eval_exception(LR"(
function f(i) { return i > 2 ? f(i-1) : new bar(); }
f(4);
)"));

    EX_EQUAL("TypeError: Function is not constructable\ntest:1:1-1:18", expect_eval_exception(L"new parseInt(42);"));

    EX_EQUAL("SyntaxError: Illegal return statement\neval:1:1-1:11\ntest:1:1-1:19", expect_eval_exception(L"eval('return 42;');"));
    EX_EQUAL("SyntaxError: Illegal break statement\neval:1:1-1:7\ntest:1:1-1:15", expect_eval_exception(L"eval('break;');"));
    EX_EQUAL("SyntaxError: Illegal continue statement\neval:1:1-1:10\ntest:1:1-1:18", expect_eval_exception(L"eval('continue;');"));
}

void test_es3_statements() {
    gc_heap h{8192};

    //
    // do..while
    //
    RUN_TEST(L"s=''; i=1;do{s+=i;}while(++i<3); s", value{string{h,"12"}});
    RUN_TEST(L"s=''; i=1;do{s+=i;break;}while(++i<3); s", value{string{h,"1"}});
    RUN_TEST(L"s=''; i=1;do{if(i==1)continue;s+=i;}while(++i<3); s", value{string{h,"2"}});

    //
    // switch
    //
    RUN_TEST(L"switch(1){}", value::undefined);
    RUN_TEST(L"switch(1){case 0:x=42;break;case 1:x=12;break;} x", value{12.});

    RUN_TEST_SPEC(R"(
s='';
function c(x) { s+=x; return x; }
switch (42) {
    case c('42'): break;
    case c(40+1): break;
    case c(42): s+='Hit!';break;
    default:
        s+='xx';
}
s;//$string '424142Hit!'
)");
    RUN_TEST_SPEC(R"(
s = '';
switch(1+2) {
case 2.0:break;
case 3.0: s+='a';
default:
    s+='b';
    s+='c';
}
s;//$string 'abc'
)");
    RUN_TEST_SPEC(R"(
function f(y) {
    s='';
    function t(x) { s+=x; }
    function c(x) { t('c'+x); return x; };
    switch (y) {
        case c(1): t('s1');
        case c(2): t('s2'); break;
        case c(3): t('s3');
        default: t('def');
        case c(4): t('s4');
        case c(5): t('s5'); break;
        case c(6): t('s6');
    }
}

f(0); s //$ string 'c1c2c3c4c5c6defs4s5'
f(1); s //$ string 'c1s1s2'
f(2); s //$ string 'c1c2s2'
f(3); s //$ string 'c1c2c3s3defs4s5'
f(4); s //$ string 'c1c2c3c4s4s5'
f(5); s //$ string 'c1c2c3c4c5s5'
f(6); s //$ string 'c1c2c3c4c5c6s6'
f(7); s //$ string 'c1c2c3c4c5c6defs4s5'

)");


    //
    // Laballed statements / break+continue to identifer
    //
    RUN_TEST(L"x:42", value{42.});

    expect_eval_exception(L"a:a:2;"); // duplicate label
    expect_eval_exception(L"a:2;while(1){break a;}"); // invalid label reference
    expect_eval_exception(L"function f(){break a;} a:f();"); // invalid label reference

    RUN_TEST_SPEC(R"(
s='';
a: x: for (i=0;i<3;++i){ s+=i; break a; }
s; //$string '0'
)");
    RUN_TEST_SPEC(R"(
s='';
a: for (i=0;i<3;++i){ b:for(j=0;j<4;++j){s+=i+'-'+j; break a;} }
s; //$string '0-0'
)");
    RUN_TEST_SPEC(R"(
s='';
a: for (i=0;i<3;++i){ b:for(j=0;j<4;++j){s+=i+'-'+j; break b;} }
s; //$string '0-01-02-0'
)");
    RUN_TEST_SPEC(R"(
s='';
a: for (i=0;i<3;++i){ s+=i; continue a; }
s; //$string '012'
)");
    RUN_TEST_SPEC(R"(
s='';
a: for (i=0;i<3;++i){ b:for(j=0;j<4;++j){s+=i+'-'+j; continue a;} }
s; //$string '0-01-02-0'
)");
    RUN_TEST_SPEC(R"(

s='';
a: for (i=0;i<3;++i){ b:for(j=0;j<4;++j){s+=i+'-'+j; continue b;} }
s; //$string '0-00-10-20-31-01-11-21-32-02-12-22-3'
)");

    //
    // Function expressions
    //

    RUN_TEST(L"a=function(){return 42;}; a();", value{42.0});
    RUN_TEST(L"a=function x(i){return i?i*x(i-1):1;}; a(4);", value{24.});


    RUN_TEST_SPEC(R"(
i=0; a:while(1){ switch(4) { case function(){return i++;}(): break a; } }
i //$ number 5
)");

    //
    // throw/try/catch/finally
    //
    RUN_TEST_SPEC(R"(
function f() {
    try {
        return 42;
    } catch (e) {
        return 60;
    }
}
f(); //$number 42

try {
    throw 42;
} catch (e) {
    e; //$number 42
}

o = {};
try {
    throw o;
} catch (e) {
    e === o; //$boolean true
}

s = '';
try {
    throw 1;
    s += 'a';
} catch (e) {
    s += 'e:' + e;
} finally {
    s += 'f';
}
s; //$string 'e:1f'

s = '';
try {
    try {
        throw 1;
        s += 'a';
    } catch (e) {
        s += 'e:' + e;
        throw 42;
    } finally {
        s += 'f';
    }
} catch (e2) {
    s += 'e2:' + e2;
} finally {
    s += 'f2';
}
s; //$string 'e:1fe2:42f2'

s = '';
try {
    try {
        throw 1;
        s += 'a';
    } catch (e) {
        s += 'e:' + e;
        throw 42;
    } finally {
        s += 'f';
        throw 43;
        s += 'xx';
    }
} catch (e2) {
    s += 'e2:' + e2;
} finally {
    s += 'f2';
}
s; //$string 'e:1fe2:43f2'
)");

    // ES5.1 12.4 (See Annex D) - `this` should be undefined in the function?
    RUN_TEST(L"x=13; g=0; try { throw function() { this.x=42; }; } catch (f) { f(); g=x; } g", value{42.});
    RUN_TEST(L"(function() { try { throw function() { return !('a' in this); }; } catch(e) { var a = true; return e(); } })()", value{true});
}

void test_error_object() {
    if (tested_version() == version::es1) {
        expect_eval_exception(L"new Error();");
        return;
    }

    RUN_TEST_SPEC(R"(
Error.prototype.constructor.length; //$number 1
e1 = Error.prototype.constructor(42);
e1 instanceof Error;
s=''; for (k in e1) { s+=k+','; }; s; //$string ''
e1.message; //$string '42'
e1.name; //$ string 'Error'
e1.message=false;
e1.name=true;
e1.message; //$boolean false
e1.name; //$boolean true
e1 instanceof Error; //$boolean true

se = new SyntaxError('test');
se.name;    //$string 'SyntaxError'
se.message; //$string 'test'
se.toString(); //$string 'SyntaxError: test'
SyntaxError.name = 'BlAh';
se.toString(); //$string 'SyntaxError: test'
se.name = 'Foo';
se.toString(); //$string 'Foo: test'
se instanceof SyntaxError; //$boolean true
)");
    RUN_TEST_SPEC(R"(
EvalError.prototype == Error.prototype; //$boolean false
RangeError.prototype == Error.prototype; //$boolean false
ReferenceError.prototype == Error.prototype; //$boolean false
SyntaxError.prototype == Error.prototype; //$boolean false
TypeError.prototype == Error.prototype; //$boolean false
URIError.prototype == Error.prototype; //$boolean false

new Error() instanceof Error; //$boolean true

EvalError.prototype instanceof Error; //$boolean true
EvalError.prototype.name;//$string 'EvalError'
EvalError.prototype.constructor.length;//$number 1
EvalError.prototype.constructor(2).toString();//$string 'EvalError: 2'
new EvalError() instanceof Error; //$boolean true
new EvalError() instanceof EvalError; //$boolean true

'SyntaxError' in global; //$boolean true
old_se = SyntaxError;
SyntaxError = 42;
SyntaxError; //$number 42
delete SyntaxError;
'SyntaxError' in global; //$boolean false

f=null; try { eval(','); } catch (e) { f=e; }
f.name;//$string 'SyntaxError'
f instanceof old_se;
SyntaxError = old_se;

// ES3, 11.8.6, 11.8.7
try { [] instanceof 42; } catch (e) { e.toString(); } //$string 'TypeError: number is not an object'
try { [] in 'x'; } catch (e) { e.toString(); } //$string 'TypeError: string is not an object'

// ES5.1, 15.11.2.1,15.11.4.3 message is the empty string when no argument is given to the error constructor
new Error().message; //$string ''
new Error('xx').message; //$string 'xx'
Error.prototype.message; //$string ''
Error.prototype.message='test';
new Error().message; //$string 'test'
new Error(undefined).message; //$string 'test'
new Error('yy').message; //$string 'yy'

)");

    RUN_TEST_SPEC(R"(
x = eval;
x(60); //$number 60
eval=42;
eval;//$number 42
)");

    if (tested_version() >= version::es3) {
        RUN_TEST_SPEC(R"(
try { eval("*"); } catch(e) { e.toString(); } //$string 'SyntaxError: Invalid argument to eval'
try { eval('/') } catch(e) { e instanceof SyntaxError; } //$boolean true
)");
    }

    //
    // ReferenceError
    //
    // ES3, 8.7.1 and 8.7.2
    RUN_TEST_SPEC(R"(
// 8.7.1
try {
    a.b;
} catch (re) {
    re.name;    //$string 'ReferenceError'
    re.message; //$string 'a is not defined'
    re instanceof Error; //$boolean true
    re.toString(); //$string 'ReferenceError: a is not defined'
}

// 8.7.2
try {
    ++42;
} catch (re) {
    re.name;    //$string 'ReferenceError'
    re.message; //$string '42 is not a reference'
    re.toString(); //$string 'ReferenceError: 42 is not a reference'
}

)");

    if (tested_version() >= version::es5) {
        // ES5.1, 8.72 and  11.13.1
        RUN_TEST_SPEC(R"(
'use strict';
try { a = 42; } catch (e) { e.toString(); } //$string 'ReferenceError: a is not defined'
try { b ^= 'x'; } catch (e) { e.toString(); } //$string 'ReferenceError: b is not defined'
try { c++; } catch (e) { e.toString(); } //$string 'ReferenceError: c is not defined'
try { --d; } catch (e) { e.toString(); } //$string 'ReferenceError: d is not defined'
try { e >>>= 42; } catch (e) { e.toString(); } //$string 'ReferenceError: e is not defined'
try { f /=2 } catch (e) { e.toString(); } //$string 'ReferenceError: f is not defined'
try { 'x' in g } catch (e) { e.toString(); } //$string 'ReferenceError: g is not defined'
try { h() } catch (e) { e.toString(); } //$string 'ReferenceError: h is not defined'
try { new i() } catch (e) { e.toString(); } //$string 'ReferenceError: i is not defined'
try { var o=Object.defineProperty({}, 'x',{value:40,configurable:true,enumerable:true}); o.x=42; } catch (e) { e.toString(); } //$string 'TypeError: Cannot assign to read only property x in strict mode'
try { var o={get a(){}}; o.a=42; } catch (e) { e.toString(); } //$string 'TypeError: Cannot assign to read only property a in strict mode'
try { Object.prototype=42; } catch (e) { e.toString(); } //$string 'TypeError: Cannot assign to read only property prototype in strict mode'
try { (function f(){f=1;})(); } catch (e) { e.toString(); } //$string 'TypeError: Cannot assign to read only property f in strict mode'
try { var o=Object.preventExtensions({}); o.a=42;} catch (e) { e.toString(); } //$string 'TypeError: Cannot add property a to non-extensible object in strict mode'
)");
    }
}

void test_boolean_object() {
    gc_heap h{256};

    //
    // Boolean
    //
    // ES3, 15.6.4
    RUN_TEST(L"Boolean()", value{false});
    RUN_TEST(L"Boolean(true)", value{true});
    RUN_TEST(L"Boolean(42)", value{true});
    RUN_TEST(L"Boolean(0)", value{false});
    RUN_TEST(L"Boolean('')", value{false});
    RUN_TEST(L"Boolean('x')", value{true});
    RUN_TEST(L"new Boolean('x').valueOf()", value{true});
    RUN_TEST(L"0 + new Boolean()", value{0.});
    RUN_TEST(L"0 + new Boolean(1)", value{1.});
    RUN_TEST(L"'' + new Boolean(0)", value{string{h, "false"}});
    RUN_TEST(L"'' + new Boolean(1)", value{string{h, "true"}});
    RUN_TEST(L"Object(true).toString()", value{string{h, "true"}});

    if (tested_version() < version::es3) {
        return;
    }

    // ES3, 15.6.4.2, 15.6.4.3
    RUN_TEST_SPEC(R"(
try { Boolean.prototype.toString.call(42); } catch (e) { e.toString(); } //$string 'TypeError: Number is not a Boolean'
try { Boolean.prototype.valueOf.call({}); } catch (e) { e.toString(); } //$string 'TypeError: Object is not a Boolean'
)");
}

void test_number_object() {
    gc_heap h{8192};

    // Number
    RUN_TEST(L"Number()", value{0.});
    RUN_TEST(L"Number(42.42)", value{42.42});
    RUN_TEST(L"Number.MIN_VALUE", value{5e-324});
    RUN_TEST(L"Number('1.2')", value{1.2});
    RUN_TEST(L"Number('1,2')", value{NAN});
    RUN_TEST(L"new Number(42.42).toString()", value{string{h, "42.42"}});
    RUN_TEST(L"''+new Number(60)", value{string{h, "60"}});
    RUN_TEST(L"new Number(123).valueOf()", value{123.0});
    RUN_TEST(L"Object(42).toString(10)", value{string{h, "42"}});
    RUN_TEST_SPEC(R"(
(NaN).toString(2);                  //$string 'NaN'
(-Infinity).toString(2);            //$string '-Infinity'
(0).toString(2);                    //$string '0'
(12345678).toString(2);             //$string '101111000110000101001110'
(12345678).toString(3);             //$string '212020020002100'
(12345678).toString(4);             //$string '233012011032'
(42.252).toString(2);               //$string '101010.01000000100000110001001001101110100101111000111'
(42.252).toString(8);               //$string '52.2010142233513616'
(42.252).toString(10);              //$string '42.252'
(42.252).toString(16);              //$string '2a.4083126e978e'
(42.252).toString(32);              //$string '1a.821h4rknho'
(42.252).toString(36);              //$string '16.92lb8co6x2vznswircxlayvi'
(1e100).toString(16);               //$string '1249ad2594c37d0000000000000000000000000000000000000000000000000000000000000000000000'
(123456789e-12).toString(16);       //$string '0.00081742df088c208'
(-612345.12).toString(24);          //$string '-1k729.2l2l2l2kedl'
var s = (-612345.12).toString(5);
s.length;                           //$number 1110
s;                                  //$string '-124043340.0244444444444444121042341102200121104413010402414031133310020314013031132212214013430142344311442401223310424410132031331203132411201030422232434322430131040111042232314023012001224111242113300204014301142314123232032220143221033143142143040013414423202401211001331204102310320223234021320001003444343032324211010242430033132402203024144403200422444400443342011043323404234302341241024042202334444221121113213002010301134044044410234312201321221120033200031100443121330312223214341320001321133234442332143040242224000414020003221133230220440413214400210202103241111120110433222132121203310342434433340223013130244401313213002341020133440344340110203410132012312332322211022014212212040043301131230300340140421240342121432421034043221421444130222244431000434420343344132423210022304144232312240031310400030114211001001341224333003242143444003422132221301034313032442432414231114302242011331410201303443314431203440241314301223412143232141010303210233243320444331143030334340313334441414243214323313023322332221232411103134132304244403204240304042401210100113231113310110433134103343123441330343221211'
)");

    if (tested_version() < version::es3) {
        RUN_TEST_SPEC(R"(
var np = Number.prototype;
np.toLocaleString || np.toFixed || np.toExponential || np.toPrecision; //$undefined
)");
        return;
    }


    RUN_TEST_SPEC(R"(
// ES3, 15.7.4.2
try {
    Number.prototype.toString.call('test');
} catch (e) {
    e.toString(); //$string 'TypeError: String is not a Number'
}

// ES3, 15.7.4.4
try {
    Number.prototype.valueOf.call('test');
} catch (e) {
    e.toString(); //$string 'TypeError: String is not a Number'
}

(42).toLocaleString(); //$string '42'
)");

    // toFixed(), ES3, 15.7.4.5
    RUN_TEST_SPEC(R"(
Number.prototype.toFixed.length; //$number 1

// ES3, 15.7.4.5
try {
    (123).toFixed(21);
} catch (e) {
    e.toString(); //$string 'RangeError: fractionDigits out of range in Number.toFixed()'
}

(NaN).toFixed(); //$string 'NaN'
(0).toFixed(); //$string '0'
(42).toFixed(); //$string '42'
(-42).toFixed(); //$string '-42'

(1000000000000000128).toString(); //$string '1000000000000000100'
(1000000000000000128).toFixed(0); //$string '1000000000000000128'

(123).toFixed(4.999); //$string '123.0000'
)");

    // toExponential(), ES3, 15.7.4.6
    RUN_TEST_SPEC(R"(
Number.prototype.toExponential.length; //$number 1

try {
    (123).toExponential(-1);
} catch (e) {
    e.toString(); //$string 'RangeError: fractionDigits out of range in Number.toExponential()'
}

(NaN).toExponential(); //$string 'NaN'
(-Infinity).toExponential(); //$string '-Infinity'
(0).toExponential(20); //$string '0.00000000000000000000e+0'
(100000).toExponential(); //$string '1e+5'
(100001).toExponential(); //$string '1.00001e+5'
(100000).toExponential(1); //$string '1.0e+5'
(100001).toExponential(1); //$string '1.0e+5'
(1234).toExponential(); //$string '1.234e+3'
(1234).toExponential(1); //$string '1.2e+3'
(1e10).toExponential(15); //$string '1.000000000000000e+10'
(1.12345678e-22).toExponential(20); //$string '1.12345678000000011537e-22'
)");


#if 1
    // toPrecision(), ES3, 15.7.4.7
    RUN_TEST_SPEC(R"(
Number.prototype.toPrecision.length; //$number 1

try {
    (123).toPrecision(0);
} catch (e) {
    e.toString(); //$string 'RangeError: precision out of range in Number.toPrecision()'
}


(NaN).toPrecision(1); //$string 'NaN'
(-Infinity).toPrecision(1); //$string '-Infinity'
(1000000000000000128).toPrecision(); //$string '1000000000000000100'
(0).toPrecision(10); //$string '0.000000000'
(1234).toPrecision(1); //$string '1e+3'
(1234).toPrecision(2); //$string '1.2e+3'
(1234).toPrecision(3); //$string '1.23e+3'
(1234).toPrecision(4); //$string '1234'
(1234).toPrecision(5); //$string '1234.0'
(1234).toPrecision(6); //$string '1234.00'
(-1234.22).toPrecision(6); //$string '-1234.22'
(-1e-4).toPrecision(8); //$string '-0.00010000000'
)");
#endif
}

void test_console() {
    RUN_TEST_SPEC(R"(
console.assert(1);
console.assert(true);//$undefined
)");
    if (tested_version() >= version::es3) {
        RUN_TEST_SPEC(R"(
try {
    console.assert(0);
} catch (e) {
    e.toString(); //$string 'AssertionError: 0 == true'
}

try {
    console.assert(0,1,2);
} catch (e) {
    e.toString(); //$string 'AssertionError: 1 2'
}
)");
    } else {
        try {
            RUN_TEST(L"console.assert(0)", value::undefined);
            REQUIRE(!"Should have thrown");
        } catch (const std::exception&) {
        }
    }
}

void test_main() {
    eval_tests();
    if (tested_version() >= version::es3) {
        test_es3_statements();
    }
    test_boolean_object();
    test_number_object();
    test_global_functions();
    test_math_functions();
    test_error_object();
    test_long_object_chain();
    test_eval_exception();
    test_console();
}