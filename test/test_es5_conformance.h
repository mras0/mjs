//
// The below tests are derived from the final version of the ECMAScript 5 Conformance Suite
// (obtained early in 2019 at https://archive.codeplex.com/?p=es5conform)
//
// Copyright (c) 2009 Microsoft Corporation
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided
// that the following conditions are met:
//    * Redistributions of source code must retain the above copyright notice, this list of conditions and
//      the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
//      the following disclaimer in the documentation and/or other materials provided with the distribution.
//    * Neither the name of Microsoft nor the names of its contributors may be used to
//      endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

const char* const helper_functions = R"#(
// ----------------------------------------------
// helpers that unittests can use (typically in
// their prereq function).
// ----------------------------------------------
function fnExists(/*arguments*/) {
  for (var i=0; i<arguments.length; i++) {
     if (typeof(arguments[i]) !== "function") return false;
     }
  return true;
}

function fnGlobalObject() {
  return (function () {return this}).call(null);
  }

function compareValues(v1, v2)
{
  if (v1 === 0 && v2 === 0)
    return 1 / v1 === 1 / v2;
  if (v1 !== v1 && v2 !== v2)
    return true;
  return v1 === v2;
}

function isSubsetOf(aSubset, aArray) {
  if (aArray.length < aSubset.length) {
    return false;
  }

  var sortedSubset = [].concat(aSubset).sort();
  var sortedArray = [].concat(aArray).sort();

  nextSubsetMember:
  for (var i = 0, j = 0; i < sortedSubset.length; i++) {
    var v = sortedSubset[i];
    while (j < sortedArray.length) {
      if (compareValues(v, sortedArray[j++])) {
        continue nextSubsetMember;
      }
    }

    return false;
  }

  return true;
}
)#";

struct {
    const char* id;
    const char* description;
    const char* code;
    const char* prelude = "";
} tests[] = {
    {
        "7.8.4-1-s",
        R"(A directive preceeding an 'use strict' directive may not contain an OctalEscapeSequence)",
R"#(
  try 
  {
    eval(' "asterisk: \\052" /* octal escape sequences forbidden in strict mode*/ ; "use strict";');
    return false;
  }
  catch (e) {return e instanceof SyntaxError;  }
 )#"
    },
    {
        "8.7.2-3-1-s",
        R"(eval - a property named 'eval' is permitted)",
R"#(
  'use strict';

  var o = { eval: 42};
  return true;
 )#"
    },
    {
        "10.4.2-1-1",
        R"(Indirect call to eval has context set to global context)",
R"#(
  var _eval = eval;
  var __10_4_2_1_1_1 = "str1";
  if(_eval("\'str\' === __10_4_2_1_1_1") === true &&  // indirect eval
     eval("\'str1\' === __10_4_2_1_1_1") === true)   // direct eval
    return true;
 )#",
    R"(var __10_4_2_1_1_1 = "str";)"
    },
    {
        "10.4.2-1-2",
        R"(Indirect call to eval has context set to global context (nested function))",
R"#(
  var _eval = eval;
  var __10_4_2_1_2 = "str1";
  function foo()
  {
    var __10_4_2_1_2 = "str2";
    if(_eval("\'str\' === __10_4_2_1_2") === true &&  // indirect eval
       eval("\'str2\' === __10_4_2_1_2") === true)    // direct eval
      return true;
  }
  return foo();
 )#",
        R"(var __10_4_2_1_2 = "str";)"
    },
    {
        "10.4.2-1-3",
        R"(Indirect call to eval has context set to global context (catch block))",
R"#(

  var _eval = eval;
  var __10_4_2_1_3 = "str1";
  try
  {
    throw "error";
  }
  catch(e)  
  {
    var __10_4_2_1_3 = "str2";
    if(_eval("\'str\' === __10_4_2_1_3") === true &&  // indirect eval
       eval("\'str2\' === __10_4_2_1_3") === true)    // direct eval
      return true;
  }
 )#",
    R"(var __10_4_2_1_3 = "str";)"
    },
    {
        "10.4.2-1-4",
        R"(Indirect call to eval has context set to global context (with block))",
R"#(
  var o = new Object();  
  o.__10_4_2_1_4 = "str2";
  var _eval = eval;
  var __10_4_2_1_4 = "str1";
  with(o)
  {
    if(_eval("\'str\' === __10_4_2_1_4") === true &&  // indirect eval
       eval("\'str2\' === __10_4_2_1_4") === true)    // direct eval
      return true;
  }
 )#",
    R"(var __10_4_2_1_4 = "str";)"
    },
    {
        "10.4.2-1-5",
        R"(Indirect call to eval has context set to global context (inside another eval))",
R"#(
  var __10_4_2_1_5 = "str1";
  var r = eval("\
              var _eval = eval; \
              var __10_4_2_1_5 = \'str2\'; \
              _eval(\"\'str\' === __10_4_2_1_5 \") && \
              eval(\"\'str2\' === __10_4_2_1_5\")\
            ");   
  if(r == true)
    return true;
 )#",
    R"(var __10_4_2_1_5 = "str";)"
    },
    {
        "10.4.2-2-c-1",
        R"(Direct val code in non-strict mode - can instantiate variable in calling context)",
R"#(
  var x = 0;
  return function inner() {
     eval("var x = 1");
     if (x === 1)
        return true;
     } ();
   )#"
    },
    {
        "10.4.2-3-c-1-s",
        R"(Direct val code in strict mode - cannot instantiate variable in calling context)",
R"#(
  var x = 0;
  return function inner() {
     eval("'use strict';var x = 1");
     if (x === 0)
        return true;
     } ();
 )#"
    },
    {
        "10.4.2-3-c-2-s",
        R"(Calling code in strict mode - eval cannot instantiate variable in calling context)",
R"#(
  var x = 0;
  return function inner() {
     'use strict';
     eval("var x = 1");
     if (x === 0)
        return true;
     } ();
 )#"
    },
    {
        "10.4.3-1-1-s",
        R"(this is not coerced to an object in strict mode (Number))",
R"#(

  function foo()
  {
    'use strict';
    return typeof(this);
  } 

  function bar()
  {
    return typeof(this);
  }


  if(foo.call(1) === 'number' && bar.call(1) === 'object')  
    return true;
 )#"
    },
    {
        "10.4.3-1-2-s",
        R"(this is not coerced to an object in strict mode (string))",
R"#(

  function foo()
  {
    'use strict';
    return typeof(this);
  } 

  function bar()
  {
    return typeof(this);
  }


  if(foo.call('1') === 'string' && bar.call('1') === 'object')  
    return true;
 )#"
    },
    {
        "10.4.3-1-3-s",
        R"(this is not coerced to an object in strict mode (undefined))",
R"#(

  function foo()
  {
    'use strict';
    return typeof(this);
  } 

  function bar()
  {
    return typeof(this);
  }


  if(foo.call(undefined) === 'undefined' && bar.call() === 'object')  
    return true;
 )#"
    },
    {
        "10.4.3-1-4-s",
        R"(this is not coerced to an object in strict mode (boolean))",
R"#(

  function foo()
  {
    'use strict';
    return typeof(this);
  } 

  function bar()
  {
    return typeof(this);
  }


  if(foo.call(true) === 'boolean' && bar.call(true) === 'object')  
    return true;
 )#"
    },
    {
        "10.4.3-1-5-s",
        R"(this is not coerced to an object in strict mode (function))",
R"#(

  function foo()
  {
    'use strict';
    return typeof(this);
  } 

  function bar()
  {
    return typeof(this);
  }

  function foobar()
  {
  }

  if(foo.call(foobar) === 'function' && bar.call(foobar) === 'function')  
    return true;
 )#"
    },
    {
        "10.6-10-c-ii-1-s",
        R"(arguments[i] remains same after changing actual parameters in strict mode)",
R"#(
  function foo(a,b,c)
  {
    'use strict';
    a = 1; b = 'str'; c = 2.1;
    if(arguments[0] === 10 && arguments[1] === 'sss' && arguments[2] === 1)
      return true;   
  }
  return foo(10,'sss',1);
 )#"
    },
    {
        "10.6-10-c-ii-1",
        R"(arguments[i] change with actual parameters)",
R"#(
  function foo(a,b,c)
  {
    a = 1; b = 'str'; c = 2.1;
    if(arguments[0] === 1 && arguments[1] === 'str' && arguments[2] === 2.1)
      return true;   
  }
  return foo(10,'sss',1);
 )#"
    },
    {
        "10.6-10-c-ii-2-s",
        R"(arguments[i] doesn't map to actual parameters in strict mode)",
R"#(
  
  function foo(a,b,c)
  {
    'use strict';    
    arguments[0] = 1; arguments[1] = 'str'; arguments[2] = 2.1;
    if(10 === a && 'sss' === b && 1 === c)
      return true;   
  }
  return foo(10,'sss',1);
 )#"
    },
    {
        "10.6-10-c-ii-2",
        R"(arguments[i] map to actual parameter)",
R"#(
  
  function foo(a,b,c)
  {
    arguments[0] = 1; arguments[1] = 'str'; arguments[2] = 2.1;
    if(1 === a && 'str' === b && 2.1 === c)
      return true;   
  }
  return foo(10,'sss',1);
 )#"
    },
    {
        "10.6-12-1",
        R"(Accessing callee property of Arguments object is allowed)",
R"#(
  try 
  {
    arguments.callee;
    return true;
  }
  catch (e) {
  }
 )#"
    },
    {
        "10.6-12-2",
        R"(arguments.callee has correct attributes)",
R"#(
  
  var desc = Object.getOwnPropertyDescriptor(arguments,"callee");
  if(desc.configurable === true &&
     desc.enumerable === false &&
     desc.writable === true &&
     desc.hasOwnProperty('get') == false &&
     desc.hasOwnProperty('put') == false)
    return true;   
 )#"
    },
    {
        "10.6-13-1",
        R"(Accessing caller property of Arguments object is allowed)",
R"#(
  try 
  {
    arguments.caller;
    return true;
  }
  catch (e) {
  }
 )#"
    },
    {
        "10.6-13-b-1-s",
        R"(Accessing caller property of Arguments object throws TypeError in strict mode)",
R"#(
  'use strict';
  try 
  {
    arguments.caller;
  }
  catch (e) {
    if(e instanceof TypeError)
      return true;
  }
 )#"
    },
    {
        "10.6-13-b-2-s",
        R"(arguments.caller exists in strict mode)",
R"#(
  
  'use strict';    
  var desc = Object.getOwnPropertyDescriptor(arguments,"caller");
  return desc!== undefined;
 )#"
    },
    {
        "10.6-13-b-3-s",
        R"(arguments.caller is non-configurable in strict mode)",
R"#(
  
  'use strict';    
  var desc = Object.getOwnPropertyDescriptor(arguments,"caller");
  if(desc.configurable === false &&
     desc.enumerable === false &&
     desc.hasOwnProperty('value') == false &&
     desc.hasOwnProperty('writable') == false &&
     desc.hasOwnProperty('get') == true &&
     desc.hasOwnProperty('set') == true )
    return true;
 )#"
    },
    {
        "10.6-13-c-1-s",
        R"(Accessing callee property of Arguments object throws TypeError in strict mode)",
R"#(
  'use strict';
  try 
  {
    arguments.callee;
  }
  catch (e) {
    if(e instanceof TypeError)
      return true;
  }
 )#"
    },
    {
        "10.6-13-c-2-s",
        R"(arguments.callee is exists in strict mode)",
R"#(
  
  'use strict';    
  var desc = Object.getOwnPropertyDescriptor(arguments,"callee");
  return desc !== undefined;
 )#"
    },
    {
        "10.6-13-c-3-s",
        R"(arguments.callee is non-configurable in strict mode)",
R"#(
  
  'use strict';    
  var desc = Object.getOwnPropertyDescriptor(arguments,"callee");
  if(desc.configurable === false &&
     desc.enumerable === false &&
     desc.hasOwnProperty('value') == false &&
     desc.hasOwnProperty('writable') == false &&
     desc.hasOwnProperty('get') == true &&
     desc.hasOwnProperty('set') == true)
    return true;
 )#"
    },
    {
        "10.6-5-1",
        R"([[Prototype]] property of Arguments is set to Object prototype object)",
R"#(
  if(Object.getPrototypeOf(arguments) === Object.getPrototypeOf({}))
    return true;
 )#"
    },
    {
        "10.6-6-1",
        R"('length property of arguments object exists)",
R"#(
  
  var desc = Object.getOwnPropertyDescriptor(arguments,"length");
  return desc !== undefined
 )#"
    },
    {
        "10.6-6-2",
        R"('length' property of arguments object has correct attributes)",
R"#(
  
  var desc = Object.getOwnPropertyDescriptor(arguments,"length");
  if(desc.configurable === true &&
     desc.enumerable === false &&
     desc.writable === true )
    return true;
 )#"
    },
    {
        "10.6-6-3",
        R"('length' property of arguments object for 0 argument function exists)",
R"#(
      var arguments= undefined;
	return (function () {return arguments.length !== undefined})();
 )#"
    },
    {
        "10.6-6-4",
        R"('length' property of arguments object for 0 argument function call is 0 even with formal parameters)",
R"#(
      var arguments= undefined;
	return (function (a,b,c) {return arguments.length === 0})();
 )#"
    },
    {
        "11.1.4-0",
        R"(elements elided at the end of an array do not contribute to its length)",
R"#(
  var a = [,];
  if (a.length === 1) {
    return true;
  }
 )#"
    },
    {
        "11.1.5-0-1",
        R"(Object literal - get set property)",
R"#(
  var s1 = "In getter";
  var s2 = "In setter";
  var s3 = "Modified by setter";
  eval("var o = {get foo(){ return s1;},set foo(arg){return s2 = s3}};");
  if(o.foo !== s1) 
    return false;
  o.foo=10;
  if(s2 !== s3) 
    return false;
  return true;
 )#"
    },
    {
        "11.1.5-0-2",
        R"(Object literal - multiple get set properties)",
R"#(
  var s1 = "First getter";
  var s2 = "First setter";
  var s3 = "Second getter";
  eval("var o = {get foo(){ return s1;},set foo(arg){return s2 = s3}, get bar(){ return s3}, set bar(arg){ s3 = arg;}};");
  if(o.foo !== s1) 
    return false;
  o.foo = 10;
  if(s2 !== s3) 
    return false;
  if(o.bar !== s3)
    return false;
  o.bar = "Second setter";
  if(o.bar !== "Second setter")
    return false;
  return true;
 )#"
    },
    {
        "11.1.5@4-4-a-1-s",
        R"(Object literal - SyntaxError for duplicate date property name in strict mode)",
R"#(
  
  try
  {
    eval("'use strict'; ({foo:0,foo:1});");
  }
  catch(e)
  {
    if(e instanceof SyntaxError)
      return true;
  }
 )#"
    },
    {
        "11.1.5@4-4-a-2",
        R"(Object literal - Duplicate data property name allowd if not in strict mode)",
R"#(
  
  eval("({foo:0,foo:1});");
  return true;
  )#"
    },
    {
        "11.1.5@4-4-a-3",
        R"(Object literal - Duplicate data property name allowed gets last defined value)",
R"#(
  
  var o = eval("({foo:0,foo:1});");
  return o.foo===1;
  )#"
    },
    {
        "11.1.5@4-4-b-1",
        R"(Object literal - SyntaxError if a data property definition is followed by get accessor definition with the same name)",
R"#(
  try
  {
    eval("({foo : 1, get foo(){}});");
  }
  catch(e)
  {
    if(e instanceof SyntaxError)
      return true;
  }
 )#"
    },
    {
        "11.1.5@4-4-b-2",
        R"(Object literal - SyntaxError if a data property definition is followed by set accessor definition with the same name)",
R"#(
  try
  {
    eval("({foo : 1, set foo(x){}});");
  }
  catch(e)
  {
    if(e instanceof SyntaxError)
      return true;
  }
 )#"
    },
    {
        "11.1.5@4-4-c-1",
        R"(Object literal - SyntaxError if a get accessor property definition is followed by a data property definition with the same name)",
R"#(
  try
  {
    eval("({get foo(){}, foo : 1});");
  }
  catch(e)
  {
    if(e instanceof SyntaxError)
      return true;
  }
 )#"
    },
    {
        "11.1.5@4-4-c-2",
        R"(Object literal - SyntaxError if a set accessor property definition is followed by a data property definition with the same name)",
R"#(
  try
  {
    eval("({set foo(x){}, foo : 1});");
  }
  catch(e)
  {
    if(e instanceof SyntaxError)
      return true;
  }
 )#"
    },
    {
        "11.1.5@4-4-d-1",
        R"(Object literal - SyntaxError for duplicate property name (get,get))",
R"#(
  try
  {
    eval("({get foo(){}, get foo(){}});");
  }
  catch(e)
  {
    if(e instanceof SyntaxError)
      return true;
  }
 )#"
    },
    {
        "11.1.5@4-4-d-2",
        R"(Object literal - SyntaxError for duplicate property name (set,set))",
R"#(
  try
  {
    eval("({set foo(arg){}, set foo(arg1){}});");
  }
  catch(e)
  {
    if(e instanceof SyntaxError)
      return true;
  }
 )#"
    },
    {
        "11.1.5@4-4-d-3",
        R"(Object literal - SyntaxError for duplicate property name (get,set,get))",
R"#(
  try
  {
    eval("({get foo(){}, set foo(arg){}, get foo(){}});");
  }
  catch(e)
  {
    if(e instanceof SyntaxError)
      return true;
  }
 )#"
    },
    {
        "11.1.5@4-4-d-4",
        R"(Object literal - SyntaxError for duplicate property name (set,get,set))",
R"#(
  try
  {
    eval("({set foo(arg){}, get foo(){}, set foo(arg1){}});");
  }
  catch(e)
  {
    if(e instanceof SyntaxError)
      return true;
  }
 )#"
    },
    {
        "11.1.5@5-4-1",
        R"(Object literal - property descriptor for assignment expression)",
R"#(

  var o = {foo : 1};
  var desc = Object.getOwnPropertyDescriptor(o,"foo");
  if(desc.value === 1 &&
     desc.writable === true &&
     desc.enumerable === true &&
     desc.configurable === true)
    return true;
 )#"
    },
    {
        "11.1.5@6-3-1",
        R"(Object literal - property descriptor for get property assignment)",
R"#(

  eval("var o = {get foo(){return 1;}};");
  var desc = Object.getOwnPropertyDescriptor(o,"foo");
  if(desc.enumerable === true &&
     desc.configurable === true)
    return true;
 )#"
    },
    {
        "11.1.5@6-3-2",
        R"(Object literal - property descriptor for get property assignment should not create a set function)",
R"#(

  eval("var o = {get foo(){return 1;}};");
  var desc = Object.getOwnPropertyDescriptor(o,"foo");
  return desc.set === undefined
 )#"
    },
    {
        "11.1.5@7-3-1",
        R"(Object literal - property descriptor for set property assignment)",
R"#(

  eval("var o = {set foo(arg){return 1;}};");
  var desc = Object.getOwnPropertyDescriptor(o,"foo");
  if(desc.enumerable === true &&
     desc.configurable === true)
    return true;
 )#"
    },
    {
        "11.1.5@7-3-2",
        R"(Object literal - property descriptor for set property assignment should not create a get function)",
R"#(

  eval("var o = {set foo(arg){}};");
  var desc = Object.getOwnPropertyDescriptor(o,"foo");
  return desc.get === undefined
 )#"
    },
    {
        "11.13.1-1-1",
        R"(simple assignment throws ReferenceError if LeftHandSide is not a reference (string))",
R"#(
  try {
    eval("42 = 42");
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-1-2",
        R"(simple assignment throws ReferenceError if LeftHandSide is not a reference (string))",
R"#(
  try {
    eval("'x' = 42");
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-1-3",
        R"(simple assignment throws ReferenceError if LeftHandSide is not a reference (boolean))",
R"#(
  try {
    eval("true = 42");
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-1-4",
        R"(simple assignment throws ReferenceError if LeftHandSide is not a reference (null))",
R"#(
  try {
    eval("null = 42");
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-1-5-s",
        R"(simple assignment throws ReferenceError if LeftHandSide is an unresolvable reference in strict mode)",
R"#(
  'use strict';
  
  try {
    __ES3_1_test_suite_test_11_13_1_unique_id_0__ = 42;
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-1-6-s",
        R"(simple assignment throws ReferenceError if LeftHandSide is an unresolvable reference in strict mode (base obj undefined))",
R"#(
  'use strict';
  
  try {
    __ES3_1_test_suite_test_11_13_1_unique_id_0__.x = 42;
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-1-7-s",
        R"(simple assignment throws TypeError if LeftHandSide is a property reference with a primitive base value (this is undefined))",
R"#(
  'use strict';
  
  try {
    this.x = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-1",
        R"(simple assignment creates property on the global object if LeftHandSide is an unresolvable reference)",
R"#(
  function foo() {
    __ES3_1_test_suite_test_11_13_1_unique_id_3__ = 42;
  }
  foo();
  
  // in the browser, the "window" serves as the global object.
  if (!global['window']) global['window']=global; // In case we're not running in a browser
  var desc = Object.getOwnPropertyDescriptor(window, '__ES3_1_test_suite_test_11_13_1_unique_id_3__');
  if (desc.value === 42 &&
      desc.writable === true &&
      desc.enumerable === true &&
      desc.configurable === true) {
    delete __ES3_1_test_suite_test_11_13_1_unique_id_3__;
    return true;
  }  
 )#"
    },
    {
        "11.13.1-4-10-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.length))",
R"#(
  'use strict';

  try {
    Number.length = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-11-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Date.length))",
R"#(
  'use strict';

  try {
    Date.length = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-12-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (RegExp.length))",
R"#(
  'use strict';

  try {
    RegExp.length = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-13-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Error.length))",
R"#(
  'use strict';

  try {
    Error.length = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-14-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.MAX_VALUE))",
R"#(
  'use strict';

  try {
    Number.MAX_VALUE = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-15-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.MIN_VALUE))",
R"#(
  'use strict';

  try {
    Number.MIN_VALUE = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-16-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.NaN))",
R"#(
  'use strict';

  try {
    Number.NaN = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-17-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.NEGATIVE_INFINITY))",
R"#(
  'use strict';

  try {
    Number.NEGATIVE_INFINITY = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-18-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.POSITIVE_INFINITY))",
R"#(
  'use strict';

  try {
    Number.POSITIVE_INFINITY = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-19-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.E))",
R"#(
  'use strict';

  try {
    Math.E = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-2-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Global.NaN))",
R"#(

  function test(o) {
    'use strict';

    try {
      o.NaN = 42;
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
  }

  return test(this);
 )#"
    },
    {
        "11.13.1-4-20-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.LN10))",
R"#(
  'use strict';

  try {
    Math.LN10 = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-21-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.LN2))",
R"#(
  'use strict';

  try {
    Math.LN2 = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-22-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.LOG2E))",
R"#(
  'use strict';

  try {
    Math.LOG2E = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-23-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.LOG10E))",
R"#(
  'use strict';

  try {
    Math.LOG10E = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-24-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.PI))",
R"#(
  'use strict';

  try {
    Math.PI = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-25-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.SQRT1_2))",
R"#(
  'use strict';

  try {
    Math.SQRT1_2 = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-26-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.SQRT2))",
R"#(
  'use strict';

  try {
    Math.SQRT2 = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-27-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Global.undefined))",
R"#(

  function test(o) {
    'use strict';

    try {
      o.undefined = 42;
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
  }

  return test(this);
 )#"
    },
    {
        "11.13.1-4-3-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Global.Infinity))",
R"#(

  function test(o) {
    'use strict';

    try {
      o.Infinity = 42;
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
  }

  return test(this);
 )#"
    },
    {
        "11.13.1-4-4-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Global.length))",
R"#(

  function test(o) {
    'use strict';

    try {
      o.length = 42;
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
  }

  return test(this);
 )#"
    },
    {
        "11.13.1-4-5-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Object.length))",
R"#(
  'use strict';
  
  try {
    Object.length = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-6-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Function.length))",
R"#(
  'use strict';
  
  try {
    Function.length = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-7-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Array.length))",
R"#(
  'use strict';
  
  try {
    Array.length = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-8-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (String.length))",
