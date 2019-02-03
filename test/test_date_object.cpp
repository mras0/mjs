#include "test.h"
#include "test_spec.h"
#include <mjs/array_object.h>

using namespace mjs;

void test_main() {
    // Timezones/locale not supported

    RUN_TEST_SPEC(R"(
typeof Date(); //$string 'string'
typeof Date(2000,0,1); //$string 'string'

new Date(1234).valueOf(); //$ number 1234
new Date(1234).getTime(); //$ number 1234
var t = Date.UTC(2000,0,1); t //$number 946684800000
var d = new Date(t);
d.valueOf(); //$ number 946684800000
d.getTime(); //$ number 946684800000
var t2 = Date.UTC(2000,0,1,12,34,56,7890); t2 //$ number 946730103890
var d2 = new Date(t2);
d2.getFullYear(); //$ number 2000
d2.getMonth(); //$ number 0
d2.getDate(); //$ number 1
d2.getDay(); //$ number 6
d2.getHours(); //$ number 12
d2.getMinutes(); //$ number 35
d2.getSeconds(); //$ number 3
d2.getMilliseconds(); //$ number 890
d2.getUTCFullYear(); //$ number 2000
d2.getUTCMonth(); //$ number 0
d2.getUTCDate(); //$ number 1
d2.getUTCDay(); //$ number 6
d2.getUTCHours(); //$ number 12
d2.getUTCMinutes(); //$ number 35
d2.getUTCSeconds(); //$ number 3
d2.getUTCMilliseconds(); //$ number 890

d2.getTimezoneOffset();//$number 0

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

d3.toString(); //$string 'Sat Jan 01 2000 12:34:56 UTC'

Date.parse('dsds'); //$number NaN

+d3; //$ number 946730096789
d3.valueOf(); //$ number 946730096789
Date.parse(d3.toString()); //$ number 946730096000
Date.parse(d3.toGMTString()); //$ number 946730096000
Date.parse(d3.toUTCString()); //$ number 946730096000
Date.parse(d3.toLocaleString()); //$ number 946730096000
d4 = new Date(d3);
+d4; //$number 946730096000

d4=new Date(+d3); d4.setTime(5678); //$ number 5678
d4.getTime(); //$ number 5678

d4=new Date(+d3); d4.setMilliseconds(123); //$ number 946730096123
+d4; //$ number 946730096123
d4=new Date(+d3); d4.setUTCMilliseconds(123); //$ number 946730096123

new Date(+d3).setUTCSeconds(9); //$number 946730049789
new Date(+d3).setUTCSeconds(9,123); //$number 946730049123
new Date(+d3).setUTCMinutes(9); //$number 946728596789
new Date(+d3).setUTCMinutes(1,2,3); //$number 946728062003
new Date(+d3).setUTCHours(9); //number 946719296789 
new Date(+d3).setUTCHours(23,22,21,9999); //number 946768950999 
new Date(+d3).setUTCDate(9); //$number 947421296789
new Date(+d3).setUTCMonth(9); //$number 970403696789
new Date(+d3).setUTCMonth(0,31); //$number 949322096789
new Date().setUTCYear; //$undefined
new Date(+d3).setUTCFullYear(1988); //$number 568038896789
new Date(+d3).setUTCFullYear(2019,0,4); //$number 1546605296789

new Date(+d3).setSeconds(9); //$number 946730049789
new Date(+d3).setMinutes(9); //$number 946728596789
new Date(+d3).setHours(9); //number 946719296789 
new Date(+d3).setDate(9); //$number 947421296789
new Date(+d3).setMonth(9); //$number 970403696789
new Date(+d3).setYear(99); //$number 915194096789
new Date(+d3).setFullYear(1988); //$number 568038896789

invalid = new Date(NaN);
invalid.toString(); //$string 'Invalid Date'
invalid.toLocaleString(); //$string 'Invalid Date'
invalid.toGMTString(); //$string 'Invalid Date'
invalid.toUTCString(); //$string 'Invalid Date'

+d3; //$ number 946730096789
)");

    if (tested_version() < version::es3) {
            RUN_TEST_SPEC(R"(
        dp = Date.prototype; dp.toDateString||dp.toTimeString||dp.toLocaleDateString||dp.toLocaleTimeString;//$undefined
        )");
    } else {
        RUN_TEST_SPEC(R"(
d = new Date(1546632840054);
d.toDateString(); //$string 'Fri Jan 04 2019'
d.toLocaleDateString(); //$string 'Fri Jan 04 2019'
d.toTimeString(); //$string '20:14:00'
d.toLocaleTimeString(); //$string '20:14:00'

invalid = new Date(NaN);
invalid.toDateString();       //$string 'Invalid Date'
invalid.toLocaleDateString(); //$string 'Invalid Date'
invalid.toTimeString();       //$string 'Invalid Date'
invalid.toLocaleTimeString(); //$string 'Invalid Date'

// ES3, 15.9.5, 15.9.5.9, 15.9.5.27

function e(func) { try { Date.prototype[func].call({}); } catch (e) { return e.toString(); } };
e('toString'); //$string 'TypeError: Object is not a Date'
e('getTime'); //$string 'TypeError: Object is not a Date'
e('setTime'); //$string 'TypeError: Object is not a Date'
e('toDateString'); //$string 'TypeError: Object is not a Date'
e('getFullYear'); //$string 'TypeError: Object is not a Date'
e('setDate'); //$string 'TypeError: Object is not a Date'

)");

    }


    if (tested_version() < version::es5) {
        RUN_TEST_SPEC(R"(
        Date.now||Date.prototype.toISOString||Date.prototype.toJSON; //$undefined
        )");
    } else {
        RUN_TEST_SPEC(R"(
var n = Date.now();
typeof n; //$string 'number'
n >= 1547231254870; //$boolean true

new Date(123456789876).toISOString(); //$string '1973-11-29T21:33:09.876Z'
try { new Date(NaN).toISOString(); } catch (e) { e.toString(); } //$string 'RangeError: Invalid Date'
Date.parse('2019-01-11T18:56:42');       //$number 1547233002000
Date.parse('2019-01-11T18:56:42Z');      //$number 1547233002000
Date.parse('2019-01-11T18:56:42.712Z');  //$number 1547233002712
Date.parse('2019-01-11T18:56:42.712Zz'); //$number NaN
Date.parse(new Date(4343242362321).toISOString()); //$number 4343242362321
Date.parse(new Date(1547239125092).toISOString()); //$number 1547239125092
Date.parse(new Date(1547233002000).toISOString()); //$number 1547233002000

n2 = Date.now();
Date.parse(new Date(n2).toISOString()) === n2; //$boolean true

Date.prototype.toJSON.length; //$number 1

new Date(NaN).toJSON();         //$null
new Date(1231231).toJSON();     //$string '1970-01-01T00:20:31.231Z'

function mkd(x) { this.x = x; }
mkd.prototype.valueOf = function() { return this.x; }
mkd.prototype.toISOString = function() { return new Date(this.x).toISOString(); }
mkd.prototype.toJSON = Date.prototype.toJSON;

new mkd(NaN).toJSON();          //$null
try { new mkd('abc').toJSON(); } catch (e) { e.toString(); } //$string 'RangeError: Invalid Date'
new mkd(1234).toJSON();         //$string '1970-01-01T00:00:01.234Z'
new mkd(123456789876).toJSON(); //$string '1973-11-29T21:33:09.876Z'
        )");

        // TODO: FIXME: Needs ES5.1, 15.3.4.3 changes to work
#if 0
        try { Date.prototype.toJSON.call(undefined); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert undefined to object'
        try { Date.prototype.toJSON.call(null); } catch (e) { e.toString(); } //$string 'TypeError: Cannot convert null to object'
#endif
    }
}