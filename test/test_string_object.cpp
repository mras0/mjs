#include "test.h"
#include "test_spec.h"

using namespace mjs;

void test_main() {
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

    // Array like access came in ES5
    if (tested_version() < version::es5) {
        RUN_TEST_SPEC(R"(
s = new String('test');
ks='';for(k in new String('01')){ks+=k+',';} ks; //$string ''
s[5]; //$undefined
s[3]; //$undefined
s[3] = 42;
s[3]; //$number 42
delete s[3]; //$boolean true
s[3]; //$undefined
)");
    } else {
        RUN_TEST_SPEC(R"(
s = new String('test');
3 in s; //boolean $true
ks='';for(k in new String('01')){ks+=k+',';} ks; //$string '0,1,'
s[5]; //$undefined
s[3]; //$string 't'
s[3] = 42;
s[3]; //$string 't'
delete s[3]; //$boolean false
s[3]; //$string 't'
s[5] = 'XY';
s[5][1]; //$string 'Y'
s.length;//$number 4
5 in s;                         //$boolean true
s.hasOwnProperty('5');          //$boolean true
s.propertyIsEnumerable('5');    //$boolean true
s.length;//$number 4
delete s[5];                    //$boolean true
s[5];                           //$undefined
5 in s;                         //$boolean false
s.hasOwnProperty('5');          //$boolean false
s.propertyIsEnumerable('5');    //$boolean false
s.length;//$number 4

s.hasOwnProperty('3'); //$boolean true
s.propertyIsEnumerable('3'); //$boolean true
)");
    }

    if (tested_version() < version::es3) {
        RUN_TEST_SPEC(R"(
var sp = String.prototype;
sp.concat || sp.localeCompare || sp.match || sp.replace || sp.search || sp.slice || sp.toLocaleLowerCase || sp.toLocaleUpperCase || sp.trim;//$undefined
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

    // match
    RUN_TEST_SPEC(R"(
m='aabb'.match('b');
m.index;//$number 2
m.input;//string 'aabb'
m[0];//$string 'b'

m2=String.prototype.match.call(123,/2|/g);
m2.length;//$number 4
m2.toString();//$string ',2,,'
m2 instanceof Array;//$boolean true

String.prototype.match.call('abAcdAa',/a/g).toString();//$string 'a,a'
String.prototype.match.call('abAcdAa',/a/gi).toString();//$string 'a,A,A,a'
)");


    // search
    RUN_TEST_SPEC(R"(
String.prototype.search.call(14242, 242); //$ number 2
'123'.search(4); //$number -1
'aabba'.search(/a*b/)//$number 0

re = /x/g;
re.lastIndex = 'hello';
'yyyyyyyyx'.search(re); //$number 8
re.lastIndex; //$string 'hello'

'AAA'.search(/a/); //$number -1
'AAA'.search(/a/i); //$number 0
)");

    // replace
    RUN_TEST_SPEC(R"(
String.prototype.replace.call(1122332211,'22','test'); //$string '11test332211'
'abc'.replace('d','e');//$string 'abc'
'123321'.replace(3); //$string '12undefined321'
'aabbaabbaa'.replace(/aa/,123);//$string '123bbaabbaa'
s='xyyxyxx'.replace(/(.)(.)/,function(){return Array.prototype.join.call(arguments)+':';}); s;//$string 'xy,x,y,0,xyyxyxx:yxyxx'
s='xyyxyxx'.replace('(.)(.)',function(){return Array.prototype.join.call(arguments)+':';}); s;//$string 'xyyxyxx'
'abc'.replace(/x/g,'');//$string 'abc'
'abxc'.replace(/x/g,'');//$string 'abc'
'aabbaabbaa'.replace(/aa/g,123);//$string '123bb123bb123'
s='xyyxyxx'.replace(/(..)/g,function(){return Array.prototype.join.call(arguments)+':';});   s;//$string 'xy,xy,0,xyyxyxx:yx,yx,2,xyyxyxx:yx,yx,4,xyyxyxx:x'
s='xyyxyxx'.replace(/(.)(.)/g,function(){return Array.prototype.join.call(arguments)+':';}); s;//$string 'xy,x,y,0,xyyxyxx:yx,y,x,2,xyyxyxx:yx,y,x,4,xyyxyxx:x'
'abc'.replace(/(.)/,'$1;');//$string 'a;bc'
'abc'.replace(/(b)/,'-$&-');//$string 'a-b-c'
'abc'.replace(/(.)/g,'$1;');//$string 'a;b;c;'
'xyyxyxx'.replace(/(x)/g,'-$$$1-');//$string '-$x-yy-$x-y-$x--$x-'
"$1,$2".replace(/(\$(\d))/g,  "$$1-$1$2"); //$string '$1-$11,$1-$22'
'a a b b c c'.replace(/(b)/, "X$'Y$&Z$`W"); //$string 'a a X b c cYbZa a W b c c'
'a a b b c c'.replace(/(b)/g, "X$'Y$&Z$`W"); //$string 'a a X b c cYbZa a W X c cYbZa a b W c c'
)");

    // ES3, 15.5.4.2, 15.5.4.3
    RUN_TEST_SPEC(R"(
try { String.prototype.toString.call({}); } catch (e) { e.toString(); } //$string 'TypeError: Object is not a String'
try { String.prototype.valueOf.call({}); } catch (e) { e.toString(); } //$string 'TypeError: Object is not a String'
)");

    // ES5.1, 15.5.4.20 String.prototype.trim
    if (tested_version() < version::es5) {
        RUN_TEST(L"String.prototype.trim", value::undefined);
    } else {

        // TODO: FIXME: Needs ES5.1, 15.3.4.3 changes to work
#if 0
        //try { String.prototype.trim.call(undefined); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert undefined to object'
        //try { String.prototype.trim.call(null); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert null to object'

#endif
        RUN_TEST_SPEC(R"(
String.prototype.trim.call(''); //$string ''
String.prototype.trim.call('\n\r\t\f     '); //$string ''
String.prototype.trim.call(42); //$string '42'
String.prototype.trim.call('  \n\r\t\f\v\ufefftesttest  \t'); //$string 'testtest'

function f(s) { this.s = s; }
f.prototype.toString = function() { return this.s; }
f.prototype.t = String.prototype.trim;
new f(' \t 123 \ta\r ').t(); //$string '123 \ta'

)");
    }
}
