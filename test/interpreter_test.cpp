#include <iostream>
#include <cmath>
#include <cstring>
#include <sstream>

#include <mjs/interpreter.h>
#include <mjs/parser.h>
#include <mjs/printer.h>

#include "test_spec.h"

using namespace mjs;

void test(const std::wstring_view& text, const value& expected) {
    decltype(parse(nullptr)) bs;
    try {
        bs = parse(std::make_shared<source_file>(L"test", text));
    } catch (const std::exception& e) {
        std::wcout << "Parse failed for \"" << text << "\": " << e.what() <<  "\n";
        throw;
    }
    auto pb = [&bs]() {
        print(std::wcout, *bs);
        std::wcout << '\n';
        for (const auto& s: bs->l()) {
            std::wcout << *s << "\n";
        }
    };
    try {
        interpreter i{*bs};
        value res{};
        for (const auto& s: bs->l()) {
            res = i.eval(*s).result;
        }
        if (res != expected) {
            std::wcout << "Test failed: " << text << " expecting " << expected.type() << " " << expected << " got " << res.type() << " " << res << "\n";
            pb();
            THROW_RUNTIME_ERROR("Test failed");
        }
    } catch (const std::exception& e) {
        std::wcout << "Test failed: " << text << " uexpected exception thrown: " << e.what() << "\n";
        pb();
        throw;
    }

    object::garbage_collect({});
    if (object::object_count()) {
        std::wcout << "Total number of objects left: " << object::object_count() <<  " while testing '" << text << "'\n";
        THROW_RUNTIME_ERROR("Leaks");
    }
}

