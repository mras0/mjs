#include "test.h"
#include "test_spec.h"

using namespace mjs;

void test_main() {
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
        RUN_TEST_SPEC(R"(
        op=Object.prototype; op.toLocaleString||op.hasOwnProperty||op.propertyIsEnumerable;//$undefined
        )");
    } else {
        RUN_TEST_SPEC(R"(
Object.prototype.toLocaleString.length; //$number 0
Object.prototype.toLocaleString.call(new Number(42)); //$string '42'
Object.prototype.toLocaleString.call(new String('test')); //$string 'test'
function f() { this.x = 'str'; }
f.prototype.toString = function() { return this.x; }
new f().toLocaleString(); //$string 'str'


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

// ES3, 8.6.2.6
function C() {}
C.prototype.toString = function() { return this; }
C.prototype.valueOf = function() { return this; }
try { +(new C()); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert object to primitive value'
try { ''+(new C()); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert object to primitive value'

// ES3, 9.9
try { (undefined).toString(); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert undefined to object'
try { (null).toString(); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert null to object'

// getter/setters musn't interfere with the standard use
o = { get:42, set:43 };
o.get;//$number 42
o.set;//$number 43
)");
    }

    //
    // ES5
    //

    if (tested_version() < version::es5) {
        RUN_TEST_SPEC(R"(
Object.getPrototypeOf || Object.getOwnPropertyDescriptor  || Object.getOwnPropertyNames ||
Object.create ||  Object.defineProperty || Object.defineProperties || Object.seal ||
Object.freeze || Object.preventExtensions || Object.isSealed || Object.isFrozen ||
Object.isExtensible || Object.keys || Object.isPrototypeOf; //$undefined
        )");
    } else {

        RUN_TEST_SPEC(R"(
Object.getPrototypeOf.length; //$number 1
try { Object.getPrototypeOf(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.getPrototypeOf(undefined); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.getPrototypeOf(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { Object.getPrototypeOf(42); } catch (e) { e.toString(); } //$string 'TypeError: 42 is not an object'
try { Object.getPrototypeOf('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'
Object.getPrototypeOf(new Number(42)) === Number.prototype; //$boolean true
Object.getPrototypeOf(new String('x')) === Number.prototype; //$boolean false
Object.getPrototypeOf(new String('x')) === String.prototype; //$boolean true
Object.getPrototypeOf({}) === Object.prototype; //$boolean true
Object.getPrototypeOf(Object.prototype); //$null

Object.prototype.isPrototypeOf.length;//$number 1
Object.prototype.isPrototypeOf(42); //$boolean false
Object.prototype.isPrototypeOf({}); //$boolean true
String.prototype.isPrototypeOf('x'); //$boolean false
String.prototype.isPrototypeOf(42); //$boolean false
String.prototype.isPrototypeOf(new String('x')); //$boolean true

function o(){};
function p(){};
p.prototype=new o();
i=new p();
o.prototype.isPrototypeOf(i); //$boolean true
p.prototype.isPrototypeOf(i); //$boolean true
String.prototype.isPrototypeOf(i); //$boolean false
Object.prototype.isPrototypeOf(i); //$boolean true
Function.prototype.isPrototypeOf(i); //$boolean false

Object.getOwnPropertyNames.length; //$number 1
function opn(o) { return Object.getOwnPropertyNames(o).toString(); }
try { opn(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { opn(undefined); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { opn(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { opn(42); } catch (e) { e.toString(); } //$string 'TypeError: 42 is not an object'
try { opn('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'
opn(new Number(42)); //$string ''
opn([1,2]); //$string '0,1,length'
opn(new String('abc')); //$string '0,1,2,length'
opn({}); //$string ''
opn({a:0,b:0,c:0}); //$string 'a,b,c'

function a() {}
a.prototype.x = 42;
opn(new a()); //$string ''
s='';for (k in new a) { s+=k+','; } s; //$string  'x,'

Object.keys.length; //$number 1
function keys(o) { return Object.keys(o).toString(); }
try { keys(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { keys(undefined); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { keys(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { keys(42); } catch (e) { e.toString(); } //$string 'TypeError: 42 is not an object'
try { keys('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'
keys(new Number(42)); //$string ''
keys([1,2]); //$string '0,1'
keys(new String('abc')); //$string '0,1,2'
keys({}); //$string ''
keys({a:0,b:0,c:0}); //$string 'a,b,c'

function a() {}
a.prototype.x = 42;
keys(new a()); //$string ''
s='';for (k in new a) { s+=k+','; } s; //$string  'x,'

Object.getOwnPropertyDescriptor.length; //$number 2
function gopd(o,p) {
    var d = Object.getOwnPropertyDescriptor(o,p);
    if (d === undefined) return d;
    var r = '{';
    for (k in d) {
        r += k + ': ' + d[k] + ',';
    }
    return r + '}';
}
try { gopd(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { gopd(undefined); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { gopd(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { gopd(42); } catch (e) { e.toString(); } //$string 'TypeError: 42 is not an object'
try { gopd('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'
gopd(Object,'sadjasdisds'); //$undefined
gopd(Object,'prototype'); //$string '{value: [object Object],writable: false,enumerable: false,configurable: false,}'
gopd(new String('test'),'length'); //$string '{value: 4,writable: false,enumerable: false,configurable: false,}'
gopd(new String('test'),3); //$string '{value: t,writable: false,enumerable: true,configurable: false,}'
gopd(new String('test'),5); //$undefined
gopd([0,1,2,3,4],3); //$string '{value: 3,writable: true,enumerable: true,configurable: true,}'
gopd([0,1,2,3,4],5); //$undefined

o = {};
o2 = {x:o};
Object.getOwnPropertyDescriptor(o2,'x').value === o; //$boolean true

Object.defineProperty.length;//$number 3
try { Object.defineProperty(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.defineProperty(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { Object.defineProperty(42); } catch (e) { e.toString(); } //$string 'TypeError: 42 is not an object'
try { Object.defineProperty('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'
try { Object.defineProperty({},'x'); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.defineProperty({},'x',60); } catch (e) { e.toString(); } //$string 'TypeError: 60 is not an object'

gopd(Object.defineProperty({},'x',{}),'x'); //$string '{value: undefined,writable: false,enumerable: false,configurable: false,}'
gopd(Object.defineProperty({},'x',{value:42,writable:true}),'x'); //$string '{value: 42,writable: true,enumerable: false,configurable: false,}'
gopd(Object.defineProperty({},'x',{value:42,enumerable:true}),'x'); //$string '{value: 42,writable: false,enumerable: true,configurable: false,}'
gopd(Object.defineProperty({},'x',{value:42,configurable:true}),'x'); //$string '{value: 42,writable: false,enumerable: false,configurable: true,}'
o = {};
'a' in o; //$boolean false
Object.defineProperty(o,'a',{value:'aVal',writable:true,enumerable:true,configurable:true}) === o; //$boolean true
'a' in o; //$boolean true
gopd(o,'a'); //$string '{value: aVal,writable: true,enumerable: true,configurable: true,}'

o2={};
Object.defineProperty(o2,'x',{value:0,writable:0,enumerable:'',configurable:null}); o2.x;//$number 0
Object.defineProperty(o2,'x',{value:0,writable:false,enumerable:false,configurable:false}); o2.x;//$number 0
Object.defineProperty(o2,'x',{}); o2.x;//$number 0
try { Object.defineProperty(o2,'x',{value:1,writable:false,enumerable:false,configurable:false});} catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: x'
try { Object.defineProperty(o2,'x',{value:0,writable:false,enumerable:false,configurable:true}); } catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: x'
try { Object.defineProperty(o2,'x',{value:0,writable:false,enumerable:true,configurable:false}); } catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: x'
try { Object.defineProperty(o2,'x',{value:0,writable:true,enumerable:false,configurable:false}); } catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: x'
try { Object.defineProperty(o2,'x',{get:function(){},set:function(){}}); } catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: x'
o2.x;//$number 0

o3={};
Object.defineProperty(o3,42,{value:42,writable:true,enumerable:true,configurable:true}); o3[42];//$number 42
Object.defineProperty(o3,42,{value:60,writable:true,enumerable:true,configurable:true}); o3[42];//$number 60
Object.defineProperty(o3,42,{value:90,writable:false,enumerable:false,configurable:false}); o3[42];//$number 90
gopd(o3,42); //$string '{value: 90,writable: false,enumerable: false,configurable: false,}'

a = [1,2,3];
Object.defineProperty(a,'length',{value:5}).toString(); //$string '1,2,3,,'
Object.defineProperty(a,'0',{value:42}).toString(); //$string '42,2,3,,'
gopd(a, 'length'); //$string '{value: 5,writable: true,enumerable: false,configurable: false,}'

s = new String('abc');
gopd(s, 'length'); //$string '{value: 3,writable: false,enumerable: false,configurable: false,}'
Object.defineProperty(s, 'length', {});
Object.defineProperty(s, 'length', {value:3});
try { Object.defineProperty(s, 'length', {value:4}); } catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: length'
try { Object.defineProperty(s, '0', {value:3}); } catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: 0'

try { Object.defineProperty({}, 'x', {get:42}); } catch (e) { e.toString(); } //$string 'TypeError: Getter for "x" must be a function: 42'
try { Object.defineProperty({}, 'x', {set:42}); } catch (e) { e.toString(); } //$string 'TypeError: Setter for "x" must be a function: 42'

Object.defineProperties.length;//$number 2
try { Object.defineProperties(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.defineProperties(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { Object.defineProperties(42); } catch (e) { e.toString(); } //$string 'TypeError: 42 is not an object'
try { Object.defineProperties({},null); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert null to object'
try { Object.defineProperties({},{foo:42}); } catch (e) { e.toString(); } //$string 'TypeError: 42 is not an object'

function allProps(o) {
    var res = '{';
    var ps = Object.getOwnPropertyNames(o);
    for (var i = 0; i < ps.length; ++i) {
        var k = ps[i];
        res += k + ':{';
        var d = Object.getOwnPropertyDescriptor(o, k);
        for (var k2 in d) {
            res += k2 + ':' + d[k2] + ',';
        }
        res += '}';
    }
    return res + '}';
}

allProps(Object.defineProperties({},42)); //$string '{}'
allProps(Object.defineProperties({},{foo:{value:42}})); //$string '{foo:{value:42,writable:false,enumerable:false,configurable:false,}}'
o = {};
allProps(Object.defineProperties(o,{foo:{value:'x',writable:true,enumerable:true,configurable:true}})); //$string '{foo:{value:x,writable:true,enumerable:true,configurable:true,}}'
allProps(Object.defineProperties(o,{bar:{value:'z'}})); //$string '{foo:{value:x,writable:true,enumerable:true,configurable:true,}bar:{value:z,writable:false,enumerable:false,configurable:false,}}'
allProps(Object.defineProperties(o,{baz:{value:'q'},foo:{value:42}})); //$string '{foo:{value:42,writable:true,enumerable:true,configurable:true,}bar:{value:z,writable:false,enumerable:false,configurable:false,}baz:{value:q,writable:false,enumerable:false,configurable:false,}}'
allProps(Object.defineProperties(o,{})); //$string '{foo:{value:42,writable:true,enumerable:true,configurable:true,}bar:{value:z,writable:false,enumerable:false,configurable:false,}baz:{value:q,writable:false,enumerable:false,configurable:false,}}'
try { allProps(Object.defineProperties(o,{bar:{value:'w'}})); } catch(e) { e.toString(); } //$string 'TypeError: cannot redefine property: bar'



Object.preventExtensions.length;//$number 1
Object.isExtensible.length;//$number 1

try { Object.preventExtensions(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.preventExtensions(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { Object.preventExtensions('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'

try { Object.isExtensible(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.isExtensible(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { Object.isExtensible('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'

o = {x:42};
Object.isExtensible(o); //$boolean true
o === Object.preventExtensions(o); //$boolean true
Object.isExtensible(o); //$boolean false
'x' in o; //$boolean true
o.x=60;
o.x;//$number 60
allProps(o); //$string '{x:{value:60,writable:true,enumerable:true,configurable:true,}}'
delete o.x; //$boolean true
'x' in o; //$boolean false
o.y = 12;
o.y; //$undefined
'y' in o; //$boolean false
try { Object.defineProperty(o,'x',{value:42}); } catch (e) { e.toString(); } //$string 'TypeError: cannot define property: x, object is not extensible'

a = Object.preventExtensions([1,2,3]);
a.length = 10;
a.toString(); //$string '1,2,3,,,,,,,'
1 in a; //$boolean true
3 in a; //$boolean false
a[3] = 42;
3 in a; //$boolean false



Object.seal.length;//$number 1
Object.isSealed.length;//$number 1

try { Object.seal(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.seal(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { Object.seal('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'

try { Object.isSealed(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.isSealed(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { Object.isSealed('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'

var o = {x:42,y:12};
Object.isExtensible(o); //$boolean true
Object.isSealed(o); //$boolean false
allProps(o); //$string '{x:{value:42,writable:true,enumerable:true,configurable:true,}y:{value:12,writable:true,enumerable:true,configurable:true,}}'
Object.seal(o) === o; //$boolean true
Object.isExtensible(o); //$boolean false
Object.isSealed(o); //$boolean true
allProps(o); //$string '{x:{value:42,writable:true,enumerable:true,configurable:false,}y:{value:12,writable:true,enumerable:true,configurable:false,}}'

var z = Object.defineProperty({}, 'q', {value:42});
Object.isSealed(z); //$boolean false
Object.preventExtensions(z);
Object.isSealed(z); //$boolean true



Object.freeze.length;//$number 1
Object.isFrozen.length;//$number 1

try { Object.freeze(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.freeze(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { Object.freeze('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'

try { Object.isFrozen(); } catch (e) { e.toString(); } //$string 'TypeError: undefined is not an object'
try { Object.isFrozen(null); } catch (e) { e.toString(); } //$string 'TypeError: null is not an object'
try { Object.isFrozen('x'); } catch (e) { e.toString(); } //$string 'TypeError: \'x\' is not an object'

var o = {x:42,y:12};
Object.isExtensible(o); //$boolean true
Object.isSealed(o); //$boolean false
Object.isFrozen(o); //$boolean false
allProps(o); //$string '{x:{value:42,writable:true,enumerable:true,configurable:true,}y:{value:12,writable:true,enumerable:true,configurable:true,}}'
Object.freeze(o) === o; //$boolean true
Object.isExtensible(o); //$boolean false
Object.isSealed(o); //$boolean true
Object.isFrozen(o); //$boolean true
allProps(o); //$string '{x:{value:42,writable:false,enumerable:true,configurable:false,}y:{value:12,writable:false,enumerable:true,configurable:false,}}'

var z = Object.defineProperty({}, 'q', {value:42,writable:false,enumerable:true,configurable:false});
allProps(z); //$string '{q:{value:42,writable:false,enumerable:true,configurable:false,}}'
Object.isFrozen(z); //$boolean false
Object.preventExtensions(z);
Object.isFrozen(z); //$boolean true
allProps(z); //$string '{q:{value:42,writable:false,enumerable:true,configurable:false,}}'



Object.create.length;//$number 2
try { Object.create(); } catch (e) { e.toString(); } //$string 'TypeError: Invalid object prototype'
try { Object.create(true); } catch (e) { e.toString(); } //$string 'TypeError: Invalid object prototype'
try { Object.create(42); } catch (e) { e.toString(); } //$string 'TypeError: Invalid object prototype'
try { Object.create(null,null); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert null to object'

o = Object.create(null);
Object.getPrototypeOf(o); //$null
allProps(o); //$string '{}'

o2={};
o = Object.create(o2, {x:{value:42}});
Object.getPrototypeOf(o) === o2; //$boolean true
allProps(o); //$string '{x:{value:42,writable:false,enumerable:false,configurable:false,}}'
)");

RUN_TEST_SPEC(R"(
// ES5.1 8.10.5-9
try { Object.defineProperty({},'x',{get:function(){},writable:false}); } catch (e) { e.toString(); }    //$string 'TypeError: Accessor property descriptor may not have value or writable attributes'
try { Object.defineProperty({},'x',{get:function(){},value:0}); } catch (e) { e.toString(); }           //$string 'TypeError: Accessor property descriptor may not have value or writable attributes'
try { Object.defineProperty({},'x',{set:function(a){},writable:false}); } catch (e) { e.toString(); }   //$string 'TypeError: Accessor property descriptor may not have value or writable attributes'
try { Object.defineProperty({},'x',{set:function(a){},value:0}); } catch (e) { e.toString(); }          //$string 'TypeError: Accessor property descriptor may not have value or writable attributes'


var o = Object.defineProperty({},'x',{get:function(){},configurable:false});
try { Object.defineProperty(o,'x',{get:function(){}}); } catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: x'
try { Object.defineProperty(o,'x',{set:function(){}}); } catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: x'

function gopd(o,p) {
    var d = Object.getOwnPropertyDescriptor(o,p);
    if (d === undefined) return d;
    var r = '{';
    for (k in d) {
        r += k + ': ' + d[k] + ',';
    }
    return r + '}';
}

o = {};
Object.defineProperty(o, 'x', {get: function() { return 42; }});
o.x;//$number 42
o.x=60;
o.x;//$number 42
gopd(o,'x');//$string '{get: function () { return 42; },set: undefined,enumerable: false,configurable: false,}'

o = {n:1, q:'xyz'};
o.q; //$string 'xyz'
Object.defineProperty(o, 'q', {get: function() { return this.n; }, set: function(m) { this.n=m+1; }});
o.q;    //$number 1
o.q=42;
o.q;    //$number 43
gopd(o,'q');//$string '{get: function () { return this.n; },set: function (m) { this.n=m+1; },enumerable: true,configurable: true,}'

o = {};
Object.defineProperty(o, 'z', {set: function(x){this.x=x;},configurable:true});
o.x;//$undefined
o.z;//$undefined
o.z = 123;
o.x;//$number 123
o.z;//$undefined
gopd(o,'z');//$string '{get: undefined,set: function (x){this.x=x;},enumerable: false,configurable: true,}'
Object.defineProperty(o, 'z', {configurable:false});
gopd(o,'z');//$string '{get: undefined,set: function (x){this.x=x;},enumerable: false,configurable: false,}'

try { Object.defineProperty(o, 'z', {configurable:true, enumerable:true}); } catch (e) { e.toString(); } //$string 'TypeError: cannot redefine property: z'

o = {};
Object.defineProperty(o, 'z', {configurable:true, enumerable:true});
gopd(o,'z');//$string '{value: undefined,writable: false,enumerable: true,configurable: true,}'

o = {n:42};
gopd(o,'n');//$string '{value: 42,writable: true,enumerable: true,configurable: true,}'
Object.defineProperty(o, 'n', {configurable:false, enumerable:false});
gopd(o,'n');//$string '{value: 42,writable: true,enumerable: false,configurable: false,}'
o = { x:42, get if (   )     { return this.x; }, set if (a) { this.x=a*2; }, };
gopd(o,'x');  //$string '{value: 42,writable: true,enumerable: true,configurable: true,}'
gopd(o,'if'); //$string '{get: function get if(   )     { return this.x; },set: function set if(a) { this.x=a*2; },enumerable: true,configurable: true,}'
o.x;//$number 42
o.if;//$number 42
o.x = 'x';
o.if;//$string 'x'
o.if = 2;
o.x; //$number 4
o.if; //$number 4
delete o.x;
'if' in o; //$boolean true
o.if;//$undefined
delete o.if; //$boolean true
'if' in o; //$boolean false

var x = 60;
o = { get q(){ return x; } };
o.q;//$number 60
o.q=12;
o.q;//$number 60
x=22;
o.q;//$number 22

o = (function(){ var x=42; return { get q(){ return x; }, set q (a) { x=a+19; } }; })();
o.q;//$number 42
o.q=12;
o.q;//$number 31

// Redefine accessor property to data property

o = Object.defineProperty({}, 'p', { get: function(){}, configurable: true });
gopd(o,'p');//$string '{get: function (){},set: undefined,enumerable: false,configurable: true,}'
Object.defineProperty(o, 'p', { value: 42 });
gopd(o,'p');//$string '{value: 42,writable: false,enumerable: false,configurable: true,}'

)");
    }
}

