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

template<typename ExceptionType = eval_exception>
std::string expect_exception(const std::wstring_view& text) {
    decltype(parse(nullptr, tested_version())) bs;
    try {
        bs = parse(std::make_shared<source_file>(L"test", text), tested_version());
    } catch (const std::exception& e) {
        std::wcout << "Parse failed for \"" << text << "\": " << e.what() <<  "\n";
        throw;
    }

    gc_heap h{1<<20}; // Use local heap, even if expected lives in another heap
    try {
        interpreter i{h, tested_version(), *bs};
        (void) i.eval_program();
    } catch (const ExceptionType& e) {
        h.garbage_collect();
        if (h.use_percentage()) {
            std::wcout << "Leaks during processing of\n" << text << "\n";
            THROW_RUNTIME_ERROR("Leaks");
        }
        return e.what();
    } catch (const std::exception& e) {
        std::wcout << "Unexpected exception thrown: " << e.what() << " while processing\n" << text << "\n"; 
        throw;
    }

    std::wcout << "Exception not thrown in\n" << text << "\n";
    THROW_RUNTIME_ERROR("Expected exception not thrown");
}

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
    }

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

    // with statement
    RUN_TEST_SPEC(R"(
var a = new Object();
a.x = 42; //$ number 42
with (a) {
    x; //$ number 42
    x = 60;
    y = true;
}
global['x']; //$ undefined
global['y']; //$ boolean true
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

    // Boolean
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
    // TODO: Math
    // TODO: Date

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

    // Test arguments
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
)");

    // parseInt
    RUN_TEST_SPEC(R"(
parseInt('0'); //$ number 0 
parseInt(42); //$ number 42
parseInt('10', 2); //$ number 2
parseInt('123'); //$ number 123
parseInt(' \t123', 10); //$ number 123
parseInt(' 0123'); //$ number 83
parseInt(' 123', 8); //$ number 83
parseInt(' 0x123'); //$ number 291
parseInt(' 123', 16); //$ number 291
parseInt(' 123', 37); //$ number NaN
parseInt(' 123', 1); //$ number NaN
parseInt(' x', 1); //$ number NaN
)");

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

unescape('') //$ string ''
unescape('test') //$ string 'test'
unescape('%28%29') //$ string '()'
unescape('%u1234').charCodeAt(0) //$ number 4660

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
)");
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

