#include "test.h"
#include "test_spec.h"

using namespace mjs;

void test_main() {
    if (tested_version() < version::es5) {
        RUN_TEST(L"global['JSON']", value::undefined);
        return;
    }

    RUN_TEST_SPEC(R"(
JSON instanceof Object; //$boolean true
Object.isExtensible(JSON); //$boolean true
Object.getOwnPropertyNames(JSON).toString(); //$string 'parse,stringify'
JSON.toString(); //$string '[object JSON]'
try { JSON(); } catch (e) { e.toString(); } //$string 'TypeError: object is not a function'

JSON.stringify.length;          //$number 3
JSON.stringify();               //$undefined
JSON.stringify(undefined);      //$undefined
JSON.stringify(function(){});   //$undefined
JSON.stringify(null);           //$string 'null'
JSON.stringify(false);          //$string 'false'
JSON.stringify(true);           //$string 'true'
JSON.stringify(42);             //$string '42'
JSON.stringify(-1.232);         //$string '-1.232'
JSON.stringify(NaN);            //$string 'null'
JSON.stringify(-Infinity);      //$string 'null'
JSON.stringify('');             //$string '""'
JSON.stringify('abc');          //$string '"abc"'
JSON.stringify('xb"\u1234\u001F"\b\f\n\r\t');  //$string '"xb\\"\u1234\\u001f\\"\\b\\f\\n\\r\\t"'
JSON.stringify(new Date(686452434213)); //$string '"1991-10-03T01:13:54.213Z"'

function J(x) { this.x = x; }
J.prototype.toJSON = function(key) { return new String(this.x + ',' + key); }
JSON.stringify(new J('xyz')); //$string '"xyz,"'

JSON.stringify(new Number(-60.12)); //$string '-60.12'
JSON.stringify(new Boolean(true)); //$string 'true'

JSON.stringify([]); //$string '[]'
JSON.stringify([1]); //$string '[1]'
JSON.stringify([1,,'x']); //$string '[1,null,"x"]'
JSON.stringify([[1,2],[3,4]]); //$string '[[1,2],[3,4]]'

try { a=[]; a[0]=a; JSON.stringify(a); } catch (e) { e.toString(); } //$string 'TypeError: Object must be acyclic'

try { o={}; o.x=o; JSON.stringify(o); } catch (e) { e.toString(); } //$string 'TypeError: Object must be acyclic'

JSON.stringify({}); //$string '{}'
JSON.stringify({a:1,b:'x'}); //$string '{"a":1,"b":"x"}'

o = {x:42};
Object.defineProperty(o, 'y', {enumerable:true,value:60});
Object.defineProperty(o, 'z', {enumerable:false,value:61});
JSON.stringify(o); //$string '{"x":42,"y":60}'

JSON.stringify({if:{a:42},x:{b:'x'}}); //$string '{"if":{"a":42},"x":{"b":"x"}}'

JSON.stringify([], null, null);                 //$string '[]'
JSON.stringify([], null, {});                   //$string '[]'
JSON.stringify([], null, 42);                   //$string '[]'
JSON.stringify([], null, true);                 //$string '[]'
JSON.stringify([], null, new Number(42));       //$string '[]'
JSON.stringify([], null, ' ');                  //$string '[]'
JSON.stringify([], null, new String(' '));      //$string '[]'

JSON.stringify([1,2], null, null);              //$string '[1,2]'
JSON.stringify([1,2], null, true);              //$string '[1,2]'
JSON.stringify([1,2], null, {});                //$string '[1,2]'
JSON.stringify([1,2], null, 3);                 //$string '[\n   1,\n   2\n]'
JSON.stringify([1,2], null, 42);                //$string '[\n          1,\n          2\n]'
JSON.stringify([1,2], null, new Number(42));    //$string '[\n          1,\n          2\n]'
JSON.stringify([1,2], null, 'x \n \f');         //$string '[\nx \n \f1,\nx \n \f2\n]'
JSON.stringify([1,2], null, new String(' '));   //$string '[\n 1,\n 2\n]'
JSON.stringify([1,2], null, new String('0123456789abcdef'));   //$string '[\n01234567891,\n01234567892\n]'

JSON.stringify([[1,2],[3,4]], undefined, 4);    //$string '[\n    [\n        1,\n        2\n    ],\n    [\n        3,\n        4\n    ]\n]'

JSON.stringify({}, null, null);                 //$string '{}'
JSON.stringify({}, null, {});                   //$string '{}'
JSON.stringify({}, null, 42);                   //$string '{}'
JSON.stringify({}, null, true);                 //$string '{}'
JSON.stringify({}, null, new Number(42));       //$string '{}'
JSON.stringify({}, null, ' ');                  //$string '{}'
JSON.stringify({}, null, new String(' '));      //$string '{}'

JSON.stringify({ a: 42,b:60 }, null, null);                 //$string '{"a":42,"b":60}'
JSON.stringify({ a: 42,b:60 }, null, {});                   //$string '{"a":42,"b":60}'
JSON.stringify({ a: 42,b:60 }, null, 42);                   //$string '{\n          "a": 42,\n          "b": 60\n}'
JSON.stringify({ a: 42,b:60 }, null, true);                 //$string '{"a":42,"b":60}'
JSON.stringify({ a: 42,b:60 }, null, new Number(42));       //$string '{\n          "a": 42,\n          "b": 60\n}'
JSON.stringify({ a: 42,b:60 }, null, ' ');                  //$string '{\n "a": 42,\n "b": 60\n}'
JSON.stringify({ a: 42,b:60 }, null, new String(' '));      //$string '{\n "a": 42,\n "b": 60\n}'
JSON.stringify({ a: 42,b:60 }, null, new String('\n\r\f\t1234534341231')); //$string '{\n\n\r\f\t123453"a": 42,\n\n\r\f\t123453"b": 60\n}'

function fmt(n, v) { return n+'='+v+':'+typeof v;}
function r(k,v) { return JSON.stringify(this)+','+fmt(k,v); }
JSON.stringify(undefined, r); //$string '"{},=undefined:undefined"'
JSON.stringify({}, r); //$string '"{\\"\\":{}},=[object Object]:object"'
JSON.stringify('x', r); //$string '"{\\"\\":\\"x\\"},=x:string"'
JSON.stringify({a:1,2:'xz'}, r); //$string '"{\\"\\":{\\"a\\":1,\\"2\\":\\"xz\\"}},=[object Object]:object"'
JSON.stringify({ax:42,232:'xyz'}, r, 'dssdsds\ndsdsd23213'); //$string '"{\\"\\":{\\"ax\\":42,\\"232\\":\\"xyz\\"}},=[object Object]:object"'

function r2(k,v) { return typeof v !== 'object' ? fmt(v) : v; }
JSON.stringify({}, r2); //$string '{}'
JSON.stringify({ax:42,232:'xyz'}, r2, 'dssdsds\ndsdsd23213'); //$string '{\ndssdsds\nds"ax": "42=undefined:undefined",\ndssdsds\nds"232": "xyz=undefined:undefined"\n}'
JSON.stringify([1,2,{a:3}],r2,2); //$string '[\n  "1=undefined:undefined",\n  "2=undefined:undefined",\n  {\n    "a": "3=undefined:undefined"\n  }\n]'
JSON.stringify({x:[new Date(686452434213),2],y:'abc'},r2,'   xs'); //$string '{\n   xs"x": [\n   xs   xs"1991-10-03T01:13:54.213Z=undefined:undefined",\n   xs   xs"2=undefined:undefined"\n   xs],\n   xs"y": "abc=undefined:undefined"\n}'

JSON.stringify({a:1,b:2,c:3,d:4,42:'x',43:'y',44:'z'}, ['qq',60]); //$string '{}'
JSON.stringify({a:1,b:2,c:3,d:4,42:'x',43:'y',44:'z'}, [new String('a'),'c','c',new Number(42),43]); //$string '{"a":1,"c":3,"42":"x","43":"y"}'
JSON.stringify({a:{a:42,b:43},b:{a:59,b:60}}, ['b']); //$string '{"b":{"b":60}}'


JSON.parse.length;//$number 2

try { JSON.parse(); } catch (e) { e.toString(); }               //$string 'SyntaxError: Unexpected token "u" in JSON at position 0'
try { JSON.parse(''); } catch (e) { e.toString(); }             //$string 'SyntaxError: Unexpected end of input at position 0'
try { JSON.parse('nu'); } catch (e) { e.toString(); }           //$string 'SyntaxError: Unexpected token "n" in JSON at position 0'
try { JSON.parse('null null'); } catch (e) { e.toString(); }    //$string 'SyntaxError: Unexpected token null in JSON at position 5'
try { JSON.parse('-'); } catch (e) { e.toString(); }            //$string 'SyntaxError: Invalid number in JSON at position 0'
try { JSON.parse('--'); } catch (e) { e.toString(); }           //$string 'SyntaxError: Invalid number in JSON at position 0'
try { JSON.parse('-.'); } catch (e) { e.toString(); }           //$string 'SyntaxError: Invalid number in JSON at position 0'
try { JSON.parse('.'); } catch (e) { e.toString(); }            //$string 'SyntaxError: Unexpected token "." in JSON at position 0'
try { JSON.parse('.1'); } catch (e) { e.toString(); }           //$string 'SyntaxError: Unexpected token "." in JSON at position 0'
try { JSON.parse('0.1e'); } catch (e) { e.toString(); }         //$string 'SyntaxError: Invalid number in JSON at position 0'
try { JSON.parse('0.1e'); } catch (e) { e.toString(); }         //$string 'SyntaxError: Invalid number in JSON at position 0'
try { JSON.parse('0.1e-'); } catch (e) { e.toString(); }        //$string 'SyntaxError: Invalid number in JSON at position 0'
try { JSON.parse('0.1e-.'); } catch (e) { e.toString(); }       //$string 'SyntaxError: Invalid number in JSON at position 0'
try { JSON.parse(' "xxx'); } catch (e) { e.toString(); }        //$string 'SyntaxError: Unterminated string in JSON at position 1'
try { JSON.parse('"\\"'); } catch (e) { e.toString(); }         //$string 'SyntaxError: Unterminated escape sequence in JSON at position 2'
try { JSON.parse('"\\u123"'); } catch (e) { e.toString(); }     //$string 'SyntaxError: Unterminated escape sequence in JSON at position 2'
try { JSON.parse('"\\u123q"'); } catch (e) { e.toString(); }    //$string 'SyntaxError: Invalid escape sequence in JSON at position 2'
try { JSON.parse('"\\q"'); } catch (e) { e.toString(); }        //$string 'SyntaxError: Invalid escape sequence "\\q" in JSON at position 2'

JSON.parse('null'); //$null
JSON.parse('false'); //$boolean false
JSON.parse('true'); //$boolean true
JSON.parse('1'); //$number 1
JSON.parse('  \t\n\r 23  '); //$number 23
JSON.parse('-42.256'); //$number -42.256
JSON.parse('-1E+3'); //$number -1e3
JSON.parse('-9.23232e-2'); //$number -9.23232e-2
JSON.parse('""'); //$string ''
JSON.parse('"Hello world!"'); //$string 'Hello world!'
JSON.parse('"x\\u001f\\"\\/\\\\\\b\\f\\n\\r\\t \\u1234"'); //$string 'x\u001f"/\\\b\f\n\r\t \u1234'

a=JSON.parse(" [ \n  ]"); Array.isArray(a); //$boolean true
a.toString(); //$string ''

a=JSON.parse(" [ 1,null,3,\"x\"  ]"); Array.isArray(a); //$boolean true
a[0];//$number 1
a[1];//$null
a[2];//$number 3
a[3];//$string 'x'
a.toString(); //$string '1,,3,x'

a=JSON.parse(" [ [], [1,2], [3,4] ] ");
a.length;//$number 3
a[0].length;//$number 0
a[1].length;//$number 2
a[2].length;//$number 2
a.toString(); //$string ',1,2,3,4'

try { JSON.parse('[ '); } catch (e) { e.toString(); }        //$string 'SyntaxError: Unexpected end of input at position 2'
try { JSON.parse('[1,2,] '); } catch (e) { e.toString(); }   //$string 'SyntaxError: Unexpected token rbracket in JSON at position 5'

o = JSON.parse(" {   \n\r\t }"); o instanceof Object;//$boolean true
Object.getOwnPropertyNames(o).toString(); //$string ''

o = JSON.parse(" {\"a\":42, \"x\": \"abcd\", \"z\" \n: [1,2,3]}"); o instanceof Object;//$boolean true
Object.getOwnPropertyNames(o).toString(); //$string 'a,x,z'
o.z.toString();//$string '1,2,3'

o = JSON.parse("{\"x\":42,\"x\":43,\"x\":44}");
o.x;//$number 44

try { JSON.parse('{'); } catch (e) { e.toString(); }            //$string 'SyntaxError: Unexpected end of input at position 1'
try { JSON.parse('{\"x\" 42}'); } catch (e) { e.toString(); }   //$string 'SyntaxError: Unexpected token 42 in JSON at position 5'
try { JSON.parse('{\"x\":}'); } catch (e) { e.toString(); }     //$string 'SyntaxError: Unexpected token rbrace in JSON at position 5'
try { JSON.parse('{\"x\":2,}'); } catch (e) { e.toString(); }   //$string 'SyntaxError: Unexpected token rbrace in JSON at position 7'


JSON.parse("42",undefined);     //$number 42
JSON.parse("42",null);          //$number 42
JSON.parse("42",false);         //$number 42
JSON.parse("42",42);            //$number 42
JSON.parse("42",'x');           //$number 42
JSON.parse("42",{foo:"bar"});   //$number 42


function fmt(x) { return x+':'+typeof x; }

s=''; r=JSON.parse("42", function(k,v) {s+=fmt(this[''])+','+fmt(k)+','+fmt(v); return 'x';}); r;//$string 'x'
s; //$string '42:number,:string,42:number'

s=''; r=JSON.parse("[1,null,2]", function(k,v) { s+=this.toString()+','+fmt(k)+','+fmt(v)+' '; return v;}); r.toString(); //$string '1,,2'
s;//$string '1,,2,0:string,1:number 1,,2,1:string,null:object 1,,2,2:string,2:number [object Object],:string,1,,2:object '

r=JSON.parse("[0,1,\"x\",\"z\"]", function(k,v) { return k%2==0 ? v : undefined; }); r.toString(); //$string '0,,x,'
0 in r;//$boolean true
1 in r;//$boolean false
2 in r;//$boolean true
3 in r;//$boolean false

s=''; r=JSON.parse("{\"x\":42, \"y\":{ }}", function(k,v) {s+=JSON.stringify(this)+','+fmt(k)+','+fmt(v); return v;});
JSON.stringify(r);//$string '{"x":42,"y":{}}'
s; //$string '{"x":42,"y":{}},x:string,42:number{"x":42,"y":{}},y:string,[object Object]:object{"":{"x":42,"y":{}}},:string,[object Object]:object'

r=JSON.parse('{"x":42,"y":43,"z":44}', function(k,v){return v%2==0?v:undefined;}); r;//$undefined

r=JSON.parse('{"x":42,"y":43,"z":44}', function(k,v){return typeof v == "object"||v%2==0?v:undefined;});
JSON.stringify(r);//$string '{"x":42,"z":44}'

)");

}
