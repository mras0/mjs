#include <iostream>
#include <cmath>
#include <cstring>
#include <sstream>

#include <mjs/interpreter.h>
#include <mjs/parser.h>
#include <mjs/printer.h>
#include <mjs/object.h>

#include "test_spec.h"
#include "test.h"

using namespace mjs;

template<typename ExceptionType = eval_exception>
std::string expect_exception(const std::wstring_view& text) {
    decltype(parse(nullptr)) bs;
    try {
        bs = parse(std::make_shared<source_file>(L"test", text));
    } catch (const std::exception& e) {
        std::wcout << "Parse failed for \"" << text << "\": " << e.what() <<  "\n";
        throw;
    }

    gc_heap h{1<<20}; // Use local heap, even if expected lives in another heap
    try {
        interpreter i{h, *bs};
        i.eval(*bs);
    } catch (const ExceptionType& e) {
        h.garbage_collect();
        if (h.calc_used()) {
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

    run_test(L"undefined", value::undefined);
    run_test(L"null", value::null);
    run_test(L"false", value{false});
    run_test(L"true", value{true});
    run_test(L"123", value{123.});
    run_test(L"0123", value{83.});
    run_test(L"0x123", value{291.});
    run_test(L"'te\"st'", value{string{h, "te\"st"}});
    run_test(L"\"te'st\"", value{string{h, "te'st"}});
    run_test(L"/*42*/60", value{60.0});
    run_test(L"x=12//\n+34;x", value{46.0});
    run_test(L"-7.5 % 2", value{-1.5});
    run_test(L"1+2*3", value{7.});
    run_test(L"''+(0.1+0.2)", value{string{h, "0.30000000000000004"}});
    run_test(L"x = 42; 'test ' + 2 * (6 - 4 + 1) + ' ' + x", value{string{h, "test 6 42"}});
    run_test(L"y=1/2; z='string'; y+z", value{string{h, "0.5string"}});
    run_test(L"var x=2; x++;", value{2.0});
    run_test(L"var x=2; x++; x", value{3.0});
    run_test(L"var x=2; x--;", value{2.0});
    run_test(L"var x=2; x--; x", value{1.0});
    run_test(L"var x = 42; delete x; x", value::undefined);
    run_test(L"void(2+2)", value::undefined);
    run_test(L"typeof(2)", value{string{h, "number"}});
    run_test(L"x=4.5; ++x", value{5.5});
    run_test(L"x=4.5; --x", value{3.5});
    run_test(L"x=42; +x;", value{42.0});
    run_test(L"x=42; -x;", value{-42.0});
    run_test(L"x=42; !x;", value{false});
    run_test(L"x=42; ~x;", value{(double)~42});
    run_test(L"1<<2", value{4.0});
    run_test(L"-5>>2", value{-2.0});
    run_test(L"-5>>>2", value{1073741822.0});
    run_test(L"1 < 2", value{true});
    run_test(L"1 > 2", value{false});
    run_test(L"1 <= 2", value{true});
    run_test(L"1 >= 2", value{false});
    run_test(L"1 == 2", value{false});
    run_test(L"1 != 2", value{true});
    run_test(L"255 & 128", value{128.0});
    run_test(L"255 ^ 128", value{127.0});
    run_test(L"64 | 128", value{192.0});
    run_test(L"42 || 13", value{42.0});
    run_test(L"42 && 13", value{13.0});
    run_test(L"1 ? 2 : 3", value{2.0});
    run_test(L"0 ? 2 : 1+2", value{3.0});
    run_test(L"x=2.5; x+=4; x", value{6.5});
    run_test(L"function f(x,y) { return x*x+y; } f(2, 3)", value{7.0});
    run_test(L"function f(){ i = 42; }; f(); i", value{42.0});
    run_test(L"i = 1; function f(){ var i = 42; }; f(); i", value{1.0});
    run_test(L"x=42; function f(x) { return x; } f()", value::undefined);
    run_test(L";", value::undefined);
    run_test(L"if (1) 2;", value{2.0});
    run_test(L"if (0) 2;", value::undefined);
    run_test(L"if (0) 2; else ;", value::undefined);
    run_test(L"if (0) 2; else 3;", value{3.0});
    run_test(L"1,2", value{2.0});
    run_test(L"x=5; while(x-3) { x = x - 1; } x", value{3.0});
    run_test(L"x=2; y=0; while(1) { if(x) {x = x - 1; y = y + 2; continue; y = y + 1000; } else break; y = y + 1;} y", value{4.0});
    run_test(L"var x = 0; for(var i = 10, dec = 1; i; i = i - dec) x = x + i; x", value{55.0});
    run_test(L"var x=0; for (i=2; i; i=i-1) x=x+i; x+i", value{3.0});
    run_test(L"for (var i=0;!i;) { i=1 }", value{1.});
    
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
k; //$ undefined
var o = new Object(); o.x=42; o.y=60;
r(o); //$ string 'x,y'
k; //$ string 'y'
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
k; //$ undefined
var o = new Object(); o['test test'] = 60; o.prop = 12;
r(o); //$ string 'test test,prop'
k; //$ undefined

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
x; //$ undefined
y; //$ boolean true
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
x; //$ undefined
y; //$ boolean true
a.x; //$ number 123
a.y; //$ number 1

with(42) {
    toString(); //$ string '42'
}
)");


    run_test(L"function sum() {  var s = 0; for (var i = 0; i < arguments.length; ++i) s += arguments[i]; return s; } sum(1,2,3)", value{6.0});
    // Object
    run_test(L"''+Object(null)", value{string{h, "[object Object]"}});
    run_test(L"o=Object(null); o.x=42; o.y=60; o.x+o['y']", value{102.0});
    run_test(L"a=Object(null);b=Object(null);a.x=b;a.x.y=42;a['x']['y']", value{42.0});
    run_test(L"'' + new Object", value{string{h, "[object Object]"}});
    run_test(L"'' + new Object()", value{string{h, "[object Object]"}});
    run_test(L"'' + new Object(null)", value{string{h, "[object Object]"}});
    run_test(L"'' + new Object(undefined)", value{string{h, "[object Object]"}});
    run_test(L"o = new Object;o.x=42; new Object(o).x", value{42.0});

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

    run_test(L"function  f ( x   ,\ny )  { return x + y;  }; f.toString()", value{string{h, "function f( x   ,\ny )  { return x + y;  }"}});
    run_test(L"a=parseInt;a.toString()", value{string{h, "function parseInt() { [native code] }"}});
    // Array
    run_test(L"Array.length", value{1.0});
    run_test(L"Array.prototype.length", value{0.0});
    run_test(L"new Array().length", value{0.0});
    run_test(L"new Array(60).length", value{60.0});
    run_test(L"new Array(1,2,3,4).length", value{4.0});
    run_test(L"new Array(1,2,3,4)[3]", value{4.0});
    run_test(L"new Array(1,2,3,4)[4]", value::undefined);
    run_test(L"a=new Array(); a[5]=42; a.length", value{6.0});
    run_test(L"a=new Array(); a[5]=42; a[3]=2; a.length", value{6.0});
    run_test(L"a=new Array(1,2,3,4); a.length=2; a.length", value{2.0});
    run_test(L"a=new Array(1,2,3,4); a.length=2; a[3]", value::undefined);
    run_test(L"''+Array()", value{string{h, ""}});
    run_test(L"a=Array();a[0]=1;''+a", value{string{h, "1"}});
    run_test(L"''+Array(1,2,3,4)", value{string{h, "1,2,3,4"}});
    run_test(L"Array(1,2,3,4).join()", value{string{h, "1,2,3,4"}});
    run_test(L"Array(1,2,3,4).join('')", value{string{h, "1234"}});
    run_test(L"Array(4).join()", value{string{h, ",,,"}});
    run_test(L"Array(1,2,3,4).reverse()+''", value{string{h, "4,3,2,1"}});
    run_test(L"''+Array('March', 'Jan', 'Feb', 'Dec').sort()", value{string{h, "Dec,Feb,Jan,March"}});
    run_test(L"''+Array(1,30,4,21).sort()", value{string{h, "1,21,30,4"}});
    run_test(L"function c(x,y) { return x-y; }; ''+Array(1,30,4,21).sort(c)", value{string{h, "1,4,21,30"}});
    run_test(L"new Array(1).toString()", value{string{h, ""}});
    run_test(L"new Array(1,2).toString()", value{string{h, "1,2"}});
    run_test(L"+new Array(1)", value{0.});
    run_test(L"+new Array(1,2)", value{NAN});
    // Make sure we handle "large" arrays
    run_test(L"a=new Array(500);for (var i=0; i<a.length; ++i) a[i] = i; sum=0; for (var i=0; i<a.length; ++i) sum += a[i]; sum", value{499*500/2.});

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

    // String
    run_test(L"String()", value{string{h, ""}});
    run_test(L"String('test')", value{string{h, "test"}});
    run_test(L"String('test').length", value{4.0});
    run_test(L"s = new String('test'); s.length = 42; s.length", value{4.0});
    run_test(L"''+new String()", value{string{h, ""}});
    run_test(L"''+new String('test')", value{string{h, "test"}});
    run_test(L"Object('testXX').valueOf()", value{string{h, "testXX"}});
    run_test(L"String.fromCharCode()", value{string{h, ""}});
    run_test(L"String.fromCharCode(65,66,67+32)", value{string{h, "ABc"}});
    run_test(L"'test'.charAt(0)", value{string{h, "t"}});
    run_test(L"'test'.charAt(2)", value{string{h, "s"}});
    run_test(L"'test'.charAt(5)", value{string{h, ""}});
    run_test(L"'test'.charCodeAt(0)", value{(double)'t'});
    run_test(L"'test'.charCodeAt(2)", value{(double)'s'});
    run_test(L"'test'.charCodeAt(5)", value{NAN});
    run_test(L"''.indexOf()", value{-1.});
    run_test(L"'11 undefined'.indexOf()", value{3.});
    run_test(L"'testfesthest'.indexOf('XX')", value{-1.});
    run_test(L"'testfesthest'.indexOf('est')", value{1.});
    run_test(L"'testfesthest'.indexOf('est',3)", value{5.});
    run_test(L"'testfesthest'.indexOf('est',7)", value{9.});
    run_test(L"'testfesthest'.indexOf('est',11)", value{-1.});
    run_test(L"'testfesthest'.lastIndexOf('estX')", value{-1.});
    run_test(L"'testfesthest'.lastIndexOf('est')", value{9.});
    run_test(L"'testfesthest'.lastIndexOf('est',1)", value{1.});
    run_test(L"'testfesthest'.lastIndexOf('est',3)", value{1.});
    run_test(L"'testfesthest'.lastIndexOf('est',7)", value{5.});
    run_test(L"'testfesthest'.lastIndexOf('est', 22)", value{9.});
    run_test(L"''.split()+''", value{string{h, ""}});
    run_test(L"'1 2 3'.split()+''", value{string{h, "1 2 3"}});
    run_test(L"'abcd'.split('')+''", value{string{h, "a,b,c,d"}});
    run_test(L"'1 2 3'.split('not found')+''", value{string{h, "1 2 3"}});
    run_test(L"'1 2 3'.split(' ')+''", value{string{h, "1,2,3"}});
    run_test(L"'foo bar'.substring()", value{string{h, "foo bar"}});
    run_test(L"'foo bar'.substring(-1)", value{string{h, "foo bar"}});
    run_test(L"'foo bar'.substring(42)", value{string{h, ""}});
    run_test(L"'foo bar'.substring(3)", value{string{h, " bar"}});
    run_test(L"'foo bar'.substring(0, 1)", value{string{h, "f"}});
    run_test(L"'foo bar'.substring(1, 0)", value{string{h, "f"}});
    run_test(L"'foo bar'.substring(1000, -1)", value{string{h, "foo bar"}});
    run_test(L"'foo bar'.substring(1, 4)", value{string{h, "oo "}});
    run_test(L"'ABc'.toLowerCase()", value{string{h, "abc"}});
    run_test(L"'ABc'.toUpperCase()", value{string{h, "ABC"}});
    // Boolean
    run_test(L"Boolean()", value{false});
    run_test(L"Boolean(true)", value{true});
    run_test(L"Boolean(42)", value{true});
    run_test(L"Boolean(0)", value{false});
    run_test(L"Boolean('')", value{false});
    run_test(L"Boolean('x')", value{true});
    run_test(L"new Boolean('x').valueOf()", value{true});
    run_test(L"0 + new Boolean()", value{0.});
    run_test(L"0 + new Boolean(1)", value{1.});
    run_test(L"'' + new Boolean(0)", value{string{h, "false"}});
    run_test(L"'' + new Boolean(1)", value{string{h, "true"}});
    run_test(L"Object(true).toString()", value{string{h, "true"}});
    // Number
    run_test(L"Number()", value{0.});
    run_test(L"Number(42.42)", value{42.42});
    run_test(L"Number.MIN_VALUE", value{5e-324});
    run_test(L"Number('1.2')", value{1.2});
    run_test(L"Number('1,2')", value{NAN});
    run_test(L"new Number(42.42).toString()", value{string{h, "42.42"}});
    run_test(L"''+new Number(60)", value{string{h, "60"}});
    run_test(L"new Number(123).valueOf()", value{123.0});
    run_test(L"Object(42).toString(10)", value{string{h, "42"}});
    // TODO: Math
    // TODO: Date

    // wat
    run_test(L"!!('')", value{false});
    run_test(L"\"\" == false", value{true});
    run_test(L"null == false", value{false});
    run_test(L"+true", value{1.0});
    run_test(L"true + true", value{2.0});
    run_test(L"!!('0' && Object(null))", value{true});

    run_test(L"function X() { this.val = 42; }; new X().val", value{42.0});
    run_test(L"function A() { this.n=1; }; function B() { this.n=2;} function g() { return this.n; } A.prototype.foo = g; new A().foo()", value{1.});
    run_test(L"function A() { this.n=1; }; function B() { this.n=2;} function g() { return this.n; } A.prototype.foo = g; new B().foo", value::undefined);
    run_test(L"function f() { this.a = 1; this.b = 2; } var o = new f(); f.prototype.b = 3; f.prototype.c = 4; '' + new Array(o.a,o.b,o.c,o.d)", value{string{h, "1,2,4,"}});
    run_test(L"var o = new Object(); o.a = 2; function m(){return this.a+1; }; o.m = m; o.m()", value{3.});

    run_test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; si.prop", value{string{h, "some value"}});
    run_test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; si.foo", value{string{h, "bar"}});
    run_test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.prop", value::undefined);
    run_test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.foo", value::undefined);
    run_test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.prototype.prop", value::undefined);
    run_test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.prototype.foo", value{string{h, "bar"}});

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
    run_test(LR"(
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
    EX_EQUAL("identifier_expression{not_callable} is not a function\ntest:1:2-1:16", expect_exception<>(LR"( not_callable(); )"));
    EX_EQUAL("identifier_expression{x} is not a function\ntest:2:16-2:19\ntest:3:19-3:22\ntest:4:17-4:20\ntest:5:40-5:43", expect_exception<>(LR"(
function a() { x(); }
   function b() { a(); }
 function c() { b(); }
                                       c();
)"));

    EX_EQUAL("call_expression{identifier_expression{bar}, {}} is not an object\ntest:2:24-2:50\ntest:2:24-2:39\ntest:2:24-2:39\ntest:3:1-3:5", expect_exception<>(LR"(
function f(i) { return i > 2 ? f(i-1) : new bar(); }
f(4);
)"));

    EX_EQUAL("call_expression{identifier_expression{parseInt}, {literal_expression{token{numeric_literal, 42}}}} is not constructable\ntest:1:1-1:17", expect_exception<eval_exception>(L"new parseInt(42);"));
}

int main() {
    try {
        eval_tests();
        test_global_functions();
        test_math_functions();
        test_date_functions();
        test_long_object_chain();
        test_eval_exception();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

}

