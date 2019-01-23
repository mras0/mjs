//
// Taken from https://github.com/kangax/compat-table
// License for this file:
//
// Copyright (c) 2010-2013 Juriy Zaytsev
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE. 

tests = [
    {
        "name": "1.1 Object/array literal extensions, Getter accessors",
        "exec": "(function () {\r\n      return ({ get x(){ return 1 } }).x === 1;\r\n    })()"
    },
    {
        "name": "1.2 Object/array literal extensions, Setter accessors",
        "exec": "(function () {\r\n      var value = 0;\r\n      ({ set x(v){ value = v; } }).x = 1;\r\n      return value === 1;\r\n    })()"
    },
    {
        "name": "1.3 Object/array literal extensions, Trailing commas in object literals",
        "exec": "(function () {\r\n      return { a: true, }.a === true;\r\n    })()"
    },
    {
        "name": "1.4 Object/array literal extensions, Trailing commas in array literals",
        "exec": "(function () {\r\n      return [1,].length === 1;\r\n    })()"
    },
    {
        "name": "1.5 Object/array literal extensions, Reserved words as property names",
        "exec": "(function () {\r\n      return ({ if: 1 }).if === 1;\r\n    })()"
    },
    {
        "name": "2.1 Object static methods, Object.create",
        "exec": "(function () {\r\n      return typeof Object.create == 'function';\r\n    })()"
    },
    {
        "name": "2.2 Object static methods, Object.defineProperty",
        "exec": "(function () {\r\n      return typeof Object.defineProperty == 'function';\r\n    })()"
    },
    {
        "name": "2.3 Object static methods, Object.defineProperties",
        "exec": "(function () {\r\n      return typeof Object.defineProperties == 'function';\r\n    })()"
    },
    {
        "name": "2.4 Object static methods, Object.getPrototypeOf",
        "exec": "(function () {\r\n      return typeof Object.getPrototypeOf == 'function';\r\n    })()"
    },
    {
        "name": "2.5 Object static methods, Object.keys",
        "exec": "(function () {\r\n      return typeof Object.keys == 'function';\r\n    })()"
    },
    {
        "name": "2.6 Object static methods, Object.seal",
        "exec": "(function () {\r\n      return typeof Object.seal == 'function';\r\n    })()"
    },
    {
        "name": "2.7 Object static methods, Object.freeze",
        "exec": "(function () {\r\n      return typeof Object.freeze == 'function';\r\n    })()"
    },
    {
        "name": "2.8 Object static methods, Object.preventExtensions",
        "exec": "(function () {\r\n      return typeof Object.preventExtensions == 'function';\r\n    })()"
    },
    {
        "name": "2.9 Object static methods, Object.isSealed",
        "exec": "(function () {\r\n      return typeof Object.isSealed == 'function';\r\n    })()"
    },
    {
        "name": "2.10 Object static methods, Object.isFrozen",
        "exec": "(function () {\r\n      return typeof Object.isFrozen == 'function';\r\n    })()"
    },
    {
        "name": "2.11 Object static methods, Object.isExtensible",
        "exec": "(function () {\r\n      return typeof Object.isExtensible == 'function';\r\n    })()"
    },
    {
        "name": "2.12 Object static methods, Object.getOwnPropertyDescriptor",
        "exec": "(function () {\r\n      return typeof Object.getOwnPropertyDescriptor == 'function';\r\n    })()"
    },
    {
        "name": "2.13 Object static methods, Object.getOwnPropertyNames",
        "exec": "(function () {\r\n      return typeof Object.getOwnPropertyNames == 'function';\r\n    })()"
    },
    {
        "name": "3.1 Array methods, Array.isArray",
        "exec": "(function () {\r\n      return typeof Array.isArray == 'function';\r\n    })()"
    },
    {
        "name": "3.2 Array methods, Array.prototype.indexOf",
        "exec": "(function () {\r\n      return typeof Array.prototype.indexOf == 'function';\r\n    })()"
    },
    {
        "name": "3.3 Array methods, Array.prototype.lastIndexOf",
        "exec": "(function () {\r\n      return typeof Array.prototype.lastIndexOf == 'function';\r\n    })()"
    },
    {
        "name": "3.4 Array methods, Array.prototype.every",
        "exec": "(function () {\r\n      return typeof Array.prototype.every == 'function';\r\n    })()"
    },
    {
        "name": "3.5 Array methods, Array.prototype.some",
        "exec": "(function () {\r\n      return typeof Array.prototype.some == 'function';\r\n    })()"
    },
    {
        "name": "3.6 Array methods, Array.prototype.forEach",
        "exec": "(function () {\r\n      return typeof Array.prototype.forEach == 'function';\r\n    })()"
    },
    {
        "name": "3.7 Array methods, Array.prototype.map",
        "exec": "(function () {\r\n      return typeof Array.prototype.map == 'function';\r\n    })()"
    },
    {
        "name": "3.8 Array methods, Array.prototype.filter",
        "exec": "(function () {\r\n      return typeof Array.prototype.filter == 'function';\r\n    })()"
    },
    {
        "name": "3.9 Array methods, Array.prototype.reduce",
        "exec": "(function () {\r\n      return typeof Array.prototype.reduce == 'function';\r\n    })()"
    },
    {
        "name": "3.10 Array methods, Array.prototype.reduceRight",
        "exec": "(function () {\r\n      return typeof Array.prototype.reduceRight == 'function';\r\n    })()"
    },
    {
        "name": "3.11 Array methods, Array.prototype.sort: compareFn must be function or undefined",
        "exec": "(function () {\r\n      try {\r\n        [1,2].sort(null);\r\n        return false;\r\n      } catch (enull) {}\r\n      try {\r\n        [1,2].sort(true);\r\n        return false;\r\n      } catch (etrue) {}\r\n      try {\r\n        [1,2].sort({});\r\n        return false;\r\n      } catch (eobj) {}\r\n      try {\r\n        [1,2].sort([]);\r\n        return false;\r\n      } catch (earr) {}\r\n      try {\r\n        [1,2].sort(/a/g);\r\n        return false;\r\n      } catch (eregex) {}\r\n      return true;\r\n    })()"
    },
    {
        "name": "3.12 Array methods, Array.prototype.sort: compareFn may be explicit undefined",
        "exec": "(function () {\r\n      try {\r\n        var arr = [2, 1];\r\n        return arr.sort(undefined) === arr && arr[0] === 1 && arr[1] === 2;\r\n      } catch (e) {\r\n        return false;\r\n      }\r\n    })()"
    },
    {
        "name": "4.1 String properties and methods, Property access on strings",
        "exec": "(function () {\r\n      return \"foobar\"[3] === \"b\";\r\n    })()"
    },
    {
        "name": "4.2 String properties and methods, String.prototype.trim",
        "exec": "(function () {\r\n      return typeof String.prototype.trim == 'function';\r\n    })()"
    },
    {
        "name": "5.1 Date methods, Date.prototype.toISOString",
        "exec": "(function () {\r\n      return typeof Date.prototype.toISOString == 'function';\r\n    })()"
    },
    {
        "name": "5.2 Date methods, Date.now",
        "exec": "(function () {\r\n      return typeof Date.now == 'function';\r\n    })()"
    },
    {
        "name": "5.3 Date methods, Date.prototype.toJSON",
        "exec": "(function () {\r\n      try {\r\n        return Date.prototype.toJSON.call(new Date(NaN)) === null;\r\n      } catch (e) {\r\n        return false;\r\n      }\r\n    })()"
    },
    {
        "name": "5 Function.prototype.bind",
        "exec": "(function () {\r\n    return typeof Function.prototype.bind == 'function';\r\n  })()"
    },
    {
        "name": "6 JSON",
        "exec": "(function () {\r\n    return typeof JSON == 'object';\r\n  })()"
    },
    {
        "name": "8.1 Immutable globals, undefined",
        "exec": "(function () {\r\n      undefined = 12345;\r\n      var result = typeof undefined == 'undefined';\r\n      undefined = void 0;\r\n      return result;\r\n    })()"
    },
    {
        "name": "8.2 Immutable globals, NaN",
        "exec": "(function () {\r\n      NaN = false;\r\n      var result = typeof NaN == 'number';\r\n      NaN = Math.sqrt(-1);\r\n      return result;\r\n    })()"
    },
    {
        "name": "8.3 Immutable globals, Infinity",
        "exec": "(function () {\r\n      Infinity = false;\r\n      var result = typeof Infinity == 'number';\r\n      Infinity = 1/0;\r\n      return result;\r\n    })()"
    },
    {
        "name": "9.1 Miscellaneous, Function.prototype.apply permits array-likes",
        "exec": "(function () {\r\n      try {\r\n        return (function(a,b) { return a === 1 && b === 2; }).apply({}, {0:1, 1:2, length:2});\r\n      } catch (e) {\r\n        return false;\r\n      }\r\n    })()"
    },
    {
        "name": "9.2 Miscellaneous, parseInt ignores leading zeros",
        "exec": "(function () {\r\n      return parseInt('010') === 10;\r\n    })()"
    },
    {
        "name": "9.3 Miscellaneous, Function \"prototype\" property is non-enumerable",
        "exec": "(function () {\r\n      return !Function().propertyIsEnumerable('prototype');\r\n    })()"
    },
    {
        "name": "9.4 Miscellaneous, Arguments toStringTag is \"Arguments\"",
        "exec": "(function () {\r\n      return (function(){ return Object.prototype.toString.call(arguments) === '[object Arguments]'; }());\r\n    })()"
    },
    {
        "name": "9.5 Miscellaneous, Zero-width chars in identifiers",
        "exec": "(function () {\r\n      var _\\u200c\\u200d = true;\r\n      return _\\u200c\\u200d;\r\n    })()"
    },
    {
        "name": "9.6 Miscellaneous, Unreserved words",
        "exec": "(function () {\r\n      var abstract, boolean, byte, char, double, final, float, goto, int, long,\r\n        native, short, synchronized, transient, volatile;\r\n      return true;\r\n    })()"
    },
    {
        "name": "9.7 Miscellaneous, Enumerable properties can be shadowed by non-enumerables",
        "exec": "(function () {\r\n      var result = true;\r\n      Object.prototype.length = 42;\r\n      for (var i in Function) {\r\n          if (i == 'length') {\r\n              result = false;\r\n          }\r\n      }\r\n      delete Object.prototype.length;\r\n      return result;\r\n    })()"
    },
    {
        "name": "9.8 Miscellaneous, Thrown functions have proper \"this\" values",
        "exec": "(function () {\r\n      try {\r\n        throw function() { return !('a' in this); };\r\n      }\r\n      catch(e) {\r\n        var a = true;\r\n        return e();\r\n      }\r\n    })()"
    },
    {
        "name": "10.1 Strict mode, reserved words",
        "exec": "(function () {\r\n      'use strict';\r\n      var words = 'implements,interface,let,package,private,protected,public,static,yield'.split(',');\r\n      for (var i = 0; i < 9; i+=1) {\r\n        try { eval('var ' + words[i]); return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      }\r\n      return true;\r\n    })()"
    },
    {
        "name": "10.2 Strict mode, \"this\" is undefined in functions",
        "exec": "(function () {\r\n      'use strict';\r\n      return this === undefined && (function(){ return this === undefined; }).call();\r\n    })()"
    },
    {
        "name": "10.3 Strict mode, \"this\" is not coerced to object in primitive methods",
        "exec": "(function () {\r\n      'use strict';\r\n      return (function(){ return typeof this === 'string' }).call('')\r\n        && (function(){ return typeof this === 'number' }).call(1)\r\n        && (function(){ return typeof this === 'boolean' }).call(true);\r\n    })()"
    },
    {
        "name": "10.4 Strict mode, \"this\" is not coerced to object in primitive accessors",
        "exec": "(function () {\r\n      'use strict';\r\n\r\n      function test(Class, instance) {\r\n        Object.defineProperty(Class.prototype, 'test', {\r\n          get: function () { passed = passed && this === instance; },\r\n          set: function () { passed = passed && this === instance; },\r\n          configurable: true\r\n        });\r\n\r\n        var passed = true;\r\n        instance.test;\r\n        instance.test = 42;\r\n        return passed;\r\n      }\r\n\r\n      return test(String, '')\r\n        && test(Number, 1)\r\n        && test(Boolean, true);\r\n    })()"
    },
    {
        "name": "10.5 Strict mode, legacy octal is a SyntaxError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { eval('010');     return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      try { eval('\"\\\\010\"'); return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      return true;\r\n    })()"
    },
    {
        "name": "10.6 Strict mode, assignment to unresolvable identifiers is a ReferenceError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { eval('__i_dont_exist = 1'); } catch (err) { return err instanceof ReferenceError; }\r\n    })()"
    },
    {
        "name": "10.7 Strict mode, assignment to eval or arguments is a SyntaxError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { eval('eval = 1');      return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      try { eval('arguments = 1'); return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      try { eval('eval++');        return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      try { eval('arguments++');   return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      return true;\r\n    })()"
    },
    {
        "name": "10.8 Strict mode, assignment to non-writable properties is a TypeError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { Object.defineProperty({},\"x\",{ writable: false }).x = 1; return false; } catch (err) { if (!(err instanceof TypeError)) return false; }\r\n      try { Object.preventExtensions({}).x = 1;                      return false; } catch (err) { if (!(err instanceof TypeError)) return false; }\r\n      try { ({ get x(){ } }).x = 1;                                  return false; } catch (err) { if (!(err instanceof TypeError)) return false; }\r\n      try { (function f() { f = 123; })();                           return false; } catch (err) { if (!(err instanceof TypeError)) return false; }\r\n      return true;\r\n    })()"
    },
    {
        "name": "10.9 Strict mode, eval or arguments bindings is a SyntaxError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { eval('var eval');                return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      try { eval('var arguments');           return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      try { eval('(function(eval){})');      return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      try { eval('(function(arguments){})'); return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      try { eval('try{}catch(eval){}');      return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      try { eval('try{}catch(arguments){}'); return false; } catch (err) { if (!(err instanceof SyntaxError)) return false; }\r\n      return true;\r\n    })()"
    },
    {
        "name": "10.10 Strict mode, arguments.caller removed or is a TypeError",
        "exec": "(function () {\r\n      'use strict';\r\n      if ('caller' in arguments) {\r\n        try { arguments.caller; return false; } catch (err) { if (!(err instanceof TypeError)) return false; }\r\n      }\r\n      return true;\r\n    })()"
    },
    {
        "name": "10.11 Strict mode, arguments.callee is a TypeError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { arguments.callee; return false; } catch (err) { if (!(err instanceof TypeError)) return false; }\r\n      return true;\r\n    })()"
    },
    {
        "name": "10.12 Strict mode, (function(){}).caller and (function(){}).arguments is a TypeError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { (function(){}).caller;    return false; } catch (err) { if (!(err instanceof TypeError)) return false; }\r\n      try { (function(){}).arguments; return false; } catch (err) { if (!(err instanceof TypeError)) return false; }\r\n      return true;\r\n    })()"
    },
    {
        "name": "10.13 Strict mode, arguments is unmapped",
        "exec": "(function () {\r\n      'use strict';\r\n      return (function(x){\r\n        x = 2;\r\n        return arguments[0] === 1;\r\n      })(1) && (function(x){\r\n        arguments[0] = 2;\r\n        return x === 1;\r\n      })(1);\r\n    })()"
    },
    {
        "name": "10.14 Strict mode, eval() can't create bindings",
        "exec": "(function () {\r\n      'use strict';\r\n      try { eval('var __some_unique_variable;'); __some_unique_variable; } catch (err) { return err instanceof ReferenceError; }\r\n    })()"
    },
    {
        "name": "10.15 Strict mode, deleting bindings is a SyntaxError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { eval('var x; delete x;'); } catch (err) { return err instanceof SyntaxError; }\r\n    })()"
    },
    {
        "name": "10.16 Strict mode, deleting non-configurable properties is a TypeError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { delete Object.prototype; } catch (err) { return err instanceof TypeError; }\r\n    })()"
    },
    {
        "name": "10.17 Strict mode, \"with\" is a SyntaxError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { eval('with({}){}'); } catch (err) { return err instanceof SyntaxError; }\r\n    })()"
    },
    {
        "name": "10.18 Strict mode, repeated parameter names is a SyntaxError",
        "exec": "(function () {\r\n      'use strict';\r\n      try { eval('function f(x, x) { }'); } catch (err) { return err instanceof SyntaxError; }\r\n    })()"
    },
    {
        "name": "10.19 Strict mode, function expressions with matching name and argument are valid",
        "exec": "(function () {\r\n      var foo = function bar(bar) {'use strict'};\r\n      return typeof foo === 'function';\r\n    })()"
    }
];

var failed = 0;
function run(exec) { try { return eval(exec); } catch (e) { return e; } }
origEval=eval;tests.forEach(function(t) {
    eval=origEval;
    var res = run(t.exec);
    console.log((res === true ? "OK   " : "FAIL ") + t.name);
    if (res !== true) {
        console.log('Result:',res);
        ++failed;
    }
});
console.assert(failed === 0);
console.log('All tests OK');
