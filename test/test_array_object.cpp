#include "test.h"
#include "test_spec.h"
#include <mjs/array_object.h>
#include <cmath>

using namespace mjs;

void test_main() {
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
var s = ''; for (var k in a) s += k + ','; s //$string '2,7,false,'
a.length=4;
s = ''; for (var k in a) s += k + ','; s //$string '2,false,'
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

// Sparse/Large array
    var a = new Array();
    a[1000000] = 42;
    a[1000000]; //$number 42
    a.length;//$number 1000001
    a[123]; //$undefined
    a[999999]; //$undefined
    a[1000001]; //$undefined

)");

    if (tested_version() < version::es5) {
        RUN_TEST_SPEC(R"(
Array.isArray; //$undefined
ap = Array.prototype;
ap['indexOf'] || ap['lastIndexOf'] || ap['every'] || ap['some'] || ap['forEach'] || ap['map'] || ap['filter'] || ap['reduce'] || ap['reduceRight']; //$undefined
)");
    }

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

    if (tested_version() == version::es3) {
        // ES3, 15.4.4.2, 15.4.4.3
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
try { Array.prototype.toLocaleString.call({length:3,0:42,2:'x'}); } catch (e) { e.toString(); } //$string 'TypeError: Object is not an array'
        )");
    } else {
        // ES5.1: Array.prototype.to(Locale)String() are generic
        RUN_TEST_SPEC(R"(
function f() { this.length=2; this[0]='x'; this[1]='y'; }
f.prototype.toString = Array.prototype.toString;
''+new f(); //$string 'x,y'

[].toLocaleString(); //$string ''
function t(x) { this.x = x; }
t.prototype.toLocaleString = function() { return this.x; };
[new t(42)].toLocaleString(); //$string '42'
a=[new t(1), new t(2), undefined, new t(3), null]; a[null]=new t(4); a.toLocaleString(); //$string '1,2,,3,'
Array.prototype.toLocaleString.call({length:3,0:42,2:'x'}); //$string '42,,x'
        )");

        // TODO: Requires 15.3.4.3 changes
        // try { Array.prototype.indexOf.call(); } catch (e) { e.toString(); } //$string 'TypeError: cannot convert undefined to object'

        RUN_TEST_SPEC(R"(

Array.prototype.indexOf.length;//$number 1
Array.prototype.indexOf.call({}, 1); //$number -1
Array.prototype.indexOf.call({length:3,2:1}, 1); //$number 2
Array.prototype.indexOf.call({length:3,2:1}, 2); //$number -1
Array.prototype.indexOf.call({length:4,2:1,3:1}, 1); //$number 2
Array.prototype.indexOf.call({length:4,2:1,3:1}, 1, 1); //$number 2
Array.prototype.indexOf.call({length:4,2:1,3:1}, 1, 3); //$number 3
Array.prototype.indexOf.call({length:4,2:1,3:1}, 1, 4); //$number -1
Array.prototype.indexOf.call({length:4,2:1,3:1}, 1, -1); //$number 3
Array.prototype.indexOf.call({length:4,2:1,3:1}, 1, -2); //$number 2
Array.prototype.indexOf.call({length:4,2:1,3:1}, 1, -3); //$number 2
Array.prototype.indexOf.call({length:4,2:1,3:1}, 1, -12); //$number 2
['a','bb','b','c'].indexOf('b'); //$number 2
Array.prototype.indexOf.call('test', 's'); //$number 2
[1,2,3].indexOf(); //$number -1
[1,undefined].indexOf(); //$number 1

Array.prototype.lastIndexOf.length;//$number 1
Array.prototype.lastIndexOf.call({}, 1); //$number -1
Array.prototype.lastIndexOf.call({length:3,2:1}, 1); //$number 2
Array.prototype.lastIndexOf.call({length:3,2:1}, 2); //$number -1
Array.prototype.lastIndexOf.call({length:4,2:1,3:1}, 1); //$number 3
Array.prototype.lastIndexOf.call({length:4,2:1,3:1}, 1, 1); //$number -1
Array.prototype.lastIndexOf.call({length:4,2:1,3:1}, 1, 2); //$number 2
Array.prototype.lastIndexOf.call({length:4,2:1,3:1}, 1, 3); //$number 3
Array.prototype.lastIndexOf.call({length:4,2:1,3:1}, 1, 4); //$number 3
Array.prototype.lastIndexOf.call({length:4,2:1,3:1}, 1, -1); //$number 3
Array.prototype.lastIndexOf.call({length:4,2:1,3:1}, 1, -2); //$number 2
Array.prototype.lastIndexOf.call({length:4,2:1,3:1}, 1, -3); //$number -1
Array.prototype.lastIndexOf.call({length:4,2:1,3:1}, 1, -12); //$number -1
['a','bb','b','c'].lastIndexOf('b'); //$number 2
Array.prototype.lastIndexOf.call('test', 's'); //$number 2
[1,2,3].lastIndexOf(); //$number -1
[1,undefined].lastIndexOf(); //$number 1


try { [].every(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not a function'
try { [].every(42); } catch (e) { e.toString(); } //$string 'TypeError: 42 is not a function'

function fmt(v) { return typeof(v)+':'+v; }
function fmtall(t,a,b,c) { return 'this='+fmt(t)+',a='+fmt(a)+',b='+fmt(b)+',c='+fmt(c); }

Array.prototype.every.length;//$number 1
s=''; [].every(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }); //$boolean true
s; //$string ''
s=''; [1,2,3].every(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }); //$boolean true
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3\nthis=object:[object Global],a=number:2,b=number:1,c=object:1,2,3\nthis=object:[object Global],a=number:3,b=number:2,c=object:1,2,3\n'
s=''; [1,2,3].every(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }, ['z']); //$boolean true
s; //$string 'this=object:z,a=number:1,b=number:0,c=object:1,2,3\nthis=object:z,a=number:2,b=number:1,c=object:1,2,3\nthis=object:z,a=number:3,b=number:2,c=object:1,2,3\n'
s=''; [1,2,3].every(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return false; }); //$boolean false
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3\n'
s=''; b=[1,2,3].every(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return b!==2; }); b //$boolean false
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3\nthis=object:[object Global],a=number:2,b=number:1,c=object:1,2,3\nthis=object:[object Global],a=number:3,b=number:2,c=object:1,2,3\n'
s=''; Array.prototype.every.call({length:4,1:'a',3:'c',5:'d'}, function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }, ['z']); //$boolean true
s; //$string 'this=object:z,a=string:a,b=number:1,c=object:[object Object]\nthis=object:z,a=string:c,b=number:3,c=object:[object Object]\n'

