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

)");
    }

    //
    // ES5
    //

    if (tested_version() < version::es5) {
        RUN_TEST_SPEC(R"(
Object.getPrototypeOf || Object.getOwnPrObjectertyDescriptor || Object.getOwnPrObjectertyNames ||
Object.create ||  Object.definePrObjecterty || Object.definePrObjecterties || Object.seal ||
Object.freeze || Object.preventExtensions || Object.isSealed || Object.isFrozen ||
Object.isExtensible || Object.keys; //$undefined
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

Object.getOwnPrObjectertyDescriptor ||
Object.create ||  Object.definePrObjecterty || Object.definePrObjecterties || Object.seal ||
Object.freeze || Object.preventExtensions || Object.isSealed || Object.isFrozen ||
Object.isExtensible; //$undefined
)");
    }
}