void test_date_functions() {
    // TODO: Test local time
    // TODO: Test mutators
    // TODO: Test to*String()
    // TODO: Test NaN Dates

    RUN_TEST_SPEC(R"(
new Date(1234).valueOf(); //$ number 1234
new Date(1234).getTime(); //$ number 1234
var t = Date.UTC(2000,0,1); t //$number 946684800000
var d = new Date(t);
d.valueOf(); //$ number 946684800000
d.getTime(); //$ number 946684800000
var t2 = Date.UTC(2000,0,1,12,34,56,7890); t2 //$ number 946730103890
var d2 = new Date(t2);
d2.getUTCFullYear(); //$ number 2000
d2.getUTCMonth(); //$ number 0
d2.getUTCDate(); //$ number 1
d2.getUTCDay(); //$ number 6
d2.getUTCHours(); //$ number 12
d2.getUTCMinutes(); //$ number 35
d2.getUTCSeconds(); //$ number 3
d2.getUTCMilliseconds(); //$ number 890

var t3 = Date.UTC(2000,0,1,12,34,56,789); t3 //$ number 946730096789
var d3 = new Date(t3);
d3.getUTCFullYear(); //$ number 2000
d3.getUTCMonth(); //$ number 0
d3.getUTCDate(); //$ number 1
d3.getUTCDay(); //$ number 6
d3.getUTCHours(); //$ number 12
d3.getUTCMinutes(); //$ number 34
d3.getUTCSeconds(); //$ number 56
d3.getUTCMilliseconds(); //$ number 789

d3.setTime(5678); d3.getTime() //$ number 5678

//TODO: Use getTimezoneOffset() to create a local time 

)");
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
    EX_EQUAL("ReferenceError: not_callable is not defined\ntest:1:2-1:18", expect_exception<>(LR"( not_callable(); )"));
    EX_EQUAL("TypeError: 42 is not a function\ntest:2:16-2:21\ntest:3:19-3:22\ntest:4:17-4:20\ntest:5:40-5:43", expect_exception<>(LR"( x = 42;
function a() { x(); }
   function b() { a(); }
 function c() { b(); }
                                       c();
)"));

    EX_EQUAL("ReferenceError: bar is not defined\ntest:2:17-2:52\ntest:2:32-2:39\ntest:2:32-2:39\ntest:3:1-3:5", expect_exception<>(LR"(
function f(i) { return i > 2 ? f(i-1) : new bar(); }
f(4);
)"));

    EX_EQUAL("TypeError: Function is not constructable\ntest:1:1-1:18", expect_exception<eval_exception>(L"new parseInt(42);"));

    EX_EQUAL("SyntaxError: Illegal return statement\neval:1:1-1:11\ntest:1:1-1:19", expect_exception<eval_exception>(L"eval('return 42;');"));
    EX_EQUAL("SyntaxError: Illegal break statement\neval:1:1-1:7\ntest:1:1-1:15", expect_exception<eval_exception>(L"eval('break;');"));
    EX_EQUAL("SyntaxError: Illegal continue statement\neval:1:1-1:10\ntest:1:1-1:18", expect_exception<eval_exception>(L"eval('continue;');"));
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

    expect_exception<eval_exception>(L"a:a:2;"); // duplicate label
    expect_exception<eval_exception>(L"a:2;while(1){break a;}"); // invalid label reference
    expect_exception<eval_exception>(L"function f(){break a;} a:f();"); // invalid label reference

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
}

void test_object_object() {
    RUN_TEST_SPEC(R"(
delete Object.prototype; //$boolean false
c = Object.prototype.constructor;
c.length; //$number 1
o = c(); 
o.toString(); //$string '[object Object]'
o.valueOf() == o; //$boolean true
)");

    //
    // ES3
    //

    if (tested_version() < version::es3) {
        return;
    }

    RUN_TEST_SPEC(R"(
Object.prototype.toLocaleString === Object.prototype.toString; //$boolean true

o = {x:42};
'toLocaleString' in o; //$boolean true
'x' in o; //$boolean true
o.hasOwnProperty('toLocaleString'); //$boolean false
o.hasOwnProperty('x'); //$boolean true
o.propertyIsEnumerable('x'); //$boolean true

a = [42];
a.hasOwnProperty('length'); //$boolean true
a.propertyIsEnumerable('length'); //$boolean false
a.hasOwnProperty('prototype'); //$boolean false
a.propertyIsEnumerable('prototype'); //$boolean false
a.hasOwnProperty('0'); //$boolean true
a.hasOwnProperty(0); //$boolean true
a.propertyIsEnumerable(0); //$boolean true
a.hasOwnProperty(); //$ boolean false
a.propertyIsEnumerable();//$ boolean false
a[undefined]=1;
a.hasOwnProperty(); //$ boolean true
a.propertyIsEnumerable();//$ boolean true

global.hasOwnProperty('parseInt'); //$boolean true
global.propertyIsEnumerable('parseInt'); //$boolean false

        )");
}

void test_function_object() {
    gc_heap h{256};
    // In ES3 prototype only has attributes DontDelete, in ES1 it's DontEnum
    const string expected_keys{h, tested_version() < version::es3 ? "" : "prototype,"};
    RUN_TEST(L"s=''; function a(){}; for(k in a) { s+=k+','; }; s", value{expected_keys});
    // length and are arguments should be DontDelete|DontEnum|ReadOnly
    RUN_TEST_SPEC(R"(
    function a(x,y,z){};
    delete a.length; //$boolean false
    a.length;//$number 3
    delete a.arguments; //$boolean false
    a.arguments; //$null
    delete a.prototype.constructor; //$boolean true
    Function.prototype(1,2,3); //$undefined
    Function.toString(); //$string 'function Function() { [native code] }'
    Function.prototype.toString(); //$string 'function () { [native code] }'
)");
    if (tested_version() < version::es3) {
        // In ES1 prototype should be DontEnum only
        RUN_TEST(L"function a(){}; (delete a.prototype)", value{true});
        // Shouldn't have call/apply
        RUN_TEST(L"fp = Function.prototype; fp['call'] || fp['apply']", value::undefined);
        return;
    }

    // In ES3 prototype is DontDelete
    RUN_TEST(L"function a(){}; (delete a.prototype)", value{false});

    RUN_TEST_SPEC(R"(
Function.prototype.apply.length; //$number 2
function f(a,b,c) { return ''+this['x']+a+b+c; }
x=42;
f.apply(); //$string '42undefinedundefinedundefined'
f.apply(null); //$string '42undefinedundefinedundefined'
f.apply({}); //$string 'undefinedundefinedundefinedundefined'
f.apply({x:60},['a','b']); //$string '60abundefined'
f.apply({x:'z'},[1,2,3,4]); //$string 'z123'

String.prototype.charAt.apply('test',[2]); //$string 's'

String.prototype.apply = Function.prototype.apply;
try { ('h').apply(); } catch (e) {
    e.toString(); //$ string 'TypeError: String is not a function'
}
)");

    RUN_TEST_SPEC(R"(
Function.prototype.call.length; //$number 1
function f(a,b,c) { return ''+this['x']+a+b+c; }
x=42;
f.call(); //$string '42undefinedundefinedundefined'
f.call(null); //$string '42undefinedundefinedundefined'
f.call({}); //$string 'undefinedundefinedundefinedundefined'
f.call({x:60},'a','b'); //$string '60abundefined'
f.call({x:'z'},1,2,3,4); //$string 'z123'

String.prototype.charAt.call('test',2); //$string 's'

String.prototype.call = Function.prototype.call;
try { ('h').call(); } catch (e) {
    e.toString(); //$ string 'TypeError: String is not a function'
}
)");
}

void test_regexp_object() {
    if (tested_version() == version::es1) {
        expect_exception<eval_exception>(L"new RegExp();");
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
}

void test_error_object() {
    if (tested_version() == version::es1) {
        expect_exception<eval_exception>(L"new Error();");
        return;
    }

    // TODO: Check that all the correct exception types are thrown (ES3, 15.11.6)

    RUN_TEST_SPEC(R"(
Error.prototype.constructor.length; //$number 1
e1 = Error.prototype.constructor(42);
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
se instanceof Error; //$boolean true

EvalError.prototype == Error.prototype; //$boolean false
RangeError.prototype == Error.prototype; //$boolean false
ReferenceError.prototype == Error.prototype; //$boolean false
SyntaxError.prototype == Error.prototype; //$boolean false
TypeError.prototype == Error.prototype; //$boolean false
URIError.prototype == Error.prototype; //$boolean false

'SyntaxError' in global; //$boolean true
SyntaxError = 42;
SyntaxError; //$number 42
delete SyntaxError;
'SyntaxError' in global; //$boolean false
)");

    //
    // EvalError
    //
    // ES3, 15.1.2.1
    //
    // Use is optional, see ES3, 16:
    //
    // An implementation is not required to detect EvalError.
    // If it chooses not to detect EvalError, the implementation must allow eval
    // to be used indirectly and/or allow assignments to eval.

    RUN_TEST_SPEC(R"(
x = eval;
x(60); //$number 60
eval=42;
eval;//$number 42
)");

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


    // TODO: Check if these are tested elsewhere (some probably are)

    //
    // RangeError
    //
    // ES3, 15.4.2.2, 15.4.5.1, 15.7.4.5, 15.7.4.6, and 15.7.4.7

    //
    // SyntaxError
    //
    // ES3, 15.1.2.1, 15.3.2.1, 15.10.2.5, 15.10.2.9, 15.10.2.15, 15.10.2.19, and 15.10.4.1

    //
    // TypeError
    //
    // ES3, 8.6.2, 8.6.2.6, 9.9, 11.2.2, 11.2.3, 11.8.6, 11.8.7, 15.3.4.2, 15.3.4.3,
    // 15.3.4.4, 15.3.5.3, 15.4.4.2, 15.4.4.3, 15.5.4.2, 15.5.4.3, 15.6.4, 15.6.4.2,
    // 15.6.4.3, 15.7.4, 15.7.4. 2, 15.7.4.4, 15.9.5, 15.9.5.9, 15.9.5.27, 15.10.4.1,
    // and 15.10.6.

    //
    // URIError
    //
    // ES3, 15.1.3

}

void test_array_object() {
    gc_heap h{8192};

    // Array
    RUN_TEST(L"Array.length", value{1.0});
    RUN_TEST(L"Array.prototype.length", value{0.0});
    RUN_TEST(L"new Array().length", value{0.0});
    RUN_TEST(L"new Array(60).length", value{60.0});
    RUN_TEST(L"new Array(1,2,3,4).length", value{4.0});
    RUN_TEST(L"new Array(1,2,3,4)[3]", value{4.0});
    RUN_TEST(L"new Array(1,2,3,4)[4]", value::undefined);
    RUN_TEST(L"a=new Array(); a[5]=42; a.length", value{6.0});
    RUN_TEST(L"a=new Array(); a[5]=42; a[3]=2; a.length", value{6.0});
    RUN_TEST(L"a=new Array(1,2,3,4); a.length=2; a.length", value{2.0});
    RUN_TEST(L"a=new Array(1,2,3,4); a.length=2; a[3]", value::undefined);
    RUN_TEST(L"''+Array()", value{string{h, ""}});
    RUN_TEST(L"a=Array();a[0]=1;''+a", value{string{h, "1"}});
    RUN_TEST(L"''+Array(1,2,3,4)", value{string{h, "1,2,3,4"}});
    RUN_TEST(L"Array(1,2,3,4).join()", value{string{h, "1,2,3,4"}});
    RUN_TEST(L"Array(1,2,3,4).join('')", value{string{h, "1234"}});
    RUN_TEST(L"Array(4).join()", value{string{h, ",,,"}});
    RUN_TEST(L"Array(1,2,3,4).reverse()+''", value{string{h, "4,3,2,1"}});
    RUN_TEST(L"''+Array('March', 'Jan', 'Feb', 'Dec').sort()", value{string{h, "Dec,Feb,Jan,March"}});
    RUN_TEST(L"''+Array(1,30,4,21).sort()", value{string{h, "1,21,30,4"}});
    RUN_TEST(L"function c(x,y) { return x-y; }; ''+Array(1,30,4,21).sort(c)", value{string{h, "1,4,21,30"}});
    RUN_TEST(L"new Array(1).toString()", value{string{h, ""}});
    RUN_TEST(L"new Array(1,2).toString()", value{string{h, "1,2"}});
    RUN_TEST(L"+new Array(1)", value{0.});
    RUN_TEST(L"+new Array(1,2)", value{NAN});
    // Make sure we handle "large" arrays
    RUN_TEST(L"a=new Array(500);for (var i=0; i<a.length; ++i) a[i] = i; sum=0; for (var i=0; i<a.length; ++i) sum += a[i]; sum", value{499*500/2.});

    RUN_TEST_SPEC(R"(
var a = new Array();
a[false] = 0;
a.length; //$number 0
a[2] = 42;
a.length; //$number 3
a[7] = 100;
a.length; //$number 8
var s = ''; for (var k in a) s += k + ','; s //$string 'false,2,7,'
a.length=4;
s = ''; for (var k in a) s += k + ','; s //$string 'false,2,'
delete a[2];
a.length; //$number 4
s = ''; for (var k in a) s += k + ','; s //$string 'false,'

a = new Array(); a['10']=1; a.length //$number 11

a = new Array(); a[1e3]=1; a.length //$number 1001
s = ''; for (var k in a) s += k + ','; s //$string '1000,'

a = new Array(); a['1e3'] = 42; a.length //$number 0
s = ''; for (var k in a) s += k + ','; s //$string '1e3,'

a = new Array(); a[4294967296]=1; a.length //$number 0

 s = ''; for (var k in a) s += k + ','; s //$string '4294967296,'
)");

    if (tested_version() < version::es3) {
        RUN_TEST_SPEC(R"(
ap = Array.prototype;
ap['toLocaleString'] || ap['concat'] || ap['push'] || ap['pop'] || ap['shift'] || ap['unshift'] || ap['slice'] || ap['splice'] //undefined

// Array.prototype.toString is generic in ES1
function f() { this.length=2; this[0]='x'; this[1]='y'; }
f.prototype.toString = Array.prototype.toString;
''+new f();//$string 'x,y'

// Can assign invalid lengths
var a = new Array();
a.length = 42.5;
a.length; //$number 42
        )");
        return;
    }

    RUN_TEST_SPEC(R"(
function f() { this.length=2; this[0]='x'; this[1]='y'; }
f.prototype.toString = Array.prototype.toString;
try { ''+new f(); } catch (e) {
    e.toString(); //$string 'TypeError: f is not an array'
}
[].toLocaleString(); //$string ''
function t(x) { this.x = x; }
t.prototype.toLocaleString = function() { return this.x; };
[new t(42)].toLocaleString(); //$string '42'
a=[new t(1), new t(2), undefined, new t(3), null]; a[null]=new t(4); a.toLocaleString(); //$string '1,2,,3,'
)");

    RUN_TEST_SPEC(R"(
Array.prototype.pop.length;//$number 0
[].pop(); //$undefined
a=[1,2,'x'];
a.pop(); //$string 'x'
a.join(); //$string '1,2'

o = {length:2, 1:42};
Array.prototype.pop.call(o); //$number 42
o.length; //$number 1
'1' in o; //$boolean false

o={length:'test'};
Array.prototype.pop.apply(o);
o.length;//$number 0
)");

    RUN_TEST_SPEC(R"(
Array.prototype.push.length;//$number 1
a=[];
a.push();
a.length;//$number 0
a.push(1,2,3,4);
a.join();//$string '1,2,3,4'

o={length:42};
o.push = Array.prototype.push;
o.push('x','y','z'); //$number 45
o.length;//$number 45
o[42];//$string 'x'
o[43];//$string 'y'
o[44];//$string 'z'
0 in o;//$boolean false
 Array.prototype.join.call(o); //$string ',,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,x,y,z'
)");

    RUN_TEST_SPEC(R"(
Array.prototype.shift.length;//$number 0
a=[];
a.shift();//$undefined
a.length;//$number 0

a=[1,2,3];
a.shift();//$number 1
a.join();//$string '2,3'

o={ 3:100, 4: 101, 6: 101, length: 7 };
Array.prototype.shift.call(o); //$undefined
o.length;//$ number 6
Array.prototype.join.call(o); //$string ',,100,101,,101'

o2={length:'x'}
Array.prototype.shift.call(o2); //$undefined
o2.length;//$number 0
)");

    RUN_TEST_SPEC(R"(
Array.prototype.unshift.length;//$number 1
a=[];
a.unshift();//$number 0
a.length;//$number 0

a=[1,2,3];
a.unshift(10,11,12);//$number 6
a.join();//$string '10,11,12,1,2,3'

o = {length:3,0:'a',2:'c',3:'d',4:'e'};
var s = ''; for (k in o) { s += k+':'+o[k]+','; }; s; // $string '0:a,2:c,3:d,4:e,length:3,'
Array.prototype.unshift.call(o, 1); //$number 4
s = ''; for (k in o) { s += k+':'+o[k]+','; }; s; // $string '0:1,1:a,3:c,4:e,length:4'
)");

    RUN_TEST_SPEC(R"(
Array.prototype.slice.length;//$number 2
a=[0,1,2,3,4];
a.slice().toString(); //$string '0,1,2,3,4'
a.slice(2).toString(); //$string '2,3,4'
a.slice(1,-1).toString(); //$string '1,2,3'
a.slice(-2,-1).toString(); //$string '3'
a.slice(-1,-2).toString(); //$string ''

o = {length:3,0:'a',2:'c',3:'d',4:'e'};
var s = ''; for (k in o) { s += k+':'+o[k]+','; }; s; // $string '0:a,2:c,3:d,4:e,length:3,'
res = Array.prototype.slice.call(o, 1, 4);
var s = ''; for (k in o) { s += k+':'+o[k]+','; }; s; // $string '0:a,2:c,3:d,4:e,length:3,'
res instanceof Array; //$boolean true
res.length; //$number 2
res[0]; //$undefined
res[1]; //$string 'c'
)");

    RUN_TEST_SPEC(R"(
Array.prototype.splice.length;//$number 2

var a = [0,1,2,3,4];
var r = a.splice(0,0);
r.length; //$number 0
r instanceof Array; //$boolean true
r.toString(); //$string ''
a.toString(); //$string '0,1,2,3,4'

var a = [0,1,2,3,4];
a.splice(1,-1).toString(); //$string ''
a.toString(); //$string '0,1,2,3,4'

var a = [0,1,2,3,4];
a.splice(2,100).toString(); //$string '2,3,4'
a.toString(); //$string '0,1'

var a = [0,1,2,3,4];
a.splice(2,1).toString(); //$string '2'
a.toString(); //$string '0,1,3,4'

var a = [0,1,2,3,4];
a.splice(-2,2).toString(); //$string '3,4'
a.toString(); //$string '0,1,2'

var a = [0,1,2,3,4];
a.splice(3,1,'x','y','z').toString(); //$string '3'
a.toString(); //$string '0,1,2,x,y,z,4'

o={length:2,0:42,1:60,2:123};
Array.prototype.splice.call(o, 1, 1, 'x').toString(); //$string '60'
o.length;//$number 2
o[0];//$number 42
o[1];//$string 'x'
o[2];//$number 123

var a = [0,1,2,3,4];
a.splice(0,1,'x','y').toString();//$string '0'
a.toString();//$string 'x,y,1,2,3,4'

// Extensions (specified in ES2015)
var a = [0,1,2];
a.splice().toString(); //$string ''
a.toString(); //$string '0,1,2'

var a = [0,1,2];
a.splice(1).toString(); //$string '1,2'
a.toString(); //$string '0'
)");

    // Check that `RangeError` is thrown (15.4.2.2, 15.4.5.1)
    RUN_TEST_SPEC(R"(

// ES3, 15.4.2.2
try {
    new Array(42.5);
} catch (e) {
    e.toString(); //$string 'RangeError: Invalid array length'
}

// ES3, 15.4.5.1
try {
    a = [1,2,3];
    a.length=0.5;
} catch (e) {
    e.toString(); //$string 'RangeError: Invalid array length'
}

)");
}

void test_string_object() {
    gc_heap h{8192};

    // String
    RUN_TEST(L"String()", value{string{h, ""}});
    RUN_TEST(L"String('test')", value{string{h, "test"}});
    RUN_TEST(L"String('test').length", value{4.0});
    RUN_TEST(L"s = new String('test'); s.length = 42; s.length", value{4.0});
    RUN_TEST(L"''+new String()", value{string{h, ""}});
    RUN_TEST(L"''+new String('test')", value{string{h, "test"}});
    RUN_TEST(L"Object('testXX').valueOf()", value{string{h, "testXX"}});
    RUN_TEST(L"String.fromCharCode()", value{string{h, ""}});
    RUN_TEST(L"String.fromCharCode(65,66,67+32)", value{string{h, "ABc"}});
    RUN_TEST_SPEC(R"(
'test'.charAt(0); //$string 't'
'test'.charAt(2); //$string 's'
'test'.charAt(5); //$string ''
'test'.charAt(-1); //$string ''

o = new Object();
o.charAt = String.prototype.charAt;
o.charAt(1); //$string 'o'

)");
    RUN_TEST_SPEC(R"(
'test'.charCodeAt(0); //$number 116
'test'.charCodeAt(2); //$number 115
'test'.charCodeAt(5); //$number NaN
'test'.charCodeAt(-1); //$number NaN

o = new Object();
o.charCodeAt = String.prototype.charCodeAt;
o.charCodeAt(0); //$number 91
o.charCodeAt(-1); //$number NaN
)");

    RUN_TEST(L"''.indexOf()", value{-1.});
    RUN_TEST(L"'11 undefined'.indexOf()", value{3.});
    RUN_TEST(L"'testfesthest'.indexOf('XX')", value{-1.});
    RUN_TEST(L"'testfesthest'.indexOf('est')", value{1.});
    RUN_TEST(L"'testfesthest'.indexOf('est',3)", value{5.});
    RUN_TEST(L"'testfesthest'.indexOf('est',7)", value{9.});
    RUN_TEST(L"'testfesthest'.indexOf('est',11)", value{-1.});
    RUN_TEST(L"'testfesthest'.lastIndexOf('estX')", value{-1.});
    RUN_TEST(L"'testfesthest'.lastIndexOf('est')", value{9.});
    RUN_TEST(L"'testfesthest'.lastIndexOf('est',1)", value{1.});
    RUN_TEST(L"'testfesthest'.lastIndexOf('est',3)", value{1.});
    RUN_TEST(L"'testfesthest'.lastIndexOf('est',7)", value{5.});
    RUN_TEST(L"'testfesthest'.lastIndexOf('est', 22)", value{9.});
    RUN_TEST(L"''.split()+''", value{string{h, ""}});
    RUN_TEST(L"'1 2 3'.split()+''", value{string{h, "1 2 3"}});
    RUN_TEST(L"'abcd'.split('')+''", value{string{h, "a,b,c,d"}});
    RUN_TEST(L"'1 2 3'.split('not found')+''", value{string{h, "1 2 3"}});
    RUN_TEST(L"'1 2 3'.split(' ')+''", value{string{h, "1,2,3"}});
    RUN_TEST(L"'foo bar'.substring()", value{string{h, "foo bar"}});
    RUN_TEST(L"'foo bar'.substring(-1)", value{string{h, "foo bar"}});
    RUN_TEST(L"'foo bar'.substring(42)", value{string{h, ""}});
    RUN_TEST(L"'foo bar'.substring(3)", value{string{h, " bar"}});
    RUN_TEST(L"'foo bar'.substring(0, 1)", value{string{h, "f"}});
    RUN_TEST(L"'foo bar'.substring(1, 0)", value{string{h, "f"}});
    RUN_TEST(L"'foo bar'.substring(1000, -1)", value{string{h, "foo bar"}});
    RUN_TEST(L"'foo bar'.substring(1, 4)", value{string{h, "oo "}});
    RUN_TEST(L"'ABc'.toLowerCase()", value{string{h, "abc"}});
    RUN_TEST(L"'ABc'.toUpperCase()", value{string{h, "ABC"}});

    if (tested_version() < version::es3) {
        RUN_TEST_SPEC(R"(
var sp = String.prototype;
sp.concat || sp.localeCompare || sp.match || sp.replace || sp.search || sp.slice || sp.toLocaleLowerCase || sp.toLocaleUpperCase;//$undefined
)");
        return;
    }


    // *locale*
    RUN_TEST_SPEC(R"(
'abc'.localeCompare('abc');//$number 0
'abc'.localeCompare('Abc');//$number -1
'Abc'.localeCompare('abc');//$number 1
 'undefined'.localeCompare();//$number 0

'ABc'.toLocaleLowerCase();//$string 'abc'
'ABc'.toLocaleUpperCase();//$string 'ABC'
)");

    // concat
    RUN_TEST_SPEC(R"(
String.prototype.concat.length;//$number 1
''.concat(); //$string ''
'x'.concat('y'); //$string 'xy'
String.prototype.concat.call(42,43); //$string '4243'
)");

    // slice
    RUN_TEST_SPEC(R"(
String.prototype.slice.length;//$number 2
'012345'.slice(); //$string '012345'
'012345'.slice(1); //$string '12345'
'012345'.slice(1,3); //$string '12'
'012345'.slice(-3,-1); //$string '34'
String.prototype.slice.call(12345,-3,7); //$string '345'
)");

#if 0
    // match
    RUN_TEST_SPEC(R"(
String.prototype.match.length;//$number 1
)");
    // replace
    RUN_TEST_SPEC(R"(
String.prototype.replace.length;//$number 1
)");
    // search
    RUN_TEST_SPEC(R"(
String.prototype.search.length;//$number 1
)");
#endif
}

int main() {
    try {
        //test_string_object(); std::wcout << "TODO: Remove from " << __FILE__ << ":" << __LINE__ << "\n";

        for (const auto ver: supported_versions) {
            tested_version(ver);

            eval_tests();
            if (tested_version() >= version::es3) {
                test_es3_statements();
            }
            test_object_object();
            test_function_object();
            test_array_object();
            test_string_object();
            test_global_functions();
            test_math_functions();
            test_date_functions();
            test_regexp_object();
            test_error_object();
            test_long_object_chain();
            test_eval_exception();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        std::cerr << "Version tested: " << tested_version() << "\n";
        return 1;
    }

}