Array.prototype.some.length;//$number 1
s=''; [].some(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }); //$boolean false
s; //$string ''
s=''; [1,2,3].some(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }); //$boolean true
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3\n'
s=''; [1,2,3].some(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return false; }); //$boolean false
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3\nthis=object:[object Global],a=number:2,b=number:1,c=object:1,2,3\nthis=object:[object Global],a=number:3,b=number:2,c=object:1,2,3\n'
s=''; b=[1,2,3].some(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return b===1; }); b;//$boolean true
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3\nthis=object:[object Global],a=number:2,b=number:1,c=object:1,2,3\n'
s=''; Array.prototype.some.call({length:4,1:'a',3:'c',5:'d'}, function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }, ['z']); //$boolean true
s; //$string 'this=object:z,a=string:a,b=number:1,c=object:[object Object]\n'

Array.prototype.forEach.length;//$number 1
s=''; r=[].forEach(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }); r;//$undefined
s; //$string ''
s=''; r=[1,2,3].forEach(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }); r;//$undefined
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3\nthis=object:[object Global],a=number:2,b=number:1,c=object:1,2,3\nthis=object:[object Global],a=number:3,b=number:2,c=object:1,2,3\n'
s=''; r=[1,2,3].forEach(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return false; }); r;//$undefined
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3\nthis=object:[object Global],a=number:2,b=number:1,c=object:1,2,3\nthis=object:[object Global],a=number:3,b=number:2,c=object:1,2,3\n'
s=''; r=[1,2,3].forEach(function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return b===1; }); r;//$undefined
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3\nthis=object:[object Global],a=number:2,b=number:1,c=object:1,2,3\nthis=object:[object Global],a=number:3,b=number:2,c=object:1,2,3\n'
s=''; r=Array.prototype.forEach.call({length:4,1:'a',3:'c',5:'d'}, function(a,b,c) { s+=fmtall(this,a,b,c)+'\n'; return true; }, ['z']); r;//$undefined
s; //$string 'this=object:z,a=string:a,b=number:1,c=object:[object Object]\nthis=object:z,a=string:c,b=number:3,c=object:[object Object]\n'


Array.prototype.map.length;//$number 1
try {[].map();}catch(e){e.toString();} //$string 'TypeError: undefined is not a function'

b = [].map(function(a,b,c){});
b instanceof Array;//$boolean true
b.toString();//$string ''

a = [1,2,3];
b = a.map(function(a,b,c){return fmtall(this,a,b,c);});
b instanceof Array; //$boolean true
a===b;//$boolean false
a.toString();//$string '1,2,3'
b.toString();//$string 'this=object:[object Global],a=number:1,b=number:0,c=object:1,2,3,this=object:[object Global],a=number:2,b=number:1,c=object:1,2,3,this=object:[object Global],a=number:3,b=number:2,c=object:1,2,3'

['x'].map(function(a,b,c){return fmtall(this,a,b,c);}, new Number(42)).toString(); //$string 'this=object:42,a=string:x,b=number:0,c=object:x'

a = {length:5,0:2,3:4};
b = Array.prototype.map.call(a, function(a,b) { return a+':'+b; });
a instanceof Array; //$boolean false
a.length;//$number 5
a[1];//$undefined
b instanceof Array; //$boolean true
b.length;//$number 5
b[1];//$undefined
b.toString();//$string '2:0,,,4:3,'