void eval_tests() {
    test(L"undefined", value::undefined);
    test(L"null", value::null);
    test(L"false", value{false});
    test(L"true", value{true});
    test(L"123", value{123.});
    test(L"0123", value{83.});
    test(L"0x123", value{291.});
    test(L"'te\"st'", value{string{"te\"st"}});
    test(L"\"te'st\"", value{string{"te'st"}});
    test(L"/*42*/60", value{60.0});
    test(L"x=12//\n+34;x", value{46.0});
    test(L"-7.5 % 2", value{-1.5});
    test(L"1+2*3", value{7.});
    test(L"x = 42; 'test ' + 2 * (6 - 4 + 1) + ' ' + x", value{string{"test 6 42"}});
    test(L"y=1/2; z='string'; y+z", value{string{"0.5string"}});
    test(L"var x=2; x++;", value{2.0});
    test(L"var x=2; x++; x", value{3.0});
    test(L"var x=2; x--;", value{2.0});
    test(L"var x=2; x--; x", value{1.0});
    test(L"var x = 42; delete x; x", value::undefined);
    test(L"void(2+2)", value::undefined);
    test(L"typeof(2)", value{string{"number"}});
    test(L"x=4.5; ++x", value{5.5});
    test(L"x=4.5; --x", value{3.5});
    test(L"x=42; +x;", value{42.0});
    test(L"x=42; -x;", value{-42.0});
    test(L"x=42; !x;", value{false});
    test(L"x=42; ~x;", value{(double)~42});
    test(L"1<<2", value{4.0});
    test(L"-5>>2", value{-2.0});
    test(L"-5>>>2", value{1073741822.0});
    test(L"1 < 2", value{true});
    test(L"1 > 2", value{false});
    test(L"1 <= 2", value{true});
    test(L"1 >= 2", value{false});
    test(L"1 == 2", value{false});
    test(L"1 != 2", value{true});
    test(L"255 & 128", value{128.0});
    test(L"255 ^ 128", value{127.0});
    test(L"64 | 128", value{192.0});
    test(L"42 || 13", value{42.0});
    test(L"42 && 13", value{13.0});
    test(L"1 ? 2 : 3", value{2.0});
    test(L"0 ? 2 : 1+2", value{3.0});
    test(L"x=2.5; x+=4; x", value{6.5});
    test(L"function f(x,y) { return x*x+y; } f(2, 3)", value{7.0});
    test(L"function f(){ i = 42; }; f(); i", value{42.0});
    test(L"i = 1; function f(){ var i = 42; }; f(); i", value{1.0});
    test(L";", value::undefined);
    test(L"if (1) 2;", value{2.0});
    test(L"if (0) 2;", value::undefined);
    test(L"if (0) 2; else ;", value::undefined);
    test(L"if (0) 2; else 3;", value{3.0});
    test(L"1,2", value{2.0});
    test(L"x=5; while(x-3) { x = x - 1; } x", value{3.0});
    test(L"x=2; y=0; while(1) { if(x) {x = x - 1; y = y + 2; continue; y = y + 1000; } else break; y = y + 1;} y", value{4.0});
    test(L"var x = 0; for(var i = 10, dec = 1; i; i = i - dec) x = x + i; x", value{55.0});
    test(L"var x=0; for (i=2; i; i=i-1) x=x+i; x+i", value{3.0});
    test(L"for (var i=0;!i;) { i=1 }", value{1.});
    
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


    test(L"function sum() {  var s = 0; for (var i = 0; i < arguments.length; ++i) s += arguments[i]; return s; } sum(1,2,3)", value{6.0});
    // Object
    test(L"''+Object(null)", value{string{"[object Object]"}});
    test(L"o=Object(null); o.x=42; o.y=60; o.x+o['y']", value{102.0});
    test(L"a=Object(null);b=Object(null);a.x=b;a.x.y=42;a['x']['y']", value{42.0});
    test(L"'' + new Object", value{string{L"[object Object]"}});
    test(L"'' + new Object()", value{string{L"[object Object]"}});
    test(L"'' + new Object(null)", value{string{L"[object Object]"}});
    test(L"'' + new Object(undefined)", value{string{L"[object Object]"}});
    test(L"o = new Object;o.x=42; new Object(o).x", value{42.0});
    // TODO: Function constructor
    test(L"function  f ( x   ,\ny )  { return x + y;  }; f.toString()", value{string{"function f( x   ,\ny )  { return x + y;  }"}});
    test(L"a=parseInt;a.toString()", value{string{"function parseInt() { [native code] }"}});
    // Array
    test(L"Array.length", value{1.0});
    test(L"Array.prototype.length", value{0.0});
    test(L"new Array().length", value{0.0});
    test(L"new Array(60).length", value{60.0});
    test(L"new Array(1,2,3,4).length", value{4.0});
    test(L"new Array(1,2,3,4)[3]", value{4.0});
    test(L"new Array(1,2,3,4)[4]", value::undefined);
    test(L"a=new Array(); a[5]=42; a.length", value{6.0});
    test(L"a=new Array(); a[5]=42; a[3]=2; a.length", value{6.0});
    test(L"a=new Array(1,2,3,4); a.length=2; a.length", value{2.0});
    test(L"a=new Array(1,2,3,4); a.length=2; a[3]", value::undefined);
    test(L"''+Array()", value{string{""}});
    test(L"a=Array();a[0]=1;''+a", value{string{"1"}});
    test(L"''+Array(1,2,3,4)", value{string{"1,2,3,4"}});
    test(L"Array(1,2,3,4).join()", value{string{"1,2,3,4"}});
    test(L"Array(1,2,3,4).join('')", value{string{"1234"}});
    test(L"Array(4).join()", value{string{",,,"}});
    test(L"Array(1,2,3,4).reverse()+''", value{string{"4,3,2,1"}});
    test(L"''+Array('March', 'Jan', 'Feb', 'Dec').sort()", value{string{"Dec,Feb,Jan,March"}});
    test(L"''+Array(1,30,4,21).sort()", value{string{"1,21,30,4"}});
    test(L"function c(x,y) { return x-y; }; ''+Array(1,30,4,21).sort(c)", value{string{"1,4,21,30"}});
    test(L"new Array(1).toString()", value{string{}});
    test(L"new Array(1,2).toString()", value{string{"1,2"}});
    test(L"+new Array(1)", value{0.});
    test(L"+new Array(1,2)", value{NAN});
    // String
    test(L"String()", value{string{""}});
    test(L"String('test')", value{string{"test"}});
    test(L"''+new String()", value{string{""}});
    test(L"''+new String('test')", value{string{"test"}});
    test(L"Object('testXX').valueOf()", value{string{"testXX"}});
    test(L"String.fromCharCode()", value{string{""}});
    test(L"String.fromCharCode(65,66,67+32)", value{string{"ABc"}});
    test(L"'test'.charAt(0)", value{string{"t"}});
    test(L"'test'.charAt(2)", value{string{"s"}});
    test(L"'test'.charAt(5)", value{string{""}});
    test(L"'test'.charCodeAt(0)", value{(double)'t'});
    test(L"'test'.charCodeAt(2)", value{(double)'s'});
    test(L"'test'.charCodeAt(5)", value{NAN});
    test(L"''.indexOf()", value{-1.});
    test(L"'11 undefined'.indexOf()", value{3.});
    test(L"'testfesthest'.indexOf('XX')", value{-1.});
    test(L"'testfesthest'.indexOf('est')", value{1.});
    test(L"'testfesthest'.indexOf('est',3)", value{5.});
    test(L"'testfesthest'.indexOf('est',7)", value{9.});
    test(L"'testfesthest'.indexOf('est',11)", value{-1.});
    test(L"'testfesthest'.lastIndexOf('estX')", value{-1.});
    test(L"'testfesthest'.lastIndexOf('est')", value{9.});
    test(L"'testfesthest'.lastIndexOf('est',1)", value{1.});
    test(L"'testfesthest'.lastIndexOf('est',3)", value{1.});
    test(L"'testfesthest'.lastIndexOf('est',7)", value{5.});
    test(L"'testfesthest'.lastIndexOf('est', 22)", value{9.});
    test(L"''.split()+''", value{string{}});
    test(L"'1 2 3'.split()+''", value{string{"1 2 3"}});
    test(L"'abcd'.split('')+''", value{string{"a,b,c,d"}});
    test(L"'1 2 3'.split('not found')+''", value{string{"1 2 3"}});
    test(L"'1 2 3'.split(' ')+''", value{string{"1,2,3"}});
    test(L"'foo bar'.substring()", value{string{"foo bar"}});
    test(L"'foo bar'.substring(-1)", value{string{"foo bar"}});
    test(L"'foo bar'.substring(42)", value{string{""}});
    test(L"'foo bar'.substring(3)", value{string{" bar"}});
    test(L"'foo bar'.substring(0, 1)", value{string{"f"}});
    test(L"'foo bar'.substring(1, 0)", value{string{"f"}});
    test(L"'foo bar'.substring(1000, -1)", value{string{"foo bar"}});
    test(L"'foo bar'.substring(1, 4)", value{string{"oo "}});
    test(L"'ABc'.toLowerCase()", value{string{"abc"}});
    test(L"'ABc'.toUpperCase()", value{string{"ABC"}});
    // Boolean
    test(L"Boolean()", value{false});
    test(L"Boolean(true)", value{true});
    test(L"Boolean(42)", value{true});
    test(L"Boolean(0)", value{false});
    test(L"Boolean('')", value{false});
    test(L"Boolean('x')", value{true});
    test(L"new Boolean('x').valueOf()", value{true});
    test(L"0 + new Boolean()", value{0.});
    test(L"0 + new Boolean(1)", value{1.});
    test(L"'' + new Boolean(0)", value{string{"false"}});
    test(L"'' + new Boolean(1)", value{string{"true"}});
    test(L"Object(true).toString()", value{string{"true"}});
    // Number
    test(L"Number()", value{0.});
    test(L"Number(42.42)", value{42.42});
    test(L"Number.MIN_VALUE", value{5e-324});
    test(L"Number('1.2')", value{1.2});
    test(L"Number('1,2')", value{NAN});
    test(L"new Number(42.42).toString()", value{string{"42.42"}});
    test(L"''+new Number(60)", value{string{"60"}});
    test(L"new Number(123).valueOf()", value{123.0});
    test(L"Object(42).toString(10)", value{string{"42"}});
    // TODO: Math
    // TODO: Date

    // wat
    test(L"!!('')", value{false});
    test(L"\"\" == false", value{true});
    test(L"null == false", value{false});
    test(L"+true", value{1.0});
    test(L"true + true", value{2.0});
    test(L"!!('0' && Object(null))", value{true});

    test(L"function X() { this.val = 42; }; new X().val", value{42.0});
    test(L"function A() { this.n=1; }; function B() { this.n=2;} function g() { return this.n; } A.prototype.foo = g; new A().foo()", value{1.});
    test(L"function A() { this.n=1; }; function B() { this.n=2;} function g() { return this.n; } A.prototype.foo = g; new B().foo", value::undefined);
    test(L"function f() { this.a = 1; this.b = 2; } var o = new f(); f.prototype.b = 3; f.prototype.c = 4; '' + new Array(o.a,o.b,o.c,o.d)", value{string{"1,2,4,"}});
    test(L"var o = new Object(); o.a = 2; function m(){return this.a+1; }; o.m = m; o.m()", value{3.});

    test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; si.prop", value{string{"some value"}});
    test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; si.foo", value{string{"bar"}});
    test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.prop", value::undefined);
    test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.foo", value::undefined);
    test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.prototype.prop", value::undefined);
    test(L"function s(){} s.prototype.foo = 'bar'; var si = new s(); si.prop = 'some value'; s.prototype.foo", value{string{"bar"}});
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

// TODO: Create seperate parse_test
void test_semicolon_insertion() {
    struct parse_failure {};

    auto test_parse_fails = [](const char* text) {
        try {
            parse(std::make_shared<source_file>(L"test", std::wstring(text, text+std::strlen(text))));
        } catch (const std::exception& e) {
            // std::wcerr << "\n'" << text << "' ---->\n" << e.what() << "\n\n";
            (void)e;
            return;
        }
        throw std::runtime_error(std::string("Unexpected parse success for '") + text + "'");
    };

    // TODO: Check other examples:
    test_parse_fails(R"({ 1 2 } 3)");
    test(LR"({ 1
2 } 3)", value{3.});
    test(LR"({ 1
;2 ;} 3;)", value{3.}); // Valid
    test_parse_fails(R"(for (a; b
))");
     
    test(LR"(function f(a, b) { return
        a+b;}
        f(1,2))", value::undefined);

    RUN_TEST_SPEC(R"(
a=1;b=2;c=3;
a = b
++c
; a //$ number 2
; b //$ number 2
; c //$ number 4
)");

    test_parse_fails(R"(if (a > b)
else c = d;)");

    RUN_TEST_SPEC(R"(
function c(n) { return n+1; }
a=1;b=2;d=4;e=5;
a = b + c
(d + e).toString()      //$ string '210'
)");

    RUN_TEST_SPEC(R"(x=0; x=x+1 //$ number 1
x++; x //$ number 2
)");
}

int main() {
    try {
        eval_tests();
        test_global_functions();
        test_math_functions();
        test_date_functions();
        test_semicolon_insertion();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

}

