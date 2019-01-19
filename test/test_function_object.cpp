#include "test_spec.h"
#include "test.h"

using namespace mjs;

void test_main() {
    gc_heap h{256};
    // In ES3 prototype only has attributes DontDelete, in ES1 it's DontEnum
    const string expected_keys{h, tested_version() != version::es3 ? "" : "prototype,"};
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

    // ES5.1, 15.3.5.2 "In Edition 5, the prototype property of Function instances is not enumerable. In Edition 3, this property was enumerable"
    RUN_TEST(L"function a(){}; s=''; for(k in a)s+=k+','; s", value{string{h, tested_version() != version::es3 ? "" : "prototype,"}});

    if (tested_version() < version::es5) {
        RUN_TEST(L"Function.prototype.bind", value::undefined);
    }

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

    RUN_TEST_SPEC(R"(
// ES3, 15.3.2.1
try {new Function('a;b','return a+b');} catch (e) { e.toString(); } //$string 'SyntaxError: Invalid argument to function constructor'
try {new Function('a,b','*a');} catch (e) { e.toString(); } //$string 'SyntaxError: Invalid argument to function constructor'

// ES3, 11.2.2, 11.2.3
try {new 42;} catch (e) { e.toString(); } //$string 'TypeError: 42 is not an object'
try {({})();} catch (e) { e.toString(); } //$string 'TypeError: object is not a function'
try {42();} catch (e) { e.toString(); } //$string 'TypeError: 42 is not a function'

// ES3, 15.3.4.2
try { Function.prototype.toString.call(42); } catch (e) { e.toString(); } //$string 'TypeError: Number is not a function'

// ES3, 15.3.4.3
o = {}
o.apply = Function.prototype.apply;
try { o.apply(); } catch (e) { e.toString(); } //$string 'TypeError: Object is not a function'

function g(a,b) { return '('+a+';'+b+')'; }
function f() { return g.apply(null, arguments); }
f(1,2); //$string '(1;2)'
g.apply(null, [3,4]); //$string '(3;4)'

// ES3, 15.3.4.4
o = {}
o.call = Function.prototype.call;
try { o.call(); } catch (e) { e.toString(); } //$string 'TypeError: Object is not a function'

// ES3, 15.3.5.3
function f() {}
fi = new f();
fi instanceof f; //$boolean true
f.prototype = undefined;
try { fi instanceof f; } catch (e) { e.toString(); } //$string 'TypeError: Function has non-object prototype of type undefined in instanceof check'
)");

    const wchar_t* const apply_normal_object = L"try { Number.prototype.toString.apply(42, {'length':1,0:16}); } catch (e) { e.toString(); }";
    if (tested_version() < version::es5) {
        // In ES3 the second argument to Function.prototype.apply must be an Arguments object or an Array.
        RUN_TEST(apply_normal_object, value{string{h, "TypeError: Object is not an (arguments) array"}});
    } else {
        // ES5.1, 15.3.4.3 the second argument to apply can be any object with a length property
        RUN_TEST(apply_normal_object, value{string{h, "2a"}});
    }

    // Function.prototype.bind
    if (tested_version() >= version::es5) {

        RUN_TEST_SPEC(R"(
Function.prototype.bind.length;//$number 1
try { Function.prototype.bind.call(new String(''),42); } catch (e) { e.toString(); } //$string 'TypeError: String is not a function'
r = (function(){return this}).bind()(); r === global; //$boolean true
r = (function(){return this}).bind()(undefined); r === global; //$boolean true
r = (function(){return this}).bind()(null); r === global; //$boolean true
a=(function(){return this}).bind(42);
a.length;//$number 0
r=a(); r instanceof Number; //$boolean true
r.valueOf(); //$ number 42
add1=(function(x,y){return x+y;}).bind(undefined,1);
add1(42);//$number 43
add1('x ');//$string '1x '
add1.length;//$number 1
add12=add1.bind(2,3);
add12(4); //$number 4
add12.length;//$number 0
add12x=add12.bind(4,5);
add12x(4); //$number 4
add12x.length;//$number 0

add2=(function(x,y){return x+y;}).bind(undefined,1,2,3);
add2(42); //$number 3
add2.length; //$number 0

function fmt(v) { return v+':'+typeof v; }
function fmtall(o) { var r = '{'; var p = Object.getOwnPropertyNames(o);for(var i=0;i<p.length;++i) { r+=' '+p[i]+'='+fmt(o[p[i]]); }; return r+'}'; }
function f(a,b) { s+='this='+fmtall(this)+',arguments='+fmtall(arguments); this.x=a; this.y=b; };
s = ''; x = f.bind(new String('x'),new String('y'));
s; //$string ''
r=new x(42,43,44);
s; //$string 'this={},arguments={ length=4:number 0=y:object 1=42:number 2=43:number 3=44:number callee=function f(a,b) { s+=\'this=\'+fmtall(this)+\',arguments=\'+fmtall(arguments); this.x=a; this.y=b; }:function}'
r instanceof f; //$boolean true
fmtall(r); //$string '{ x=y:object y=42:number}'
)");
    }
}