R"#(
  'use strict';

  try {
    String.length = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.13.1-4-9-s",
        R"(simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Boolean.length))",
R"#(
  'use strict';

  try {
    Boolean.length = 42;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.4.1-0-1",
        R"(delete operator as UnaryExpression)",
R"#(
  var x = 1;
  var y = 2;
  var z = 3;
  
  if( (!delete x || delete y) &&
      delete delete z)
  {
    return true;
  }  
 )#"
    },
    {
        "11.4.1-2-1",
        R"(delete operator returns true when deleting a non-reference (number))",
R"#(
  var d = delete 42;
  if (d === true) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-2-2",
        R"(delete operator returns true when deleting returned value from a function)",
R"#(
  var bIsFooCalled = false;
  var foo = function(){bIsFooCalled = true;};

  var d = delete foo();
  if(d === true && bIsFooCalled === true)
    return true;
 )#"
    },
    {
        "11.4.1-2-3",
        R"(delete operator returns true when deleting a non-reference (boolean))",
R"#(
  var d = delete true;
  if (d === true) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-2-4",
        R"(delete operator returns true when deleting a non-reference (string))",
R"#(
  var d = delete "abc";
  if (d === true) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-2-5",
        R"(delete operator returns true when deleting a non-reference (obj))",
R"#(
  var d = delete {a:0} ;
  if (d === true) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-2-6",
        R"(delete operator returns true when deleting a non-reference (null))",
R"#(
  var d = delete null;
  if (d === true) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-3-1",
        R"(delete operator returns true when deleting an unresolvable reference)",
R"#(
  // just cooking up a long/veryLikely unique name
  var d = delete __ES3_1_test_suite_test_11_4_1_3_unique_id_0__;
  if (d === true) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-3-2",
        R"(delete operator throws ReferenceError when deleting an explicitly qualified yet unresolvable reference (base obj undefined))",
R"#(
  // just cooking up a long/veryLikely unique name
  try
  {
    var d = delete __ES3_1_test_suite_test_11_4_1_3_unique_id_2__.x;
  }
  catch(e)
  {
    if (e instanceof ReferenceError)
      return true;
  }
 )#"
    },
    {
        "11.4.1-3-3",
        R"(delete operator returns true when deleting an explicitly qualified yet unresolvable reference (property undefined for base obj))",
R"#(
  var __ES3_1_test_suite_test_11_4_1_3_unique_id_3__ = {};
  var d = delete __ES3_1_test_suite_test_11_4_1_3_unique_id_3__.x;
  if (d === true) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-4.a-1",
        R"(delete operator returns true when deleting a configurable data property)",
R"#(
  var o = {};

  var desc = { value: 1, configurable: true };
  Object.defineProperty(o, "foo", desc);

  var d = delete o.foo;
  if (d === true && o.hasOwnProperty("foo") === false) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-4.a-10",
        R"(delete operator returns true for property (stringify) defined on built-in object (JSON))",
R"#(
  try {
      var o = JSON.stringify;
	  var desc;
	  try {
	  	desc = Object.getOwnPropertyDescriptor(JSON, 'stringify')
	  } 
	  catch (e) {
	  };
      var d = delete JSON.stringify;
      if (d === true && JSON.stringify === undefined) {
        return true;
      }
  } finally {
    if (desc) Object.defineProperty(JSON, 'stringify', desc)
	else JSON.stringify = o  /* this branch screws up the attributes */;
  }
 )#"
    },
    {
        "11.4.1-4.a-11",
        R"(delete operator returns true on deleting arguments propterties(arguments.callee))",
R"#(
  function foo(a,b)
  {
    return (delete arguments.callee); 
  }
  var d = delete arguments.callee;
  if(d === true && arguments.callee === undefined)
    return true;
 )#"
    },
    {
        "11.4.1-4.a-12",
        R"(delete operator returns false when deleting a property(length))",
R"#(

  var a = [1,2,3]
  a.x = 10;
  var d = delete a.length
  if(d === false && a.length === 3)
    return true;
 )#"
    },
    {
        "11.4.1-4.a-13",
        R"(delete operator returns false when deleting Array object)",
R"#(

  var a = [1,2,3]
  a.x = 10;

  var d = delete a 

  if(d === false && Array.isArray(a) === true)
    return true;
 )#"
    },
    {
        "11.4.1-4.a-14",
        R"(delete operator returns true when deleting Array elements)",
R"#(

  var a = [1,2,3]
  a.x = 10;
  var d = delete a[1]
  if(d === true && a[1] === undefined)
    return true;
 )#"
    },
    {
        "11.4.1-4.a-15",
        R"(delete operator returns true when deleting Array expandos)",
R"#(

  var a = [1,2,3]
  a.x = 10;
  var d = delete a.x;
  if( d === true && a.x === undefined)
    return true;
 )#"
    },
    {
        "11.4.1-4.a-16",
        R"(delete operator returns false on deleting arguments object)",
R"#(

  if(delete arguments === false && arguments !== undefined)
    return true;
 )#"
    },
    {
        "11.4.1-4.a-17",
        R"(delete operator returns true on deleting a arguments element)",
R"#(
  function foo(a,b)
  {
    var d = delete arguments[0];
    return (d === true && arguments[0] === undefined);  
  }

  if(foo(1,2) === true)
    return true;
 )#"
    },
    {
        "11.4.1-4.a-2",
        R"(delete operator returns true when deleting a configurable accessor property)",
R"#(
  var o = {};

  // define an accessor
  // dummy getter
  var getter = function () { return 1; }
  var desc = { get: getter, configurable: true };
  Object.defineProperty(o, "foo", desc);
    
  var d = delete o.foo;
  if (d === true && o.hasOwnProperty("foo") === false) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-4.a-3-s",
        R"(delete operator throws TypeError when deleting a non-configurable data property in strict mode)",
R"#(
  'use strict';

  var o = {};
  var desc = { value : 1 }; // all other attributes default to false
  Object.defineProperty(o, "foo", desc);
  
  // Now, deleting o.foo should throw TypeError because [[Configurable]] on foo is false.
  try {
    delete o.foo;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.4.1-4.a-3",
        R"(delete operator returns false when deleting a non-configurable data property)",
R"#(
  var o = {};
  var desc = { value : 1, configurable: false }; // all other attributes default to false
  Object.defineProperty(o, "foo", desc);
  
  // Now, deleting o.foo should fail because [[Configurable]] on foo is false.
  var d = delete o.foo;
  if (d === false && o.hasOwnProperty("foo") === true) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-4.a-4-s",
        R"(delete operator throws TypeError when when deleting a non-configurable data property in strict mode (Global.NaN))",
R"#(
  'use strict';
  
  // NaN (15.1.1.1) has [[Configurable]] set to false.
  try {
    delete this.NaN;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.4.1-4.a-4",
        R"(delete operator returns false when deleting a non-configurable data property (NaN))",
R"#(
  // NaN (15.1.1.1) has [[Configurable]] set to false.
  var d = delete NaN;
  if (d === false) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-4.a-5",
        R"(delete operator returns false when deleting the environment object inside 'with')",
R"#(
  var o = new Object();
  o.x = 1;
  var d;
  with(o)
  {
    d = delete o;
  }
  if (d === false && typeof(o) === 'object' && o.x === 1) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-4.a-6",
        R"(delete operator returns true when deleting a property inside 'with')",
R"#(
  var o = new Object();
  o.x = 1;
  var d;
  with(o)
  {
    d = delete x;
  }
  if (d === true && o.x === undefined) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-4.a-7",
        R"(delete operator inside 'eval')",
R"#(
  var x = 1;
  var d = eval("delete x");
  if (d === false && x === 1) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-4.a-8",
        R"(delete operator returns true for built-in objects (JSON))",
R"#(
  try {
      var o = JSON;
      var d = delete JSON;  
      if (d === true) {	    
        return true;
      }
  } finally {
    JSON = o;
  }
 )#"
    },
    {
        "11.4.1-4.a-9-s",
        R"(delete operator throws TypeError when deleting a non-configurable data property in strict mode (Math.LN2))",
R"#(
  'use strict';
  
  try {
    delete Math.LN2;
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.4.1-4.a-9",
        R"(delete operator returns false when deleting a non-configurable data property (Math.LN2))",
R"#(
  var d = delete Math.LN2;
  if (d === false) {
    return true;
  }
 )#"
    },
    {
        "11.4.1-5-1-s",
        R"(delete operator throws ReferenceError when deleting a direct reference to a var in strict mode)",
R"#(
    /*
  'use strict';

  var x;

  // Now, deleting 'x' directly should fail throwing a ReferenceError
  // because 'x' evaluates to a strict reference;
  try {
    delete x;
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
    */
  try {
    eval('"use strict";var x;delete x;');
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.4.1-5-1",
        R"(delete operator returns false when deleting a direct reference to a var)",
R"#(
  var x = 1;

  // Now, deleting 'x' directly should fail;
  var d = delete x;
  if(d === false && x === 1)
    return true;
 )#"
    },
    {
        "11.4.1-5-2-s",
        R"(delete operator throws ReferenceError when deleting a direct reference to a function argument in strict mode)",
R"#(
    /*
  'use strict';

  function foo(a,b) {
  
    // Now, deleting 'a' directly should fail throwing a ReferenceError
    // because 'a' is direct reference to a function argument;
    try {
      delete a;
    }
    catch (e) {
      if (e instanceof ReferenceError) {
        return true;
      }
    }
  }
  return foo(1,2);
    */
  try {
    eval('"use strict";function (a,b) { delete a; }');
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.4.1-5-2",
        R"(delete operator returns false when deleting a direct reference to a function argument)",
R"#(
  
  function foo(a,b) {
  
    // Now, deleting 'a' directly should fail
    // because 'a' is direct reference to a function argument;
    var d = delete a;
    return (d === false && a === 1);
  }
  return foo(1,2);  
 )#"
    },
    {
        "11.4.1-5-3-s",
        R"(delete operator throws ReferenceError when deleting a direct reference to a function name in strict mode)",
R"#(
    /*
  'use strict';


  var foo = function(){};

  // Now, deleting 'foo' directly should fail throwing a ReferenceError
  // because 'foo' evaluates to a strict reference;
  try {
    delete foo;
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
    */
  try {
    eval('"use strict";var foo = function(){}; delete foo;');
  }
  catch (e) {
    if (e instanceof ReferenceError) {
      return true;
    }
  }
 )#"
    },
    {
        "11.4.1-5-3",
        R"(delete operator returns false when deleting a direct reference to a function name)",
R"#(
  var foo = function(){};

  // Now, deleting 'foo' directly should fail;
  var d = delete foo;
  if(d === false && fnExists(foo))
    return true;
 )#"
    },
    {
        "11.4.1-5-4-s",
        R"(delete operator throws SyntaxError when deleting a direct reference to a function argument(object) in strict mode)",
R"#(
  try {
    eval("(function (obj){'use strict'; delete obj;})(1);");
    }
  catch (e) {
    if (e instanceof SyntaxError) return true;
    }   
  )#"
    },
    {
        "12.10-0-1",
        R"(with does not change declaration scope - vars in with are visible outside)",
R"#(
  var o = {};
  var f = function () {
	/* capture foo binding before executing with */
	return foo;
      }

  with (o) {
    var foo = "12.10-0-1";
  }

  return f()==="12.10-0-1"

 )#"
    },
    {
        "12.10-0-10",
        R"(with introduces scope - name lookup finds function parameter)",
R"#(
  function f(o) {

    function innerf(o, x) {
      with (o) {
        return x;
      }
    }

    return innerf(o, 42);
  }
  
  if (f({}) === 42) {
    return true;
  }
 )#"
    },
    {
        "12.10-0-11",
        R"(with introduces scope - name lookup finds inner variable)",
R"#(
  function f(o) {

    function innerf(o) {
      var x = 42;

      with (o) {
        return x;
      }
    }

    return innerf(o);
  }
  
  if (f({}) === 42) {
    return true;
  }
 )#"
    },
    {
        "12.10-0-12",
        R"(with introduces scope - name lookup finds property)",
R"#(
  function f(o) {

    function innerf(o) {
      with (o) {
        return x;
      }
    }

    return innerf(o);
  }
  
  if (f({x:42}) === 42) {
    return true;
  }
 )#"
    },
    {
        "12.10-0-3",
        R"(with introduces scope - that is captured by function expression)",
R"#(
  var o = {prop: "12.10-0-3 before"};
  var f;

  with (o) {
    f = function () { return prop; }
  }
  o.prop = "12.10-0-3 after";
  return f()==="12.10-0-3 after"
 )#"
    },
    {
        "12.10-0-7",
        R"(with introduces scope - scope removed when exiting with statement)",
R"#(
  var o = {foo: 1};

  with (o) {
    foo = 42;
  }

  try {
    foo;
  }
  catch (e) {
     return true;
  }
 )#"
    },
    {
        "12.10-0-8",
        R"(with introduces scope - var initializer sets like named property)",
R"#(
  var o = {foo: 42};

  with (o) {
    var foo = "set in with";
  }

  return o.foo === "set in with";
 )#"
    },
    {
        "12.10-0-9",
        R"(with introduces scope - name lookup finds outer variable)",
R"#(
  function f(o) {
    var x = 42;

    function innerf(o) {
      with (o) {
        return x;
      }
    }

    return innerf(o);
  }
  
  if (f({}) === 42) {
    return true;
  }
 )#"
    },
    {
        "12.10-2-1",
        R"(with - expression being Number)",
R"#(
  var o = 2;
  var foo = 1;
  try
  {
    with (o) {
      foo = 42;
    }
  }
  catch(e)
  {
  }
  return true;
  
 )#"
    },
    {
        "12.10-2-2",
        R"(with - expression being Boolean)",
R"#(
  var o = true;
  var foo = 1;
  try
  {
    with (o) {
      foo = 42;
    }
  }
  catch(e)
  {
  }
  return true;
  
 )#"
    },
    {
        "12.10-2-3",
        R"(with - expression being string)",
R"#(
  var o = "str";
  var foo = 1;
  try
  {
    with (o) {
      foo = 42;
    }
  }
  catch(e)
  {
  }
  return true;
  
 )#"
    },
    {
        "12.10-7-1",
        R"(with introduces scope - restores the earlier environment on exit)",
R"#(
  var a = 1;

  var o = {a : 2};
  try
  {
    with (o) {
      a = 3;
      throw 1;
      a = 4;
    }
  }
  catch(e)
  {}

  if (a === 1 && o.a === 3) {
      return true;
  }

 )#"
    },
    {
        "12.10.1-1-s",
        R"(with statement in strict mode throws SyntaxError (strict function))",
R"#(
  try {
    // wrapping it in eval since this needs to be a syntax error. The
    // exception thrown must be a SyntaxError exception.
    eval("\
          function f() {\
            \'use strict\';\
            var o = {}; \
            with (o) {};\
          }\
        ");
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.10.1-10-s",
        R"(with statement in strict mode throws SyntaxError (eval, where the container function is strict))",
R"#(
  'use strict';
  
  // wrapping it in eval since this needs to be a syntax error. The
  // exception thrown must be a SyntaxError exception. Note that eval
  // inherits the strictness of its calling context.  
  try {
    eval("\
          var o = {};\
          with (o) {}\
       ");
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.10.1-12-s",
        R"(with statement in strict mode throws SyntaxError (strict eval))",
R"#(
  try {
    eval("\
          'use strict'; \
          var o = {}; \
          with (o) {}\
        ");
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.10.1-2-s",
        R"(with statement in strict mode throws SyntaxError (nested function where container is strict))",
R"#(
  try {
    // wrapping it in eval since this needs to be a syntax error. The
    // exception thrown must be a SyntaxError exception.
    eval("\
          function foo() {\
            \'use strict\'; \
            function f() {\
                var o = {}; \
                with (o) {};\
            }\
          }\
        ");
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.10.1-3-s",
        R"(with statement in strict mode throws SyntaxError (nested strict function))",
R"#(
  try {
    // wrapping it in eval since this needs to be a syntax error. The
    // exception thrown must be a SyntaxError exception.
    eval("\
            function foo() {\
                function f() {\
                  \'use strict\'; \
                  var o = {}; \
                  with (o) {};\
                }\
              }\
        ");
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.10.1-4-s",
        R"(with statement in strict mode throws SyntaxError (strict Function))",
R"#(
  try {
    var f = Function("\
                      \'use strict\';  \
                      var o = {}; \
                      with (o) {};\
                    ");
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.10.1-5-s",
        R"(with statement allowed in nested Function even if its container Function is strict))",
R"#(
  Function("\
            \'use strict\';\
            var f1 = Function(\
                            \"var o = {}; \
                            with (o) {};\
                            \")\
          ");
  return true;
 )#"
    },
    {
        "12.10.1-7-s",
        R"(with statement in strict mode throws SyntaxError (function expression, where the container function is direct evaled from strict code))",
R"#(
  'use strict';

  try {
    eval("var f = function () {\
                var o = {}; \
                with (o) {}; \
             }\
        ");
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.10.1-8-s",
        R"(with statement in strict mode throws SyntaxError (function expression, where the container Function is strict))",
R"#(
  try {
    Function("\
              \'use strict\'; \
              var f1 = function () {\
                  var o = {}; \
                  with (o) {}; \
                }\
            ");
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.10.1-9-s",
        R"(with statement in strict mode throws SyntaxError (strict function expression))",
R"#(
  try {
    eval("\
          var f = function () {\
                \'use strict\';\
                var o = {}; \
                with (o) {}; \
              }\
        ");
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.14-1",
        R"(catch doesn't change declaration scope - var initializer in catch with same name as catch parameter changes parameter)",
R"#(
  foo = "prior to throw";
  try {
    throw new Error();
  }
  catch (foo) {
    var foo = "initializer in catch";
  }
 return foo === "prior to throw";
  
 )#"
    },
    {
        "12.14-10",
        R"(catch introduces scope - name lookup finds function parameter)",
R"#(
  function f(o) {

    function innerf(o, x) {
      try {
        throw o;
      }
      catch (e) {
        return x;
      }
    }

    return innerf(o, 42);
  }
  
  if (f({}) === 42) {
    return true;
  }
 )#"
    },
    {
        "12.14-11",
        R"(catch introduces scope - name lookup finds inner variable)",
R"#(
  function f(o) {

    function innerf(o) {
      var x = 42;

      try {
        throw o;
      }
      catch (e) {
        return x;
      }
    }

    return innerf(o);
  }
  
  if (f({}) === 42) {
    return true;
  }
 )#"
    },
    {
        "12.14-12",
        R"(catch introduces scope - name lookup finds property)",
R"#(
  function f(o) {

    function innerf(o) {
      try {
        throw o;
      }
      catch (e) {
        return e.x;
      }
    }

    return innerf(o);
  }
  
  if (f({x:42}) === 42) {
    return true;
  }
 )#"
    },
    {
        "12.14-13",
        R"(catch introduces scope - updates are based on scope)",
R"#(
  var res1 = false;
  var res2 = false;
  var res3 = false;

  var x_12_14_13 = 'local';

  function foo() {
    this.x_12_14_13  = 'instance';
  }

  try {
    throw foo;
  }
  catch (e) {
    res1 = (x_12_14_13  === 'local');
    e();
    res2 = (x_12_14_13  === 'local');
  }
  res3 = (x_12_14_13  === 'local');
  
  if (res1 === true &&
      res2 === true &&
      res3 === true) {
    return true;
  }
 )#"
    },
    {
        "12.14-2",
        R"(catch doesn't change declaration scope - var initializer in catch with same name as catch parameter changes parameter)",
R"#(
  function capturedFoo() {return foo};
  foo = "prior to throw";
  try {
    throw new Error();
  }
  catch (foo) {
    var foo = "initializer in catch";
    return capturedFoo() !== "initializer in catch";
  }
  
 )#"
    },
    {
        "12.14-3",
        R"(catch doesn't change declaration scope - var declaration are visible outside when name different from catch parameter)",
R"#(
  try {
    throw new Error();
  }
  catch (e) {
    var foo = "declaration in catch";
  }
  
  return foo === "declaration in catch";
 )#"
    },
    {
        "12.14-4",
        R"(catch introduces scope - block-local vars must shadow outer vars)",
R"#(
  var o = { foo : 42};

  try {
    throw o;
  }
  catch (e) {
    var foo;

    if (foo === undefined) {
      return true;
    }
  }
 )#"
    },
    {
        "12.14-5",
        R"(catch introduces scope - block-local functions must shadow outer functions)",
R"#(
  var o = {foo: function () { return 42;}};

  try {
    throw o;
  }
  catch (e) {
    function foo() {}
    if (foo() === undefined) {
      return true;
    }
  }
 )#"
    },
    {
        "12.14-6",
        R"(catch introduces scope - block-local function expression must shadow outer function expression)",
R"#(
  var o = {foo : function () { return 42;}};

  try {
    throw o;
  }
  catch (e) {
    var foo = function () {};
    if (foo() === undefined) {
      return true;
    }
  }
 )#"
    },
    {
        "12.14-7",
        R"(catch introduces scope - scope removed when exiting catch block)",
R"#(
  var o = {foo: 1};

  try {
    throw o;
  }
  catch (e) {
    var f = e.foo;
  }

  try {
    foo;
  }
  catch (e) {
    // actually, we need to have thrown a ReferenceError exception.
    // However, in JScript we have thrown a TypeError exception.
    // But that is a separate test.
    return true;
  }
 )#"
    },
    {
        "12.14-8",
        R"(catch introduces scope - scope removed when exiting catch block (properties))",
R"#(
  var o = {foo: 42};

  try {
    throw o;
  }
  catch (e) {
    var foo = 1;
  }

  if (o.foo === 42) {
    return true;
  }
 )#"
    },
    {
        "12.14-9",
        R"(catch introduces scope - name lookup finds outer variable)",
R"#(
  function f(o) {
    var x = 42;

    function innerf(o) {
      try {
        throw o;
      }
      catch (e) {
        return x;
      }
    }

    return innerf(o);
  }
  
  if (f({}) === 42) {
    return true;
  }
 )#"
    },
    {
        "12.2.1-1-s",
        R"(eval - a function declaring a var named 'eval' throws SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    eval('function foo() { var eval; }');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.2.1-10-s",
        R"(eval - an indirect eval assigning into 'eval' throws SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    var s = eval;
    s('eval = 42;');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.2.1-11",
        R"(arguments as var identifier in eval code is allowed)",
R"#(
    eval("var arguments;");
    return true;
 )#"
    },
    {
        "12.2.1-12-s",
        R"(arguments as local var identifier throws SyntaxError in strict mode)",
R"#(
    
    try {eval("(function (){'use strict'; var arguments;});");}
    catch (e) {
       return e instanceof SyntaxError;}
 )#"
    },
    {
        "12.2.1-12",
        R"(arguments as local var identifier is allowed)",
R"#(
    eval("(function (){var arguments;})");
    return true;
 )#"
    },
    {
        "12.2.1-13-s",
        R"(arguments as global var identifier throws SyntaxError in strict mode)",
R"#(
    var indirectEval = eval;
    try {indirectEval("'use strict'; var arguments;");}
    catch (e) {
       return e instanceof SyntaxError;}
 )#"
    },
    {
        "12.2.1-13",
        R"(arguments as global var identifier is allowed)",
R"#(
    var indirectEval = eval;
    indirectEval("var arguments;");
    return true;
 )#"
    },
    {
        "12.2.1-2-s",
        R"(eval - a function assigning into 'eval' throws SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    eval('function foo() { eval = 42; }; foo()');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.2.1-3-s",
        R"(eval - a function expr declaring a var named 'eval' throws SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    eval('(function () { var eval; })');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.2.1-4-s",
        R"(eval - a function expr assigning into 'eval' throws a SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    eval('(function () { eval = 42; })()');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.2.1-5-s",
        R"(eval - a Function declaring var named 'eval' throws SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    Function('var eval;');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.2.1-6-s",
        R"(eval - a Function assigning into 'eval' throws SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    var f = Function('eval = 42;');
    f();
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.2.1-7-s",
        R"(eval - a direct eval declaring a var named 'eval' throws SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    eval('var eval;');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.2.1-8-s",
        R"(eval - a direct eval assigning into 'eval' throws SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    eval('eval = 42;');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "12.2.1-9-s",
        R"(eval - an indirect eval declaring a var named 'eval' throws SyntaxError in strict mode)",
R"#(
  'use strict';

  try {
    var s = eval;
    s('var eval;');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-1-1-s",
        R"(Duplicate identifier throw SyntaxError in strict function declaration parameter list)",
R"#(
  
  try 
  {
    eval('"use strict"; function foo(a,a){}');
  }
  catch (e) {
    if(e instanceof SyntaxError)
    {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-1-1",
        R"(Duplicate identifier allowed in non-strict function declaration parameter list)",
R"#(
  try 
  {
    eval('function foo(a,a){}');
    return true;
  }
  catch (e) { return false }
  )#"
    },
    {
        "13.1-1-2-s",
        R"(Duplicate identifier throw SyntaxError in strict function expression parameter list)",
R"#(
  
  try 
  {
    eval('"use strict"; (function foo(a,a){})');
  }
  catch (e) {
    if(e instanceof SyntaxError)
    {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-1-2",
        R"(Duplicate identifier allowed in non-strict function expression parameter list)",
R"#(
  try 
  {
    eval('(function foo(a,a){})');
    return true;
  }
  catch (e) { return false }
  )#"
    },
    {
        "13.1-2-1-s",
        R"(eval - a function having a formal parameter named 'eval' throws EvalError in strict mode)",
R"#(
  'use strict';

  try {
    eval('"use strict"; function foo(eval) {}');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-2-1",
        R"(eval allowed as formal parameter name of a non-strict function declaration)",
R"#(
  try 
  {
    eval("function foo(eval){};");
    return true;
  }
  catch (e) {  }
 )#"
    },
    {
        "13.1-2-2-s",
        R"(eval - a function expression having a formal parameter named 'eval' throws EvalError in strict mode)",
R"#(
  try {
    eval('"use strict"; (function foo(eval) {});');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-2-2",
        R"(eval allowed as formal parameter name of a non-strict function expression)",
R"#(
    eval("(function foo(eval){});");
    return true;
 )#"
    },
    {
        "13.1-2-3-s",
        R"(eval - a function expr having a formal parameter named 'eval' throws SyntaxError if strict mode body)",
R"#(
  try {
    eval('(function (eval) {"use strict"; })');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-2-4-s",
        R"(arguments - a function declaration having a formal parameter named 'arguments' throws SyntaxError if strict mode body)",
R"#(
  try {
    eval(' function foo(arguments) {"use strict";}');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-2-5-s",
        R"(arguments- a function having a formal parameter named 'arguments' throws EvalError in strict mode)",
R"#(
  'use strict';

  try {
    eval('"use strict"; function foo(arguments) {}');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-2-5",
        R"(arguments allowed as formal parameter name of a non-strict function declaration)",
R"#(
  try 
  {
    eval("function foo(arguments){};");
    return true;
  }
  catch (e) {  }
 )#"
    },
    {
        "13.1-2-6-s",
        R"(arguments - a function expression having a formal parameter named 'arguments' throws EvalError in strict mode)",
R"#(
  try {
    eval('"use strict"; (function foo(arguments) {});');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-2-6",
        R"(arguments allowed as formal parameter name of a non-strict function expression)",
R"#(
    eval("(function foo(arguments){});");
    return true;
 )#"
    },
    {
        "13.1-2-7-s",
        R"(arguments - a function expr having a formal parameter named 'arguments' throws SyntaxError if strict mode body)",
R"#(
  try {
    eval('(function (arguments) {"use strict"; })');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-2-8-s",
        R"(arguments - a function declaration having a formal parameter named 'arguments' throws SyntaxError if strict mode body)",
R"#(
  try {
    eval(' function foo(arguments) {"use strict";}');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "13.1-3-1",
        R"(eval allowed as function identifier in non-strict function declaration)",
R"#(
  try 
  {
    eval("function eval(){};");
    return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "13.1-3-10-s",
        R"(SyntaxError if arguments used as function identifier in function expression with strict body)",
R"#(
  try 
  {
    eval("(function arguments (){'use strict';});");
    return false
  }
  catch (e) { return e instanceof SyntaxError; }
 )#"
    },
    {
        "13.1-3-11-s",
        R"(SyntaxError if arguments used as function identifier in function declaration in strict code)",
R"#(
  try 
  {
    eval("'use strict' ;function arguments (){};");
    return false
  }
  catch (e) { return e instanceof SyntaxError; }
 )#"
    },
    {
        "13.1-3-12-s",
        R"(SyntaxError if arguments used as function identifier in function expression in strict code)",
R"#(
  try 
  {
    eval("'use strict' ;(function arguments (){});");
    return false
  }
  catch (e) { return e instanceof SyntaxError; }
 )#"
    },
    {
        "13.1-3-2",
        R"(eval allowed as function identifier in non-strict function expression)",
R"#(
  try 
  {
    eval("(function eval(){});");
    return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "13.1-3-3-s",
        R"(SyntaxError if eval used as function identifier in function declaration with strict body)",
R"#(
  try 
  {
    eval("function eval(){'use strict';};");
    return false
  }
  catch (e) { return e instanceof SyntaxError; }
 )#"
    },
    {
        "13.1-3-4-s",
        R"(SyntaxError if eval used as function identifier in function expression with strict body)",
R"#(
  try 
  {
    eval("(function eval (){'use strict';});");
    return false
  }
  catch (e) { return e instanceof SyntaxError; }
 )#"
    },
    {
        "13.1-3-5-s",
        R"(SyntaxError if eval used as function identifier in function declaration in strict code)",
R"#(
  try 
  {
    eval("'use strict' ;function eval(){};");
    return false
  }
  catch (e) { return e instanceof SyntaxError; }
 )#"
    },
    {
        "13.1-3-6-s",
        R"(SyntaxError if eval used as function identifier in function expression in strict code)",
R"#(
  try 
  {
    eval("'use strict' ;(function eval(){});");
    return false
  }
  catch (e) { return e instanceof SyntaxError; }
 )#"
    },
    {
        "13.1-3-7",
        R"(arguments allowed as function identifier in non-strict function declaration)",
R"#(
  try 
  {
    eval("function arguments (){};");
    return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "13.1-3-8",
        R"(arguments allowed as function identifier in non-strict function expression)",
R"#(
  try 
  {
    eval("(function arguments (){});");
    return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "13.1-3-9-s",
        R"(SyntaxError if arguments used as function identifier in function declaration with strict body)",
R"#(
  try 
  {
    eval("function arguments (){'use strict';};");
    return false
  }
  catch (e) { return e instanceof SyntaxError; }
 )#"
    },
    {
        "14.1-1-s",
        R"('use strict' directive - correct usage)",
R"#(

  function foo()
  {
    'use strict';
     if(this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-10-s",
        R"(other directives - may follow 'use strict' directive )",
R"#(

  function foo()
  {
     "use strict";
     "bogus directive";
   if(this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-11-s",
        R"(comments may preceed 'use strict' directive )",
R"#(

  function foo()
  {
     // comment
     /* comment */ "use strict";

   if(this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-12-s",
        R"(comments may follow 'use strict' directive )",
R"#(

  function foo()
  {
  "use strict";    /* comment */   // comment
     
   if (this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-13-s",
        R"(semicolon insertion works for'use strict' directive )",
R"#(

  function foo()
  {
  "use strict"     
   if (this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-14-s",
        R"(semicolon insertion may come before 'use strict' directive )",
R"#(

  function foo()
  {
  "another directive"
  "use strict" ;    
   if (this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-15-s",
        R"(blank lines may come before 'use strict' directive )",
R"#(

  function foo()
  {






  "use strict" ;    
   if (this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-16",
        R"('use strict' directive - not recognized if it follow an empty statement)",
R"#(

  function foo()
  {
    ; 'use strict';
     if(this !== undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-17",
        R"('use strict' directive - not recognized if it follow some other statment empty statement)",
R"#(

  function foo()
  {
    var x;
 'use strict';
     if(this !== undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-2-s",
        R"("use strict" directive - correct usage double quotes)",
R"#(

  function foo()
  {
    "use strict";
     if(this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-3",
        R"('use strict' directive - not recognized if contains extra whitespace)",
R"#(

  function foo()
  {
    '  use    strict   ';
     if(this !== undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-4",
        R"('use strict' directive - not recognized if contains Line Continuation)",
R"#(

  function foo()
  {
    'use str\
ict';
     if(this !== undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-5",
        R"('use strict' directive - not recognized if contains a EscapeSequence)",
R"#(

  function foo()
  {
    'use\u0020strict';
     if(this !== undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-6",
        R"('use strict' directive - not recognized if contains a <TAB> instead of a space)",
R"#(

  function foo()
  {
    'use	strict';
     if(this !== undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-7",
        R"('use strict' directive - not recognized if upper case)",
R"#(

  function foo()
  {
    'Use Strict';
     if(this !== undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-8-s",
        R"('use strict' directive - may follow other directives)",
R"#(

  function foo()
  {
     "bogus directive";
     "use strict";
     if(this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "14.1-9-s",
        R"('use strict' directive - may occur multiple times)",
R"#(

  function foo()
  {
     'use strict';
     "use strict";
     if(this === undefined)
       return true;
  }

  return foo.call(undefined);
 )#"
    },
    {
        "15.1.1.1-0",
        R"(Global.NaN is a data property with default attribute values (false))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(global, 'NaN');
  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.1.1.2-0",
        R"(Global.Infinity is a data property with default attribute values (false))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(global, 'Infinity');
  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.1.1.3-0",
        R"(Global.undefined is a data property with default attribute values (false))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(global, 'undefined');
  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.10.6",
        R"(RegExp.prototype is itself a RegExp)",
R"#(
  var s = Object.prototype.toString.call(RegExp.prototype);
  if (s === '[object RegExp]') {
    return true;
  }
 )#"
    },
    {
        "15.10.7.1-1",
        R"(RegExp.prototype.source is of type String)",
R"#(
  if((typeof(RegExp.prototype.source)) === 'string')
    return true;
 )#"
    },
    {
        "15.10.7.1-2",
        R"(RegExp.prototype.source is a data property with default attribute values (false))",
R"#(
  var d = Object.getOwnPropertyDescriptor(RegExp.prototype, 'source');
  
  if (d.writable === false &&
      d.enumerable === false &&
      d.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.10.7.2-1",
        R"(RegExp.prototype.global is of type Boolean)",
R"#(
  if((typeof(RegExp.prototype.global)) === 'boolean')
    return true;
 )#"
    },
    {
        "15.10.7.2-2",
        R"(RegExp.prototype.global is a data property with default attribute values (false))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, 'global');
  
  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.10.7.3-1",
        R"(RegExp.prototype.ignoreCase is of type Boolean)",
R"#(
  if((typeof(RegExp.prototype.ignoreCase)) === 'boolean')
    return true;
 )#"
    },
    {
        "15.10.7.3-2",
        R"(RegExp.prototype.ignoreCase is a data property with default attribute values (false))",
R"#(
  var d = Object.getOwnPropertyDescriptor(RegExp.prototype, 'ignoreCase');
  
  if (d.writable === false &&
      d.enumerable === false &&
      d.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.10.7.4-1",
        R"(RegExp.prototype.multiline is of type Boolean)",
R"#(
  if((typeof(RegExp.prototype.multiline)) === 'boolean')
    return true;
 )#"
    },
    {
        "15.10.7.4-2",
        R"(RegExp.prototype.multiline is a data property with default attribute values (false))",
R"#(
  var d = Object.getOwnPropertyDescriptor(RegExp.prototype, 'multiline');
  
  if (d.writable === false &&
      d.enumerable === false &&
      d.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.10.7.5-1",
        R"(RegExp.prototype.lastIndex is of type Number)",
R"#(
  if((typeof(RegExp.prototype.lastIndex)) === 'number')
    return true;
 )#"
    },
    {
        "15.10.7.5-2",
        R"(RegExp.prototype.lastIndex is a data property with specified attribute values)",
R"#(
  var d = Object.getOwnPropertyDescriptor(RegExp.prototype, 'lastIndex');
  
  if (d.writable === true &&
      d.enumerable === false &&
      d.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.12-0-1",
        R"(JSON must be a built-in object)",
R"#(
  var o = JSON;
  if (typeof(o) === "object") {  
    return true;
  }
 )#"
    },
    {
        "15.12-0-2",
        R"(JSON must not support the [[Construct]] method)",
R"#(
  var o = JSON;

  try {
    var j = new JSON();
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.12-0-3",
        R"(JSON must not support the [[Call]] method)",
R"#(
  var o = JSON;

  try {
    var j = JSON();
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.12-0-4",
        R"(JSON object's properties must be non enumerable)",
R"#( 
  var o = JSON;
  var i = 0;
  for (var p in o) {
    i++;
  }
    
  if (i === 0) {
    return true;
  }
 )#"
    },
    {
        "15.12.1.1-0-1",
        R"(The JSON lexical grammer treats whitespace as a token seperator)",
R"#(
  
  try {
    JSON.parse('12\t\r\n 34'); // should produce a syntax error as whitespace results in two tokens
    }
  catch (e) {
      if (e.name === 'SyntaxError') return true;
      }
  )#"
    },
    {
        "15.12.1.1-0-2",
        R"(<VT> is not valid JSON whitespace as specified by the production JSONWhitespace.)",
R"#(
  
  try {
    JSON.parse('\u000b1234'); // should produce a syntax error 
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-0-3",
        R"(<FF> is not valid JSON whitespace as specified by the production JSONWhitespace.)",
R"#(
  
  try {
    JSON.parse('\u000c1234'); // should produce a syntax error 
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-0-4",
        R"(<NBSP> is not valid JSON whitespace as specified by the production JSONWhitespace.)",
R"#(
  
  try {
    JSON.parse('\u00a01234'); // should produce a syntax error 
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-0-5",
        R"(<ZWSPP> is not valid JSON whitespace as specified by the production JSONWhitespace.)",
R"#(
  
  try {
    JSON.parse('\u200b1234'); // should produce a syntax error 
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-0-6",
        R"(<BOM> is not valid JSON whitespace as specified by the production JSONWhitespace.)",
R"#(
  
  try {
    JSON.parse('\ufeff1234'); // should produce a syntax error a
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-0-7",
        R"(other category z spaces are not valid JSON whitespace as specified by the production JSONWhitespace.)",
R"#(
  
  try {
    // the following should produce a syntax error 
    JSON.parse('\u1680\u180e\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200a\u202f\u205f\u30001234'); 
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
     }
  )#"
    },
    {
        "15.12.1.1-0-8",
        R"(U+2028 and U+2029 are not valid JSON whitespace as specified by the production JSONWhitespace.)",
R"#(
  
  try {
    JSON.parse('\u2028\u20291234'); // should produce a syntax error 
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-0-9",
        R"(Whitespace characters can appear before/after any JSONtoken)",
R"#(

    JSON.parse('\t\r \n{\t\r \n'+
                   '"property"\t\r \n:\t\r \n{\t\r \n}\t\r \n,\t\r \n' +
                   '"prop2"\t\r \n:\t\r \n'+
                        '[\t\r \ntrue\t\r \n,\t\r \nnull\t\r \n,123.456\t\r \n]'+
                     '\t\r \n}\t\r \n');  // should JOSN parse without error
    return true;
  )#"
    },
    {
        "15.12.1.1-g1-1",
        R"(The JSON lexical grammer treats <TAB> as a whitespace character)",
R"#(
  if (JSON.parse('\t1234')!==1234) return false; // <TAB> should be ignored
  try {
    JSON.parse('12\t34'); // <TAB> should produce a syntax error as whitespace results in two tokens
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-g1-2",
        R"(The JSON lexical grammer treats <CR> as a whitespace character)",
R"#(
  if (JSON.parse('\r1234')!==1234) return false; // <cr> should be ignored
  try {
    JSON.parse('12\r34'); // <CR> should produce a syntax error as whitespace results in two tokens
    }
  catch (e) {
      if (e.name === 'SyntaxError') return true;
      }
  )#"
    },
    {
        "15.12.1.1-g1-3",
        R"(The JSON lexical grammer treats <LF> as a whitespace character)",
R"#(
  if (JSON.parse('\n1234')!==1234) return false; // <LF> should be ignored
  try {
    JSON.parse('12\n34'); // <LF> should produce a syntax error as whitespace results in two tokens
    }
  catch (e) {
      if (e.name === 'SyntaxError') return true;
      }
  )#"
    },
    {
        "15.12.1.1-g1-4",
        R"(The JSON lexical grammer treats <SP> as a whitespace character)",
R"#(
 if (JSON.parse(' 1234')!=1234) return false; // <SP> should be ignored
  try {
    JSON.parse('12 34'); // <SP> should produce a syntax error as whitespace results in two tokens
    }
  catch (e) {
      if (e.name === 'SyntaxError') return true;
      }
  )#"
    },
    {
        "15.12.1.1-g2-1",
        R"(JSONStrings can be written using double quotes)",
R"#(
  return JSON.parse('"abc"')==="abc"; 
  )#"
    },
    {
        "15.12.1.1-g2-2",
        R"(A JSONString may not be delimited by single quotes )",
R"#(
    try {
        if (JSON.parse("'abc'") ==='abc') return false;
       }
     catch (e) {
        return true;
        }
  )#"
    },
    {
        "15.12.1.1-g2-3",
        R"(A JSONString may not be delimited by Uncode escaped quotes )",
R"#(
    try {
        if (JSON.parse("\\u0022abc\\u0022") ==='abc') return false;
       }
     catch (e) {
        return true;
        }
  )#"
    },
    {
        "15.12.1.1-g2-4",
        R"(A JSONString must both begin and end with double quotes)",
R"#(
    try {
        if (JSON.parse('"ab'+"c'") ==='abc') return false;
       }
     catch (e) {
        return true;
        }
  )#"
    },
    {
        "15.12.1.1-g2-5",
        R"(A JSONStrings can contain no JSONStringCharacters (Empty JSONStrings))",
R"#(
  return JSON.parse('""')===""; 
  )#"
    },
    {
        "15.12.1.1-g4-1",
        R"(The JSON lexical grammer does not allow a JSONStringCharacter to be any of the Unicode characters U+0000 thru U+0007)",
R"#(
  try {
    JSON.parse('"\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007"'); // invalid string characters should produce a syntax error
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-g4-2",
        R"(The JSON lexical grammer does not allow a JSONStringCharacter to be any of the Unicode characters U+0008 thru U+000F)",
R"#(
  try {
    JSON.parse('"\u0008\u0009\u000a\u000b\u000c\u000d\u000e\u000f"'); // invalid string characters should produce a syntax error
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-g4-3",
        R"(The JSON lexical grammer does not allow a JSONStringCharacter to be any of the Unicode characters U+0010 thru U+0017)",
R"#(
  try {
    JSON.parse('"\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017"'); // invalid string characters should produce a syntax error
    }
  catch (e) {
      return true; // treat any exception as a pass, other tests ensure that JSON.parse throws SyntaxError exceptions
      }
  )#"
    },
    {
        "15.12.1.1-g4-4",
        R"(The JSON lexical grammer does not allow a JSONStringCharacter to be any of the Unicode characters U+0018 thru U+001F)",
R"#(
  try {
    JSON.parse('"\u0018\u0019\u001a\u001b\u001c\u001d\u001e\u001f"'); // invalid string characters should produce a syntax error
    }
  catch (e) {
      if (e.name === 'SyntaxError') return true;
      }
  )#"
    },
    {
        "15.12.1.1-g5-1",
        R"(The JSON lexical grammer allows Unicode escape sequences in a JSONString)",
R"#(
    return JSON.parse('"\\u0058"')==='X'; 
  )#"
    },
    {
        "15.12.1.1-g5-2",
        R"(A JSONStringCharacter UnicodeEscape may not have fewer than 4 hex characters)",
R"#(
    try {
        JSON.parse('"\\u005"') 
       }
     catch (e) {
        return e.name==='SyntaxError'
        }
  )#"
    },
    {
        "15.12.1.1-g5-3",
        R"(A JSONStringCharacter UnicodeEscape may not include any non=hex characters)",
R"#(
    try {
        JSON.parse('"\\u0X50"') 
       }
     catch (e) {
        return e.name==='SyntaxError'
        }
  )#"
    },
    {
        "15.12.1.1-g6-1",
        R"(The JSON lexical grammer allows '/' as a JSONEscapeCharacter after '' in a JSONString)",
R"#(
    return JSON.parse('"\\/"')==='/'; 
  )#"
    },
    {
        "15.12.1.1-g6-2",
        R"(The JSON lexical grammer allows '' as a JSONEscapeCharacter after '' in a JSONString)",
R"#(
    return JSON.parse('"\\\\"')==='\\'; 
  )#"
    },
    {
        "15.12.1.1-g6-3",
        R"(The JSON lexical grammer allows 'b' as a JSONEscapeCharacter after '' in a JSONString)",
R"#(
    return JSON.parse('"\\b"')==='\b'; 
  )#"
    },
    {
        "15.12.1.1-g6-4",
        R"(The JSON lexical grammer allows 'f' as a JSONEscapeCharacter after '' in a JSONString)",
R"#(
    return JSON.parse('"\\f"')==='\f'; 
  )#"
    },
    {
        "15.12.1.1-g6-5",
        R"(The JSON lexical grammer allows 'n' as a JSONEscapeCharacter after '' in a JSONString)",
R"#(
    return JSON.parse('"\\n"')==='\n'; 
  )#"
    },
    {
        "15.12.1.1-g6-6",
        R"(The JSON lexical grammer allows 'r' as a JSONEscapeCharacter after '' in a JSONString)",
R"#(
    return JSON.parse('"\\r"')==='\r'; 
  )#"
    },
    {
        "15.12.1.1-g6-7",
        R"(The JSON lexical grammer allows 't' as a JSONEscapeCharacter after '' in a JSONString)",
R"#(
    return JSON.parse('"\\t"')==='\t'; 
  )#"
    },
    {
        "15.12.2-0-1",
        R"(JSON.parse must exist as a function)",
R"#(
  var f = JSON.parse;

  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.12.2-0-2",
        R"(JSON.parse must exist as a function taking 2 parameters)",
R"#(
  var f = JSON.parse;

  if (typeof(f) === "function" && f.length === 2) {
    return true;
  }
 )#"
    },
    {
        "15.12.2-0-3",
        R"(JSON.parse must be deletable (configurable))",
R"#(
  var o = JSON;
  var desc = Object.getOwnPropertyDescriptor(o, "parse");
  return desc.configurable === true;
 )#"
    },
    {
        "15.12.3-0-1",
        R"(JSON.stringify must exist as be a function)",
R"#(
  var f = JSON.stringify;

  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.12.3-0-2",
        R"(JSON.stringify must exist as be a function taking 3 parameters)",
R"#(
  var f = JSON.stringify;

  if (typeof(f) === "function" && f.length === 3) {
    return true;
  }
 )#"
    },
    {
        "15.12.3-0-3",
        R"(JSON.stringify must be deletable (configurable))",
R"#(
  var o = JSON;
  var desc = Object.getOwnPropertyDescriptor(o, "stringify");
  if (desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.12.3-11-1",
        R"(JSON.stringify(undefined) returns undefined)",
R"#(
  return JSON.stringify(undefined) === undefined;
  )#"
    },
    {
        "15.12.3-11-10",
        R"(A JSON.stringify replacer function applied to a top level scalar value can return undefined.)",
R"#(
  return JSON.stringify(42, function(k, v) { return undefined }) === undefined;
  )#"
    },
    {
        "15.12.3-11-11",
        R"(A JSON.stringify replacer function applied to a top level Object can return undefined.)",
R"#(
  return JSON.stringify({prop:1}, function(k, v) { return undefined }) === undefined;
  )#"
    },
    {
        "15.12.3-11-12",
        R"(A JSON.stringify replacer function applied to a top level scalar can return an Array.)",
R"#(
  return JSON.stringify(42, function(k, v) { return v==42 ?[4,2]:v }) === '[4,2]';
  )#"
    },
    {
        "15.12.3-11-13",
        R"(A JSON.stringify replacer function applied to a top level scalar can return an Object.)",
R"#(
  return JSON.stringify(42, function(k, v) { return v==42 ? {forty:2}: v}) === '{"forty":2}';
  )#"
    },
    {
        "15.12.3-11-14",
        R"(Applying JSON.stringify to a  function returns undefined.)",
R"#(
  return JSON.stringify(function() {}) === undefined;
  )#"
    },
    {
        "15.12.3-11-15",
        R"(Applying JSON.stringify with a replacer function to a function returns the replacer value.)",
R"#(
  return JSON.stringify(function() {}, function(k,v) {return 99}) === '99';
  )#"
    },
    {
        "15.12.3-11-2",
        R"(A JSON.stringify replacer function works is applied to a top level undefined value.)",
R"#(
  return JSON.stringify(undefined, function(k, v) { return "replacement" }) === '"replacement"';
  )#"
    },
    {
        "15.12.3-11-3",
        R"(A JSON.stringify correctly works on top level string values.)",
R"#(
  return JSON.stringify("a string") === '"a string"';
  )#"
    },
    {
        "15.12.3-11-4",
        R"(JSON.stringify correctly works on top level Number values.)",
R"#(
  return JSON.stringify(123) === '123';
  )#"
    },
    {
        "15.12.3-11-5",
        R"(JSON.stringify correctly works on top level Boolean values.)",
R"#(
  return JSON.stringify(true) === 'true';
  )#"
    },
    {
        "15.12.3-11-6",
        R"(JSON.stringify correctly works on top level null values.)",
R"#(
  return JSON.stringify(null) === 'null';
  )#"
    },
    {
        "15.12.3-11-7",
        R"(JSON.stringify correctly works on top level Number objects.)",
R"#(
  return JSON.stringify(new Number(42)) === '42';
  )#"
    },
    {
        "15.12.3-11-8",
        R"(JSON.stringify correctly works on top level String objects.)",
R"#(
  return JSON.stringify(new String('wrappered')) === '"wrappered"';
  )#"
    },
    {
        "15.12.3-11-9",
        R"(JSON.stringify correctly works on top level Boolean objects.)",
R"#(
  return JSON.stringify(new Boolean(false)) === 'false';
  )#"
    },
    {
        "15.12.3-4-1",
        R"(JSON.stringify ignores replacer aruguments that are not functions or arrays..)",
R"#(
  try {
     return JSON.stringify([42],{})=== '[42]';
     }
   catch (e) {return  false}
  )#"
    },
    {
        "15.12.3-5-a-i-1",
        R"(JSON.stringify converts Number wrapper object space aruguments to Number values)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj,null, new Number(5))=== JSON.stringify(obj,null, 5);
  )#"
    },
    {
        "15.12.3-5-b-i-1",
        R"(JSON.stringify converts String wrapper object space aruguments to String values)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj,null, new String('xxx'))=== JSON.stringify(obj,null, 'xxx');
  )#"
    },
    {
        "15.12.3-6-a-1",
        R"(JSON.stringify treats numeric space arguments greater than 10 the same as a  space argument of 10.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj,null, 10)=== JSON.stringify(obj,null, 100);
  )#"
    },
    {
        "15.12.3-6-a-2",
        R"(JSON.stringify truccates non-integer numeric space arguments to their integer part.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj,null, 5.99999)=== JSON.stringify(obj,null, 5);
  )#"
    },
    {
        "15.12.3-6-b-1",
        R"(JSON.stringify treats numeric space arguments less than 1 (0.999999)the same as emptry string space argument.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj,null, 0.999999)=== JSON.stringify(obj);  /* emptry string should be same as no space arg */
  )#"
    },
    {
        "15.12.3-6-b-2",
        R"(JSON.stringify treats numeric space arguments less than 1 (0)the same as emptry string space argument.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj,null, 0)=== JSON.stringify(obj);  /* emptry string should be same as no space arg */
  )#"
    },
    {
        "15.12.3-6-b-3",
        R"(JSON.stringify treats numeric space arguments less than 1 (-5) the same as emptry string space argument.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj,null, -5)=== JSON.stringify(obj);  /* emptry string should be same as no space arg */
  )#"
    },
    {
        "15.12.3-6-b-4",
        R"(JSON.stringify treats numeric space arguments (in the range 1..10) is equivalent to a string of spaces of that length.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  var fiveSpaces = '     ';
  //               '12345'
  return JSON.stringify(obj,null, 5)=== JSON.stringify(obj, null, fiveSpaces);  
  )#"
    },
    {
        "15.12.3-7-a-1",
        R"(JSON.stringify only uses the first 10 characters of a string space arguments.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj,null, '0123456789xxxxxxxxx')=== JSON.stringify(obj,null, '0123456789');  
  )#"
    },
    {
        "15.12.3-8-a-1",
        R"(JSON.stringify treats an empty string space argument the same as a missing space argument.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj)=== JSON.stringify(obj,null, '');
  )#"
    },
    {
        "15.12.3-8-a-2",
        R"(JSON.stringify treats an Boolean space argument the same as a missing space argument.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj)=== JSON.stringify(obj,null, true);
  )#"
    },
    {
        "15.12.3-8-a-3",
        R"(JSON.stringify treats an null space argument the same as a missing space argument.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj)=== JSON.stringify(obj,null, null);
  )#"
    },
    {
        "15.12.3-8-a-4",
        R"(JSON.stringify treats an Boolean wrapper space argument the same as a missing space argument.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj)=== JSON.stringify(obj,null, new Boolean(true));
  )#"
    },
    {
        "15.12.3-8-a-5",
        R"(JSON.stringify treats non-Number or String object space arguments the same as a missing space argument.)",
R"#(
  var obj = {a1: {b1: [1,2,3,4], b2: {c1: 1, c2: 2}},a2: 'a2'};
  return JSON.stringify(obj)=== JSON.stringify(obj,null, obj);
  )#"
    },
    {
        "15.12.3@2-2-b-i-1",
        R"(JSON.stringify converts string wrapper objects returned from a toJSON call to literal strings.)",
R"#(
  var obj = {
    prop:42,
    toJSON: function () {return 'fortytwo objects'}
    };
  return JSON.stringify([obj]) === '["fortytwo objects"]';
  )#"
    },
    {
        "15.12.3@2-2-b-i-2",
        R"(JSON.stringify converts Number wrapper objects returned from a toJSON call to literal Number.)",
R"#(
  var obj = {
    prop:42,
    toJSON: function () {return new Number(42)}
    };
  return JSON.stringify([obj]) === '[42]';
  )#"
    },
    {
        "15.12.3@2-2-b-i-3",
        R"(JSON.stringify converts Boolean wrapper objects returned from a toJSON call to literal Boolean values.)",
R"#(
  var obj = {
    prop:42,
    toJSON: function () {return new Boolean(true)}
    };
  return JSON.stringify([obj]) === '[true]';
  )#"
    },
    {
        "15.12.3@2-3-a-1",
        R"(JSON.stringify converts string wrapper objects returned from replacer functions to literal strings.)",
R"#(
  return JSON.stringify([42], function(k,v) {return v===42? new String('fortytwo'):v}) === '["fortytwo"]';
  )#"
    },
    {
        "15.12.3@2-3-a-2",
        R"(JSON.stringify converts Number wrapper objects returned from replacer functions to literal numbers.)",
R"#(
  return JSON.stringify([42], function(k,v) {return v===42? new Number(84):v}) === '[84]';
  )#"
    },
    {
        "15.12.3@2-3-a-3",
        R"(JSON.stringify converts Boolean wrapper objects returned from replacer functions to literal numbers.)",
R"#(
  return JSON.stringify([42], function(k,v) {return v===42? new Boolean(false):v}) === '[false]';
  )#"
    },
    {
        "15.12.3@4-1-1",
        R"(JSON.stringify a circular object throws a error)",
R"#(
  var obj = {};
  obj.prop = obj;
  try {
     JSON.stringify(obj);
     return false;  // should not reach here
     }
   catch (e) {return true}
  )#"
    },
    {
        "15.12.3@4-1-2",
        R"(JSON.stringify a circular object throws a TypeError)",
R"#(
  var obj = {};
  obj.prop = obj;
  try {
     JSON.stringify(obj);
     return false;  // should not reach here
     }
   catch (e) {return e.name==='TypeError'}
  )#"
    },
    {
        "15.12.3@4-1-3",
        R"(JSON.stringify a indirectly circular object throws a error)",
R"#(
  var obj = {p1: {p2: {}}};
  obj.p1.p2.prop = obj;
  try {
     JSON.stringify(obj);
     return false;  // should not reach here
     }
   catch (e) {return  true}
  )#"
    },
    {
        "15.2.3.1",
        R"(Object.prototype is a data property with default attribute values (false))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, 'prototype');
  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.10-0-1",
        R"(Object.preventExtensions must exist as a function)",
R"#(
  var f = Object.preventExtensions;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.10-0-2",
        R"(Object.preventExtensions must exist as a function taking 1 parameter)",
R"#(
  if (Object.preventExtensions.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.10-1",
        R"(Object.preventExtensions throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.preventExtensions(0);
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.10-2",
        R"(Object.preventExtensions returns its arguments after setting its extensible property to false)",
R"#(
  var o  = {};
  var o2 = undefined;

  o2 = Object.preventExtensions(o);
  if (o2 === o && Object.isExtensible(o2) === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-0-1",
        R"(Object.isSealed must exist as a function)",
R"#(
  var f = Object.isSealed;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-0-2",
        R"(Object.isSealed must exist as a function taking 1 parameter)",
R"#(
  if (Object.isSealed.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-1",
        R"(Object.isSealed throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.isSealed(0);
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.11-4-1",
        R"(Object.isSealed returns false for all built-in objects (Global))",
R"#(
  // in non-strict mode, 'this' is bound to the global object.
  var b = Object.isSealed(this);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-4-10",
        R"(Object.isSealed returns false for all built-in objects (Boolean))",
R"#(
  var b = Object.isSealed(Boolean);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-4-11",
        R"(Object.isSealed returns false for all built-in objects (Boolean.prototype))",
R"#(
  var b = Object.isSealed(Boolean.prototype);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-12",
        R"(Object.isSealed returns false for all built-in objects (Number))",
R"#(
  var b = Object.isSealed(Number);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-13",
        R"(Object.isSealed returns false for all built-in objects (Number.prototype))",
R"#(
  var b = Object.isSealed(Number.prototype);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-14",
        R"(Object.isSealed returns false for all built-in objects (Math))",
R"#(
  var b = Object.isSealed(Math);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-15",
        R"(Object.isSealed returns false for all built-in objects (Date))",
R"#(
  var b = Object.isSealed(Date);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-16",
        R"(Object.isSealed returns false for all built-in objects (Date.prototype))",
R"#(
  var b = Object.isSealed(Date.prototype);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-17",
        R"(Object.isSealed returns false for all built-in objects (RegExp))",
R"#(
  var b = Object.isSealed(RegExp);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-18",
        R"(Object.isSealed returns false for all built-in objects (RegExp.prototype))",
R"#(
  var b = Object.isSealed(RegExp.prototype);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-19",
        R"(Object.isSealed returns false for all built-in objects (Error))",
R"#(
  var b = Object.isSealed(Error);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-2",
        R"(Object.isSealed returns false for all built-in objects (Object))",
R"#(
  var b = Object.isSealed(Object);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-4-20",
        R"(Object.isSealed returns false for all built-in objects (Error.prototype))",
R"#(
  var b = Object.isSealed(Error.prototype);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-21",
        R"(Object.isSealed returns false for all built-in objects (EvalError))",
R"#(
  var b = Object.isSealed(EvalError);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-22",
        R"(Object.isSealed returns false for all built-in objects (RangeError))",
R"#(
  var b = Object.isSealed(RangeError);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-23",
        R"(Object.isSealed returns false for all built-in objects (ReferenceError))",
R"#(
  var b = Object.isSealed(ReferenceError);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-24",
        R"(Object.isSealed returns false for all built-in objects (SyntaxError))",
R"#(
  var b = Object.isSealed(SyntaxError);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-25",
        R"(Object.isSealed returns false for all built-in objects (TypeError))",
R"#(
  var b = Object.isSealed(TypeError);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-26",
        R"(Object.isSealed returns false for all built-in objects (URIError))",
R"#(
  var b = Object.isSealed(URIError);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-27",
        R"(Object.isSealed returns false for all built-in objects (JSON))",
R"#(
  var b = Object.isSealed(JSON);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-3",
        R"(Object.isSealed returns false for all built-in objects (Object.prototype))",
R"#(
  var b = Object.isSealed(Object.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-4-4",
        R"(Object.isSealed returns false for all built-in objects (Function))",
R"#(
  var b = Object.isSealed(Function);
  if (b === false) {
    return true;
  }
  )#"
    },
    {
        "15.2.3.11-4-5",
        R"(Object.isSealed returns false for all built-in objects (Function.prototype))",
R"#(
  var b = Object.isSealed(Function.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-4-6",
        R"(Object.isSealed returns false for all built-in objects (Array))",
R"#(
  var b = Object.isSealed(Array);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-4-7",
        R"(Object.isSealed returns false for all built-in objects (Array.prototype))",
R"#(
  var b = Object.isSealed(Array.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.11-4-8",
        R"(Object.isSealed returns false for all built-in objects (String))",
R"#(
  var b = Object.isSealed(String);
  if (b === false) {
    return true;
  }
)#"
    },
    {
        "15.2.3.11-4-9",
        R"(Object.isSealed returns false for all built-in objects (String.prototype))",
R"#(
  var b = Object.isSealed(String.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-0-1",
        R"(Object.isFrozen must exist as a function)",
R"#(
  var f = Object.isFrozen;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-0-2",
        R"(Object.isFrozen must exist as a function taking 1 parameter)",
R"#(
  if (Object.isFrozen.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-1",
        R"(Object.isFrozen throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.isFrozen(0);
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.12-3-1",
        R"(Object.isFrozen returns false for all built-in objects (Global))",
R"#(
  // in non-strict mode, 'this' is bound to the global object.
  var b = Object.isFrozen(this);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-10",
        R"(Object.isFrozen returns false for all built-in objects (Boolean))",
R"#(
  var b = Object.isFrozen(Boolean);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-11",
        R"(Object.isFrozen returns false for all built-in objects (Boolean.prototype))",
R"#(
  var b = Object.isFrozen(Boolean.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-12",
        R"(Object.isFrozen returns false for all built-in objects (Number))",
R"#(
  var b = Object.isFrozen(Number);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-13",
        R"(Object.isFrozen returns false for all built-in objects (Number.prototype))",
R"#(
  var b = Object.isFrozen(Number.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-14",
        R"(Object.isFrozen returns false for all built-in objects (Math))",
R"#(
  var b = Object.isFrozen(Math);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-15",
        R"(Object.isFrozen returns false for all built-in objects (Date))",
R"#(
  var b = Object.isFrozen(Date);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-16",
        R"(Object.isFrozen returns false for all built-in objects (Date.prototype))",
R"#(
  var b = Object.isFrozen(Date.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-17",
        R"(Object.isFrozen returns false for all built-in objects (RegExp))",
R"#(
  var b = Object.isFrozen(RegExp);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-18",
        R"(Object.isFrozen returns false for all built-in objects (RegExp.prototype))",
R"#(
  var b = Object.isFrozen(RegExp.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-19",
        R"(Object.isFrozen returns false for all built-in objects (Error))",
R"#(
  var b = Object.isFrozen(Error);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-2",
        R"(Object.isFrozen returns false for all built-in objects (Object))",
R"#(
  var b = Object.isFrozen(Object);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-20",
        R"(Object.isFrozen returns false for all built-in objects (Error.prototype))",
R"#(
  var b = Object.isFrozen(Error.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-21",
        R"(Object.isFrozen returns false for all built-in objects (EvalError))",
R"#(
  var b = Object.isFrozen(EvalError);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-22",
        R"(Object.isFrozen returns false for all built-in objects (RangeError))",
R"#(
  var b = Object.isFrozen(RangeError);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-23",
        R"(Object.isFrozen returns false for all built-in objects (ReferenceError))",
R"#(
  var b = Object.isFrozen(ReferenceError);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-24",
        R"(Object.isFrozen returns false for all built-in objects (SyntaxError))",
R"#(
  var b = Object.isFrozen(SyntaxError);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-25",
        R"(Object.isFrozen returns false for all built-in objects (TypeError))",
R"#(
  var b = Object.isFrozen(TypeError);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-26",
        R"(Object.isFrozen returns false for all built-in objects (URIError))",
R"#(
  var b = Object.isFrozen(URIError);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-27",
        R"(Object.isFrozen returns false for all built-in objects (JSON))",
R"#(
  var b = Object.isFrozen(JSON);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-3",
        R"(Object.isFrozen returns false for all built-in objects (Object.prototype))",
R"#(
  var b = Object.isFrozen(Object.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-4",
        R"(Object.isFrozen returns false for all built-in objects (Function))",
R"#(
  var b = Object.isFrozen(Function);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-5",
        R"(Object.isFrozen returns false for all built-in objects (Function.prototype))",
R"#(
  var b = Object.isFrozen(Function.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-6",
        R"(Object.isFrozen returns false for all built-in objects (Array))",
R"#(
  var b = Object.isFrozen(Array);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-7",
        R"(Object.isFrozen returns false for all built-in objects (Array.prototype))",
R"#(
  var b = Object.isFrozen(Array.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-8",
        R"(Object.isFrozen returns false for all built-in objects (String))",
R"#(
  var b = Object.isFrozen(String);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.12-3-9",
        R"(Object.isFrozen returns false for all built-in objects (String.prototype))",
R"#(
  var b = Object.isFrozen(String.prototype);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-0-1",
        R"(Object.isExtensible must exist as a function)",
R"#(
  var f = Object.isExtensible ;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-0-2",
        R"(Object.isExtensible must exist as a function taking 1 parameter)",
R"#(
  if (Object.isExtensible.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-0-3",
        R"(Object.isExtensible is true for objects created using the Object ctor)",
R"#(
  var o = new Object();

  if (Object.isExtensible(o) === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-1",
        R"(Object.isExtensible throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.isExtensible(0);
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.13-2-1",
        R"(Object.isExtensible returns true for all built-in objects (Global))",
R"#(
  // in non-strict mode, 'this' is bound to the global object.
  var e = Object.isExtensible(this);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-10",
        R"(Object.isExtensible returns true for all built-in objects (RegExp))",
R"#(
  var e = Object.isExtensible(RegExp);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-11",
        R"(Object.isExtensible returns true for all built-in objects (Error))",
R"#(
  var e = Object.isExtensible(Error);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-12",
        R"(Object.isExtensible returns true for all built-in objects (JSON))",
R"#(
  var e = Object.isExtensible(JSON);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-13",
        R"(Object.isExtensible returns true for all built-in objects (Function.constructor))",
R"#(
  var e = Object.isExtensible(Function.constructor);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-14",
        R"(Object.isExtensible returns true for all built-in objects (Function.prototype))",
R"#(
  var e = Object.isExtensible(Function.prototype);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-15",
        R"(Object.isExtensible returns true for all built-in objects (Array.prototype))",
R"#(
  var e = Object.isExtensible(Array.prototype);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-16",
        R"(Object.isExtensible returns true for all built-in objects (String.prototype))",
R"#(
  var e = Object.isExtensible(String.prototype);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-17",
        R"(Object.isExtensible returns true for all built-in objects (Boolean.prototype))",
R"#(
  var e = Object.isExtensible(Boolean.prototype);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-18",
        R"(Object.isExtensible returns true for all built-in objects (Number.prototype))",
R"#(
  var e = Object.isExtensible(Number.prototype);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-19",
        R"(Object.isExtensible returns true for all built-in objects (Date.prototype))",
R"#(
  var e = Object.isExtensible(Date.prototype);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-2",
        R"(Object.isExtensible returns true for all built-in objects (Object))",
R"#(
  var o = {};
  var e = Object.isExtensible(o);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-20",
        R"(Object.isExtensible returns true for all built-in objects (RegExp.prototype))",
R"#(
  var e = Object.isExtensible(RegExp.prototype);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-21",
        R"(Object.isExtensible returns true for all built-in objects (Error.prototype))",
R"#(
  var e = Object.isExtensible(Error.prototype);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-3",
        R"(Object.isExtensible returns true for all built-in objects (Function))",
R"#(
  function foo() {}
 
  var e = Object.isExtensible(foo);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-4",
        R"(Object.isExtensible returns true for all built-in objects (Array))",
R"#(
  var e = Object.isExtensible(Array);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-5",
        R"(Object.isExtensible returns true for all built-in objects (String))",
R"#(
  var e = Object.isExtensible(String);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-6",
        R"(Object.isExtensible returns true for all built-in objects (Boolean))",
R"#(
  var e = Object.isExtensible(Boolean);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-7",
        R"(Object.isExtensible returns true for all built-in objects (Number))",
R"#(
  var e = Object.isExtensible(Number);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-8",
        R"(Object.isExtensible returns true for all built-in objects (Math))",
R"#(
  var e = Object.isExtensible(Math);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.13-2-9",
        R"(Object.isExtensible returns true for all built-in objects (Date))",
R"#(
  var e = Object.isExtensible(Date);
  if (e === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-0-1",
        R"(Object.keys must exist as a function)",
R"#(
  var f = Object.keys;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-0-2",
        R"(Object.keys must exist as a function taking 1 parameter)",
R"#(
  if (Object.keys.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-1-1",
        R"(Object.keys throws TypeError if type of first param is not Object)",
R"#(
  try {
    Object.keys(0);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.2.3.14-1-2",
        R"(Object.keys throws TypeError if type of first param is not Object (boolean))",
R"#(
  try {
    Object.keys(true);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.2.3.14-1-3",
        R"(Object.keys throws TypeError if type of first param is not Object (string))",
R"#(
  try {
    Object.keys('abc');
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.2.3.14-1-4",
        R"(Object.keys throws TypeError if type of first param is not Object (null))",
R"#(
  try {
    Object.keys(null);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.2.3.14-1-5",
        R"(Object.keys throws TypeError if type of first param is not Object (undefined))",
R"#(
  try {
    Object.keys(undefined);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.2.3.14-2-1",
        R"(Object.keys returns the standard built-in Array)",
R"#(
  var o = { x: 1, y: 2};

  var a = Object.keys(o);
  if (Array.isArray(a) === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-2-2",
        R"(Object.keys returns the standard built-in Array (check [[Class]])",
R"#(
  var o = { x: 1, y: 2};

  var a = Object.keys(o);
  var s = Object.prototype.toString.call(a);
  if (s === '[object Array]') {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-2-3",
        R"(Object.keys returns the standard built-in Array (Array overriden))",
R"#(
  function Array() { alert("helloe"); }

  var o = { x: 1, y: 2};

  var a = Object.keys(o);

  var s = Object.prototype.toString.call(a);
  if (s === '[object Array]') {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-2-4",
        R"(Object.keys returns the standard built-in Array that is extensible)",
R"#(
  var o = { x: 1, y: 2};

  var a = Object.keys(o);
  if (Object.isExtensible(a) === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-2-5",
        R"(Object.keys returns the standard built-in Array that is not sealed)",
R"#(
  var o = { x: 1, y: 2};

  var a = Object.keys(o);
  if (Object.isSealed(a) === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-2-6",
        R"(Object.keys returns the standard built-in Array that is not frozen)",
R"#(
  var o = { x: 1, y: 2};

  var a = Object.keys(o);
  if (Object.isFrozen(a) === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-3-1",
        R"(Object.keys returns the standard built-in Array containing own enumerable properties)",
R"#(
  var o = { x: 1, y: 2};

  var a = Object.keys(o);
  if (a.length === 2 &&
      a[0] === 'x' &&
      a[1] === 'y') {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-3-2",
        R"(Object.keys returns the standard built-in Array containing own enumerable properties (function))",
R"#(
  function foo() {}
  foo.x = 1;
  
  var a = Object.keys(foo);
  if (a.length === 1 &&
      a[0] === 'x') {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-3-3",
        R"(Object.keys returns the standard built-in Array containing own enumerable properties (array))",
R"#(
  var o = [1, 2];
  var a = Object.keys(o);
  if (a.length === 2 &&
      a[0] === '0' &&
      a[1] === '1') {
    return true;
  }
 )#"
    },
    {
        "15.2.3.14-3-4",
        R"(Object.keys of an arguments object returns the indices of the given arguments)",
R"#(
  function testArgs2(x, y, z) {
    // Properties of the arguments object are enumerable.
    var a = Object.keys(arguments);
    if (a.length === 2 && a[0] === "0" && a[1] === "1")
      return true;
  }
  function testArgs3(x, y, z) {
    // Properties of the arguments object are enumerable.
    var a = Object.keys(arguments);
    if (a.length === 3 && a[0] === "0" && a[1] === "1" && a[2] === "2")
      return true;
  }
  function testArgs4(x, y, z) {
    // Properties of the arguments object are enumerable.
    var a = Object.keys(arguments);
    if (a.length === 4 && a[0] === "0" && a[1] === "1" && a[2] === "2" && a[3] === "3")
      return true;
  }
  return testArgs2(1, 2) && testArgs3(1, 2, 3) && testArgs4(1, 2, 3, 4);
 )#"
    },
    {
        "15.2.3.14-3-5",
        R"(Object.keys must return a fresh array on each invocation)",
R"#(
  var literal = {a: 1};
  var keysBefore = Object.keys(literal);
  if (keysBefore[0] != 'a') return false;
  keysBefore[0] = 'x';
  var keysAfter = Object.keys(literal);
  return (keysBefore[0] == 'x') && (keysAfter[0] == 'a');
 )#"
    },
    {
        "15.2.3.2-0-1",
        R"(Object.getPrototypeOf must exist as a function)",
R"#(
  if (typeof(Object.getPrototypeOf) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-0-2",
        R"(Object.getPrototypeOf must exist as a function taking 1 parameter)",
R"#(
  if (Object.getPrototypeOf.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-0-3",
        R"(Object.getPrototypeOf must take 1 parameter)",
R"#(
  try
  {
    Object.getPrototypeOf();
  }
  catch(e)
  {
    if(e instanceof TypeError)
      return true;
  }
 )#"
    },
    {
        "15.2.3.2-1",
        R"(Object.getPrototypeOf throws TypeError if type of first param is not Object)",
R"#(
  try {
    Object.getPrototypeOf(0);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.2.3.2-2-1",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (Boolean))",
R"#(
  if (Object.getPrototypeOf(Boolean) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-10",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (RegExp))",
R"#(
  if (Object.getPrototypeOf(RegExp) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-11",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (Error))",
R"#(
  if (Object.getPrototypeOf(Error) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-12",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (EvalError))",
R"#(
  if (Object.getPrototypeOf(EvalError) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-13",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (RangeError))",
R"#(
  if (Object.getPrototypeOf(RangeError) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-14",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (ReferenceError))",
R"#(
  if (Object.getPrototypeOf(ReferenceError) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-15",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (SyntaxError))",
R"#(
  if (Object.getPrototypeOf(SyntaxError) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-16",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (TypeError))",
R"#(
  if (Object.getPrototypeOf(TypeError) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-17",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (URIError))",
R"#(
  if (Object.getPrototypeOf(URIError) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-18",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (JSON))",
R"#(
  if (Object.getPrototypeOf(JSON) === Object.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-2",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (custom object))",
R"#(
  function base() {}

  function derived() {}
  derived.prototype = new base();

  var d = new derived();
  var x = Object.getPrototypeOf(d);
  if (x.isPrototypeOf(d) === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-3",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (Object))",
R"#(
  if (Object.getPrototypeOf(Object) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-4",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (Function))",
R"#(
  if (Object.getPrototypeOf(Function) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-5",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (Array))",
R"#(
  if (Object.getPrototypeOf(Array) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-6",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (String))",
R"#(
  if (Object.getPrototypeOf(String) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-7",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (Number))",
R"#(
  if (Object.getPrototypeOf(Number) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-8",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (Math))",
R"#(
  if (Object.getPrototypeOf(Math) === Object.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.2-2-9",
        R"(Object.getPrototypeOf returns the [[Prototype]] of its parameter (Date))",
R"#(
  if (Object.getPrototypeOf(Date) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-0-1",
        R"(Object.getOwnPropertyDescriptor must exist as a function)",
R"#(
  if (typeof(Object.getOwnPropertyDescriptor) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-0-2",
        R"(Object.getOwnPropertyDescriptor must exist as a function taking 2 parameters)",
R"#(
  if (Object.getOwnPropertyDescriptor.length === 2) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-1",
        R"(Object.getOwnPropertyDescriptor throws TypeError if type of first param is not Object)",
R"#(
  try {
    Object.getOwnPropertyDescriptor(0, "foo");
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.2.3.3-2-1",
        R"(Object.getOwnPropertyDescriptor returns undefined for undefined property name)",
R"#(
    var o = {};
    var desc = Object.getOwnPropertyDescriptor(o, undefined);
    if (desc === undefined) {
      return true;
    }
 )#"
    },
    {
        "15.2.3.3-2-2",
        R"(Object.getOwnPropertyDescriptor returns undefined for null property name)",
R"#(
    var o = {};
    var desc = Object.getOwnPropertyDescriptor(o, null);
    if (desc === undefined) {
      return true;
    }
 )#"
    },
    {
        "15.2.3.3-4-1",
        R"(Object.getOwnPropertyDescriptor returns an object representing a data desc for valid data valued properties)",
R"#(
    var o = {};
    o["foo"] = 101;

    var desc = Object.getOwnPropertyDescriptor(o, "foo");
    if (desc.value === 101 &&
        desc.enumerable === true &&
        desc.writable === true &&
        desc.configurable === true &&
        !desc.hasOwnProperty("get") &&
        !desc.hasOwnProperty("set")) {
      return true;
    }
 )#"
    },
    {
        "15.2.3.3-4-10",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.decodeURIComponent))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global,  "decodeURIComponent");
  if (desc.value === global.decodeURIComponent &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-100",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.atan2))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "atan2");
  if (desc.value === Math.atan2 &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-101",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.ceil))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "ceil");
  if (desc.value === Math.ceil &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-102",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.cos))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "cos");
  if (desc.value === Math.cos &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-103",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.exp))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "exp");
  if (desc.value === Math.exp &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-104",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.floor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "floor");
  if (desc.value === Math.floor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-105",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.log))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "log");
  if (desc.value === Math.log &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-106",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.max))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "max");
  if (desc.value === Math.max &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-107",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.min))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "min");
  if (desc.value === Math.min &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-108",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.pow))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "pow");
  if (desc.value === Math.pow &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-109",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.random))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "random");
  if (desc.value === Math.random &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-11",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.encodeURIComponent))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global,  "encodeURIComponent");
  if (desc.value === global.encodeURIComponent &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-110",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.round))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "round");
  if (desc.value === Math.round &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-111",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.sin))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "sin");
  if (desc.value === Math.sin &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-112",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.sqrt))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "sqrt");
  if (desc.value === Math.sqrt &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-113",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.tan))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "tan");
  if (desc.value === Math.tan &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-114",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.parse))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date, "parse");
  if (desc.value === Date.parse &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-115",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.UTC))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date, "UTC");
  if (desc.value === Date.UTC &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-116",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "constructor");
  if (desc.value === Date.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-117",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getTime))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getTime");
  if (desc.value === Date.prototype.getTime &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-118",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getTimezoneOffset))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getTimezoneOffset");
  if (desc.value === Date.prototype.getTimezoneOffset &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-119",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getYear))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getYear");
  if (desc.value === Date.prototype.getYear &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-12",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.escape))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global, "escape");
  if (desc.value === global.escape &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-120",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getFullYear))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getFullYear");
  if (desc.value === Date.prototype.getFullYear &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-121",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getMonth))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getMonth");
  if (desc.value === Date.prototype.getMonth &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-122",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getDate))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getDate");
  if (desc.value === Date.prototype.getDate &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-123",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getDay))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getDay");
  if (desc.value === Date.prototype.getDay &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-124",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getHours))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getHours");
  if (desc.value === Date.prototype.getHours &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-125",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getMinutes))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getMinutes");
  if (desc.value === Date.prototype.getMinutes &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-126",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getSeconds))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getSeconds");
  if (desc.value === Date.prototype.getSeconds &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-127",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getMilliseconds))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getMilliseconds");
  if (desc.value === Date.prototype.getMilliseconds &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-128",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getUTCFullYear))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getUTCFullYear");
  if (desc.value === Date.prototype.getUTCFullYear &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-129",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getUTCMonth))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getUTCMonth");
  if (desc.value === Date.prototype.getUTCMonth &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-13",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.unescape))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global,  "unescape");
  if (desc.value === global.unescape &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-130",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getUTCDate))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getUTCDate");
  if (desc.value === Date.prototype.getUTCDate &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-131",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getUTCDay))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getUTCDay");
  if (desc.value === Date.prototype.getUTCDay &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-132",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getUTCHours))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getUTCHours");
  if (desc.value === Date.prototype.getUTCHours &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-133",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getUTCMinutes))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getUTCMinutes");
  if (desc.value === Date.prototype.getUTCMinutes &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-134",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getUTCSeconds))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getUTCSeconds");
  if (desc.value === Date.prototype.getUTCSeconds &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-135",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.getUTCMilliseconds))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "getUTCMilliseconds");
  if (desc.value === Date.prototype.getUTCMilliseconds &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-136",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setTime))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setTime");
  if (desc.value === Date.prototype.setTime &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-137",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setYear))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setYear");
  if (desc.value === Date.prototype.setYear &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-138",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setFullYear))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setFullYear");
  if (desc.value === Date.prototype.setFullYear &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-139",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setMonth))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setMonth");
  if (desc.value === Date.prototype.setMonth &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-14",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.getPrototypeOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "getPrototypeOf");
  if (desc.value === Object.getPrototypeOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-140",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setDate))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setDate");
  if (desc.value === Date.prototype.setDate &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-141",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setHours))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setHours");
  if (desc.value === Date.prototype.setHours &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-142",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setMinutes))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setMinutes");
  if (desc.value === Date.prototype.setMinutes &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-143",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setSeconds))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setSeconds");
  if (desc.value === Date.prototype.setSeconds &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-144",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setMilliseconds))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setMilliseconds");
  if (desc.value === Date.prototype.setMilliseconds &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-145",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCFullYear))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setUTCFullYear");
  if (desc.value === Date.prototype.setUTCFullYear &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-146",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCMonth))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setUTCMonth");
  if (desc.value === Date.prototype.setUTCMonth &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-147",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCDate))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setUTCDate");
  if (desc.value === Date.prototype.setUTCDate &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-148",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCHours))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setUTCHours");
  if (desc.value === Date.prototype.setUTCHours &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-149",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCMinutes))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setUTCMinutes");
  if (desc.value === Date.prototype.setUTCMinutes &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-15",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.getOwnPropertyDescriptor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "getOwnPropertyDescriptor");
  if (desc.value === Object.getOwnPropertyDescriptor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-150",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCSeconds))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setUTCSeconds");
  if (desc.value === Date.prototype.setUTCSeconds &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-151",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCMilliseconds))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "setUTCMilliseconds");
  if (desc.value === Date.prototype.setUTCMilliseconds &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-152",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toLocaleString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toLocaleString");
  if (desc.value === Date.prototype.toLocaleString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-153",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toString");
  if (desc.value === Date.prototype.toString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-154",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toUTCString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toUTCString");
  if (desc.value === Date.prototype.toUTCString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-155",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toGMTString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toGMTString");
  if (desc.value === Date.prototype.toGMTString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-156",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toTimeString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toTimeString");
  if (desc.value === Date.prototype.toTimeString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-157",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toDateString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toDateString");
  if (desc.value === Date.prototype.toDateString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-158",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toLocaleDateString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toLocaleDateString");
  if (desc.value === Date.prototype.toLocaleDateString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-159",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toLocaleTimeString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toLocaleTimeString");
  if (desc.value === Date.prototype.toLocaleTimeString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-16",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.getOwnPropertyNames))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "getOwnPropertyNames");
  if (desc.value === Object.getOwnPropertyNames &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-160",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.valueOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "valueOf");
  if (desc.value === Date.prototype.valueOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-161",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toISOString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toISOString");
  if (desc.value === Date.prototype.toISOString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-162",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toJSON))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date.prototype, "toJSON");
  if (desc.value === Date.prototype.toJSON &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-163",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RegExp.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, "constructor");
  if (desc.value === RegExp.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-164",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RegExp.prototype.compile))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, "compile");
  if (desc.value === RegExp.prototype.compile &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-165",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RegExp.prototype.exec))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, "exec");
  if (desc.value === RegExp.prototype.exec &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-166",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RegExp.prototype.test))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, "test");
  if (desc.value === RegExp.prototype.test &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-167",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RegExp.prototype.toString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, "toString");
  if (desc.value === RegExp.prototype.toString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-168",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Error.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Error.prototype, "constructor");
  if (desc.value === Error.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-169",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Error.prototype.toString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Error.prototype, "toString");
  if (desc.value === Error.prototype.toString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-17",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.create))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "create");
  if (desc.value === Object.create &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-170",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (EvalError.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(EvalError.prototype, "constructor");
  if (desc.value === EvalError.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-171",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RangeError.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RangeError.prototype, "constructor");
  if (desc.value === RangeError.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-172",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (ReferenceError.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(ReferenceError.prototype, "constructor");
  if (desc.value === ReferenceError.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-173",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (SyntaxError.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(SyntaxError.prototype, "constructor");
  if (desc.value === SyntaxError.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-174",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (TypeError.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(TypeError.prototype, "constructor");
  if (desc.value === TypeError.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-175",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (URIError.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(URIError.prototype, "constructor");
  if (desc.value === URIError.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-176",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (JSON.stringify))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(JSON, "stringify");
  if (desc.value === JSON.stringify &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-177",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (JSON.parse))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(JSON, "parse");
  if (desc.value === JSON.parse &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-178",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Global.NaN))",
R"#(
  // in non-strict mode, 'this' is bound to the global object.
  var desc = Object.getOwnPropertyDescriptor(fnGlobalObject(), "NaN");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-179",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Global.Infinity))",
R"#(
  // in non-strict mode, 'this' is bound to the global object.
  var desc = Object.getOwnPropertyDescriptor(fnGlobalObject(),  "Infinity");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-18",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.defineProperty))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "defineProperty");
  if (desc.value === Object.defineProperty &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-180",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Global.undefined))",
R"#(
  // in non-strict mode, 'this' is bound to the global object.
  var desc = Object.getOwnPropertyDescriptor(fnGlobalObject(),  "undefined");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-182",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Object.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-183",
        R"(Object.getOwnPropertyDescriptor returns undefined for non-existent properties on built-ins (Function.arguments))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Function, "arguments");

  if (desc === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-184",
        R"(Object.getOwnPropertyDescriptor returns undefined for non-existent properties on built-ins (Function.caller))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Function, "caller");

  if (desc === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-185",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Function.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Function, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-186",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Function.length))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Function, "length");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-187",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Function (instance).length))",
R"#(
  var f = Function('return 42;');

  var desc = Object.getOwnPropertyDescriptor(f, "length");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-188",
        R"(Object.getOwnPropertyDescriptor returns undefined for non-existent properties on built-ins (Function (instance).name))",
R"#(
  var f = Function('return 42;');

  var desc = Object.getOwnPropertyDescriptor(f, "name");

  if (desc === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-189",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Array.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-19",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.defineProperties))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "defineProperties");
  if (desc.value === Object.defineProperties &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-190",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (String.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-191",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (String.length))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String, "length");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-192",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (String (instance).length))",
R"#(
  var s = new String("abc");
  var desc = Object.getOwnPropertyDescriptor(s, "length");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-193",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Boolean.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Boolean, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-194",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Boolean.length))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Boolean, "length");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-195",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-196",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.MAX_VALUE))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number, "MAX_VALUE");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-197",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.MIN_VALUE))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number, "MIN_VALUE");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-198",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.NaN))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number, "NaN");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-199",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.NEGATIVE_INFINITY))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number, "NEGATIVE_INFINITY");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-2",
        R"(Object.getOwnPropertyDescriptor returns undefined for non-existent properties)",
R"#(
    var o = {};

    var desc = Object.getOwnPropertyDescriptor(o, "foo");
    if (desc === undefined) {
      return true;
    }
 )#"
    },
    {
        "15.2.3.3-4-20",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.seal))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "seal");
  if (desc.value === Object.seal &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-200",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.POSITIVE_INFINITY))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number, "POSITIVE_INFINITY");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-201",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.length))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number, "length");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-202",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.E))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "E");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-203",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.LN10))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "LN10");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-204",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.LN2))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "LN2");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-205",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.LOG2E))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "LOG2E");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-206",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.LOG10E))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "LOG10E");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-207",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.PI))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "PI");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-208",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.SQRT1_2))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "SQRT1_2");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-209",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.SQRT2))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "SQRT2");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-21",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.freeze))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "freeze");
  if (desc.value === Object.freeze &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-210",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Date.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Date, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-211",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (RegExp.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-212",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (RegExp.prototype.source))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, "source");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-213",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (RegExp.prototype.global))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, "global");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-214",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (RegExp.prototype.ignoreCase))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, "ignoreCase");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-215",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (RegExp.prototype.multiline))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RegExp.prototype, "multiline");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-216",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Error.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Error, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-217",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (EvalError.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(EvalError, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-218",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (RangeError.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(RangeError, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-219",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (ReferenceError.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(ReferenceError, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-22",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.preventExtensions))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "preventExtensions");
  if (desc.value === Object.preventExtensions &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-220",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (SyntaxError.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(SyntaxError, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-221",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (TypeError.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(TypeError, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-222",
        R"(Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (URIError.prototype))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(URIError, "prototype");

  if (desc.writable === false &&
      desc.enumerable === false &&
      desc.configurable === false &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-23",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.isSealed))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "isSealed");
  if (desc.value === Object.isSealed &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-24",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.isFrozen))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "isFrozen");
  if (desc.value === Object.isFrozen &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-25",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.isExtensible))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "isExtensible");
  if (desc.value === Object.isExtensible &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-26",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.keys))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object, "keys");
  if (desc.value === Object.keys &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-27",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object.prototype, "constructor");
  if (desc.value === Object.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-28",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.prototype.toString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object.prototype, "toString");
  if (desc.value === Object.prototype.toString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-29",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.prototype.valueOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object.prototype, "valueOf");
  if (desc.value === Object.prototype.valueOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-3",
        R"(Object.getOwnPropertyDescriptor returns an object representing an accessor desc for valid accessor properties)",
R"#(
    var o = {};

    // dummy getter
    var getter = function () { return 1; }
    var d = { get: getter };

    Object.defineProperty(o, "foo", d);

    var desc = Object.getOwnPropertyDescriptor(o, "foo");
    if (desc.get === getter &&
        desc.set === undefined &&
        desc.enumerable === false &&
        desc.configurable === false) {
      return true;
    }
 )#"
    },
    {
        "15.2.3.3-4-30",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.prototype.isPrototypeOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object.prototype, "isPrototypeOf");
  if (desc.value === Object.prototype.isPrototypeOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-31",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.prototype.hasOwnProperty))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object.prototype, "hasOwnProperty");
  if (desc.value === Object.prototype.hasOwnProperty &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-32",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.prototype.propertyIsEnumerable))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object.prototype, "propertyIsEnumerable");
  if (desc.value === Object.prototype.propertyIsEnumerable &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-33",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Object.prototype.toLocaleString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Object.prototype, "toLocaleString");
  if (desc.value === Object.prototype.toLocaleString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-34",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Function.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Function.prototype, "constructor");
  if (desc.value === Function.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-35",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Function.prototype.toString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Function.prototype, "toString");
  if (desc.value === Function.prototype.toString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-36",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Function.prototype.apply))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Function.prototype, "apply");
  if (desc.value === Function.prototype.apply &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-37",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Function.prototype.call))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Function.prototype, "call");
  if (desc.value === Function.prototype.call &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-38",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Function.prototype.bind))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Function.prototype, "bind");
  if (desc.value === Function.prototype.bind &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-39",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "constructor");
  if (desc.value === Array.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-4",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.eval))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global,  "eval");
  if (desc.value === global.eval &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-40",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.concat))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "concat");
  if (desc.value === Array.prototype.concat &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-41",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.join))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "join");
  if (desc.value === Array.prototype.join &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-42",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.reverse))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "reverse");
  if (desc.value === Array.prototype.reverse &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-43",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.slice))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "slice");
  if (desc.value === Array.prototype.slice &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-44",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.sort))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "sort");
  if (desc.value === Array.prototype.sort &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-45",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.toString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "toString");
  if (desc.value === Array.prototype.toString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-46",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.push))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "push");
  if (desc.value === Array.prototype.push &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-47",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.pop))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "pop");
  if (desc.value === Array.prototype.pop &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-48",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.shift))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "shift");
  if (desc.value === Array.prototype.shift &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-49",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.unshift))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "unshift");
  if (desc.value === Array.prototype.unshift &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-5",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.parseInt))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global,  "parseInt");
  if (desc.value === global.parseInt &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-50",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.splice))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "splice");
  if (desc.value === Array.prototype.splice &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-51",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.toLocaleString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "toLocaleString");
  if (desc.value === Array.prototype.toLocaleString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-52",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.indexOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "indexOf");
  if (desc.value === Array.prototype.indexOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-53",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.lastIndexOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "lastIndexOf");
  if (desc.value === Array.prototype.lastIndexOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-54",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.every))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "every");
  if (desc.value === Array.prototype.every &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-55",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.some))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "some");
  if (desc.value === Array.prototype.some &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-56",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.forEach))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "forEach");
  if (desc.value === Array.prototype.forEach &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-57",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.map))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "map");
  if (desc.value === Array.prototype.map &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-58",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.filter))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "filter");
  if (desc.value === Array.prototype.filter &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-59",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.reduce))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "reduce");
  if (desc.value === Array.prototype.reduce &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-6",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.parseFloat))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global, "parseFloat");
  if (desc.value === global.parseFloat &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-60",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.reduceRight))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Array.prototype, "reduceRight");
  if (desc.value === Array.prototype.reduceRight &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-61",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.fromCharCode))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String, "fromCharCode");
  if (desc.value === String.fromCharCode &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-62",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "constructor");
  if (desc.value === String.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-63",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.charAt))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "charAt");
  if (desc.value === String.prototype.charAt &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-64",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.charCodeAt))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "charCodeAt");
  if (desc.value === String.prototype.charCodeAt &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-65",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.concat))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "concat");
  if (desc.value === String.prototype.concat &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-66",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.indexOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "indexOf");
  if (desc.value === String.prototype.indexOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-67",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.lastIndexOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "lastIndexOf");
  if (desc.value === String.prototype.lastIndexOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-68",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.match))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "match");
  if (desc.value === String.prototype.match &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-69",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.replace))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "replace");
  if (desc.value === String.prototype.replace &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-7",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.isNaN))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global,  "isNaN");
  if (desc.value === global.isNaN &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-70",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.search))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "search");
  if (desc.value === String.prototype.search &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-71",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.slice))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "slice");
  if (desc.value === String.prototype.slice &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-72",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.split))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "split");
  if (desc.value === String.prototype.split &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-73",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.substring))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "substring");
  if (desc.value === String.prototype.substring &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-74",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.substr))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "substr");
  if (desc.value === String.prototype.substr &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-75",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.toLowerCase))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "toLowerCase");
  if (desc.value === String.prototype.toLowerCase &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-76",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.toString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "toString");
  if (desc.value === String.prototype.toString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-77",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.toUpperCase))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "toUpperCase");
  if (desc.value === String.prototype.toUpperCase &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-78",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.valueOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "valueOf");
  if (desc.value === String.prototype.valueOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-79",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.toLocaleLowerCase))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "toLocaleLowerCase");
  if (desc.value === String.prototype.toLocaleLowerCase &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-8",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.isFinite))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global,  "isFinite");
  if (desc.value === global.isFinite &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-80",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.toLocaleUpperCase))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "toLocaleUpperCase");
  if (desc.value === String.prototype.toLocaleUpperCase &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-81",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.localeCompare))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "localeCompare");
  if (desc.value === String.prototype.localeCompare &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-82",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.trim))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(String.prototype, "trim");
  if (desc.value === String.prototype.trim &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-84",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Boolean.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Boolean.prototype, "constructor");
  if (desc.value === Boolean.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-85",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Boolean.prototype.toString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Boolean.prototype, "toString");
  if (desc.value === Boolean.prototype.toString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-86",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Boolean.prototype.valueOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Boolean.prototype, "valueOf");
  if (desc.value === Boolean.prototype.valueOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-88",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Number.prototype.constructor))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number.prototype, "constructor");
  if (desc.value === Number.prototype.constructor &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-89",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Number.prototype.toString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number.prototype, "toString");
  if (desc.value === Number.prototype.toString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-9",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Global.decodeURI))",
R"#(
  var global = fnGlobalObject();
  var desc = Object.getOwnPropertyDescriptor(global, "decodeURI");
  if (desc.value === global.decodeURI &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-90",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Number.prototype.toLocaleString))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number.prototype, "toLocaleString");
  if (desc.value === Number.prototype.toLocaleString &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-91",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Number.prototype.toFixed))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number.prototype, "toFixed");
  if (desc.value === Number.prototype.toFixed &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-92",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Number.prototype.toExponential))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number.prototype, "toExponential");
  if (desc.value === Number.prototype.toExponential &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-93",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Number.prototype.toPrecision))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number.prototype, "toPrecision");
  if (desc.value === Number.prototype.toPrecision &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-94",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Number.prototype.valueOf))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Number.prototype, "valueOf");
  if (desc.value === Number.prototype.valueOf &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-96",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.abs))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "abs");
  if (desc.value === Math.abs &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-97",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.acos))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "acos");
  if (desc.value === Math.acos &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-98",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.asin))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "asin");
  if (desc.value === Math.asin &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.3-4-99",
        R"(Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Math.atan))",
R"#(
  var desc = Object.getOwnPropertyDescriptor(Math, "atan");
  if (desc.value === Math.atan &&
      desc.writable === true &&
      desc.enumerable === false &&
      desc.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-0-1",
        R"(Object.getOwnPropertyNames must exist as a function)",
R"#(
  if (typeof(Object.getOwnPropertyNames) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-0-2",
        R"(Object.getOwnPropertyNames must exist as a function taking 1 parameter)",
R"#(
  if (Object.getOwnPropertyNames.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-1",
        R"(Object.getOwnPropertyNames throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.getOwnPropertyNames(0);
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.4-4-1",
        R"(Object.getOwnPropertyNames returns array of property names (Global))",
R"#(
  var result = Object.getOwnPropertyNames(fnGlobalObject());
  var expResult = ["eval", "parseInt", "parseFloat", "isNaN", "isFinite", "decodeURI", "decodeURIComponent", "encodeURIComponent", "escape", "unescape", "NaN", "Infinity", "undefined"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-10",
        R"(Object.getOwnPropertyNames returns array of property names (Math))",
R"#(
  var result = Object.getOwnPropertyNames(Math);
  var expResult = ["abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "exp", "floor", "log", "max", "min", "pow", "random", "round", "sin", "sqrt", "tan", "E", "LN10", "LN2", "LOG2E", "LOG10E", "PI", "SQRT1_2", "SQRT2"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-11",
        R"(Object.getOwnPropertyNames returns array of property names (Date))",
R"#(
  var result = Object.getOwnPropertyNames(Date);
  var expResult = ["length", "parse", "UTC", "prototype"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-12",
        R"(Object.getOwnPropertyNames returns array of property names (Date.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(Date.prototype);
  var expResult = ["constructor", "getTime", "getTimezoneOffset", "getYear", "getFullYear", "getMonth", "getDate", "getDay", "getHours", "getMinutes", "getSeconds", "getMilliseconds", "getUTCFullYear", "getUTCMonth", "getUTCDate", "getUTCDay", "getUTCHours", "getUTCMinutes", "getUTCSeconds", "getUTCMilliseconds", "setTime", "setYear", "setFullYear", "setMonth", "setDate", "setHours", "setMinutes", "setSeconds", "setMilliseconds", "setUTCFullYear", "setUTCMonth", "setUTCDate", "setUTCHours", "setUTCMinutes", "setUTCSeconds", "setUTCMilliseconds", "toLocaleString", "toString", "toUTCString", "toGMTString", "toTimeString", "toDateString", "toLocaleDateString", "toLocaleTimeString", "valueOf", "toISOString", "toJSON"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-13",
        R"(Object.getOwnPropertyNames returns array of property names (RegExp.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(RegExp.prototype);
  
  // 'compile' is a JScript extension
  var expResult = ["constructor", "exec", "test", "toString", "source", "global", "ignoreCase", "multiline", "lastIndex", "compile"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-14",
        R"(Object.getOwnPropertyNames returns array of property names (Error.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(Error.prototype);
  var expResult = ["constructor", "name", "message", "toString"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-15",
        R"(Object.getOwnPropertyNames returns array of property names (EvalError.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(EvalError.prototype);
  var expResult = ["constructor", "name", "message"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-16",
        R"(Object.getOwnPropertyNames returns array of property names (RangeError.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(RangeError.prototype);
  var expResult = ["constructor", "name", "message"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-17",
        R"(Object.getOwnPropertyNames returns array of property names (ReferenceError.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(ReferenceError.prototype);
  var expResult = ["constructor", "name", "message"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-18",
        R"(Object.getOwnPropertyNames returns array of property names (SyntaxError.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(SyntaxError.prototype);
  var expResult = ["constructor", "name", "message"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-19",
        R"(Object.getOwnPropertyNames returns array of property names (TypeError.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(TypeError.prototype);
  var expResult = ["constructor", "name", "message"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-2",
        R"(Object.getOwnPropertyNames returns array of property names (Object))",
R"#(
  var result = Object.getOwnPropertyNames(Object);
  var expResult = ["getPrototypeOf", "getOwnPropertyDescriptor", "getOwnPropertyNames", "create", "defineProperty", "defineProperties", "seal", "freeze", "preventExtensions", "isSealed", "isFrozen", "isExtensible", "keys", "prototype", "length"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-20",
        R"(Object.getOwnPropertyNames returns array of property names (URIError.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(URIError.prototype);
  var expResult = ["constructor", "name", "message"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-21",
        R"(Object.getOwnPropertyNames returns array of property names (JSON))",
R"#(
  var result = Object.getOwnPropertyNames(JSON);
  var expResult = ["stringify", "parse"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-22",
        R"(Object.getOwnPropertyNames returns array of property names (Function))",
R"#(
  var result = Object.getOwnPropertyNames(Function);
  var expResult = ["prototype", "length"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-23",
        R"(Object.getOwnPropertyNames returns array of property names (function))",
R"#(
  function f(){};

  // JScript-specific
  f.length;
  
  var result = Object.getOwnPropertyNames(f);
  var expResult = ["length"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-24",
        R"(Object.getOwnPropertyNames returns array of property names (Array))",
R"#(
  var result = Object.getOwnPropertyNames(Array);
  var expResult = ["isArray", "prototype", "length"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-25",
        R"(Object.getOwnPropertyNames returns array of property names (String))",
R"#(
  var result = Object.getOwnPropertyNames(new String('abc'));
  var expResult = ["length", "0", "1", "2"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-26",
        R"(Object.getOwnPropertyNames returns array of property names (Boolean))",
R"#(
  var result = Object.getOwnPropertyNames(Boolean);
  var expResult = ["prototype", "length"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-27",
        R"(Object.getOwnPropertyNames returns array of property names (Number))",
R"#(
  // JScript specific
  Number.NaN;

  var result = Object.getOwnPropertyNames(Number);
  var expResult = ["prototype", "MAX_VALUE", "MIN_VALUE", "NaN", "NEGATIVE_INFINITY", "POSITIVE_INFINITY", "length"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-28",
        R"(Object.getOwnPropertyNames returns array of property names (RegExp))",
R"#(
  var result = Object.getOwnPropertyNames(RegExp);
  var expResult = ["prototype", "length"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-29",
        R"(Object.getOwnPropertyNames returns array of property names (Error))",
R"#(
  var result = Object.getOwnPropertyNames(Error);
  var expResult = ["prototype", "length"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-3",
        R"(Object.getOwnPropertyNames returns array of property names (Object.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(Object.prototype);
  var expResult = ["constructor", "toString", "valueOf", "isPrototypeOf", "hasOwnProperty", "propertyIsEnumerable", "toLocaleString"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-30",
        R"(Object.getOwnPropertyNames returns array of property names (EvalError))",
R"#(
  var result = Object.getOwnPropertyNames(EvalError);
  var expResult = ["prototype"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-31",
        R"(Object.getOwnPropertyNames returns array of property names (RangeError))",
R"#(
  var result = Object.getOwnPropertyNames(RangeError);
  var expResult = ["prototype"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-32",
        R"(Object.getOwnPropertyNames returns array of property names (ReferenceError))",
R"#(
  var result = Object.getOwnPropertyNames(ReferenceError);
  var expResult = ["prototype"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-33",
        R"(Object.getOwnPropertyNames returns array of property names (SyntaxError))",
R"#(
  var result = Object.getOwnPropertyNames(SyntaxError);
  var expResult = ["prototype"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-34",
        R"(Object.getOwnPropertyNames returns array of property names (TypeError))",
R"#(
  var result = Object.getOwnPropertyNames(TypeError);
  var expResult = ["prototype"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-35",
        R"(Object.getOwnPropertyNames returns array of property names (URIError))",
R"#(
  var result = Object.getOwnPropertyNames(URIError);
  var expResult = ["prototype"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-4",
        R"(Object.getOwnPropertyNames returns array of property names (Function.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(Function.prototype);
  var expResult = ["constructor", "toString", "apply", "call", "bind"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-5",
        R"(Object.getOwnPropertyNames returns array of property names (Array.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(Array.prototype);
  var expResult = ["constructor", "concat", "join", "reverse", "slice", "sort", "toString", "push", "pop", "shift", "unshift", "splice", "toLocaleString", "indexOf", "lastIndexOf", "every", "some", "forEach", "map", "filter", "reduce", "reduceRight"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-6",
        R"(Object.getOwnPropertyNames returns array of property names (String))",
R"#(
  var result = Object.getOwnPropertyNames(String);
  var expResult = ["fromCharCode", "prototype", "length"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-7",
        R"(Object.getOwnPropertyNames returns array of property names (String.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(String.prototype);
  var expResult = ["constructor", "charAt", "charCodeAt", "concat", "indexOf", "lastIndexOf", "match", "replace", "search", "slice", "split", "substring", "substr", "toLowerCase", "toString", "toUpperCase", "valueOf", "toLocaleLowerCase", "toLocaleUpperCase", "localeCompare", "trim"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-8",
        R"(Object.getOwnPropertyNames returns array of property names (Boolean.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(Boolean.prototype);
  var expResult = ["constructor", "toString", "valueOf"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-9",
        R"(Object.getOwnPropertyNames returns array of property names (Number.prototype))",
R"#(
  var result = Object.getOwnPropertyNames(Number.prototype);
  var expResult = ["constructor", "toString", "toLocaleString", "toFixed", "toExponential", "toPrecision", "valueOf"];
  if (isSubsetOf(expResult, result)) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.4-4-b-1",
        R"(Object.getOwnPropertyNames - descriptor of resultant array is all true)",
R"#(
  var obj = new Object();
  obj.x = 1;
  obj.y = 2;
  var result = Object.getOwnPropertyNames(obj);
  var desc = Object.getOwnPropertyDescriptor(result,"0");
  if (desc.enumerable === true &&
      desc.configurable === true &&
      desc.writable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.5-0-1",
        R"(Object.create must exist as a function)",
R"#(
  if (typeof(Object.create) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.5-0-2",
        R"(Object.create must exist as a function taking 2 parameters)",
R"#(
  if (Object.create.length === 2) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.5-1",
        R"(Object.create throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.create(0);
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.5-2-1",
        R"(Object.create creates new Object)",
R"#(
    function base() {}
    var b = new base();
    var prop = new Object();
    var d = Object.create(b);

    if (typeof d === 'object') {
      return true;
    }
 )#"
    },
    {
        "15.2.3.5-3-1",
        R"(Object.create sets the prototype of the passed-in object)",
R"#(
    function base() {}
    var b = new base();
    var d = Object.create(b);

    if (Object.getPrototypeOf(d) === b &&
        b.isPrototypeOf(d) === true) {
      return true;
    }
 )#"
    },
    {
        "15.2.3.5-4-1",
        R"(Object.create sets the prototype of the passed-in object and adds new properties)",
R"#(
    function base() {}
    var b = new base();
    var prop = new Object();
    var d = Object.create(b,{ "x": {value: true,writable: false},
                              "y": {value: "str",writable: false} });

    if (Object.getPrototypeOf(d) === b &&
        b.isPrototypeOf(d) === true &&
        d.x === true &&
        d.y === "str" &&
        b.x === undefined &&
        b.y === undefined) {
      return true;
    }
 )#"
    },
    {
        "15.2.3.6-0-1",
        R"(Object.defineProperty must exist as a function)",
R"#(
  var f = Object.defineProperty;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-0-2",
        R"(Object.defineProperty must exist as a function taking 3 parameters)",
R"#(
  if (Object.defineProperty.length === 3) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-1",
        R"(Object.defineProperty throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.defineProperty(true, "foo", {});
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-1",
        R"(Object.defineProperty throws TypeError if desc has 'get' and 'value' present)",
R"#(
    var o = {};

    // dummy getter
    var getter = function () { return 1; }
    var desc = { get: getter, value: 101};
  
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-10",
        R"(Object.defineProperty throws TypeError if setter is not callable but not undefined (Number))",
R"#(
    var o = {};
    
    // dummy setter
    var setter = 42;
    var desc = { set: setter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-11",
        R"(Object.defineProperty throws TypeError if setter is not callable but not undefined (Boolean))",
R"#(
    var o = {};
    
    // dummy setter
    var setter = true;
    var desc = { set: setter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-12",
        R"(Object.defineProperty throws TypeError if setter is not callable but not undefined (String))",
R"#(
    var o = {};
    
    // dummy setter
    var setter = "abc";
    var desc = { set: setter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-13",
        R"(Object.defineProperty throws TypeError if the setter in desc is not callable (Null))",
R"#(
    var o = {};
    
    // dummy setter
    var setter = null;
    var desc = { set: setter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-14",
        R"(Object.defineProperty throws TypeError if setter is not callable but not undefined (Object))",
R"#(
    var o = {};
    
    // dummy getter
    var setter = { a: 1 };
    var desc = { set: setter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-2",
        R"(Object.defineProperty throws TypeError if desc has 'get' and 'writable' present)",
R"#(
    var o = {};
    
    // dummy getter
    var getter = function () { return 1; }
    var desc = { get: getter, writable: false };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-3",
        R"(Object.defineProperty throws TypeError if desc has 'set' and 'value' present)",
R"#(
    var o = {};

    // dummy setter
    var setter = function () { }
    var desc = { set: setter, value: 101};
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-4",
        R"(Object.defineProperty throws TypeError if desc has 'set' and 'writable' present)",
R"#(
    var o = {};
    
    // dummy getter
    var setter = function () { }
    var desc = { set: setter, writable: false };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-5",
        R"(Object.defineProperty throws TypeError if getter is not callable but not undefined (Number))",
R"#(
    var o = {};
    
    // dummy getter
    var getter = 42;
    var desc = { get: getter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-6",
        R"(Object.defineProperty throws TypeError if getter is not callable but not undefined (Boolean))",
R"#(
    var o = {};
    
    // dummy getter
    var getter = true;
    var desc = { get: getter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-7",
        R"(Object.defineProperty throws TypeError if getter is not callable but not undefined (String))",
R"#(
    var o = {};
    
    // dummy getter
    var getter = "abc";
    var desc = { get: getter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-8",
        R"(Object.defineProperty throws TypeError if getter is not callable but not undefined (Null))",
R"#(
    var o = {};
    
    // dummy getter
    var getter = null;
    var desc = { get: getter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-3-9",
        R"(Object.defineProperty throws TypeError if getter is not callable but not undefined (Object))",
R"#(
    var o = {};
    
    // dummy getter
    var getter = { a: 1 };
    var desc = { get: getter };
    
    try {
      Object.defineProperty(o, "foo", desc);
    }
    catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.6-4-1",
        R"(Object.defineProperty throws TypeError when adding properties to non-extensible objects)",
R"#(
  var o = {};
  Object.preventExtensions(o);

  try {
    var desc = { value: 1 };
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
      if (e instanceof TypeError &&
          (o.hasOwnProperty("foo") === false)) {
      return true;
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-10",
        R"(Object.defineProperty throws TypeError when changing [[Enumerable]] from false to true on non-configurable accessor properties)",
R"#(
  var o = {};

  // create an accessor property; all other attributes default to false.
  // dummy getter
  var getter = function () { return 1; }
  var d1 = { get: getter, enumerable: false, configurable: false };
  Object.defineProperty(o, "foo", d1);

  // now, setting enumerable to true should fail, since [[Configurable]]
  // on the original property will be false.
  var desc = { get: getter, enumerable: true };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain unchanged.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");
      if (d2.get === getter &&
          d2.enumerable === false &&
          d2.configurable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-11",
        R"(Object.defineProperty throws TypeError when changing [[Enumerable]] from true to false on non-configurable accessor properties)",
R"#(
  var o = {};

  // create an accessor property; all other attributes default to false.
  // dummy getter
  var getter = function () { return 1; }
  var d1 = { get: getter, enumerable: true, configurable: false };
  Object.defineProperty(o, "foo", d1);

  // now, setting enumerable to true should fail, since [[Configurable]]
  // on the original property will be false.
  var desc = { get: getter, enumerable: false };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain unchanged.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");
      if (d2.get === getter &&
          d2.enumerable === true &&
          d2.configurable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-12",
        R"(Object.defineProperty throws TypeError when changing non-configurable data properties to accessor properties)",
R"#(
  var o = {};

  // create a data valued property; all other attributes default to false.
  var d1 = { value: 101, configurable: false };
  Object.defineProperty(o, "foo", d1);

  // changing "foo" to be an accessor should fail, since [[Configurable]]
  // on the original property will be false.

  // dummy getter
  var getter = function () { return 1; }

  var desc = { get: getter };
  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain a data valued property.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");
      if (d2.value === 101 &&
          d2.writable === false &&
          d2.enumerable === false &&
          d2.configurable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-13",
        R"(Object.defineProperty throws TypeError when changing non-configurable accessor properties to data properties)",
R"#(
  var o = {};

  // create an accessor property; all other attributes default to false.
 
  // dummy getter
  var getter = function () { return 1; }
  var d1 = { get: getter, configurable: false };
  Object.defineProperty(o, "foo", d1);

  // changing "foo" to be a data property should fail, since [[Configurable]]
  // on the original property will be false.
  var desc = { value: 101 };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain an accessor property.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");
      if (d2.get === getter &&
          d2.configurable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-14",
        R"(Object.defineProperty permits changing data property to accessor property for configurable properties)",
R"#(
  var o = {};

  // create a data property. In this case,
  // [[Enumerable]] and [[Configurable]] are true
  o["foo"] = 101;

  // changing "foo" to be an accessor should succeed, since [[Configurable]]
  // on the original property will be true. Existing values of [[Configurable]]
  // and [[Enumerable]] need to be preserved and the rest need to be set to
  // their default values

  // dummy getter
  var getter = function () { return 1; }
  var d1 = { get: getter };
  Object.defineProperty(o, "foo", d1);

  var d2 = Object.getOwnPropertyDescriptor(o, "foo");

  if (d2.get === getter &&
      d2.enumerable === true &&
      d2.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-4-15",
        R"(Object.defineProperty permits changing accessor property to data property for configurable properties)",
R"#(
  var o = {};

  // define an accessor property
  // dummy getter
  var getter = function () { return 1; }
  var d1 = { get: getter, configurable: true };
  Object.defineProperty(o, "foo", d1);

  // changing "foo" to be a data valued property should succeed, since
  // [[Configurable]] on the original property will be true. Existing
  // values of [[Configurable]] and [[Enumerable]] need to be preserved
  // and the rest need to be set to their default values.
  var desc = { value: 101 };
  Object.defineProperty(o, "foo", desc);
  var d2 = Object.getOwnPropertyDescriptor(o, "foo");

  if (d2.value === 101 &&
      d2.writable === false &&
      d2.enumerable === false &&
      d2.configurable === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-4-16",
        R"(Object.defineProperty throws TypeError when relaxing [[Writable]] on non-configurable data properties)",
R"#(
  var o = {};

  // create a data valued property; all other attributes default to false.
  var d1 = { value: 101 };
  Object.defineProperty(o, "foo", d1);

  // now, relaxing [[Writable]] on "foo" should fail, since both
  // [[Configurable]] and [[Writable]] on the original property will be false.
  var desc = { value: 101, writable: true };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain unchanged.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");
      if (d2.value === 101 &&
          d2.writable === false &&
          d2.enumerable === false &&
          d2.configurable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-17",
        R"(Object.defineProperty throws TypeError when changing value of non-writable non-configurable data properties)",
R"#(
  var o = {};

  // create a data valued property; all other attributes default to false.
  var d1 = { value: 101 };
  Object.defineProperty(o, "foo", d1);

  // now, trying to change the value of "foo" should fail, since both
  // [[Configurable]] and [[Writable]] on the original property will be false.
  var desc = { value: 102 };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain unchanged.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");

      if (d2.value === 101 &&
          d2.writable === false &&
          d2.enumerable === false &&
          d2.configurable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-18",
        R"(Object.defineProperty throws TypeError when changing setter of non-configurable accessor properties)",
R"#(
  var o = {};

  // create an accessor property; all other attributes default to false.
  // dummy getter
  var getter = function () { return 1;}
  var d1 = { get: getter };
  Object.defineProperty(o, "foo", d1);

  // now, trying to change the setter should fail, since [[Configurable]]
  // on the original property will be false.
  var setter = function (x) {};
  var desc = { set: setter };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain unchanged.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");
      if (d2.get === getter &&
	      d2.configurable === false &&
          d2.enumerable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-19",
        R"(Object.defineProperty permits setting a setter (if absent) of non-configurable accessor properties)",
R"#(
  var o = {};

  // create an accessor property; all other attributes default to false.
  // dummy getter
  var getter = function () { return 1;}
  var d1 = { get: getter };
  Object.defineProperty(o, "foo", d1);

  // now, trying to set the setter should succeed even though [[Configurable]]
  // on the original property will be false.
  var desc = { set: undefined };
  Object.defineProperty(o, "foo", desc);

  var d2 = Object.getOwnPropertyDescriptor(o, "foo");

  if (d2.get === getter &&
	  d2.set === undefined &&
	  d2.configurable === false &&
	  d2.enumerable === false) {
	return true;
  }
 )#"
    },
    {
        "15.2.3.6-4-2",
        R"(Object.defineProperty sets missing attributes to their default values (data properties))",
R"#(
  var o = {};

  var desc = { value: 1 };
  Object.defineProperty(o, "foo", desc);
  
  var propDesc = Object.getOwnPropertyDescriptor(o, "foo");
  
  if (propDesc.value        === 1 &&          // this is the value that was set
      propDesc.writable     === false &&      // false by default
      propDesc.enumerable   === false &&      // false by default
      propDesc.configurable === false) {      // false by default
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-4-20",
        R"(Object.defineProperty throws TypeError when changing getter (if present) of non-configurable accessor properties)",
R"#(
  var o = {};

  // create an accessor property; all other attributes default to false.
  // dummy getter/setter
  var getter = function () { return 1;}
  var d1 = { get: getter, configurable: false };
  Object.defineProperty(o, "foo", d1);

  // now, trying to change the setter should fail, since [[Configurable]]
  // on the original property will be false.
  var desc = { get: undefined };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");

      if (d2.get === getter &&
	      d2.configurable === false &&
          d2.enumerable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-21",
        R"(Object.defineProperty permits setting a getter (if absent) of non-configurable accessor properties)",
R"#(
  var o = {};

  // create an accessor property; all other attributes default to false.
  // dummy setter
  var setter = function (x) {}
  var d1 = { set: setter };
  Object.defineProperty(o, "foo", d1);

  // now, trying to set the getter should succeed even though [[Configurable]]
  // on the original property will be false. Existing values of need to be preserved.
  var getter = undefined;
  var desc = { get: getter };

  Object.defineProperty(o, "foo", desc);
  var d2 = Object.getOwnPropertyDescriptor(o, "foo");

  if (d2.get === getter &&
      d2.set === setter &&
      d2.configurable === false &&
      d2.enumerable === false) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-4-3",
        R"(Object.defineProperty sets missing attributes to their default values (accessor))",
R"#(
  var o = {};

  var getter = function () { return 1; };
  var desc = { get: getter };

  Object.defineProperty(o, "foo", desc);

  var propDesc = Object.getOwnPropertyDescriptor(o, "foo");

  if (typeof(propDesc.get) === "function" &&  // the getter must be the function that was provided
      propDesc.get === getter &&
      propDesc.enumerable   === false &&      // false by default
      propDesc.configurable === false) {      // false by default
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-4-4",
        R"(Object.defineProperty defines a data property if given a generic desc)",
R"#(
  var o = {};
  
  var desc = {};
  Object.defineProperty(o, "foo", desc);

  var propDesc = Object.getOwnPropertyDescriptor(o, "foo");
  if (propDesc.value        === undefined &&  // this is the value that was set
      propDesc.writable     === false &&      // false by default
      propDesc.enumerable   === false &&      // false by default
      propDesc.configurable === false) {      // false by default
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-4-5",
        R"(Object.defineProperty is no-op if current and desc are the same data desc)",
R"#(
  function sameDataDescriptorValues(d1, d2) {
    return (d1.value === d2.value &&
            d1.enumerable === d2.enumerable &&
            d1.writable === d2.writable &&
            d1.configurable === d2.configurable);
  }

  var o = {};

  // create a data valued property with the following attributes:
  // value: 101, enumerable: true, writable: true, configurable: true
  o["foo"] = 101;

  // query for, and save, the desc. A subsequent call to defineProperty
  // with the same desc should not disturb the property definition.
  var d1 = Object.getOwnPropertyDescriptor(o, "foo");  

  // now, redefine the property with the same descriptor
  // the property defintion should not get disturbed.
  var desc = { value: 101, enumerable: true, writable: true, configurable: true };
  Object.defineProperty(o, "foo", desc);

  var d2 = Object.getOwnPropertyDescriptor(o, "foo"); 

  if (sameDataDescriptorValues(d1, d2) === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-4-6",
        R"(Object.defineProperty is no-op if current and desc are the same accessor desc)",
R"#(
  function sameAccessorDescriptorValues(d1, d2) {
    return (d1.get == d2.get &&
            d1.enumerable == d2.enumerable &&
            d1.configurable == d2.configurable);
  }

  var o = {};

  // create an accessor property with the following attributes:
  // enumerable: true, configurable: true
  var desc = {
               get: function () {},
               enumerable: true,
               configurable: true
             };

  Object.defineProperty(o, "foo", desc);

  // query for, and save, the desc. A subsequent call to defineProperty
  // with the same desc should not disturb the property definition.
  var d1 = Object.getOwnPropertyDescriptor(o, "foo");  

  // now, redefine the property with the same descriptor
  // the property defintion should not get disturbed.
  Object.defineProperty(o, "foo", desc);

  var d2 = Object.getOwnPropertyDescriptor(o, "foo"); 

  if (sameAccessorDescriptorValues(d1, d2) === true) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.6-4-7",
        R"(Object.defineProperty throws TypeError when changing [[Configurable]] from false to true)",
R"#(
  var o = {};

  // create a data valued property; all other attributes default to false.
  var d1 = { value: 101, configurable: false };
  Object.defineProperty(o, "foo", d1);

  var desc = { value: 101, configurable: true };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain unchanged.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");
      if (d2.value === 101 &&
          d2.configurable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-8",
        R"(Object.defineProperty throws TypeError when changing [[Enumerable]] from false to true on non-configurable data properties)",
R"#(
  var o = {};

  // create a data valued property; all other attributes default to false.
  var d1 = { value: 101, enumerable: false, configurable: false };
  Object.defineProperty(o, "foo", d1);

  // now, setting enumerable to true should fail, since [[Configurable]]
  // on the original property will be false.
  var desc = { value: 101, enumerable: true };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain unchanged.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");
      if (d2.value === 101 &&
          d2.enumerable === false &&
          d2.configurable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.6-4-9",
        R"(Object.defineProperty throws TypeError when changing [[Enumerable]] from true to false on non-configurable data properties)",
R"#(
  var o = {};

  // create a data valued property with [[Enumerable]] explicitly set to true;
  // all other attributes default to false.
  var d1 = { value: 101, enumerable: true, configurable: false };
  Object.defineProperty(o, "foo", d1);

  // now, setting enumerable to false should fail, since [[Configurable]]
  // on the original property will be false.
  var desc = { value: 101, enumerable: false };

  try {
    Object.defineProperty(o, "foo", desc);
  }
  catch (e) {
    if (e instanceof TypeError) {
      // the property should remain unchanged.
      var d2 = Object.getOwnPropertyDescriptor(o, "foo");
      if (d2.value === 101 &&
          d2.enumerable === true &&
          d2.configurable === false) {
        return true;
      }
    }
  }
 )#"
    },
    {
        "15.2.3.7-0-1",
        R"(Object.defineProperties must exist as a function)",
R"#(
  var f = Object.defineProperties;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.7-0-2",
        R"(Object.defineProperties must exist as a function taking 2 parameters)",
R"#(
  if (Object.defineProperties.length === 2) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.7-1",
        R"(Object.defineProperties throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.defineProperties(0, {});
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.8-0-1",
        R"(Object.seal must exist as a function)",
R"#(
  var f = Object.seal;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.8-0-2",
        R"(Object.seal must exist as a function taking 1 parameter)",
R"#(
  if (Object.seal.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.8-1",
        R"(Object.seal throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.seal(0);
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.2.3.9-0-1",
        R"(Object.freeze must exist as a function)",
R"#(
  var f = Object.freeze;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.2.3.9-0-2",
        R"(Object.freeze must exist as a function taking 1 parameter)",
R"#(
  if (Object.freeze.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.2.3.9-1",
        R"(Object.freeze throws TypeError if type of first param is not Object)",
R"#(
    try {
      Object.freeze(0);
    }
    catch (e) {
      if (e instanceof TypeError) {
        return true;
      }
    }
 )#"
    },
    {
        "15.3.2.1-11-1-s",
        R"(Duplicate seperate parameter name in Function constructor throws SyntaxError in strict mode)",
R"#(   
  try 
  {
    Function('a','a','"use strict";');
  }
  catch (e) {
    if(e instanceof SyntaxError)
    {
      return true;
    }
  }
 )#"
    },
    {
        "15.3.2.1-11-1",
        R"(Duplicate seperate parameter name in Function constructor allowed if body not strict)",
R"#(   
    Function('a','a','return;');
    return true;
 )#"
    },
    {
        "15.3.2.1-11-2-s",
        R"(Duplicate seperate parameter name in Function constructor called from strict mode allowed if body not strict)",
R"#( 
  "use strict"; 
  Function('a','a','return;');
  return true;
 )#"
    },
    {
        "15.3.2.1-11-3-s",
        R"(Function constructor having a formal parameter named 'eval' throws SyntaxError if function body is strict mode)",
R"#(
  

  try {
    Function('eval', '"use strict";');
  }
  catch (e) {
    if (e instanceof SyntaxError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.3.2.1-11-3",
        R"(Function constructor may have a formal parameter named 'eval' if body is not strict mode)",
R"#(
  Function('eval', 'return;');
  return true;
  )#"
    },
    {
        "15.3.2.1-11-4-s",
        R"(Function constructor call from strict code with formal parameter named 'eval' does not throws SyntaxError if function body is not strict mode)",
R"#(
   "use strict";
   Function('eval', 'return;');
   return true;
  )#"
    },
    {
        "15.3.2.1-11-5-s",
        R"(Duplicate combined parameter name in Function constructor throws SyntaxError in strict mode)",
R"#(   
  try 
  {
    Function('a,a','"use strict";');
  }
  catch (e) {
    if(e instanceof SyntaxError)
    {
      return true;
    }
  }
 )#"
    },
    {
        "15.3.2.1-11-5",
        R"(Duplicate combined parameter name in Function constructor allowed if body is not strict)",
R"#(   
    Function('a,a','return;');
    return true;
 )#"
    },
    {
        "15.3.2.1-11-6-s",
        R"(Duplicate combined parameter name allowed in Function constructor called in strict mode if body not strict)",
R"#( 
  "use strict"; 
  Function('a,a','return a;');
 )#"
    },
    {
        "15.3.2.1-11-7-s",
        R"(Function constructor call from strict code with formal parameter named arguments does not throws SyntaxError if function body is not strict mode)",
R"#(
   "use strict";
   Function('arguments', 'return;');
   return true;
  )#"
    },
    {
        "15.3.3.2-1",
        R"(Function.length - data property with value 1)",
R"#(

  var desc = Object.getOwnPropertyDescriptor(Function,"length");
  if(desc.value === 1 &&
     desc.writable === false &&
     desc.enumerable === false &&
     desc.configurable === false)
    return true; 

 )#"
    },
    {
        "15.3.4.5-0-1",
        R"(Function.prototype.bind must exist as a function)",
R"#(
  var f = Function.prototype.bind;

  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-0-2",
        R"(Function.prototype.bind must exist as a function taking 1 parameter)",
R"#(
  if (Function.prototype.bind.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-13.b-1",
        R"(Function.prototype.bind, bound fn has a 'length' own property)",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o);
  if (bf.hasOwnProperty('length')) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-13.b-2",
        R"(Function.prototype.bind, 'length' set to remaining number of expected args)",
R"#(
  function foo(x, y) { }
  var o = {};
  
  var bf = foo.bind(o);
  if (bf.length === 2) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-13.b-3",
        R"(Function.prototype.bind, 'length' set to remaining number of expected args (all args prefilled))",
R"#(
  function foo(x, y) { }
  var o = {};
  
  var bf = foo.bind(o, 42, 101);
  if (bf.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-13.b-4",
        R"(Function.prototype.bind, 'length' set to remaining number of expected args (target takes 0 args))",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o);
  if (bf.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-13.b-5",
        R"(Function.prototype.bind, 'length' set to remaining number of expected args (target provided extra args))",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o, 42);
  if (bf.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-15-1",
        R"(Function.prototype.bind, 'length' is a data valued own property)",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o);
  var desc = Object.getOwnPropertyDescriptor(bf, 'length');
  if (desc.hasOwnProperty('value') === true &&
      desc.hasOwnProperty('get') === false &&
      desc.hasOwnProperty('set') === false) {
    return true;
  }    
 )#"
    },
    {
        "15.3.4.5-15-2",
        R"(Function.prototype.bind, 'length' is a data valued own property with default attributes (false))",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o);
  var desc = Object.getOwnPropertyDescriptor(bf, 'length');
  if (desc.value === 0 &&
      desc.enumerable === false &&
      desc.writable === false &&
      desc.configurable == false) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-16-1",
        R"(Function.prototype.bind, [[Extensible]] of the bound fn is true)",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o);
  var ex = Object.isExtensible(bf);
  if (ex === true) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-2-1",
        R"(Function.prototype.bind throws TypeError if the Target is not callable (but an instance of Function))",
R"#(
  foo.prototype = Function.prototype;
  // dummy function
  function foo() {}
  var f = new foo();

  try {
    f.bind();
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.3.4.5-2-2",
        R"(Function.prototype.bind throws TypeError if the Target is not callable (bind attached to object))",
R"#(
  // dummy function 
  function foo() {}
  var f = new foo();
  f.bind = Function.prototype.bind;

  try {
    f.bind();
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.3.4.5-2-3",
        R"(Function.prototype.bind allows Target to be a constructor (Number))",
R"#(
  var bnc = Number.bind(null);
  var n = bnc(42);
  if (n === 42) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-2-4",
        R"(Function.prototype.bind allows Target to be a constructor (String))",
R"#(
  var bsc = String.bind(null);
  var s = bsc("hello world");
  if (s === "hello world") {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-2-5",
        R"(Function.prototype.bind allows Target to be a constructor (Boolean))",
R"#(
  var bbc = Boolean.bind(null);
  var b = bbc(true);
  if (b === true) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-2-6",
        R"(Function.prototype.bind allows Target to be a constructor (Object))",
R"#(
  var boc = Object.bind(null);
  var o = boc(42);
  if (o == 42) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-2-7",
        R"(Function.prototype.bind throws TypeError if the Target is not callable (JSON))",
R"#(
  try {
    JSON.bind();
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.3.4.5-2-8",
        R"(Function.prototype.bind allows Target to be a constructor (Array))",
R"#(
  var bac = Array.bind(null);
  var a = bac(42);
  
  // we can also add an Array.isArray check here.
  if (a.length === 42) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-2-9",
        R"(Function.prototype.bind allows Target to be a constructor (Date))",
R"#(
  var bdc = Date.bind(null);
  var s = bdc(0, 0, 0);
  if (typeof(s) === 'string') {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-8-1",
        R"(Function.prototype.bind, type of bound function must be 'function')",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o);
  if (typeof(bf) === 'function') {
    return  true;
  }
 )#"
    },
    {
        "15.3.4.5-8-2",
        R"(Function.prototype.bind, [[Class]] of bound function must be 'Function')",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o);
  var s = Object.prototype.toString.call(bf);
  if (s === '[object Function]') {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-9-1",
        R"(Function.prototype.bind, [[Prototype]] is Function.prototype)",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o);
  if (Function.prototype.isPrototypeOf(bf)) {
    return true;
  }
 )#"
    },
    {
        "15.3.4.5-9-2",
        R"(Function.prototype.bind, [[Prototype]] is Function.prototype (using getPrototypeOf))",
R"#(
  function foo() { }
  var o = {};
  
  var bf = foo.bind(o);
  if (Object.getPrototypeOf(bf) === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.4.3.2-0-1",
        R"(Array.isArray must exist as a function)",
R"#(
  var f = Array.isArray;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.3.2-0-2",
        R"(Array.isArray must exist as a function taking 1 parameter)",
R"#(
  if (Array.isArray.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.3.2-0-3",
        R"(Array.isArray return true if its argument is an Array)",
R"#(
  var a = [];
  var b = Array.isArray(a);
  if (b === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.3.2-0-4",
        R"(Array.isArray return false if its argument is not an Array)",
R"#(
  var b_num   = Array.isArray(42);
  var b_undef = Array.isArray(undefined);
  var b_bool  = Array.isArray(true);
  var b_str   = Array.isArray("abc");
  var b_obj   = Array.isArray({});
  var b_null  = Array.isArray(null);
  
  if (b_num === false &&
      b_undef === false &&
      b_bool === false &&
      b_str === false &&
      b_obj === false &&
      b_null === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.3.2-0-5",
        R"(Array.isArray return true if its argument is an Array (Array.prototype))",
R"#(
  var b = Array.isArray(Array.prototype);
  if (b === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.3.2-0-6",
        R"(Array.isArray return true if its argument is an Array (new Array()))",
R"#(
  var a = new Array(10);
  var b = Array.isArray(a);
  if (b === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.3.2-0-7",
        R"(Array.isArray returns false if its argument is not an Array)",
R"#(
  var o = new Object();
  o[12] = 13;
  var b = Array.isArray(o);
  if (b === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-0-1",
        R"(Array.prototype.indexOf must exist as a function)",
R"#(
  var f = Array.prototype.indexOf;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-0-2",
        R"(Array.prototype.indexOf has a length property whose value is 1.)",
R"#(
  if (Array.prototype.indexOf.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-1-1",
        R"(Array.prototype.indexOf applied to undefined throws a TypeError)",
R"#(
  try {Array.prototype.indexOf.call(undefined)}
  catch (e) {
     if (e instanceof TypeError) return true;
     }
 )#"
    },
    {
        "15.4.4.14-1-2",
        R"(Array.prototype.indexOf applied to null throws a TypeError)",
R"#(
  try {Array.prototype.indexOf.call(null)}
  catch (e) {
     if (e instanceof TypeError) return true;
     }
 )#"
    },
    {
        "15.4.4.14-10-1",
        R"(Array.prototype.indexOf returns -1 for elements not present in array)",
R"#(
  var a = new Array();
  a[100] = 1;
  a[99999] = "";  
  a[10] = new Object();
  a[5555] = 5.5;
  a[123456] = "str";
  a[5] = 1E+309;
  if (a.indexOf(1) !== 100 || 
      a.indexOf("") !== 99999 ||
      a.indexOf("str") !== 123456 ||
      a.indexOf(1E+309) !== 5 ||   //Infinity
      a.indexOf(5.5) !== 5555 )
  {
    return false;
  }
  if (a.indexOf(true) === -1 && 
      a.indexOf(5) === -1 &&
      a.indexOf("str1") === -1 &&
      a.indexOf(null) === -1 &&
      a.indexOf(new Object()) === -1) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-10-2",
        R"(Array.prototype.indexOf returns -1 if 'length' is 0 and does not access any other properties)",
R"#(
  var accessed = false;
  var f = {length: 0};
  Object.defineProperty(f,"0",{get: function () {accessed = true; return 1;}});

  
  var i = Array.prototype.indexOf.call(f,1);
  
  if (i === -1 && accessed==false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-4-1",
        R"(Array.prototype.indexOf returns -1 if 'length' is 0 (empty array))",
R"#(
  var i = [].indexOf(42);
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-4-2",
        R"(Array.prototype.indexOf returns -1 if 'length' is 0 ( length overridden to null (type conversion)))",
R"#(
  
  var i = Array.prototype.indexOf.call({length: null}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-4-3",
        R"(Array.prototype.indexOf returns -1 if 'length' is 0 (length overridden to false (type conversion)))",
R"#(
  
 var i = Array.prototype.indexOf.call({length: false}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-4-4",
        R"(Array.prototype.indexOf returns -1 if 'length' is 0 (generic 'array' with length 0 ))",
R"#(
  
 var i = Array.prototype.lastIndexOf.call({length: 0}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-4-5",
        R"(Array.prototype.indexOf returns -1 if 'length' is 0 ( length overridden to '0' (type conversion)))",
R"#(
  
 var i = Array.prototype.indexOf.call({length: '0'}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-4-6",
        R"(Array.prototype.indexOf returns -1 if 'length' is 0 (subclassed Array, length overridden with obj with valueOf))",
R"#(
  
 var i = Array.prototype.indexOf.call({length: { valueOf: function () { return 0;}}}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-4-7",
        R"(Array.prototype.indexOf returns -1 if 'length' is 0 ( length is object overridden with obj w/o valueOf (toString)))",
R"#(

  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
 var i = Array.prototype.indexOf.call({length: { toString: function () { return '0';}}}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-4-8",
        R"(Array.prototype.indexOf returns -1 if 'length' is 0 (length is an empty array))",
R"#(

  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.
 var i = Array.prototype.indexOf.call({length: [ ]}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-5-1",
        R"(Array.prototype.indexOf when fromIndex is string)",
R"#(
  var a = [1,2,1,2,1,2];
  if (a.indexOf(2,"2") === 3 &&          // "2" resolves to 2  
      a.indexOf(2,"one") === 1) {       // "one" resolves to 0
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-5-2",
        R"(Array.prototype.indexOf when fromIndex is floating point number)",
R"#(
  var a = new Array(1,2,3);
  if (a.indexOf(3,0.49) === 2 &&    // 0.49 resolves to 0
      a.indexOf(1,0.51) === 0 &&    // 0.51 resolves to 0
      a.indexOf(1,1.51) === -1) {   // 1.01 resolves to 1
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-5-3",
        R"(Array.prototype.indexOf when fromIndex is boolean)",
R"#(
  var a = [1,2,3];
  if (a.indexOf(1,true) === -1 &&        // true resolves to 1
     a.indexOf(1,false) === 0 ) {       // false resolves to 0
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-5-4",
        R"(Array.prototype.indexOf returns 0 if fromIndex is 'undefined')",
R"#(
  var a = [1,2,3];
  if (a.indexOf(1,undefined) === 0) {    // undefined resolves to 0
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-5-5",
        R"(Array.prototype.indexOf returns 0 if fromIndex is null)",
R"#(
  var a = [1,2,3];
  if (a.indexOf(1,null) === 0 ) {       // null resolves to 0
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-6-1",
        R"(Array.prototype.indexOf returns -1 if fromIndex is greater than Array.length)",
R"#(
  var a = [1,2,3];
  if (a.indexOf(1,5) === -1 &&  
     a.indexOf(1,3) === -1  &&
     [ ].indexOf(1,0) === -1  ){
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-8-1",
        R"(Array.prototype.indexOf with negative fromIndex)",
R"#(
  var a = new Array(1,2,3);
  
  if (a.indexOf(2,-1) === -1 &&  
      a.indexOf(2,-2) === 1 &&  
      a.indexOf(1,-3) === 0 &&  
      a.indexOf(1,-5.3) === 0 ) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-1",
        R"(Array.prototype.indexOf must return correct index (boolean))",
R"#(
  var obj = {toString:function(){return true}};
  var _false = false;
  var a = [obj,"true", undefined,0,_false,null,1,"str",0,1,true,false,true,false];
  if (a.indexOf(true) === 10 &&  //a[10]=true
      a.indexOf(false) === 4)    //a[4] =_false
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-10",
        R"(Array.prototype.indexOf must return correct index (NaN))",
R"#(
  var _NaN = NaN;
  var a = new Array("NaN",undefined,0,false,null,{toString:function(){return NaN}},"false",_NaN,NaN);
  if (a.indexOf(NaN) === -1)  // NaN is equal to nothing, including itself.
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-2",
        R"(Array.prototype.indexOf must return correct index (Number))",
R"#(
  var obj = {toString:function(){return 0}};
  var one = 1;
  var _float = -(4/3);
  var a = new Array(false,undefined,null,"0",obj,-1.3333333333333, "str",-0,true,+0, one, 1,0, false, _float, -(4/3));
  if (a.indexOf(-(4/3)) === 14 &&      // a[14]=_float===-(4/3)
      a.indexOf(0) === 7      &&       // a[7] = +0, 0===+0
      a.indexOf(-0) === 7      &&     // a[7] = +0, -0===+0
      a.indexOf(1) === 10 )            // a[10] =one=== 1
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-3",
        R"(Array.prototype.indexOf must return correct index(string))",
R"#(
  var obj = {toString:function(){return "false"}};
  var szFalse = "false";
  var a = new Array("false1",undefined,0,false,null,1,obj,0,szFalse, "false");
  if (a.indexOf("false") === 8)  //a[8]=szFalse
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-4",
        R"(Array.prototype.indexOf must return correct index(undefined))",
R"#(
  var obj = {toString:function(){return undefined;}};
  var _undefined1 = undefined;
  var _undefined2;
  var a = new Array(true,0,false,null,1,"undefined",obj,1,_undefined2,_undefined1,undefined);
  if (a.indexOf(undefined) === 8) //a[8]=_undefined2
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-5",
        R"(Array.prototype.indexOf must return correct index (Object))",
R"#(
  var obj1 = {toString:function(){return "false"}};
  var obj2 = {toString:function(){return "false"}};
  var obj3 = obj1;
  var a = new Array(false,undefined,0,false,null,{toString:function(){return "false"}},"false",obj2,obj1,obj3);
  if (a.indexOf(obj3) === 8)  //a[8] = obj1;
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-6",
        R"(Array.prototype.indexOf must return correct index(null))",
R"#(
  var obj = {toString:function(){return null}};
  var _null = null;
  var a = new Array(true,undefined,0,false,_null,1,"str",0,1,obj,true,false,null);
  if (a.indexOf(null) === 4 )  //a[4]=_null
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-7",
        R"(Array.prototype.indexOf must return correct index (self reference))",
R"#(
  var a = new Array(0,1,2,3);  
  a[2] = a;
  if (a.indexOf(a) === 2 &&  
      a.indexOf(3) === 3 ) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-8",
        R"(Array.prototype.indexOf must return correct index (Array))",
R"#(
  var b = new Array("0,1");  
  var a = new Array(0,b,"0,1",3);  
  if (a.indexOf(b.toString()) === 2 &&  
      a.indexOf("0,1") === 2 ) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.14-9-9",
        R"(Array.prototype.indexOf must return correct index (Sparse Array))",
R"#(
  var a = new Array(0,1);  
  a[4294967294] = 2;          // 2^32-2 - is max array element
  a[4294967295] = 3;          // 2^32-1 added as non-array element property
  a[4294967296] = 4;          // 2^32   added as non-array element property
  a[4294967297] = 5;          // 2^32+1 added as non-array element property

  // start searching near the end so in case implementation actually tries to test all missing elements!!
  return (a.indexOf(2,4294967290 ) === 4294967294 &&    
      a.indexOf(3,4294967290) === -1 &&   
      a.indexOf(4,4294967290) === -1 &&  
      a.indexOf(5,4294967290) === -1   ) ;
 )#"
    },
    {
        "15.4.4.15-0-1",
        R"(Array.prototype.lastIndexOf must exist as a function)",
R"#(
  var f = Array.prototype.lastIndexOf;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-0-2",
        R"(Array.prototype.lastIndexOf has a length property whose value is 1.)",
R"#(
  if (Array.prototype.lastIndexOf.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-4-1",
        R"(Array.prototype.lastIndexOf returns -1 if 'length' is 0 (empty array))",
R"#(
  var i = [].lastIndexOf(42);
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-4-2",
        R"(Array.prototype.lastIndexOf returns -1 if 'length' is 0 ( length overridden to null (type conversion)))",
R"#(
  
  var i = Array.prototype.lastIndexOf.call({length: null}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-4-3",
        R"(Array.prototype.lastIndexOf returns -1 if 'length' is 0 (length overridden to false (type conversion)))",
R"#(
  
 var i = Array.prototype.lastIndexOf.call({length: false}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-4-4",
        R"(Array.prototype.lastIndexOf returns -1 if 'length' is 0 (generic 'array' with length 0 ))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 0;
  
 var i = Array.prototype.lastIndexOf.call({length: 0}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-4-5",
        R"(Array.prototype.lastIndexOf returns -1 if 'length' is 0 ( length overridden to '0' (type conversion)))",
R"#(
  
 var i = Array.prototype.lastIndexOf.call({length: '0'}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-4-6",
        R"(Array.prototype.lastIndexOf returns -1 if 'length' is 0 (subclassed Array, length overridden with obj with valueOf))",
R"#(
  
 var i = Array.prototype.lastIndexOf.call({length: { valueOf: function () { return 0;}}}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-4-7",
        R"(Array.prototype.lastIndexOf returns -1 if 'length' is 0 ( length is object overridden with obj w/o valueOf (toString)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
 var i = Array.prototype.lastIndexOf.call({length: { toString: function () { return '0';}}}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-4-8",
        R"(Array.prototype.lastIndexOf returns -1 if 'length' is 0 (length is an empty array))",
R"#(

  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.
 var i = Array.prototype.lastIndexOf.call({length: [ ]}, 1);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-5-1",
        R"(Array.prototype.lastIndexOf when fromIndex is string)",
R"#(
  var a = new Array(0,1,1);
  if (a.lastIndexOf(1,"1") === 1 &&          // "1" resolves to 1
      a.lastIndexOf(1,"one") === -1) {       // NaN string resolves to 0
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-5-2",
        R"(Array.prototype.lastIndexOf when fromIndex is floating point number)",
R"#(
  var a = new Array(1,2,1);
  if (a.lastIndexOf(2,1.49) === 1 &&    // 1.49 resolves to 1
      a.lastIndexOf(2,0.51) === -1 &&    // 0.51 resolves to 0
      a.lastIndexOf(1,0.51) === 0){      // 0.51 resolves to 0
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-5-3",
        R"(Array.prototype.lastIndexOf when fromIndex is boolean)",
R"#(
  var a = new Array(1,2,1);
  if (a.lastIndexOf(2,true) === 1 &&        // true resolves to 1
     a.lastIndexOf(2,false) === -1 ) {      // false resolves to 0
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-5-4",
        R"(Array.prototype.lastIndexOf when fromIndex is undefined)",
R"#(
  var a = new Array(1,2,1);
  if (a.lastIndexOf(2,undefined) === -1 &&
      a.lastIndexOf(1,undefined) === 0  &&
      a.lastIndexOf(1) === 2)   {    // undefined resolves to 0, no second argument resolves to len
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-5-5",
        R"(Array.prototype.lastIndexOf when fromIndex is null)",
R"#(
  var a = new Array(1,2,1);
  if (a.lastIndexOf(2,null) === -1 && a.lastIndexOf(1,null) === 0) {       // null resolves to 0
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-5-6",
        R"(Array.prototype.lastIndexOf when fromIndex is Number(integer))",
R"#(
  var a = new Array(1,2,1,1,2);
  if (a.lastIndexOf(2,0) === -1 &&
      a.lastIndexOf(2,1) ===  1 &&
      a.lastIndexOf(2)   ===  4 &&
      a.lastIndexOf(1,2) ===  2 ) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-6-1",
        R"(Array.prototype.lastIndexOf when fromIndex greater than Array.length)",
R"#(
  var a = new Array(1,2,3);
  if (a.lastIndexOf(3,5.4) === 2 &&  
     a.lastIndexOf(3,3.1) === 2 ) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-7-1",
        R"(Array.prototype.lastIndexOf with negative fromIndex )",
R"#(
  var a = new Array(1,2,3);
  
  if (a.lastIndexOf(2,-2) === 1 &&  
      a.lastIndexOf(2,-3) === -1 &&  
      a.lastIndexOf(1,-5.3) === -1 ) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-1",
        R"(Array.prototype.lastIndexOf must return correct index(boolean))",
R"#(
  var obj = {toString:function(){return true}};
  var _false = false;
  var a = new Array(false,true,false,obj,_false,true,"true", undefined,0,null,1,"str",0,1);
  if (a.lastIndexOf(true) === 5 &&  //a[5]=true
      a.lastIndexOf(false) === 4)    //a[4] =_false
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-10",
        R"(Array.prototype.lastIndexOf must return correct index (NaN))",
R"#(
  var _NaN = NaN;
  var a = new Array("NaN",_NaN,NaN, undefined,0,false,null,{toString:function(){return NaN}},"false");
  if (a.lastIndexOf(NaN) === -1)  // NaN matches nothing, not even itself
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-2",
        R"(Array.prototype.lastIndexOf must return correct index(Number))",
R"#(
  var obj = {toString:function(){return 0}};
  var one = 1;
  var _float = -(4/3);
  var a = new Array(+0,true,0,-0, false,undefined,null,"0",obj, _float,-(4/3),-1.3333333333333,"str",one, 1, false);
  if (a.lastIndexOf(-(4/3)) === 10 &&      // a[10]=-(4/3)
      a.lastIndexOf(0) === 3       &&       // a[3] = -0, but using === -0 and 0 are equal
      a.lastIndexOf(-0) ===3       &&      // a[3] = -0
      a.lastIndexOf(1) === 14 )            // a[14] = 1
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-3",
        R"(Array.prototype.lastIndexOf must return correct index(string))",
R"#(
  var obj = {toString:function(){return "false"}};
  var szFalse = "false";
  var a = new Array(szFalse, "false","false1",undefined,0,false,null,1,obj,0);
  if (a.lastIndexOf("false") === 1) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-4",
        R"(Array.prototype.lastIndexOf must return correct index(undefined))",
R"#(
  var obj = {toString:function(){return undefined;}};
  var _undefined1 = undefined;
  var _undefined2;
  var a = new Array(_undefined1,_undefined2,undefined,true,0,false,null,1,"undefined",obj,1);
  if (a.lastIndexOf(undefined) === 2) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-5",
        R"(Array.prototype.lastIndexOf must return correct index(Object))",
R"#(
  var obj1 = {toString:function(){return "false"}};
  var obj2 = {toString:function(){return "false"}};
  var obj3 = obj1;
  var a = new Array(obj2,obj1,obj3,false,undefined,0,false,null,{toString:function(){return "false"}},"false");
  if (a.lastIndexOf(obj3) === 2) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-6",
        R"(Array.prototype.lastIndexOf must return correct index(null))",
R"#(
  var obj = {toString:function(){return null}};
  var _null = null;
  var a = new Array(true,undefined,0,false,null,1,"str",0,1,null,true,false,undefined,_null,"null",undefined,"str",obj);
  if (a.lastIndexOf(null) === 13 ) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-7",
        R"(Array.prototype.lastIndexOf must return correct index (self reference))",
R"#(
  var a = new Array(0,1,2,3);  
  a[2] = a;
  if (a.lastIndexOf(a) === 2 &&  
      a.lastIndexOf(3) === 3 ) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-8",
        R"(Array.prototype.lastIndexOf must return correct index (Array))",
R"#(
  var b = new Array("0,1");  
  var a = new Array(0,b,"0,1",3);  
  if (a.lastIndexOf(b.toString()) === 2 &&  
      a.lastIndexOf("0,1") === 2 ) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-8-9",
        R"(Array.prototype.lastIndexOf must return correct index (Sparse Array))",
R"#(
  var a = new Array(0,1);  
  a[4294967294] = 2;          // 2^32-2 - is max array element index
  a[4294967295] = 3;          // 2^32-1 added as non-array element property
  a[4294967296] = 4;          // 2^32   added as non-array element property
  a[4294967297] = 5;          // 2^32+1 added as non-array element property
  // stop searching near the end in case implementation actually tries to test all missing elements!!
  a[4294967200] = 3;          
  a[4294967201] = 4;         
  a[4294967202] = 5;         


  return (a.lastIndexOf(2) === 4294967294 &&    
      a.lastIndexOf(3) === 4294967200 &&
      a.lastIndexOf(4) === 4294967201 &&
      a.lastIndexOf(5) === 4294967202) ;
   )#"
    },
    {
        "15.4.4.15-9-1",
        R"(Array.prototype.lastIndexOf returns -1 for elements not present)",
R"#(
  var a = new Array();
  a[100] = 1;
  a[99999] = "";  
  a[10] = new Object();
  a[5555] = 5.5;
  a[123456] = "str";
  a[5] = 1E+309;
  if (a.lastIndexOf(1) !== 100 ||
      a.lastIndexOf("") !== 99999 ||
      a.lastIndexOf("str") !== 123456 ||
      a.lastIndexOf(5.5) !== 5555 ||
      a.lastIndexOf(1E+309) !== 5 )      
  {
    return false;
  }    
  if (a.lastIndexOf(true) === -1 && 
      a.lastIndexOf(5) === -1 &&
      a.lastIndexOf("str1") === -1 &&
      a.lastIndexOf(null) === -1  &&
      a.lastIndexOf(new Object()) === -1 ) 
  {
    return true;
  }
 )#"
    },
    {
        "15.4.4.15-9-2",
        R"(Array.prototype.lastIndexOf returns -1 if 'length' is 0 and does not access any other properties)",
R"#(
  var accessed = false;
  var f = {length: 0};
  Object.defineProperty(f,"0",{get: function () {accessed = true; return 1;}});
  
  var i = Array.prototype.lastIndexOf.call(f,1);
  
  if (i === -1 && accessed==false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-0-1",
        R"(Array.prototype.every must exist as a function)",
R"#(
  var f = Array.prototype.every;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-0-2",
        R"(Array.prototype.every.length must be 1)",
R"#(
  if (Array.prototype.every.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-4-1",
        R"(Array.prototype.every throws TypeError if callbackfn is undefined)",
R"#(

  var arr = new Array(10);
  try {
    arr.every();    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.16-4-3",
        R"(Array.prototype.every throws TypeError if callbackfn is null)",
R"#(

  var arr = new Array(10);
  try {
    arr.every(null);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.16-4-4",
        R"(Array.prototype.every throws TypeError if callbackfn is boolean)",
R"#(

  var arr = new Array(10);
  try {
    arr.every(true);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.16-4-5",
        R"(Array.prototype.every throws TypeError if callbackfn is number)",
R"#(

  var arr = new Array(10);
  try {
    arr.every(5);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.16-4-6",
        R"(Array.prototype.every throws TypeError if callbackfn is string)",
R"#(

  var arr = new Array(10);
  try {
    arr.every("abc");    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.16-4-7",
        R"(Array.prototype.every throws TypeError if callbackfn is Object withouth Call internal method)",
R"#(

  var arr = new Array(10);
  try {
    arr.every( {} );    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.16-5-1-s",
        R"(Array.prototype.every - thisArg not passed to strict callbackfn)",
R"#(
  var innerThisCorrect = false;
  
  function callbackfn(val, idx, obj)
  {
    "use strict";
	innerThisCorrect = this===undefined;
	return true;
  }

  [1].every(callbackfn);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.16-5-1",
        R"(Array.prototype.every - thisArg not passed)",
R"#(
  var innerThisCorrect = false;
  
  function callbackfn(val, idx, obj)
  {
    innerThisCorrect = this===fnGlobalObject();
	return true;
  }

  [1].every(callbackfn);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.16-5-2",
        R"(Array.prototype.every - thisArg is Object)",
R"#(
  var res = false;
  var o = new Object();
  o.res = true;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  var arr = [1];
  if(arr.every(callbackfn, o) === true)
    return true;    

 )#"
    },
    {
        "15.4.4.16-5-3",
        R"(Array.prototype.every - thisArg is Array)",
R"#(
  var res = false;
  var a = new Array();
  a.res = true;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  var arr = [1];

  if(arr.every(callbackfn, a) === true)
    return true;    

 )#"
    },
    {
        "15.4.4.16-5-4",
        R"(Array.prototype.every - thisArg is object from object template(prototype))",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }
  
  function foo(){}
  foo.prototype.res = true;
  var f = new foo();
  var arr = [1];

    if(arr.every(callbackfn,f) === true)
      return true;    

 )#"
    },
    {
        "15.4.4.16-5-5",
        R"(Array.prototype.every - thisArg is object from object template)",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  function foo(){}
  var f = new foo();
  f.res = true;
  var arr = [1];

  if(arr.every(callbackfn,f) === true)
    return true;    

 )#"
    },
    {
        "15.4.4.16-5-6",
        R"(Array.prototype.every - thisArg is function)",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  function foo(){}
  foo.res = true;
  var arr = [1];

  if(arr.every(callbackfn,foo) === true)
    return true;    

 )#"
    },
    {
        "15.4.4.16-7-1",
        R"(Array.prototype.every doesn't consider new elements beyond the initial length of array after it is called)",
R"#( 
 
  var lastIndexVisited=-1; 
 
  function callbackfn(val, idx, obj)
  {
    if (idx===0) obj[10] = 10;
    lastIndexVisited=idx;
    return true;
  }
  
  [0,1].every(callbackfn);
  return lastIndexVisited === 1;  
  
 )#"
    },
    {
        "15.4.4.16-7-2",
        R"(Array.prototype.every considers new value of elements in array after the call)",
R"#( 
 
  function callbackfn(val, Idx, obj)
  {
    arr[4] = 6;
    if(val < 6)
       return true;
    else 
       return false;
  }

  var arr = [1,2,3,4,5];
  
  if(arr.every(callbackfn) === false)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.16-7-3",
        R"(Array.prototype.every doesn't visit deleted elements in array after the call)",
R"#( 
 
  function callbackfn(val, Idx, obj)
  {
    delete arr[2];
    if(val == 3)
       return false;
    else 
       return true;
  }

  var arr = [1,2,3,4,5];
  
  if(arr.every(callbackfn) === true)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.16-7-4",
        R"(Array.prototype.every doesn't visit deleted elements when Array.length is decreased)",
R"#( 
 
  function callbackfn(val, Idx, obj)
  {
    arr.length = 3;
    if(val < 4)
       return true;
    else 
       return false;
  }

  var arr = [1,2,3,4,6];
  
  if(arr.every(callbackfn) === true)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.16-7-6",
        R"(Array.prototype.every visits deleted element in array after the call when same index is also present in prototype)",
R"#( 
 
  function callbackfn(val, Idx, obj)
  {
    delete arr[2];
    if(val == 3)
       return false;
    else 
       return true;
  }

  Array.prototype[2] = 3;
  var arr = [1,2,3,4,5];
  
  var res = arr.every(callbackfn);
  delete Array.prototype[2];

  if(res === false)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.16-7-b-1",
        R"(Array.prototype.every - callbackfn not called for indexes never been assigned values)",
R"#( 
 
  var callCnt = 0.;
  function callbackfn(val, Idx, obj)
  {
    callCnt++;
    return true;
  }

  var arr = new Array(10);
  arr[1] = undefined;  
  arr.every(callbackfn);
  if( callCnt === 1)    
      return true;  
 )#"
    },
    {
        "15.4.4.16-7-c-ii-1",
        R"(Array.prototype.every - callbackfn called with correct parameters)",
R"#( 
 
  function callbackfn(val, Idx, obj)
  {
    if(obj[Idx] === val)
      return true;
  }

  var arr = [0,1,2,3,4,5,6,7,8,9];
  
  if(arr.every(callbackfn) === true)
    return true;
 )#"
    },
    {
        "15.4.4.16-7-c-ii-2",
        R"(Array.prototype.every - callbackfn takes 3 arguments)",
R"#( 
 
  function callbackfn(val, Idx, obj)
  {
    if(arguments.length === 3)   //verify if callbackfn was called with 3 parameters
       return true;
  }

  var arr = [0,1,true,null,new Object(),"five"];
  arr[999999] = -6.6;
  
  if(arr.every(callbackfn) === true)
    return true;
 )#"
    },
    {
        "15.4.4.16-7-c-ii-3",
        R"(Array.prototype.every immediately returns false if callbackfn returns false)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
    if(idx > 5)   
       return false;
    else
       return true;
  }

  var arr = [0,1,2,3,4,5,6,7,8,9];
  
  if(arr.every(callbackfn) === false && callCnt === 7) 
    return true;


 )#"
    },
    {
        "15.4.4.16-8-1",
        R"(Array.prototype.every returns true if 'length' is 0 (empty array))",
R"#(
  function cb() {}
  var i = [].every(cb);
  if (i === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-8-10",
        R"(Array.prototype.every - subclassed array when length is reduced)",
R"#(
  foo.prototype = [1, 2, 3];
  function foo() {}
  var f = new foo();
  f.length = 2;
  
  function cb(val, idx)
  {
    if(idx>1)
      return false;
    else
      return true;    
  }
  var i = f.every(cb);
  
  if (i === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-8-11",
        R"(Array.prototype.every returns true when all calls to callbackfn return true)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
    return true;
  }

  var arr = [0,1,2,3,4,5,6,7,8,9];
  
  if(arr.every(callbackfn) === true && callCnt === 10) 
    return true;
 )#"
    },
    {
        "15.4.4.16-8-12",
        R"(Array.prototype.every doesn't mutate the array on which it is called on)",
R"#(

  function callbackfn(val, idx, obj)
  {
    return true;
  }
  var arr = [1,2,3,4,5];
  arr.every(callbackfn);
  if(arr[0] === 1 &&
     arr[1] === 2 &&
     arr[2] === 3 &&
     arr[3] === 4 &&
     arr[4] === 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.16-8-13",
        R"(Array.prototype.every doesn't visit expandos)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
    return true;
  }

  var arr = [0,1,2,3,4,5,6,7,8,9];
  arr["i"] = 10;
  arr[true] = 11;
  
  if(arr.every(callbackfn) === true && callCnt === 10) 
    return true;
 )#"
    },
    {
        "15.4.4.16-8-2",
        R"(Array.prototype.every returns true if 'length' is 0 (subclassed Array, length overridden to null (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = null;
  
  function cb(){}
  var i = f.every(cb);
  
  if (i === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-8-3",
        R"(Array.prototype.every returns true if 'length' is 0 (subclassed Array, length overridden to false (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = false;

  function cb(){}
  var i = f.every(cb);
  
  if (i === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-8-4",
        R"(Array.prototype.every returns true if 'length' is 0 (subclassed Array, length overridden to 0 (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 0;

  function cb(){}
  var i = f.every(cb);
  
  if (i === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-8-5",
        R"(Array.prototype.every returns true if 'length' is 0 (subclassed Array, length overridden to '0' (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = '0';
  
  function cb(){}
  var i = f.every(cb);
  
  if (i === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-8-6",
        R"(Array.prototype.every returns true if 'length' is 0 (subclassed Array, length overridden with obj with valueOf))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { valueOf: function () { return 0;}};
  f.length = o;
  
  function cb(){}
  var i = f.every(cb);
  
  if (i === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-8-7",
        R"(Array.prototype.every returns true if 'length' is 0 (subclassed Array, length overridden with obj w/o valueOf (toString)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  function cb(){}
  var i = f.every(cb);
  
  if (i === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.16-8-8",
        R"(Array.prototype.every returns true if 'length' is 0 (subclassed Array, length overridden with [])",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  var i = f.every(cb);
  
  if (i === true) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-0-1",
        R"(Array.prototype.some must exist as a function)",
R"#(
  var f = Array.prototype.some;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-0-2",
        R"(Array.prototype.some.length must be 1)",
R"#(
  if (Array.prototype.some.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-4-1",
        R"(Array.prototype.some throws TypeError if callbackfn is undefined)",
R"#(

  var arr = new Array(10);
  try {
    arr.some();    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.17-4-2",
        R"(Array.prototype.some throws ReferenceError if callbackfn is unreferenced)",
R"#(

  var arr = new Array(10);
  try {
    arr.some(foo);    
  }
  catch(e) {
    if(e instanceof ReferenceError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.17-4-3",
        R"(Array.prototype.some throws TypeError if callbackfn is null)",
R"#(

  var arr = new Array(10);
  try {
    arr.some(null);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.17-4-4",
        R"(Array.prototype.some throws TypeError if callbackfn is boolean)",
R"#(

  var arr = new Array(10);
  try {
    arr.some(true);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.17-4-5",
        R"(Array.prototype.some throws TypeError if callbackfn is number)",
R"#(

  var arr = new Array(10);
  try {
    arr.some(5);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.17-4-6",
        R"(Array.prototype.some throws TypeError if callbackfn is string)",
R"#(

  var arr = new Array(10);
  try {
    arr.some("abc");    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.17-4-7",
        R"(Array.prototype.some throws TypeError if callbackfn is Object withouth Call internal method)",
R"#(

  var arr = new Array(10);
  try {
    arr.some(new Object());    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.17-4-8",
        R"(Array.prototype.some throws TypeError if callbackfn is object)",
R"#(

  var arr = new Array(10);
  try {
    arr.some({});    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.17-4-9",
        R"(Array.prototype.some returns -1 if 'length' is 0 (subclassed Array, length overridden with [0])",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [0];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  var i = f.some(cb);
  
  if (i === -1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-5-1-s",
        R"(Array.prototype.some - thisArg not passed to strict callbackfn)",
R"#(
  var innerThisCorrect = false;
  
  function callbackfn(val, idx, obj)
  {
    "use strict";
	innerThisCorrect = this===undefined;
	return true;
  }

  [1].some(callbackfn);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.17-5-1",
        R"(Array.prototype.some - thisArg not passed)",
R"#(
  var innerThisCorrect = false;
  
  function callbackfn(val, idx, obj)
  {
    innerThisCorrect = this===fnGlobalObject();
    return true;
  }
  [1].some(callbackfn);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.17-5-2",
        R"(Array.prototype.some - thisArg is Object)",
R"#(
  var res = false;
  var o = new Object();
  o.res = true;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  var arr = [1];
  if(arr.some(callbackfn, o) === true)
    return true;    

 )#"
    },
    {
        "15.4.4.17-5-3",
        R"(Array.prototype.some - thisArg is Array)",
R"#(
  var res = false;
  var a = new Array();
  a.res = true;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  var arr = [1];

  if(arr.some(callbackfn, a) === true)
    return true;    

 )#"
    },
    {
        "15.4.4.17-5-4",
        R"(Array.prototype.some - thisArg is object from object template(prototype))",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }
  
  function foo(){}
  foo.prototype.res = true;
  var f = new foo();
  var arr = [1];

    if(arr.some(callbackfn,f) === true)
      return true;    

 )#"
    },
    {
        "15.4.4.17-5-5",
        R"(Array.prototype.some - thisArg is object from object template)",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  function foo(){}
  var f = new foo();
  f.res = true;
  var arr = [1];

  if(arr.some(callbackfn,f) === true)
    return true;    

 )#"
    },
    {
        "15.4.4.17-5-6",
        R"(Array.prototype.some - thisArg is function)",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  function foo(){}
  foo.res = true;
  var arr = [1];

  if(arr.some(callbackfn,foo) === true)
    return true;    

 )#"
    },
    {
        "15.4.4.17-7-1",
        R"(Array.prototype.some doesn't consider new elements added to array after it is called)",
R"#( 
 
  var lastIndexVisited=-1; 
 
  function callbackfn(val, idx, obj)
  {
    if (idx===0) obj[10] = 10;
    lastIndexVisited=idx;
    return false
  }
  
  [0,1].some(callbackfn);
  return lastIndexVisited === 1;  
  
 )#"
    },
    {
        "15.4.4.17-7-2",
        R"(Array.prototype.some considers new value of elements in array after it is called)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    arr[4] = 6;
    if(val < 6)
      return false;
    else 
      return true;
  }

  var arr = [1,2,3,4,5];
  
  if(arr.some(callbackfn) === true)    
    return true;  
  
 )#"
    },
    {
        "15.4.4.17-7-3",
        R"(Array.prototype.some doesn't visit deleted elements in array after it is called)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    delete arr[2];
    if(val !== 3)
      return false;
    else 
      return true;
  }

  var arr = [1,2,3,4,5];
  
  if(arr.some(callbackfn) === false)    
    return true;  
  
 )#"
    },
    {
        "15.4.4.17-7-4",
        R"(Array.prototype.some doesn't visit deleted elements when Array.length is decreased)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    arr.length = 3;
    if(val < 4)
      return false;
    else 
      return true;
  }
  
  var arr = [1,2,3,4,6];
  
  if(arr.some(callbackfn) === false)    
    return true;    
 )#"
    },
    {
        "15.4.4.17-7-5",
        R"(Array.prototype.some doesn't consider newly added elements in sparse array)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    arr[1000] = 5;
    if(val < 5)
      return false;
    else 
      return true;
  }

  var arr = new Array(10);
  arr[1] = 1;
  arr[2] = 2;
  
  if(arr.some(callbackfn) === false)    
    return true;  
 
 )#"
    },
    {
        "15.4.4.17-7-6",
        R"(Array.prototype.some visits deleted element in array after the call when same index is also present in prototype)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    delete arr[4];
    if(val < 5)
      return false;
    else 
      return true;
  }


  Array.prototype[4] = 5;
  var arr = [1,2,3,4,5];
  
  var res = arr.some(callbackfn) ;
  delete Array.prototype[4];
  if(res === true)    
    return true;  
  
 )#"
    },
    {
        "15.4.4.17-7-b-1",
        R"(Array.prototype.some - callbackfn not called for indexes never been assigned values)",
R"#( 

  var callCnt = 0; 
  function callbackfn(val, idx, obj)
  {
    callCnt++;
    return false;
  }

  var arr = new Array(10);
  arr[1] = undefined;
  arr.some(callbackfn);
  if(callCnt === 1)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.17-7-c-ii-1",
        R"(Array.prototype.some - callbackfn called with correct parameters)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    if(obj[idx] === val)
      return false;
    else
      return true;
  }

  var arr = [0,1,2,3,4,5,6,7,8,9];
  
  if(arr.some(callbackfn) === false)
    return true;


 )#"
    },
    {
        "15.4.4.17-7-c-ii-2",
        R"(Array.prototype.some - callbackfn takes 3 arguments)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    if(arguments.length === 3)   //verify if callbackfn was called with 3 parameters
      return false;
    else
      return true;
  }

  var arr = [0,1,true,null,new Object(),"five"];
  arr[999999] = -6.6;
  
  if(arr.some(callbackfn) === false)
    return true;


 )#"
    },
    {
        "15.4.4.17-7-c-ii-3",
        R"(Array.prototype.some immediately returns true if callbackfn returns true)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
    if(idx > 5)   
      return true;
    else
      return false;
  }

  var arr = [0,1,2,3,4,5,6,7,8,9];
  
  if(arr.some(callbackfn) === true && callCnt === 7) 
    return true;
 )#"
    },
    {
        "15.4.4.17-8-1",
        R"(Array.prototype.some returns false if 'length' is 0 (empty array))",
R"#(
  function cb(){}
  var i = [].some(cb);
  if (i === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-8-10",
        R"(Array.prototype.some - subclassed array when length is reduced)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 2;
  
  function cb(val)
  {
    if(val > 2)
      return false;
    else
      return true;
  }
  var i = f.some(cb);
  
  if (i === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-8-11",
        R"(Array.prototype.some returns false when all calls to callbackfn return false)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
    return false;
  }

  var arr = [0,1,2,3,4,5,6,7,8,9];
  
  if(arr.some(callbackfn) === false && callCnt === 10) 
    return true;
 )#"
    },
    {
        "15.4.4.17-8-12",
        R"(Array.prototype.some doesn't mutate the array on which it is called on)",
R"#(

  function callbackfn(val, idx, obj)
  {
    return true;
  }
  var arr = [1,2,3,4,5];
  arr.some(callbackfn);
  if(arr[0] === 1 &&
     arr[1] === 2 &&
     arr[2] === 3 &&
     arr[3] === 4 &&
     arr[4] === 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.17-8-13",
        R"(Array.prototype.some doesn't visit expandos)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
    return false;
  }

  var arr = [0,1,2,3,4,5,6,7,8,9];
  arr["i"] = 10;
  arr[true] = 11;
  
  if(arr.some(callbackfn) === false && callCnt === 10) 
    return true;
 )#"
    },
    {
        "15.4.4.17-8-2",
        R"(Array.prototype.some returns false if 'length' is 0 (subclassed Array, length overridden to null (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = null;
  
  function cb(){}
  var i = f.some(cb);
  
  if (i === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-8-3",
        R"(Array.prototype.some returns false if 'length' is 0 (subclassed Array, length overridden to false (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = false;
  
  function cb(){}
  var i = f.some(cb);
  
  if (i === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-8-4",
        R"(Array.prototype.some returns false if 'length' is 0 (subclassed Array, length overridden to 0 (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 0;
  
  function cb(){}
  var i = f.some(cb);
  
  if (i === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-8-5",
        R"(Array.prototype.some returns false if 'length' is 0 (subclassed Array, length overridden to '0' (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = '0';
  
  function cb(){}
  var i = f.some(cb);
  
  if (i === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-8-6",
        R"(Array.prototype.some returns false if 'length' is 0 (subclassed Array, length overridden with obj with valueOf))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { valueOf: function () { return 0;}};
  f.length = o;
  
  function cb(){}
  var i = f.some(cb);
  
  if (i === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-8-7",
        R"(Array.prototype.some returns false if 'length' is 0 (subclassed Array, length overridden with obj w/o valueOf (toString)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.

  function cb(){}
  var i = f.some(cb);
  
  if (i === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.17-8-8",
        R"(Array.prototype.some returns false if 'length' is 0 (subclassed Array, length overridden with [])",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  var i = f.some(cb);
  
  if (i === false) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-0-1",
        R"(Array.prototype.forEach must exist as a function)",
R"#(
  var f = Array.prototype.forEach;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-0-2",
        R"(Array.prototype.forEach.length must be 1)",
R"#(
  if (Array.prototype.forEach.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-4-1",
        R"(Array.prototype.forEach throws TypeError if callbackfn is undefined)",
R"#(

  var arr = new Array(10);
  try {
    arr.forEach();    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.18-4-2",
        R"(Array.prototype.forEach throws ReferenceError if callbackfn is unreferenced)",
R"#(

  var arr = new Array(10);
  try {
    arr.forEach(foo);    
  }
  catch(e) {
    if(e instanceof ReferenceError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.18-4-3",
        R"(Array.prototype.forEach throws TypeError if callbackfn is null)",
R"#(

  var arr = new Array(10);
  try {
    arr.forEach(null);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.18-4-4",
        R"(Array.prototype.forEach throws TypeError if callbackfn is boolean)",
R"#(

  var arr = new Array(10);
  try {
    arr.forEach(true);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.18-4-5",
        R"(Array.prototype.forEach throws TypeError if callbackfn is number)",
R"#(

  var arr = new Array(10);
  try {
    arr.forEach(5);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.18-4-6",
        R"(Array.prototype.forEach throws TypeError if callbackfn is string)",
R"#(

  var arr = new Array(10);
  try {
    arr.forEach("abc");    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.18-4-7",
        R"(Array.prototype.forEach throws TypeError if callbackfn is Object without Call internal method)",
R"#(

  var arr = new Array(10);
  try {
    arr.forEach(new Object());    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.18-4-8",
        R"(Array.prototype.forEach throws TypeError if callbackfn is object)",
R"#(

  var arr = new Array(10);
  try {
    arr.forEach({});    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.18-5-1-s",
        R"(Array.prototype.forEach - thisArg not passed to strict callbackfn)",
R"#(
  var innerThisCorrect = false;
  
  function callbackfn(val, idx, obj)
  {
    "use strict";
	innerThisCorrect = this===undefined;
	return true;
  }

  [1].forEach(callbackfn);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.18-5-1",
        R"(Array.prototype.forEach - thisArg not passed)",
R"#(
  var innerThisCorrect = false;

  function callbackfn(val, idx, obj)
  {
    innerThisCorrect = this===fnGlobalObject();
    return true;
  }

  [1].forEach(callbackfn)
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.18-5-2",
        R"(Array.prototype.forEach - thisArg is Object)",
R"#(
  var res = false;
  var o = new Object();
  o.res = true;
  var result;
  function callbackfn(val, idx, obj)
  {
    result = this.res;
  }

  var arr = [1];
  arr.forEach(callbackfn,o)
  if( result === true)
    return true;    

 )#"
    },
    {
        "15.4.4.18-5-3",
        R"(Array.prototype.forEach - thisArg is Array)",
R"#(
  var res = false;
  var a = new Array();
  a.res = true;
  var result;
  function callbackfn(val, idx, obj)
  {
    result = this.res;
  }

  var arr = [1];
  arr.forEach(callbackfn,a)
  if( result === true)
    return true;    

 )#"
    },
    {
        "15.4.4.18-5-4",
        R"(Array.prototype.forEach - thisArg is object from object template(prototype))",
R"#(
  var res = false;
  var result;
  function callbackfn(val, idx, obj)
  {
    result = this.res;
  }
  
  function foo(){}
  foo.prototype.res = true;
  var f = new foo();
  var arr = [1];
  arr.forEach(callbackfn,f)
  if( result === true)
    return true;    

 )#"
    },
    {
        "15.4.4.18-5-5",
        R"(Array.prototype.forEach - thisArg is object from object template)",
R"#(
  var res = false;
  var result;
  function callbackfn(val, idx, obj)
  {
    result = this.res;
  }

  function foo(){}
  var f = new foo();
  f.res = true;
  
  var arr = [1];
  arr.forEach(callbackfn,f)
  if( result === true)
    return true;    

 )#"
    },
    {
        "15.4.4.18-5-6",
        R"(Array.prototype.forEach - thisArg is function)",
R"#(
  var res = false;
  var result;
  function callbackfn(val, idx, obj)
  {
    result = this.res;
  }

  function foo(){}
  foo.res = true;
  
  var arr = [1];
  arr.forEach(callbackfn,foo)
  if( result === true)
    return true;    

 )#"
    },
    {
        "15.4.4.18-7-1",
        R"(Array.prototype.forEach doesn't consider new elements added to array after the call)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++; 
    arr[2] = 3;
    arr[5] = 6;
  }

  var arr = [1,2,,4,5];
  arr.forEach(callbackfn);
  if( callCnt === 5)    
    return true;
 )#"
    },
    {
        "15.4.4.18-7-2",
        R"(Array.prototype.forEach doesn't visit deleted elements in array after the call)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    if(callCnt == 0)
      delete arr[3];
    callCnt++;
  }

  var arr = [1,2,3,4,5];
  arr.forEach(callbackfn)
  if( callCnt === 4)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.18-7-3",
        R"(Array.prototype.forEach doesn't visit deleted elements when Array.length is decreased)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    arr.length=3;
    callCnt++;
  }

  var arr = [1,2,3,4,5];
  arr.forEach(callbackfn);
  if( callCnt === 3)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.18-7-4",
        R"(Array.prototype.forEach doesn't consider newly added elements in sparse array)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    arr[1000] = 3;
    callCnt++;
  }

  var arr = new Array(10);
  arr[1] = 1;
  arr[2] = 2;
  arr.forEach(callbackfn);
  if( callCnt === 2)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.18-7-5",
        R"(Array.prototype.forEach visits deleted element in array after the call when same index is also present in prototype)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    delete arr[4];
    callCnt++;
  }

  Array.prototype[4] = 5;

  var arr = [1,2,3,4,5];
  arr.forEach(callbackfn)
  delete Array.prototype[4];
  if( callCnt === 5)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.18-7-b-1",
        R"(Array.prototype.forEach - callbackfn not called for indexes never been assigned values)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
  }

  var arr = new Array(10);
  arr[1] = undefined;
  arr.forEach(callbackfn);
  if( callCnt === 1)    
    return true;    
 )#"
    },
    {
        "15.4.4.18-7-c-ii-1",
        R"(Array.prototype.forEach - callbackfn called with correct parameters)",
R"#( 
 
  var bPar = true;
  var bCalled = false;
  function callbackfn(val, idx, obj)
  {
    bCalled = true;
    if(obj[idx] !== val)
      bPar = false;
  }

  var arr = [0,1,true,null,new Object(),"five"];
  arr[999999] = -6.6;
  arr.forEach(callbackfn);
  if(bCalled === true && bPar === true)
    return true;
 )#"
    },
    {
        "15.4.4.18-7-c-ii-2",
        R"(Array.prototype.forEach - callbackfn takes 3 arguments)",
R"#( 
 
  var parCnt = 3;
  var bCalled = false
  function callbackfn(val, idx, obj)
  { 
    bCalled = true;
    if(arguments.length !== 3)
      parCnt = arguments.length;   //verify if callbackfn was called with 3 parameters
  }

  var arr = [0,1,2,3,4,5,6,7,8,9];
  arr.forEach(callbackfn);
  if(bCalled === true && parCnt === 3)
    return true;

 )#"
    },
    {
        "15.4.4.18-8-1",
        R"(Array.prototype.forEach doesn't call callbackfn if 'length' is 0 (empty array))",
R"#(
  var callCnt = 0;
  function cb(){callCnt++}
  var i = [].forEach(cb);
  if (callCnt === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-8-10",
        R"(Array.prototype.forEach - subclassed array when length is reduced)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 1;
  
  var callCnt = 0;
  function cb(){callCnt++}
  var i = f.forEach(cb);  
  if (callCnt === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-8-11",
        R"(Array.prototype.forEach doesn't mutate the array on which it is called on)",
R"#(

  function callbackfn(val, idx, obj)
  {
    return true;
  }
  var arr = [1,2,3,4,5];
  arr.forEach(callbackfn);
  if(arr[0] === 1 &&
     arr[1] === 2 &&
     arr[2] === 3 &&
     arr[3] === 4 &&
     arr[4] === 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.18-8-12",
        R"(Array.prototype.forEach doesn't visit expandos)",
R"#(

  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
  }
  var arr = [1,2,3,4,5];
  arr["i"] = 10;
  arr[true] = 11;

  arr.forEach(callbackfn);
  if(callCnt == 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.18-8-2",
        R"(Array.prototype.forEach doesn't call callbackfn if 'length' is 0 (subclassed Array, length overridden to null (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = null;
  
  var callCnt = 0;
  function cb(){callCnt++}
  var i = f.forEach(cb);  
  if (callCnt === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-8-3",
        R"(Array.prototype.forEach doesn't call callbackfn if 'length' is 0 (subclassed Array, length overridden to false (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = false;

  var callCnt = 0;
  function cb(){callCnt++}
  var i = f.forEach(cb);  
  if (callCnt === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-8-4",
        R"(Array.prototype.forEach doesn't call callbackfn if 'length' is 0 (subclassed Array, length overridden to 0 (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 0;

  var callCnt = 0;
  function cb(){callCnt++}
  var i = f.forEach(cb);  
  if (callCnt === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-8-5",
        R"(Array.prototype.forEach doesn't call callbackfn if 'length' is 0 (subclassed Array, length overridden to '0' (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = '0';
  
  var callCnt = 0;
  function cb(){callCnt++}
  var i = f.forEach(cb);  
  if (callCnt === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-8-6",
        R"(Array.prototype.forEach doesn't call callbackfn if 'length' is 0 (subclassed Array, length overridden with obj with valueOf))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { valueOf: function () { return 0;}};
  f.length = o;
  
  var callCnt = 0;
  function cb(){callCnt++}
  var i = f.forEach(cb);  
  if (callCnt === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-8-7",
        R"(Array.prototype.forEach doesn't call callbackfn if 'length' is 0 (subclassed Array, length overridden with obj w/o valueOf (toString)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  var callCnt = 0;
  function cb(){callCnt++}
  var i = f.forEach(cb);  
  if (callCnt === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-8-8",
        R"(Array.prototype.forEach doesn't call callbackfn if 'length' is 0 (subclassed Array, length overridden with [])",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.
  var callCnt = 0;
  function cb(){callCnt++}
  var i = f.forEach(cb);  
  if (callCnt === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.18-8-9",
        R"(Array.prototype.forEach doesn't call callbackfn if 'length' is 0 (subclassed Array, length overridden with [0])",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [0];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  var callCnt = 0;
  function cb(){callCnt++}
  var i = f.forEach(cb);  
  if (callCnt === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.19-0-1",
        R"(Array.prototype.map must exist as a function)",
R"#(
  var f = Array.prototype.map;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.4.19-0-2",
        R"(Array.prototype.map.length must be 1)",
R"#(
  if (Array.prototype.map.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.19-4-1",
        R"(Array.prototype.map throws TypeError if callbackfn is undefined)",
R"#(

  var arr = new Array(10);
  try {
    arr.map();    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.19-4-2",
        R"(Array.prototype.map throws ReferenceError if callbackfn is unreferenced)",
R"#(

  var arr = new Array(10);
  try {
    arr.map(foo);    
  }
  catch(e) {
    if(e instanceof ReferenceError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.19-4-3",
        R"(Array.prototype.map throws TypeError if callbackfn is null)",
R"#(

  var arr = new Array(10);
  try {
    arr.map(null);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.19-4-4",
        R"(Array.prototype.map throws TypeError if callbackfn is boolean)",
R"#(

  var arr = new Array(10);
  try {
    arr.map(true);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.19-4-5",
        R"(Array.prototype.map throws TypeError if callbackfn is number)",
R"#(

  var arr = new Array(10);
  try {
    arr.map(5);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.19-4-6",
        R"(Array.prototype.map throws TypeError if callbackfn is string)",
R"#(

  var arr = new Array(10);
  try {
    arr.map("abc");    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.19-4-7",
        R"(Array.prototype.map throws TypeError if callbackfn is Object without Call internal method)",
R"#(

  var arr = new Array(10);
  try {
    arr.map(new Object());    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.19-4-8",
        R"(Array.prototype.map throws TypeError if callbackfn is object)",
R"#(

  var arr = new Array(10);
  try {
    arr.map({});    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.19-5-1-s",
        R"(Array.prototype.map - thisArg not passed to strict callbackfn)",
R"#(
  var innerThisCorrect = false;
  
  function callbackfn(val, idx, obj)
  {
    "use strict";
	innerThisCorrect = this===undefined;
	return true;
  }

  [1].map(callbackfn);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.19-5-1",
        R"(Array.prototype.map - thisArg not passed)",
R"#(
  var innerThisCorrect = false;
  
  function callbackfn(val, idx, obj)
  {
     innerThisCorrect = this===fnGlobalObject();
     return true;
  }

  [1].map(callbackfn);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.19-5-2",
        R"(Array.prototype.map - thisArg is Object)",
R"#(
  var res = false;
  var o = new Object();
  o.res = true;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  var srcArr = [1];
  var resArr = srcArr.map(callbackfn,o);
  if( resArr[0] === true)
    return true;    

 )#"
    },
    {
        "15.4.4.19-5-3",
        R"(Array.prototype.map - thisArg is Array)",
R"#(
  var res = false;
  var a = new Array();
  a.res = true;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  var srcArr = [1];
  var resArr = srcArr.map(callbackfn,a);
  if( resArr[0] === true)
    return true;    

 )#"
    },
    {
        "15.4.4.19-5-4",
        R"(Array.prototype.map - thisArg is object from object template(prototype))",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }
  
  function foo(){}
  foo.prototype.res = true;
  var f = new foo();
  
  var srcArr = [1];
  var resArr = srcArr.map(callbackfn,f);
  if( resArr[0] === true)
    return true;    

 )#"
    },
    {
        "15.4.4.19-5-5",
        R"(Array.prototype.map - thisArg is object from object template)",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  function foo(){}
  var f = new foo();
  f.res = true;
  
  var srcArr = [1];
  var resArr = srcArr.map(callbackfn,f);
  if( resArr[0] === true)
    return true;    

 )#"
    },
    {
        "15.4.4.19-5-6",
        R"(Array.prototype.map - thisArg is function)",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  function foo(){}
  foo.res = true;
  
  var srcArr = [1];
  var resArr = srcArr.map(callbackfn,foo);
  if( resArr[0] === true)
    return true;    

 )#"
    },
    {
        "15.4.4.19-5-7",
        R"(Array.prototype.map returns an empty array if 'length' is 0 (subclassed Array, length overridden with obj w/o valueOf (toString)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.

  function cb(){}
  var a = f.map(cb);
  
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.19-5-8",
        R"(Array.prototype.map returns an empty array if 'length' is 0 (subclassed Array, length overridden with [])",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  var a = f.map(cb);
  
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.19-8-1",
        R"(Array.prototype.map doesn't consider new elements added beyond the initial length of array after it is called)",
R"#( 
 
  var lastIndexVisited=-1; 
 
  function callbackfn(val, idx, obj)
  {
    if (idx===0) obj[10] = 10;
    lastIndexVisited=idx;
    return true;
  }
  
  [0,1].map(callbackfn);
  return lastIndexVisited === 1;  
  
 )#"
    },
    {
        "15.4.4.19-8-2",
        R"(Array.prototype.map considers new value of elements in array after it is called)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {    
    srcArr[4] = -1;
    if(val > 0)
      return 1;
    else
      return 0;
  }

  var srcArr = [1,2,3,4,5];
  var resArr = srcArr.map(callbackfn);
  if(resArr.length === 5 && resArr[4] === 0)
    return true;  
  
 )#"
    },
    {
        "15.4.4.19-8-3",
        R"(Array.prototype.map doesn't visit deleted elements in array after the call)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    delete srcArr[4];
    if(val > 0)
      return 1;
    else
      return 0;

  }

  var srcArr = [1,2,3,4,5];
  var resArr = srcArr.map(callbackfn);
  if(resArr.length === 5 && resArr[4] === undefined)
    return true;  
  
 )#"
    },
    {
        "15.4.4.19-8-4",
        R"(Array.prototype.map doesn't visit deleted elements when Array.length is decreased)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    srcArr.length = 2;
    callCnt++;
    return 1;
  }

  var srcArr = [1,2,3,4,5];
  var resArr = srcArr.map(callbackfn);
  if(resArr.length === 5  && callCnt === 2 && resArr[2] === undefined)
    return true;  
  
 )#"
    },
    {
        "15.4.4.19-8-5",
        R"(Array.prototype.map doesn't consider newly added elements in sparse array)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    srcArr[1000] = 3;
    callCnt++;
    return val;
  }

  var srcArr = new Array(10);
  srcArr[1] = 1;
  srcArr[2] = 2;
  var resArr = srcArr.map(callbackfn);
  if( resArr.length === 10 && callCnt === 2)    
      return true;  
  
 )#"
    },
    {
        "15.4.4.19-8-6",
        R"(Array.prototype.map visits deleted element in array after the call when same index is also present in prototype)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    delete srcArr[4];
    if(val > 0)
      return 1;
    else
      return 0;

  }

  Array.prototype[4] = 5;
  var srcArr = [1,2,3,4,5];
  var resArr = srcArr.map(callbackfn);
  delete Array.prototype[4];
  if(resArr.length === 5 && resArr[4] === 1)
    return true;  
  
 )#"
    },
    {
        "15.4.4.19-8-b-1",
        R"(Array.prototype.map - callbackfn not called for indexes never been assigned values)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
    return 1;
  }

  var srcArr = new Array(10);
  srcArr[1] = undefined; //explicitly assigning a value
  var resArr = srcArr.map(callbackfn);
  if( resArr.length === 10 && callCnt === 1)
      return true;    
 )#"
    },
    {
        "15.4.4.19-8-c-ii-1",
        R"(Array.prototype.map - callbackfn called with correct parameters)",
R"#( 
 
  var bPar = true;
  var bCalled = false;
  function callbackfn(val, idx, obj)
  {
    bCalled = true;
    if(obj[idx] !== val)
      bPar = false;
  }

  var srcArr = [0,1,true,null,new Object(),"five"];
  srcArr[999999] = -6.6;
  resArr = srcArr.map(callbackfn);
  
  if(bCalled === true && bPar === true)
    return true;
 )#"
    },
    {
        "15.4.4.19-8-c-ii-2",
        R"(Array.prototype.map - callbackfn takes 3 arguments)",
R"#( 
 
  var parCnt = 3;
  var bCalled = false
  function callbackfn(val, idx, obj)
  { 
    bCalled = true;
    if(arguments.length !== 3)
      parCnt = arguments.length;   //verify if callbackfn was called with 3 parameters
  }

  var srcArr = [0,1,2,3,4,5,6,7,8,9];
  var resArr = srcArr.map(callbackfn);
  if(bCalled === true && parCnt === 3)
    return true;


 )#"
    },
    {
        "15.4.4.19-8-c-iii-1",
        R"(Array.prototype.map - getOwnPropertyDescriptor(all true) of returned array element)",
R"#(
  
  function callbackfn(val, idx, obj){
	  if(val % 2)
	    return (2 * val + 1); 
	  else
	    return (val / 2);
  }
  var srcArr = [0,1,2,3,4];
  var resArr = srcArr.map(callbackfn);
  if (resArr.length > 0){
     var desc = Object.getOwnPropertyDescriptor(resArr, 1) 
     if(desc.value === 3 &&        //srcArr[1] = 2*1+1 = 3
       desc.writable === true &&
       desc.enumerable === true &&
       desc.configurable === true){
         return true;
    }
  }
 )#"
    },
    {
        "15.4.4.19-9-1",
        R"(Array.prototype.map doesn't mutate the Array on which it is called on)",
R"#(

  function callbackfn(val, idx, obj)
  {
    return true;
  }
  var srcArr = [1,2,3,4,5];
  srcArr.map(callbackfn);
  if(srcArr[0] === 1 &&
     srcArr[1] === 2 &&
     srcArr[2] === 3 &&
     srcArr[3] === 4 &&
     srcArr[4] === 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.19-9-2",
        R"(Array.prototype.map returns new Array with same number of elements and values the result of callbackfn)",
R"#(

  function callbackfn(val, idx, obj)
  {
    return val + 10;
  }
  var srcArr = [1,2,3,4,5];
  var resArr = srcArr.map(callbackfn);
  if(resArr[0] === 11 &&
     resArr[1] === 12 &&
     resArr[2] === 13 &&
     resArr[3] === 14 &&
     resArr[4] === 15)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.19-9-3",
        R"(Array.prototype.map - subclassed array when length is reduced)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 1;
  
  function cb(){}
  var a = f.map(cb);
  
  if (Array.isArray(a) &&
      a.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.19-9-4",
        R"(Array.prototype.map doesn't visit expandos)",
R"#(

  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
  }
  var srcArr = [1,2,3,4,5];
  srcArr["i"] = 10;
  srcArr[true] = 11;

  var resArr = srcArr.map(callbackfn);
  if(callCnt == 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.20-0-1",
        R"(Array.prototype.filter must exist as a function)",
R"#(
  var f = Array.prototype.filter;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-0-2",
        R"(Array.prototype.filter.length must be 1)",
R"#(
  if (Array.prototype.filter.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-10-1",
        R"(Array.prototype.filter doesn't mutate the Array on which it is called on)",
R"#(

  function callbackfn(val, idx, obj)
  {
    return true;
  }
  var srcArr = [1,2,3,4,5];
  srcArr.filter(callbackfn);
  if(srcArr[0] === 1 &&
     srcArr[1] === 2 &&
     srcArr[2] === 3 &&
     srcArr[3] === 4 &&
     srcArr[4] === 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.20-10-2",
        R"(Array.prototype.filter returns new Array with length equal to number of true returned by callbackfn)",
R"#(

  function callbackfn(val, idx, obj)
  {
    if(val % 2)
      return true;
    else
      return false;
  }
  var srcArr = [1,2,3,4,5];
  var resArr = srcArr.filter(callbackfn);
  if(resArr.length === 3 &&
     resArr[0] === 1 &&
     resArr[1] === 3 &&
     resArr[2] === 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.20-10-3",
        R"(Array.prototype.filter - subclassed array when length is reduced)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 1;
  
  function cb(){return true;}
  var a = f.filter(cb);
  
  if (Array.isArray(a) &&
      a.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-10-4",
        R"(Array.prototype.filter doesn't visit expandos)",
R"#(

  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
  }
  var srcArr = [1,2,3,4,5];
  srcArr["i"] = 10;
  srcArr[true] = 11;

  var resArr = srcArr.filter(callbackfn);
  if(callCnt == 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.20-4-1",
        R"(Array.prototype.filter throws TypeError if callbackfn is undefined)",
R"#(

  var arr = new Array(10);
  try {
    arr.filter();    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.20-4-2",
        R"(Array.prototype.filter throws ReferenceError if callbackfn is unreferenced)",
R"#(

  var arr = new Array(10);
  try {
    arr.filter(foo);    
  }
  catch(e) {
    if(e instanceof ReferenceError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.20-4-3",
        R"(Array.prototype.filter throws TypeError if callbackfn is null)",
R"#(

  var arr = new Array(10);
  try {
    arr.filter(null);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.20-4-4",
        R"(Array.prototype.filter throws TypeError if callbackfn is boolean)",
R"#(

  var arr = new Array(10);
  try {
    arr.filter(true);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.20-4-5",
        R"(Array.prototype.filter throws TypeError if callbackfn is number)",
R"#(

  var arr = new Array(10);
  try {
    arr.filter(5);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.20-4-6",
        R"(Array.prototype.filter throws TypeError if callbackfn is string)",
R"#(

  var arr = new Array(10);
  try {
    arr.filter("abc");    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.20-4-7",
        R"(Array.prototype.filter throws TypeError if callbackfn is Object without [[Call]] internal method)",
R"#(

  var arr = new Array(10);
  try {
    arr.filter(new Object());    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.20-4-8",
        R"(Array.prototype.filter throws TypeError if callbackfn is object)",
R"#(

  var arr = new Array(10);
  try {
    arr.filter({});    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.20-5-1-s",
        R"(Array.prototype.filter - thisArg not passed to strict callbackfn)",
R"#(
  var innerThisCorrect = false;
  
  function callbackfn(val, idx, obj)
  {
    "use strict";
	innerThisCorrect = this===undefined;
	return true;
  }

  [1].filter(callbackfn);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.20-5-1",
        R"(Array.prototype.filter - thisArg not passed)",
R"#(
  var innerThisCorrect = false;
  
  function callbackfn(val, idx, obj)
  {
    innerThisCorrect = this===fnGlobalObject();
    return true;
  }

  [1].filter(callbackfn);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.20-5-2",
        R"(Array.prototype.filter - thisArg is Object)",
R"#(
  var res = false;
  var o = new Object();
  o.res = true;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  var srcArr = [1];
  var resArr = srcArr.filter(callbackfn,o);
  if( resArr.length === 1)
    return true;
 )#"
    },
    {
        "15.4.4.20-5-3",
        R"(Array.prototype.filter - thisArg is Array)",
R"#(
  var res = false;
  var a = new Array();
  a.res = true;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  var srcArr = [1];
  var resArr = srcArr.filter(callbackfn,a);
  if( resArr.length === 1)
    return true;

 )#"
    },
    {
        "15.4.4.20-5-4",
        R"(Array.prototype.filter - thisArg is object from object template(prototype))",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }
  
  function foo(){}
  foo.prototype.res = true;
  var f = new foo();

  var srcArr = [1];
  var resArr = srcArr.filter(callbackfn,f);
  if( resArr.length === 1)
    return true;    

 )#"
    },
    {
        "15.4.4.20-5-5",
        R"(Array.prototype.filter - thisArg is object from object template)",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  function foo(){}
  var f = new foo();
  f.res = true;
  
  var srcArr = [1];
  var resArr = srcArr.filter(callbackfn,f);
  if( resArr.length === 1)
    return true;    

 )#"
    },
    {
        "15.4.4.20-5-6",
        R"(Array.prototype.filter - thisArg is function)",
R"#(
  var res = false;
  function callbackfn(val, idx, obj)
  {
    return this.res;
  }

  function foo(){}
  foo.res = true;
  
  var srcArr = [1];
  var resArr = srcArr.filter(callbackfn,foo);
  if( resArr.length === 1)
    return true;    

 )#"
    },
    {
        "15.4.4.20-6-1",
        R"(Array.prototype.filter returns an empty array if 'length' is 0 (empty array))",
R"#(
  function cb(){}
  var a = [].filter(cb);
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-6-2",
        R"(Array.prototype.filter returns an empty array if 'length' is 0 (subclassed Array, length overridden to null (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = null;
  
  function cb(){}
  var a = f.filter(cb);
  
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-6-3",
        R"(Array.prototype.filter returns an empty array if 'length' is 0 (subclassed Array, length overridden to false (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = false;
  
  function cb(){}
  var a = f.filter(cb);
  
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-6-4",
        R"(Array.prototype.filter returns an empty array if 'length' is 0 (subclassed Array, length overridden to 0 (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 0;
  
  function cb(){}
  var a = f.filter(cb);
  
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-6-5",
        R"(Array.prototype.filter returns an empty array if 'length' is 0 (subclassed Array, length overridden to '0' (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = '0';
  
  function cb(){}
  var a = f.filter(cb);
  
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-6-6",
        R"(Array.prototype.filter returns an empty array if 'length' is 0 (subclassed Array, length overridden with obj with valueOf))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { valueOf: function () { return 0;}};
  f.length = o;
  
  function cb(){}
  var a = f.filter(cb);
  
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-6-7",
        R"(Array.prototype.filter returns an empty array if 'length' is 0 (subclassed Array, length overridden with obj w/o valueOf (toString)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.

  function cb(){}
  var a = f.filter(cb);
  
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-6-8",
        R"(Array.prototype.filter returns an empty array if 'length' is 0 (subclassed Array, length overridden with [])",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  var a = f.filter(cb);
  
  if (Array.isArray(a) &&
      a.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.20-9-1",
        R"(Array.prototype.filter doesn't consider new elements added beyond the initial length of array after it is called)",
R"#( 
  var lastIndexVisited=-1; 
 
  function callbackfn(val, idx, obj)
  {
    if (idx===0) obj[10] = 10;
    lastIndexVisited=idx;
    return true;
  }

  [0,1].filter(callbackfn);
  return lastIndexVisited === 1;  
  
 )#"
    },
    {
        "15.4.4.20-9-2",
        R"(Array.prototype.filter considers new value of elements in array after it is called)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {    
    srcArr[2] = -1;
    srcArr[4] = -1;
    if(val > 0)
      return true;
    else
      return false;
  }

  var srcArr = [1,2,3,4,5];
  var resArr = srcArr.filter(callbackfn);
  if(resArr.length === 3 && resArr[0] === 1 && resArr[2] === 4)
      return true;  
  
 )#"
    },
    {
        "15.4.4.20-9-3",
        R"(Array.prototype.filter doesn't visit deleted elements in array after the call)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    delete srcArr[2];
    delete srcArr[4];
    if(val > 0)
      return true;
    else
      return false;
   }

  var srcArr = [1,2,3,4,5];
  var resArr = srcArr.filter(callbackfn);
  if(resArr.length === 3 && resArr[0] === 1 && resArr[2] === 4 )    // two elements deleted
      return true;  
  
 )#"
    },
    {
        "15.4.4.20-9-4",
        R"(Array.prototype.filter doesn't visit deleted elements when Array.length is decreased)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    srcArr.length = 2;
    return true;
  }

  var srcArr = [1,2,3,4,6];
  var resArr = srcArr.filter(callbackfn);
  if(resArr.length === 2 )
      return true;  
  
 )#"
    },
    {
        "15.4.4.20-9-5",
        R"(Array.prototype.filter process consider newly added elements within original length of a in sparse array)",
R"#(
  var lastIndexVisited=-1; 
 
  function callbackfn(val, idx, obj)
  {
    if (idx===0) obj[99] = 99;
    lastIndexVisited=idx;
    return true;
  }

  var arr=new Array(100);
  arr[0]=0;
  arr[5]=5;
  arr.filter(callbackfn);
  return lastIndexVisited === 99;    
 )#"
    },
    {
        "15.4.4.20-9-6",
        R"(Array.prototype.filter visits deleted element in array after the call when same index is also present in prototype)",
R"#( 
 
  function callbackfn(val, idx, obj)
  {
    delete srcArr[2];
    delete srcArr[4];
    if(val > 0)
      return true;
    else
      return false;
   }

  Array.prototype[4] = 5;
  var srcArr = [1,2,3,4,5];
  var resArr = srcArr.filter(callbackfn);
  delete Array.prototype[4];
  if(resArr.length === 4 && resArr[0] === 1 && resArr[3] == 5)    // only one element deleted
      return true;  
  
 )#"
    },
    {
        "15.4.4.20-9-b-1",
        R"(Array.prototype.filter - callbackfn not called for indexes never been assigned values)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(val, idx, obj)
  {
    callCnt++;
    return false;
  }

  var srcArr = new Array(10);
  srcArr[1] = undefined; //explicitly assigning a value
  var resArr = srcArr.filter(callbackfn);
  if( resArr.length === 0 && callCnt === 1)
      return true;    
 )#"
    },
    {
        "15.4.4.20-9-c-ii-1",
        R"(Array.prototype.filter - callbackfn called with correct parameters)",
R"#( 
 
  var bPar = true;
  var bCalled = false;
  function callbackfn(val, idx, obj)
  {
    bCalled = true;
    if(obj[idx] !== val)
      bPar = false;
  }

  var srcArr = [0,1,true,null,new Object(),"five"];
  srcArr[999999] = -6.6;
  var resArr = srcArr.filter(callbackfn);
  
  if(bCalled === true && bPar === true)
    return true;
 )#"
    },
    {
        "15.4.4.20-9-c-ii-2",
        R"(Array.prototype.filter - callbackfn takes 3 arguments)",
R"#( 
 
  var parCnt = 3;
  var bCalled = false
  function callbackfn(val, idx, obj)
  { 
    bCalled = true;
    if(arguments.length !== 3)
      parCnt = arguments.length;   //verify if callbackfn was called with 3 parameters
  }

  var srcArr = [0,1,2,3,4,5,6,7,8,9];
  var resArr = srcArr.filter(callbackfn);
  if(bCalled === true && parCnt === 3)
    return true;
 )#"
    },
    {
        "15.4.4.20-9-c-iii-1",
        R"(Array.prototype.filter - getOwnPropertyDescriptor(all true) of returned array element)",
R"#(
  
  function callbackfn(val, idx, obj){
    if(val % 2)
      return true; 
    else
      return false;
  }
  var srcArr = [0,1,2,3,4];
  var resArr = srcArr.filter(callbackfn);
  if (resArr.length > 0){
     var desc = Object.getOwnPropertyDescriptor(resArr, 1) 
     if(desc.value === 3 &&        //srcArr[1] = true
       desc.writable === true &&
       desc.enumerable === true &&
       desc.configurable === true){
         return true;
    }
  }
 )#"
    },
    {
        "15.4.4.21-0-1",
        R"(Array.prototype.reduce must exist as a function)",
R"#(
  var f = Array.prototype.reduce;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.4.21-0-2",
        R"(Array.prototype.reduce.length must be 1)",
R"#(
  if (Array.prototype.reduce.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.21-10-1",
        R"(Array.prototype.reduce doesn't mutate the Array on which it is called on)",
R"#(

  function callbackfn(prevVal, curVal,  idx, obj)
  {
    return 1;
  }
  var srcArr = [1,2,3,4,5];
  srcArr.reduce(callbackfn);
  if(srcArr[0] === 1 &&
     srcArr[1] === 2 &&
     srcArr[2] === 3 &&
     srcArr[3] === 4 &&
     srcArr[4] === 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.21-10-2",
        R"(Array.prototype.reduce reduces the array in ascending order of indices)",
R"#(

  function callbackfn(prevVal, curVal,  idx, obj)
  {
    return prevVal + curVal;
  }
  var srcArr = ['1','2','3','4','5'];
  if(srcArr.reduce(callbackfn) === '12345')
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.21-10-3",
        R"(Array.prototype.reduce - subclassed array of length 1)",
R"#(
  foo.prototype = [1];
  function foo() {}
  var f = new foo();
  
  function cb(){}
  if(f.reduce(cb) === 1)
    return true;
 )#"
    },
    {
        "15.4.4.21-10-4",
        R"(Array.prototype.reduce - subclassed array with length more than 1)",
R"#(
  foo.prototype = new Array(1, 2, 3, 4);
  function foo() {}
  var f = new foo();
  
  function cb(prevVal, curVal, idx, obj){return prevVal + curVal;}
  if(f.reduce(cb) === 10)
    return true;
 )#"
    },
    {
        "15.4.4.21-10-5",
        R"(Array.prototype.reduce reduces the array in ascending order of indices(initialvalue present))",
R"#(

  function callbackfn(prevVal, curVal,  idx, obj)
  {
    return prevVal + curVal;
  }
  var srcArr = ['1','2','3','4','5'];
  if(srcArr.reduce(callbackfn,'0') === '012345')
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.21-10-6",
        R"(Array.prototype.reduce - subclassed array when initialvalue provided)",
R"#(
  foo.prototype = [1,2,3,4];
  function foo() {}
  var f = new foo();
  
  function cb(prevVal, curVal, idx, obj){return prevVal + curVal;}
  if(f.reduce(cb,-1) === 9)
    return true;
 )#"
    },
    {
        "15.4.4.21-10-7",
        R"(Array.prototype.reduce - subclassed array with length 1 and initialvalue provided)",
R"#(
  foo.prototype = [1];
  function foo() {}
  var f = new foo();
  
  function cb(prevVal, curVal, idx, obj){return prevVal + curVal;}
  if(f.reduce(cb,-1) === 0)
    return true;
 )#"
    },
    {
        "15.4.4.21-10-8",
        R"(Array.prototype.reduce doesn't visit expandos)",
R"#(

  var callCnt = 0;
  function callbackfn(prevVal, curVal,  idx, obj)
  {
    callCnt++;
    return curVal;
  }
  var srcArr = ['1','2','3','4','5'];
  srcArr["i"] = 10;
  srcArr[true] = 11;
  srcArr.reduce(callbackfn);

  if(callCnt == 4)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.21-4-1",
        R"(Array.prototype.reduce throws TypeError if callbackfn is undefined)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduce();    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.21-4-2",
        R"(Array.prototype.reduce throws ReferenceError if callbackfn is unreferenced)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduce(foo);    
  }
  catch(e) {
    if(e instanceof ReferenceError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.21-4-3",
        R"(Array.prototype.reduce throws TypeError if callbackfn is null)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduce(null);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.21-4-4",
        R"(Array.prototype.reduce throws TypeError if callbackfn is boolean)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduce(true);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.21-4-5",
        R"(Array.prototype.reduce throws TypeError if callbackfn is number)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduce(5);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.21-4-6",
        R"(Array.prototype.reduce throws TypeError if callbackfn is string)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduce("abc");    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.21-4-7",
        R"(Array.prototype.reduce throws TypeError if callbackfn is Object without [[Call]] internal method)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduce(new Object());    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.21-4-8",
        R"(Array.prototype.reduce throws TypeError if callbackfn is object)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduce({});    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.21-5-1",
        R"(Array.prototype.reduce throws TypeError if 'length' is 0 (empty array), no initVal)",
R"#(
  function cb(){}
  
  try {
    [].reduce(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.21-5-2",
        R"(Array.prototype.reduce throws TypeError if 'length' is 0 (subclassed Array, length overridden to null (type conversion)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = null;
  
  function cb(){}
  try {
    f.reduce(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.21-5-3",
        R"(Array.prototype.reduce throws TypeError if 'length' is 0 (subclassed Array, length overridden to false (type conversion)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = false;
  
  function cb(){}
  try {
    f.reduce(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.21-5-4",
        R"(Array.prototype.reduce throws TypeError if 'length' is 0 (subclassed Array, length overridden to 0 (type conversion)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 0;
  
  function cb(){}
  try {
    f.reduce(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.21-5-5",
        R"(Array.prototype.reduce throws TypeError if 'length' is 0 (subclassed Array, length overridden to '0' (type conversion)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = '0';
  
  function cb(){}
  try {
    f.reduce(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.21-5-6",
        R"(Array.prototype.reduce throws TypeError if 'length' is 0 (subclassed Array, length overridden with obj with valueOf), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { valueOf: function () { return 0;}};
  f.length = o;
  
  function cb(){}
  try {
    f.reduce(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.21-5-7",
        R"(Array.prototype.reduce throws TypeError if 'length' is 0 (subclassed Array, length overridden with obj w/o valueOf (toString)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.

  function cb(){}
  try {
    f.reduce(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.21-5-8",
        R"(Array.prototype.reduce throws TypeError if 'length' is 0 (subclassed Array, length overridden with []), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  try {
    f.reduce(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.21-7-1",
        R"(Array.prototype.reduce returns initialValue if 'length' is 0 and initialValue is present (empty array))",
R"#(
  function cb(){}
  
  try {
    if([].reduce(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.21-7-2",
        R"(Array.prototype.reduce returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden to null (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = null;
  
  function cb(){}
  try {
    if(f.reduce(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.21-7-3",
        R"(Array.prototype.reduce returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden to false (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = false;
  
  function cb(){}
  try {
    if(f.reduce(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.21-7-4",
        R"(Array.prototype.reduce returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden to 0 (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 0;
  
  function cb(){}
  try {
    if(f.reduce(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.21-7-5",
        R"(Array.prototype.reduce returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden to '0' (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = '0';
  
  function cb(){}
  try {
    if(f.reduce(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.21-7-6",
        R"(Array.prototype.reduce returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden with obj with valueOf))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { valueOf: function () { return 0;}};
  f.length = o;
  
  function cb(){}
  try {
    if(f.reduce(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.21-7-7",
        R"(Array.prototype.reduce returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden with obj w/o valueOf (toString)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.

  function cb(){}
  try {
    if(f.reduce(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.21-7-8",
        R"(Array.prototype.reduce returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden with []))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  try {
    if(f.reduce(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.21-7-9",
        R"(Array.prototype.reduce returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden with [0]))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [0];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  try {
    if(f.reduce(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.21-8-c-1",
        R"(Array.prototype.reduce throws TypeError when Array is empty and initialValue is not present)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
  }

  var arr = new Array(10);
  try {
    arr.reduce(callbackfn);
  } 
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }
 )#"
    },
    {
        "15.4.4.21-8-c-2",
        R"(Array.prototype.reduce throws TypeError when elements assigned values are deleted by reducing array length and initialValue is not present)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
  }

  var arr = new Array(10);
  arr[9] = 1;
  arr.length = 5;
  try {
    arr.reduce(callbackfn);
  } 
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }
 )#"
    },
    {
        "15.4.4.21-8-c-3",
        R"(Array.prototype.reduce throws TypeError when elements assigned values are deleted and initialValue is not present)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
  }

  var arr = [1,2,3,4,5];
  delete arr[0];
  delete arr[1];
  delete arr[2];
  delete arr[3];
  delete arr[4];
  try {
    arr.reduce(callbackfn);
  } 
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }
 )#"
    },
    {
        "15.4.4.21-9-1",
        R"(Array.prototype.reduce doesn't consider new elements added beyond the initial length of array after it is called)",
R"#( 
 
  var lastIndexVisited=-1; 
 
  function callbackfn(preval, val, idx, obj)
  {
    if (idx===0) obj[10] = 10;
    lastIndexVisited=idx;
    return true;
  }
  
  [0,1].reduce(callbackfn);
  return lastIndexVisited === 1;  
  
 )#"
    },
    {
        "15.4.4.21-9-2",
        R"(Array.prototype.reduce considers new value of elements in array after it is called)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
    arr[3] = -2;
    arr[4] = -1;
    return prevVal + curVal;
  }

  var arr = [1,2,3,4,5];
  if(arr.reduce(callbackfn) === 3)
    return true;  
  
 )#"
    },
    {
        "15.4.4.21-9-3",
        R"(Array.prototype.reduce doesn't visit deleted elements in array after the call)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)  
  {
    delete arr[3];
    delete arr[4];
    return prevVal + curVal;    
  }

  var arr = ['1',2,3,4,5];
  if(arr.reduce(callbackfn) === "123"  )    // two elements deleted
    return true;  
  
 )#"
    },
    {
        "15.4.4.21-9-4",
        R"(Array.prototype.reduce doesn't visit deleted elements when Array.length is decreased)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
    arr.length = 2;
    return prevVal + curVal;
  }

  var arr = [1,2,3,4,5];
  if(arr.reduce(callbackfn) === 3 )
    return true;  
  
 )#"
    },
    {
        "15.4.4.21-9-5",
        R"(Array.prototype.reduce - callbackfn not called for array with one element)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(prevVal, curVal, idx, obj)
  {
    callCnt++;
    return 2;
  }

  var arr = [1];
  if(arr.reduce(callbackfn) === 1 && callCnt === 0 )
    return true;    
 )#"
    },
    {
        "15.4.4.21-9-6",
        R"(Array.prototype.reduce visits deleted element in array after the call when same index is also present in prototype)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)  
  {
    delete arr[3];
    delete arr[4];
    return prevVal + curVal;    
  }

  Array.prototype[4] = 5;
  var arr = ['1',2,3,4,5];
  var res = arr.reduce(callbackfn);
  delete Array.prototype[4];

  if(res === "1235"  )    //one element acually deleted
    return true;  
  
 )#"
    },
    {
        "15.4.4.21-9-b-1",
        R"(Array.prototype.reduce returns initialvalue when Array is empty and initialValue is present)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
  }

  var arr = new Array(10);

  if(arr.reduce(callbackfn,5) === 5)
      return true;  

 )#"
    },
    {
        "15.4.4.21-9-c-1",
        R"(Array.prototype.reduce - callbackfn not called for indexes never been assigned values)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(prevVal, curVal, idx, obj)
  {
    callCnt++;
    return curVal;
  }

  var arr = new Array(10);
  arr[0] = arr[1] = undefined; //explicitly assigning a value
  if( arr.reduce(callbackfn) === undefined && callCnt === 1)
    return true;    
 )#"
    },
    {
        "15.4.4.21-9-c-ii-1",
        R"(Array.prototype.reduce - callbackfn called with correct parameters (initialvalue not passed))",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
    if(idx > 0 && obj[idx] === curVal && obj[idx-1] === prevVal)
      return curVal;
    else 
      return false;
  }

  var arr = [0,1,true,null,new Object(),"five"];
  if( arr.reduce(callbackfn) === "five") 
    return true;
 )#"
    },
    {
        "15.4.4.21-9-c-ii-2",
        R"(Array.prototype.reduce - callbackfn called with correct parameters (initialvalue passed))",
R"#( 
 
  var bParCorrect = false;
  function callbackfn(prevVal, curVal, idx, obj)
  {
    if(idx === 0 && obj[idx] === curVal && prevVal === initialValue)
      return curVal;
    else if(idx > 0 && obj[idx] === curVal && obj[idx-1] === prevVal)
      return curVal;
    else
      return false;
  }

  var arr = [0,1,true,null,new Object(),"five"];
  var initialValue = 5.5;
  if( arr.reduce(callbackfn,initialValue) === "five") 
    return true;
 )#"
    },
    {
        "15.4.4.21-9-c-ii-3",
        R"(Array.prototype.reduce - callbackfn takes 4 arguments)",
R"#( 
 
  var bCalled = false;
  function callbackfn(prevVal, curVal, idx, obj)
  { 
    bCalled = true;
    if(prevVal === true && arguments.length === 4)   
      return true;
    else
      return false;
  }
  var arr = [0,1,2,3,4,5,6,7,8,9];
  if(arr.reduce(callbackfn,true) === true && bCalled === true)
    return true;
 )#"
    },
    {
        "15.4.4.21-9-c-ii-4-s",
        R"(Array.prototype.reduce - null passed as thisValue to strict callbackfn)",
R"#( 
  var innerThisCorrect = false;
  function callbackfn(prevVal, curVal, idx, obj)
  { 
     innerThisCorrect = this===null;
     return true;
  }
  [0].reduce(callbackfn,true);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.21-9-c-ii-4",
        R"(Array.prototype.reduce - null passed as this Value to callbackfn)",
R"#( 
  var innerThisCorrect = false;
  function callbackfn(prevVal, curVal, idx, obj)
  { 
     innerThisCorrect = this===fnGlobalObject();
     return true;
  }
  [0].reduce(callbackfn,true);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.22-0-1",
        R"(Array.prototype.reduceRight must exist as a function)",
R"#(
  var f = Array.prototype.reduceRight;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.4.4.22-0-2",
        R"(Array.prototype.reduceRight.length must be 1)",
R"#(
  if (Array.prototype.reduceRight.length === 1) {
    return true;
  }
 )#"
    },
    {
        "15.4.4.22-10-1",
        R"(Array.prototype.reduceRight doesn't mutate the Array on which it is called on)",
R"#(

  function callbackfn(prevVal, curVal,  idx, obj)
  {
    return 1;
  }
  var srcArr = [1,2,3,4,5];
  srcArr.reduceRight(callbackfn);
  if(srcArr[0] === 1 &&
     srcArr[1] === 2 &&
     srcArr[2] === 3 &&
     srcArr[3] === 4 &&
     srcArr[4] === 5)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.22-10-2",
        R"(Array.prototype.reduceRight reduces array in descending order of indices)",
R"#(

  function callbackfn(prevVal, curVal,  idx, obj)
  {
    return prevVal + curVal;
  }
  var srcArr = ['1','2','3','4','5'];
  if(srcArr.reduceRight(callbackfn) === '54321')
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.22-10-3",
        R"(Array.prototype.reduceRight - subclassed array with length 1)",
R"#(
  foo.prototype = [1];
  function foo() {}
  var f = new foo();
  
  function cb(){}
  if(f.reduceRight(cb) === 1)
    return true;
 )#"
    },
    {
        "15.4.4.22-10-4",
        R"(Array.prototype.reduceRight - subclassed array with length more than 1)",
R"#(
  foo.prototype = new Array(0, 1, 2, 3);
  function foo() {}
  var f = new foo();
  
  function cb(prevVal, curVal, idx, obj){return prevVal + curVal;}
  if(f.reduceRight(cb) === 6)
    return true;
 )#"
    },
    {
        "15.4.4.22-10-5",
        R"(Array.prototype.reduceRight reduces array in descending order of indices(initialvalue present))",
R"#(

  function callbackfn(prevVal, curVal,  idx, obj)
  {
    return prevVal + curVal;
  }
  var srcArr = ['1','2','3','4','5'];
  if(srcArr.reduceRight(callbackfn,'6') === '654321')
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.22-10-6",
        R"(Array.prototype.reduceRight - subclassed array when initialvalue provided)",
R"#(
  foo.prototype = new Array(0, 1, 2, 3);
  function foo() {}
  var f = new foo();
  
  function cb(prevVal, curVal, idx, obj){return prevVal + curVal;}
  if(f.reduceRight(cb,"4") === "43210")
    return true;
 )#"
    },
    {
        "15.4.4.22-10-7",
        R"(Array.prototype.reduceRight - subclassed array when length to 1 and initialvalue provided)",
R"#(
  foo.prototype = [1];
  function foo() {}
  var f = new foo();
  
  function cb(prevVal, curVal, idx, obj){return prevVal + curVal;}
  if(f.reduceRight(cb,"4") === "41")
    return true;
 )#"
    },
    {
        "15.4.4.22-10-8",
        R"(Array.prototype.reduceRight doesn't visit expandos)",
R"#(

  var callCnt = 0;
  function callbackfn(prevVal, curVal,  idx, obj)
  {
    callCnt++;
  }
  var srcArr = ['1','2','3','4','5'];
  srcArr["i"] = 10;
  srcArr[true] = 11;

  srcArr.reduceRight(callbackfn);

  if(callCnt == 4)
  {
    return true;
  }

 )#"
    },
    {
        "15.4.4.22-4-1",
        R"(Array.prototype.reduceRight throws TypeError if callbackfn is undefined)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduceRight();    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.22-4-2",
        R"(Array.prototype.reduceRight throws ReferenceError if callbackfn is unreferenced)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduceRight(foo);    
  }
  catch(e) {
    if(e instanceof ReferenceError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.22-4-3",
        R"(Array.prototype.reduceRight throws TypeError if callbackfn is null)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduceRight(null);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.22-4-4",
        R"(Array.prototype.reduceRight throws TypeError if callbackfn is boolean)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduceRight(true);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.22-4-5",
        R"(Array.prototype.reduceRight throws TypeError if callbackfn is number)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduceRight(5);    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.22-4-6",
        R"(Array.prototype.reduceRight throws TypeError if callbackfn is string)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduceRight("abc");    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.22-4-7",
        R"(Array.prototype.reduceRight throws TypeError if callbackfn is Object without [[Call]] internal method)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduceRight(new Object());    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.22-4-8",
        R"(Array.prototype.reduceRight throws TypeError if callbackfn is object)",
R"#(

  var arr = new Array(10);
  try {
    arr.reduceRight({});    
  }
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }

 )#"
    },
    {
        "15.4.4.22-5-1",
        R"(Array.prototype.reduceRight throws TypeError if 'length' is 0 (empty array), no initVal)",
R"#(
  function cb(){}
  
  try {
    [].reduceRight(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.22-5-2",
        R"(Array.prototype.reduceRight throws TypeError if 'length' is 0 (subclassed Array, length overridden to null (type conversion)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = null;
  
  function cb(){}
  try {
    f.reduceRight(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.22-5-3",
        R"(Array.prototype.reduceRight throws TypeError if 'length' is 0 (subclassed Array, length overridden to false (type conversion)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = false;
  
  function cb(){}
  try {
    f.reduceRight(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.22-5-4",
        R"(Array.prototype.reduceRight throws TypeError if 'length' is 0 (subclassed Array, length overridden to 0 (type conversion)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 0;
  
  function cb(){}
  try {
    f.reduceRight(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.22-5-5",
        R"(Array.prototype.reduceRight throws TypeError if 'length' is 0 (subclassed Array, length overridden to '0' (type conversion)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = '0';
  
  function cb(){}
  try {
    f.reduceRight(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.22-5-6",
        R"(Array.prototype.reduceRight throws TypeError if 'length' is 0 (subclassed Array, length overridden with obj with valueOf), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { valueOf: function () { return 0;}};
  f.length = o;
  
  function cb(){}
  try {
    f.reduceRight(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.22-5-7",
        R"(Array.prototype.reduceRight throws TypeError if 'length' is 0 (subclassed Array, length overridden with obj w/o valueOf (toString)), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.

  function cb(){}
  try {
    f.reduceRight(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.22-5-8",
        R"(Array.prototype.reduceRight throws TypeError if 'length' is 0 (subclassed Array, length overridden with []), no initVal)",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  try {
    f.reduceRight(cb);
  }
  catch (e) {
    if (e instanceof TypeError) {
      return true;
    }
  }
 )#"
    },
    {
        "15.4.4.22-7-1",
        R"(Array.prototype.reduceRight returns initialValue if 'length' is 0 and initialValue is present (empty array))",
R"#(
  function cb(){}
  
  try {
    if([].reduceRight(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.22-7-2",
        R"(Array.prototype.reduceRight returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden to null (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = null;
  
  function cb(){}
  try {
    if(f.reduceRight(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.22-7-3",
        R"(Array.prototype.reduceRight returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden to false (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = false;
  
  function cb(){}
  try {
    if(f.reduceRight(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.22-7-4",
        R"(Array.prototype.reduceRight returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden to 0 (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = 0;
  
  function cb(){}
  try {
    if(f.reduceRight(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.22-7-5",
        R"(Array.prototype.reduceRight returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden to '0' (type conversion)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  f.length = '0';
  
  function cb(){}
  try {
    if(f.reduceRight(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.22-7-6",
        R"(Array.prototype.reduceRight returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden with obj with valueOf))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { valueOf: function () { return 0;}};
  f.length = o;
  
  function cb(){}
  try {
    if(f.reduceRight(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.22-7-7",
        R"(Array.prototype.reduceRight returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden with obj w/o valueOf (toString)))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  var o = { toString: function () { return '0';}};
  f.length = o;
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.

  function cb(){}
  try {
    if(f.reduceRight(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.22-7-8",
        R"(Array.prototype.reduceRight returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden with []))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  try {
    if(f.reduceRight(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.22-7-9",
        R"(Array.prototype.reduceRight returns initialValue if 'length' is 0 and initialValue is present (subclassed Array, length overridden with [0]))",
R"#(
  foo.prototype = new Array(1, 2, 3);
  function foo() {}
  var f = new foo();
  
  f.length = [0];
  
  // objects inherit the default valueOf method of the Object object;
  // that simply returns the itself. Since the default valueOf() method
  // does not return a primitive value, ES next tries to convert the object
  // to a number by calling its toString() method and converting the
  // resulting string to a number.
  //
  // The toString( ) method on Array converts the array elements to strings,
  // then returns the result of concatenating these strings, with commas in
  // between. An array with no elements converts to the empty string, which
  // converts to the number 0. If an array has a single element that is a
  // number n, the array converts to a string representation of n, which is
  // then converted back to n itself. If an array contains more than one element,
  // or if its one element is not a number, the array converts to NaN.

  function cb(){}
  try {
    if(f.reduceRight(cb,1) === 1)
      return true;
  }
  catch (e) {  }  
 )#"
    },
    {
        "15.4.4.22-8-c-1",
        R"(Array.prototype.reduceRight throws TypeError when Array is empty and initialValue is not present)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
  }

  var arr = new Array(10);
  try {
    arr.reduceRight(callbackfn);
  } 
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }
 )#"
    },
    {
        "15.4.4.22-8-c-2",
        R"(Array.prototype.reduceRight throws TypeError when elements assigned values are deleted by reducign array length and initialValue is not present)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
  }

  var arr = new Array(10);
  arr[9] = 1;
  arr.length = 5;
  try {
    arr.reduceRight(callbackfn);
  } 
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }
 )#"
    },
    {
        "15.4.4.22-8-c-3",
        R"(Array.prototype.reduceRight throws TypeError when elements assigned values are deleted and initialValue is not present)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
  }

  var arr = [1,2,3,4,5];
  delete arr[0];
  delete arr[1];
  delete arr[2];
  delete arr[3];
  delete arr[4];
  try {
    arr.reduceRight(callbackfn);
  } 
  catch(e) {
    if(e instanceof TypeError)
      return true;  
  }
 )#"
    },
    {
        "15.4.4.22-9-1",
        R"(Array.prototype.reduceRight doesn't consider new elements added beyond the initial length of array after it is called)",
R"#( 
 
  var lastIndexVisited=-1; 
 
  function callbackfn(preval, val, idx, obj)
  {
    if (idx===0) obj[10] = 10;
    lastIndexVisited=idx;
    return true;
  }
  
  [0,1].reduceRight(callbackfn);
  return lastIndexVisited === 1;  
  
 )#"
    },
    {
        "15.4.4.22-9-2",
        R"(Array.prototype.reduceRight considers new value of elements in array after it is called)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
    arr[3] = -2;
    arr[0] = -1;
    return prevVal + curVal;
  }

  var arr = [1,2,3,4,5];
  if(arr.reduceRight(callbackfn) === 13)
    return true;  
  
 )#"
    },
    {
        "15.4.4.22-9-3",
        R"(Array.prototype.reduceRight doesn't consider unvisited deleted elements in array after the call)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)  
  {
    delete arr[1];
    delete arr[4];
    return prevVal + curVal;    
  }

  var arr = ['1',2,3,4,5];
  if(arr.reduceRight(callbackfn) === "121" )    // two elements deleted
    return true;  
  
 )#"
    },
    {
        "15.4.4.22-9-4",
        R"(Array.prototype.reduceRight doesn't consider unvisited deleted elements when Array.length is decreased)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
    arr.length = 2;
    return prevVal + curVal;
  }

  var arr = [1,2,3,4,5];
  if(arr.reduceRight(callbackfn) === 12 )
    return true;  
  
 )#"
    },
    {
        "15.4.4.22-9-5",
        R"(Array.prototype.reduceRight - callbackfn not called for array with one element)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(prevVal, curVal, idx, obj)
  {
    callCnt++;
    return 2;
  }

  var arr = [1];
  if(arr.reduceRight(callbackfn) === 1 && callCnt === 0 )
    return true;    
 )#"
    },
    {
        "15.4.4.22-9-6",
        R"(Array.prototype.reduceRight visits deleted element in array after the call when same index is also present in prototype)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)  
  {
    delete arr[1];
    delete arr[2];
    return prevVal + curVal;    
  }
  Array.prototype[2] = 6;
  var arr = ['1',2,3,4,5];
  var res = arr.reduceRight(callbackfn);
  delete Array.prototype[2];

  if(res === "151" )    //one element deleted
    return true;  
  
 )#"
    },
    {
        "15.4.4.22-9-7",
        R"(Array.prototype.reduceRight stops calling callbackfn once the array is deleted during the call)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)  
  {
    delete o.arr;
    return prevVal + curVal;    
  }

  var o = new Object();
  o.arr = ['1',2,3,4,5];
  if(o.arr.reduceRight(callbackfn) === 9 )    //two elements visited
    return true;  
  
 )#"
    },
    {
        "15.4.4.22-9-b-1",
        R"(Array.prototype.reduceRight returns initialvalue when Array is empty and initialValue is not present)",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
  }

  var arr = new Array(10);
  
  if(arr.reduceRight(callbackfn,5) === 5)
    return true;  
 )#"
    },
    {
        "15.4.4.22-9-c-1",
        R"(Array.prototype.reduceRight - callbackfn not called for indexes never been assigned values)",
R"#( 
 
  var callCnt = 0;
  function callbackfn(prevVal, curVal, idx, obj)
  {
    callCnt++;
    return curVal;
  }

  var arr = new Array(10);
  arr[0] = arr[1] = undefined; //explicitly assigning a value
  if( arr.reduceRight(callbackfn) === undefined && callCnt === 1)
    return true;    
 )#"
    },
    {
        "15.4.4.22-9-c-ii-1",
        R"(Array.prototype.reduceRight - callbackfn called with correct parameters (initialvalue not passed))",
R"#( 
 
  function callbackfn(prevVal, curVal, idx, obj)
  {
    if(idx+1 < obj.length && obj[idx] === curVal && obj[idx+1] === prevVal)
      return curVal;
    else 
      return false;
  }

  var arr = [0,1,true,null,new Object(),"five"];
  if( arr.reduceRight(callbackfn) === 0) 
    return true;
 )#"
    },
    {
        "15.4.4.22-9-c-ii-2",
        R"(Array.prototype.reduceRight - callbackfn called with correct parameters (initialValue passed))",
R"#( 
 
  var bParCorrect = false;
  function callbackfn(prevVal, curVal, idx, obj)
  {
    if(idx === obj.length-1 && obj[idx] === curVal && prevVal === initialValue)
      return curVal;
    else if (idx+1 < obj.length && obj[idx] === curVal && obj[idx+1] === prevVal)
      return curVal;
    else
      return false;
  }

  var arr = [0,1,true,null,new Object(),"five"];
  var initialValue = 5.5;
  if( arr.reduceRight(callbackfn,initialValue) === 0) 
    return true;
 )#"
    },
    {
        "15.4.4.22-9-c-ii-3",
        R"(Array.prototype.reduceRight - callbackfn takes 4 arguments)",
R"#( 
 
  var bCalled = false;
  function callbackfn(prevVal, curVal, idx, obj)
  { 
    bCalled = true;
    if(prevVal === true && arguments.length === 4)   
      return true;
    else
      return false;
  }
  var arr = [0,1,2,3,4,5,6,7,8,9];
  if(arr.reduceRight(callbackfn,true) === true && bCalled === true)
    return true;
 )#"
    },
    {
        "15.4.4.22-9-c-ii-4-s",
        R"(Array.prototype.reduceRight - null passed as thisValue to strict callbackfn)",
R"#( 
  var innerThisCorrect = false;
  function callbackfn(prevVal, curVal, idx, obj)
  { 
     innerThisCorrect = this===null;
     return true;
  }
  [0].reduceRight(callbackfn,true);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.4.22-9-c-ii-4",
        R"(Array.prototype.reduceRight - null passed as this Value to callbackfn)",
R"#( 
  var innerThisCorrect = false;
  function callbackfn(prevVal, curVal, idx, obj)
  { 
     innerThisCorrect = this===fnGlobalObject();
     return true;
  }
  [0].reduceRight(callbackfn,true);
  return innerThisCorrect;    
 )#"
    },
    {
        "15.4.5-1",
        R"(Array instances have [[Class]] set to 'Array')",
R"#(
  var a = [];
  var s = Object.prototype.toString.call(a);
  if (s === '[object Array]') {
    return true;
  }
 )#"
    },
    {
        "15.4.5.1-3.d-1",
        R"(Throw RangeError if attempt to set array length property to 4294967296 (2**32))",
R"#(
  try {
      [].length = 4294967296 ;
  } catch (e) {
	if (e instanceof RangeError) return true;
  }
 )#"
    },
    {
        "15.4.5.1-3.d-2",
        R"(Throw RangeError if attempt to set array length property to 4294967297 (1+2**32))",
R"#(
  try {
      [].length = 4294967297 ;
  } catch (e) {
	if (e instanceof RangeError) return true;
  }
 )#"
    },
    {
        "15.4.5.1-3.d-3",
        R"(Set array length property to max value 4294967295 (2**32-1,))",
R"#(  
  var a =[];
  a.length = 4294967295 ;
  return a.length===4294967295 ;
 )#"
    },
    {
        "15.4.5.1-5-1",
        R"(Defining a property named 4294967295 (2**32-1)(not an array element))",
R"#(  
  var a =[];
  a[4294967295] = "not an array element" ;
  return a[4294967295] === "not an array element";
 )#"
    },
    {
        "15.4.5.1-5-2",
        R"(Defining a property named 4294967295 (2**32-1) doesn't change length of the array)",
R"#(  
  var a =[0,1,2];
  a[4294967295] = "not an array element" ;
  return a.length===3;
 )#"
    },
    {
        "15.5.4.20-0-1",
        R"(String.prototype.trim must exist as a function)",
R"#(
  var f = String.prototype.trim;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-0-2",
        R"(String.prototype.trim must exist as a function taking 0 parameters)",
R"#(
  if (String.prototype.trim.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-1-1",
        R"(String.prototype.trim throws TypeError when string is undefined)",
R"#(
  try
  {
    String.prototype.trim.call(undefined);  
  }
  catch(e)
  {
    if(e instanceof TypeError)
      return true;
  }
 )#"
    },
    {
        "15.5.4.20-1-2",
        R"(String.prototype.trim throws TypeError when string is null)",
R"#(
  try
  {
    String.prototype.trim.call(null);  
  }
  catch(e)
  {
    if(e instanceof TypeError)
      return true;
  }
 )#"
    },
    {
        "15.5.4.20-1-3",
        R"(String.prototype.trim works for primitive type boolean)",
R"#(
  try
  {
    if(String.prototype.trim.call(true) == "true")
      return true;
  }
  catch(e)
  {
  }
 )#"
    },
    {
        "15.5.4.20-1-4",
        R"(String.prototype.trim works for primitive type number)",
R"#(
  try
  {
    if(String.prototype.trim.call(0) == "0") 
      return true;
  }
  catch(e)
  {
  }
 )#"
    },
    {
        "15.5.4.20-1-5",
        R"(String.prototype.trim works for an Object)",
R"#(
  try
  {
    if(String.prototype.trim.call({})=="[object Object]")
      return true;
  }
  catch(e)
  {
  }
 )#"
    },
    {
        "15.5.4.20-1-6",
        R"(String.prototype.trim works for an String)",
R"#(
  try
  {
    if(String.prototype.trim.call(new String()) == "")
      return true;
  }
  catch(e)
  {
  }
 )#"
    },
    {
        "15.5.4.20-1-7",
        R"(String.prototype.trim works for a primitive string)",
R"#(
  try
  {
    if(String.prototype.trim.call("abc") === "abc")  
      return true;
  }
  catch(e)
  {
  }
 )#"
    },
    {
        "15.5.4.20-4-1",
        R"(String.prototype.trim handles multiline string with whitepace and lineterminators)",
R"#(
var s = "\u0009a b\
c \u0009"

            
  if (s.trim() === "a bc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-10",
        R"(String.prototype.trim handles whitepace and lineterminators (\uFEFFabc))",
R"#(
  if ("\uFEFFabc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-11",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\u0009))",
R"#(
  if ("abc\u0009".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-12",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\u000B))",
R"#(
  if ("abc\u000B".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-13",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\u000C))",
R"#(
  if ("abc\u000C".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-14",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\u0020))",
R"#(
  if ("abc\u0020".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-16",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\u00A0))",
R"#(
  if ("abc\u00A0".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-18",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\uFEFF))",
R"#(
  if ("abc\uFEFF".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-19",
        R"(String.prototype.trim handles whitepace and lineterminators (\u0009abc\u0009))",
R"#(
  if ("\u0009abc\u0009".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-2",
        R"(String.prototype.trim handles whitepace and lineterminators ( \u0009abc \u0009))",
R"#(
  if (" \u0009abc \u0009".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-20",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000Babc\u000B))",
R"#(
  if ("\u000Babc\u000B".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-21",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000Cabc\u000C))",
R"#(
  if ("\u000Cabc\u000C".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-22",
        R"(String.prototype.trim handles whitepace and lineterminators (\u0020abc\u0020))",
R"#(
  if ("\u0020abc\u0020".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-24",
        R"(String.prototype.trim handles whitepace and lineterminators (\u00A0abc\u00A0))",
R"#(
  if ("\u00A0abc\u00A0".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-27",
        R"(String.prototype.trim handles whitepace and lineterminators (\u0009\u0009))",
R"#(
  if ("\u0009\u0009".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-28",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000B\u000B))",
R"#(
  if ("\u000B\u000B".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-29",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000C\u000C))",
R"#(
  if ("\u000C\u000C".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-3",
        R"(String.prototype.trim handles whitepace and lineterminators (\u0009abc))",
R"#(
  if ("\u0009abc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-30",
        R"(String.prototype.trim handles whitepace and lineterminators (\u0020\u0020))",
R"#(
  if ("\u0020\u0020".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-32",
        R"(String.prototype.trim handles whitepace and lineterminators (\u00A0\u00A0))",
R"#(
  if ("\u00A0\u00A0".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-34",
        R"(String.prototype.trim handles whitepace and lineterminators (\uFEFF\uFEFF))",
R"#(
  if ("\uFEFF\uFEFF".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-35",
        R"(String.prototype.trim handles whitepace and lineterminators (ab\u0009c))",
R"#(
  if ("ab\u0009c".trim() === "ab\u0009c") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-36",
        R"(String.prototype.trim handles whitepace and lineterminators (ab\u000Bc))",
R"#(
  if ("ab\u000Bc".trim() === "ab\u000Bc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-37",
        R"(String.prototype.trim handles whitepace and lineterminators (ab\u000Cc))",
R"#(
  if ("ab\u000Cc".trim() === "ab\u000Cc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-38",
        R"(String.prototype.trim handles whitepace and lineterminators (ab\u0020c))",
R"#(
  if ("ab\u0020c".trim() === "ab\u0020c") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-39",
        R"(String.prototype.trim handles whitepace and lineterminators (ab\u0085c))",
R"#(
  if ("ab\u0085c".trim() === "ab\u0085c") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-4",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000Babc))",
R"#(
  if ("\u000Babc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-40",
        R"(String.prototype.trim handles whitepace and lineterminators (ab\u00A0c))",
R"#(
  if ("ab\u00A0c".trim() === "ab\u00A0c") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-41",
        R"(String.prototype.trim handles whitepace and lineterminators (ab\u200Bc))",
R"#(
  if ("ab\u200Bc".trim() === "ab\u200Bc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-42",
        R"(String.prototype.trim handles whitepace and lineterminators (ab\uFEFFc))",
R"#(
  if ("ab\uFEFFc".trim() === "ab\uFEFFc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-43",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000Aabc))",
R"#(
  if ("\u000Aabc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-44",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000Dabc))",
R"#(
  if ("\u000Dabc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-45",
        R"(String.prototype.trim handles whitepace and lineterminators (\u2028abc))",
R"#(
  if ("\u2028abc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-46",
        R"(String.prototype.trim handles whitepace and lineterminators (\u2029abc))",
R"#(
  if ("\u2029abc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-47",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\u000A))",
R"#(
  if ("abc\u000A".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-48",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\u000D))",
R"#(
  if ("abc\u000D".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-49",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\u2028))",
R"#(
  if ("abc\u2028".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-5",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000Cabc))",
R"#(
  if ("\u000Cabc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-50",
        R"(String.prototype.trim handles whitepace and lineterminators (abc\u2029))",
R"#(
  if ("abc\u2029".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-51",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000Aabc\u000A))",
R"#(
  if ("\u000Aabc\u000A".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-52",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000Dabc\u000D))",
R"#(
  if ("\u000Dabc\u000D".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-53",
        R"(String.prototype.trim handles whitepace and lineterminators (\u2028abc\u2028))",
R"#(
  if ("\u2028abc\u2028".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-54",
        R"(String.prototype.trim handles whitepace and lineterminators (\u2029abc\u2029))",
R"#(
  if ("\u2029abc\u2029".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-55",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000A\u000A))",
R"#(
  if ("\u000A\u000A".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-56",
        R"(String.prototype.trim handles whitepace and lineterminators (\u000D\u000D))",
R"#(
  if ("\u000D\u000D".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-57",
        R"(String.prototype.trim handles whitepace and lineterminators (\u2028\u2028))",
R"#(
  if ("\u2028\u2028".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-58",
        R"(String.prototype.trim handles whitepace and lineterminators (\u2029\u2029))",
R"#(
  if ("\u2029\u2029".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-59",
        R"(String.prototype.trim handles whitepace and lineterminators (\u2029abc as a multiline string))",
R"#(
  var s = "\u2029\
           abc";
  if (s.trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-6",
        R"(String.prototype.trim handles whitepace and lineterminators (\u0020abc))",
R"#(
  if ("\u0020abc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-60",
        R"(String.prototype.trim handles whitepace and lineterminators (string with just blanks))",
R"#(
  if ("    ".trim() === "") {
    return true;
  }
 )#"
    },
    {
        "15.5.4.20-4-8",
        R"(String.prototype.trim handles whitepace and lineterminators (\u00A0abc))",
R"#(
  if ("\u00A0abc".trim() === "abc") {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-1-1",
        R"(String object supports bracket notation to lookup of data properties)",
R"#(
  var s = new String("hello world");
  s.foo = 1;
  
  if (s["foo"] === 1) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-1-2",
        R"(String value supports bracket notation to lookup data properties)",
R"#(
  var s = String("hello world");
  
  if (s["foo"] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-3-1",
        R"(String object indexing returns undefined for missing data properties)",
R"#(
  var s = new String("hello world");
  
  if (s["foo"] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-3-2",
        R"(String value indexing returns undefined for missing data properties)",
R"#(
  var s = String("hello world");
  
  if (s["foo"] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-3-3",
        R"(String object indexing returns undefined if the numeric index (NaN) is not an array index)",
R"#(
  var s = new String("hello world");

  if (s[NaN] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-3-4",
        R"(String object indexing returns undefined if the numeric index (Infinity) is not an array index)",
R"#(
  var s = new String("hello world");

  if (s[Infinity] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-3-5",
        R"(String object indexing returns undefined if the numeric index ( 2^32-1) is not an array index)",
R"#(
  var s = new String("hello world");

  if (s[Math.pow(2, 32)-1]===undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-3-6",
        R"(String value indexing returns undefined if the numeric index (NaN) is not an array index)",
R"#(
  var s = String("hello world");

  if (s[NaN] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-3-7",
        R"(String value indexing returns undefined if the numeric index (Infinity) is not an array index)",
R"#(
  var s = String("hello world");

  if (s[Infinity] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-3-8",
        R"(String value indexing returns undefined if the numeric index ( >= 2^32-1) is not an array index)",
R"#(
  var s = String("hello world");

  if (s[Math.pow(2, 32)-1]===undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-7-1",
        R"(String object indexing returns undefined if the numeric index is less than 0)",
R"#(
  var s = new String("hello world");

  if (s[-1] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-7-2",
        R"(String value indexing returns undefined if the numeric index is less than 0)",
R"#(
  var s = String("hello world");

  if (s[-1] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-7-3",
        R"(String object indexing returns undefined if the numeric index is greater than the string length)",
R"#(
  var s = new String("hello world");

  if (s[11] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.5.5.5.2-7-4",
        R"(String value indexing returns undefined if the numeric index is greater than the string length)",
R"#(
  var s = String("hello world");

  if (s[11] === undefined) {
    return true;
  }
 )#"
    },
    {
        "15.7.3-1",
        R"(Number constructor - [[Prototype]] is the Function prototype object)",
R"#(
  if (Function.prototype.isPrototypeOf(Number) === true) {
    return true;
  }
 )#"
    },
    {
        "15.7.3-2",
        R"(Number constructor - [[Prototype]] is the Function prototype object (using getPrototypeOf))",
R"#(
  var p = Object.getPrototypeOf(Number);
  if (p === Function.prototype) {
    return true;
  }
 )#"
    },
    {
        "15.7.3.1-1",
        R"(Number.prototype is a data property with default attribute values (false))",
R"#(
  var d = Object.getOwnPropertyDescriptor(Number, 'prototype');
  
  if (d.writable === false &&
      d.enumerable === false &&
      d.configurable === false) {
    return true;
  }
 )#"
    },
    {
        "15.7.3.1-2",
        R"(Number.prototype, initial value is the Number prototype object)",
R"#(
  // assume that Number.prototype has not been modified.
  return Object.getPrototypeOf(new Number(42))===Number.prototype;
 )#"
    },
    {
        "15.7.4-1",
        R"(Number prototype object: its [[Class]] must be 'Number')",
R"#(
  var numProto = Object.getPrototypeOf(new Number(42));
  var s = Object.prototype.toString.call(numProto );
  return (s === '[object Number]') ;
 )#"
    },
    {
        "15.9.4.4-0-1",
        R"(Date.now must exist as a function)",
R"#(
  var f = Date.now;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.9.4.4-0-2",
        R"(Date.now must exist as a function taking 0 parameters)",
R"#(
  if (Date.now.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.9.5.43-0-1",
        R"(Date.prototype.toISOString must exist as a function)",
R"#(
  var f = Date.prototype.toISOString;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.9.5.43-0-2",
        R"(Date.prototype.toISOString must exist as a function taking 0 parameters)",
R"#(
  if (Date.prototype.toISOString.length === 0) {
    return true;
  }
 )#"
    },
    {
        "15.9.5.44-0-1",
        R"(Date.prototype.toJSON must exist as a function)",
R"#(
  var f = Date.prototype.toJSON;
  if (typeof(f) === "function") {
    return true;
  }
 )#"
    },
    {
        "15.9.5.44-0-2",
        R"(Date.prototype.toJSON must exist as a function taking 1 parameter)",
R"#(
  if (Date.prototype.toJSON.length === 1) {
    return true;
  }
 )#"
    },
};