Array.prototype.filter.length;//$number 1
x=[].filter(function(){});
x instanceof Array;
x.length; //$number 0

a = [1,2,3,4];
b = a.filter(function(a){return a%2==0;});
b instanceof Array;
a.toString(); //$string '1,2,3,4'
b.toString(); //$string '2,4'

a = [1,2,3,4];
b = a.filter(function(a){return false;});
b instanceof Array;
a.toString(); //$string '1,2,3,4'
b.toString(); //$string ''

a = [0,1,2];
s = '';
x = a.filter(function(a,b,c){s+=fmtall(this,a,b,c)+'\n'; return b===1;},1234);
x.toString(); //$string '1'
s; //$string 'this=object:1234,a=number:0,b=number:0,c=object:0,1,2\nthis=object:1234,a=number:1,b=number:1,c=object:0,1,2\nthis=object:1234,a=number:2,b=number:2,c=object:0,1,2\n'

ao = { length: 4, 0: 1, 3:2 };
s = '';
x = Array.prototype.filter.call(ao, function(a,b,c){s+=fmtall(this,a,b,c)+'\n'; return true;});
x instanceof Array; //$boolean true
ao instanceof Array; //$boolean false
x.length; //$number 2
x.toString(); //$string '1,2'
s; //$string 'this=object:[object Global],a=number:1,b=number:0,c=object:[object Object]\nthis=object:[object Global],a=number:2,b=number:3,c=object:[object Object]\n'


Array.prototype.reduce.length; //$number 1
Array.prototype.reduceRight.length; //$number 1
try {[].reduce();}catch(e){e.toString();} //$string 'TypeError: undefined is not a function'
try {[].reduceRight();}catch(e){e.toString();} //$string 'TypeError: undefined is not a function'
try {[].reduce(function(){});}catch(e){e.toString();} //$string 'TypeError: cannot reduce empty array'
try {[].reduceRight(function(){});}catch(e){e.toString();} //$string 'TypeError: cannot reduce empty array'
try {Array.prototype.reduce.call({length:10,10:42},function(){});}catch(e){e.toString();} //$string 'TypeError: cannot reduce empty array'
[].reduce(function(){},42);//$number 42
[].reduceRight(function(){},'x');//$string 'x'
[].reduce(function(){},undefined);//$undefined
[].reduceRight(function(){},undefined);//$undefined

function r(a,b,c,d){s+=fmt(this)+','+fmt(a)+','+fmt(b)+','+fmt(c)+','+fmt(d)+'\n';return a-b;};
a = [2,4,10];
s='';a.reduce(r); //$number -12
s; //$string 'object:[object Global],number:2,number:4,number:1,object:2,4,10\nobject:[object Global],number:-2,number:10,number:2,object:2,4,10\n'
s='';a.reduceRight(r); //$number 4
s; //$string 'object:[object Global],number:10,number:4,number:1,object:2,4,10\nobject:[object Global],number:6,number:2,number:0,object:2,4,10\n'
a instanceof Array; //$boolean true
a.toString(); //$string '2,4,10'

o={length:3,0:10,2:40};
r=Array.prototype.reduce.call(o, function(a,v){return ''+a+v;}, 30); r;//$string '301040'
o={length:4,0:10,1:undefined,2:40};
r=Array.prototype.reduceRight.call(o, function(a,v){return ''+a+v;}, 30); r;//$string '3040undefined10'


Array.isArray.length; //$number 1
Array.isArray();//$boolean false
Array.isArray(undefined);//$boolean false
Array.isArray(null);//$boolean false
Array.isArray(42);//$boolean false
Array.isArray('x');//$boolean false
Array.isArray({length:1,0:42});//$boolean false
Array.isArray([]);//$boolean true
)");
    }

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

    RUN_TEST_SPEC(R"(
Array.prototype.concat.length;           //$number 1
[].concat().toString();                  //$string ''
a=[1,2];
a.concat().toString();                   //$string '1,2'
a.concat(3).toString();                  //$string '1,2,3'
a.concat(3,'x').toString();              //$string '1,2,3,x'
a.toString();                            //$string '1,2'
[,,2].concat(1,undefined,2).toString();  //$string ',,2,1,,2'

function f() { this.length = 4; this['1']='a'; this['3']='z'; this['4'] = 'w'; }
f.prototype.concat = Array.prototype.concat;
f.prototype.toString = function() { return 'f'+this.length; };
a=(new f()).concat(6,7,8);
a.length;//$ number 4
a instanceof Array; //$boolean true
a.toString(); //$string 'f4,6,7,8'

Array.prototype.concat.call(42,1,2).toString(); //$string '42,1,2'

b=['a','b','c'].concat([1,2,3],[[4,5,6]]);
b.length;//$number 7
b[6].length;//$number 3
b.toString();//$string 'a,b,c,1,2,3,4,5,6'

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
