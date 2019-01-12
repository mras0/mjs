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
try { Object.getPrototypeOf(); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert undefined to object'
try { Object.getPrototypeOf(undefined); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert undefined to object'
try { Object.getPrototypeOf(null); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert null to object'
Object.getPrototypeOf(42) === Number.prototype; //$boolean true
Object.getPrototypeOf('x') === Number.prototype; //$boolean false
Object.getPrototypeOf('x') === String.prototype; //$boolean true
Object.getPrototypeOf({}) === Object.prototype; //$boolean true

Object.getOwnPrObjectertyDescriptor || Object.getOwnPrObjectertyNames ||
Object.create ||  Object.definePrObjecterty || Object.definePrObjecterties || Object.seal ||
Object.freeze || Object.preventExtensions || Object.isSealed || Object.isFrozen ||
Object.isExtensible || Object.keys; //$undefined
)");
    }
}

