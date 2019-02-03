#include "test_es5_conformance.h"

#include "test.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include <mjs/char_conversions.h>
#include <mjs/parser.h>
#include <mjs/interpreter.h>

using namespace mjs;

constexpr int expected_failures[] = {
       2, // 10.4.2-1-1           Indirect call to eval has context set to global context
       3, // 10.4.2-1-2           Indirect call to eval has context set to global context (nested function)
       4, // 10.4.2-1-3           Indirect call to eval has context set to global context (catch block)
       5, // 10.4.2-1-4           Indirect call to eval has context set to global context (with block)
       6, // 10.4.2-1-5           Indirect call to eval has context set to global context (inside another eval)
      24, // 10.6-13-b-3-s        arguments.caller is non-configurable in strict mode
      27, // 10.6-13-c-3-s        arguments.callee is non-configurable in strict mode
      39, // 11.1.5@4-4-b-1       Object literal - SyntaxError if a data property definition is followed by get accessor definition with the same name
      40, // 11.1.5@4-4-b-2       Object literal - SyntaxError if a data property definition is followed by set accessor definition with the same name
      41, // 11.1.5@4-4-c-1       Object literal - SyntaxError if a get accessor property definition is followed by a data property definition with the same name
      42, // 11.1.5@4-4-c-2       Object literal - SyntaxError if a set accessor property definition is followed by a data property definition with the same name
      43, // 11.1.5@4-4-d-1       Object literal - SyntaxError for duplicate property name (get,get)
      44, // 11.1.5@4-4-d-2       Object literal - SyntaxError for duplicate property name (set,set)
      45, // 11.1.5@4-4-d-3       Object literal - SyntaxError for duplicate property name (get,set,get)
      46, // 11.1.5@4-4-d-4       Object literal - SyntaxError for duplicate property name (set,get,set)
      59, // 11.13.1-4-1          simple assignment creates property on the global object if LeftHandSide is an unresolvable reference
      64, // 11.13.1-4-14-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.MAX_VALUE)
      65, // 11.13.1-4-15-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.MIN_VALUE)
      66, // 11.13.1-4-16-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.NaN)
      67, // 11.13.1-4-17-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.NEGATIVE_INFINITY)
      68, // 11.13.1-4-18-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Number.POSITIVE_INFINITY)
      69, // 11.13.1-4-19-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.E)
      71, // 11.13.1-4-20-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.LN10)
      72, // 11.13.1-4-21-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.LN2)
      73, // 11.13.1-4-22-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.LOG2E)
      74, // 11.13.1-4-23-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.LOG10E)
      75, // 11.13.1-4-24-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.PI)
      76, // 11.13.1-4-25-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.SQRT1_2)
      77, // 11.13.1-4-26-s       simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Math.SQRT2)
      80, // 11.13.1-4-4-s        simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Global.length)
     114, // 11.4.1-4.a-9-s       delete operator throws TypeError when deleting a non-configurable data property in strict mode (Math.LN2)
     115, // 11.4.1-4.a-9         delete operator returns false when deleting a non-configurable data property (Math.LN2)
     116, // 11.4.1-5-1-s         delete operator throws ReferenceError when deleting a direct reference to a var in strict mode
     118, // 11.4.1-5-2-s         delete operator throws ReferenceError when deleting a direct reference to a function argument in strict mode
     120, // 11.4.1-5-3-s         delete operator throws ReferenceError when deleting a direct reference to a function name in strict mode
     129, // 12.10-0-8            with introduces scope - var initializer sets like named property
     145, // 12.14-1              catch doesn't change declaration scope - var initializer in catch with same name as catch parameter changes parameter
     149, // 12.14-13             catch introduces scope - updates are based on scope
     150, // 12.14-2              catch doesn't change declaration scope - var initializer in catch with same name as catch parameter changes parameter
     158, // 12.2.1-1-s           eval - a function declaring a var named 'eval' throws EvalError in strict mode
     159, // 12.2.1-10-s          eval - an indirect eval assigning into 'eval' throws EvalError in strict mode
     165, // 12.2.1-2-s           eval - a function assigning into 'eval' throws EvalError in strict mode
     166, // 12.2.1-3-s           eval - a function expr declaring a var named 'eval' throws EvalError in strict mode
     167, // 12.2.1-4-s           eval - a function expr assigning into 'eval' throws a EvalError in strict mode
     168, // 12.2.1-5-s           eval - a Function declaring var named 'eval' throws EvalError in strict mode
     169, // 12.2.1-6-s           eval - a Function assigning into 'eval' throws EvalError in strict mode
     170, // 12.2.1-7-s           eval - a direct eval declaring a var named 'eval' throws EvalError in strict mode
     171, // 12.2.1-8-s           eval - a direct eval assigning into 'eval' throws EvalError in strict mode
     172, // 12.2.1-9-s           eval - an indirect eval declaring a var named 'eval' throws EvalError in strict mode
     181, // 13.1-2-3-s           eval - a function expr having a formal parameter named 'eval' throws SyntaxError if strict mode body
     182, // 13.1-2-4-s           arguments - a function declaration having a formal parameter named 'arguments' throws SyntaxError if strict mode body
     187, // 13.1-2-7-s           arguments - a function expr having a formal parameter named 'arguments' throws SyntaxError if strict mode body
     188, // 13.1-2-8-s           arguments - a function declaration having a formal parameter named 'arguments' throws SyntaxError if strict mode body
     190, // 13.1-3-10-s          SyntaxError if arguments used as function identifier in function expression with strict body
     191, // 13.1-3-11-s          SyntaxError if arguments used as function identifier in function declaration in strict code
     192, // 13.1-3-12-s          SyntaxError if arguments used as function identifier in function expression in strict code
     194, // 13.1-3-3-s           SyntaxError if eval used as function identifier in function declaration with strict body
     195, // 13.1-3-4-s           SyntaxError if eval used as function identifier in function expression with strict body
     196, // 13.1-3-5-s           SyntaxError if eval used as function identifier in function declaration in strict code
     197, // 13.1-3-6-s           SyntaxError if eval used as function identifier in function expression in strict code
     200, // 13.1-3-9-s           SyntaxError if arguments used as function identifier in function declaration with strict body
     254, // 15.12.1.1-g4-1       The JSON lexical grammer does not allow a JSONStringCharacter to be any of the Unicode characters U+0000 thru U+0007
     255, // 15.12.1.1-g4-2       The JSON lexical grammer does not allow a JSONStringCharacter to be any of the Unicode characters U+0008 thru U+000F
     256, // 15.12.1.1-g4-3       The JSON lexical grammer does not allow a JSONStringCharacter to be any of the Unicode characters U+0010 thru U+0017
     257, // 15.12.1.1-g4-4       The JSON lexical grammer does not allow a JSONStringCharacter to be any of the Unicode characters U+0018 thru U+001F
     467, // 15.2.3.3-4-116       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.constructor)
     499, // 15.2.3.3-4-145       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCFullYear)
     500, // 15.2.3.3-4-146       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCMonth)
     501, // 15.2.3.3-4-147       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCDate)
     502, // 15.2.3.3-4-148       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCHours)
     503, // 15.2.3.3-4-149       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCMinutes)
     505, // 15.2.3.3-4-150       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCSeconds)
     506, // 15.2.3.3-4-151       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.setUTCMilliseconds)
     507, // 15.2.3.3-4-152       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toLocaleString)
     509, // 15.2.3.3-4-154       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toUTCString)
     510, // 15.2.3.3-4-155       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toGMTString)
     513, // 15.2.3.3-4-158       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toLocaleDateString)
     514, // 15.2.3.3-4-159       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Date.prototype.toLocaleTimeString)
     519, // 15.2.3.3-4-163       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RegExp.prototype.constructor)
     520, // 15.2.3.3-4-164       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RegExp.prototype.compile)
     524, // 15.2.3.3-4-168       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Error.prototype.constructor)
     527, // 15.2.3.3-4-170       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (EvalError.prototype.constructor)
     528, // 15.2.3.3-4-171       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RangeError.prototype.constructor)
     529, // 15.2.3.3-4-172       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (ReferenceError.prototype.constructor)
     530, // 15.2.3.3-4-173       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (SyntaxError.prototype.constructor)
     531, // 15.2.3.3-4-174       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (TypeError.prototype.constructor)
     532, // 15.2.3.3-4-175       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (URIError.prototype.constructor)
     540, // 15.2.3.3-4-183       Object.getOwnPropertyDescriptor returns undefined for non-existent properties on built-ins (Function.arguments)
     554, // 15.2.3.3-4-196       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.MAX_VALUE)
     555, // 15.2.3.3-4-197       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.MIN_VALUE)
     556, // 15.2.3.3-4-198       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.NaN)
     557, // 15.2.3.3-4-199       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.NEGATIVE_INFINITY)
     560, // 15.2.3.3-4-200       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Number.POSITIVE_INFINITY)
     562, // 15.2.3.3-4-202       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.E)
     563, // 15.2.3.3-4-203       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.LN10)
     564, // 15.2.3.3-4-204       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.LN2)
     565, // 15.2.3.3-4-205       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.LOG2E)
     566, // 15.2.3.3-4-206       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.LOG10E)
     567, // 15.2.3.3-4-207       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.PI)
     568, // 15.2.3.3-4-208       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.SQRT1_2)
     569, // 15.2.3.3-4-209       Object.getOwnPropertyDescriptor returns data desc (all false) for properties on built-ins (Math.SQRT2)
     597, // 15.2.3.3-4-34        Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Function.prototype.constructor)
     602, // 15.2.3.3-4-39        Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.constructor)
     604, // 15.2.3.3-4-40        Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Array.prototype.concat)
     628, // 15.2.3.3-4-62        Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.constructor)
     641, // 15.2.3.3-4-74        Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.substr)
     651, // 15.2.3.3-4-84        Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Boolean.prototype.constructor)
     654, // 15.2.3.3-4-88        Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (Number.prototype.constructor)
     669, // 15.2.3.4-4-1         Object.getOwnPropertyNames returns array of property names (Global)
     670, // 15.2.3.4-4-10        Object.getOwnPropertyNames returns array of property names (Math)
     671, // 15.2.3.4-4-11        Object.getOwnPropertyNames returns array of property names (Date)
     672, // 15.2.3.4-4-12        Object.getOwnPropertyNames returns array of property names (Date.prototype)
     673, // 15.2.3.4-4-13        Object.getOwnPropertyNames returns array of property names (RegExp.prototype)
     674, // 15.2.3.4-4-14        Object.getOwnPropertyNames returns array of property names (Error.prototype)
     675, // 15.2.3.4-4-15        Object.getOwnPropertyNames returns array of property names (EvalError.prototype)
     676, // 15.2.3.4-4-16        Object.getOwnPropertyNames returns array of property names (RangeError.prototype)
     677, // 15.2.3.4-4-17        Object.getOwnPropertyNames returns array of property names (ReferenceError.prototype)
     678, // 15.2.3.4-4-18        Object.getOwnPropertyNames returns array of property names (SyntaxError.prototype)
     679, // 15.2.3.4-4-19        Object.getOwnPropertyNames returns array of property names (TypeError.prototype)
     680, // 15.2.3.4-4-2         Object.getOwnPropertyNames returns array of property names (Object)
     681, // 15.2.3.4-4-20        Object.getOwnPropertyNames returns array of property names (URIError.prototype)
     682, // 15.2.3.4-4-21        Object.getOwnPropertyNames returns array of property names (JSON)
     683, // 15.2.3.4-4-22        Object.getOwnPropertyNames returns array of property names (Function)
     684, // 15.2.3.4-4-23        Object.getOwnPropertyNames returns array of property names (function)
     685, // 15.2.3.4-4-24        Object.getOwnPropertyNames returns array of property names (Array)
     686, // 15.2.3.4-4-25        Object.getOwnPropertyNames returns array of property names (String)
     687, // 15.2.3.4-4-26        Object.getOwnPropertyNames returns array of property names (Boolean)
     688, // 15.2.3.4-4-27        Object.getOwnPropertyNames returns array of property names (Number)
     689, // 15.2.3.4-4-28        Object.getOwnPropertyNames returns array of property names (RegExp)
     690, // 15.2.3.4-4-29        Object.getOwnPropertyNames returns array of property names (Error)
     691, // 15.2.3.4-4-3         Object.getOwnPropertyNames returns array of property names (Object.prototype)
     692, // 15.2.3.4-4-30        Object.getOwnPropertyNames returns array of property names (EvalError)
     693, // 15.2.3.4-4-31        Object.getOwnPropertyNames returns array of property names (RangeError)
     694, // 15.2.3.4-4-32        Object.getOwnPropertyNames returns array of property names (ReferenceError)
     695, // 15.2.3.4-4-33        Object.getOwnPropertyNames returns array of property names (SyntaxError)
     696, // 15.2.3.4-4-34        Object.getOwnPropertyNames returns array of property names (TypeError)
     697, // 15.2.3.4-4-35        Object.getOwnPropertyNames returns array of property names (URIError)
     698, // 15.2.3.4-4-4         Object.getOwnPropertyNames returns array of property names (Function.prototype)
     699, // 15.2.3.4-4-5         Object.getOwnPropertyNames returns array of property names (Array.prototype)
     700, // 15.2.3.4-4-6         Object.getOwnPropertyNames returns array of property names (String)
     701, // 15.2.3.4-4-7         Object.getOwnPropertyNames returns array of property names (String.prototype)
     702, // 15.2.3.4-4-8         Object.getOwnPropertyNames returns array of property names (Boolean.prototype)
     703, // 15.2.3.4-4-9         Object.getOwnPropertyNames returns array of property names (Number.prototype)
     714, // 15.2.3.6-3-1         Object.defineProperty throws TypeError if desc has 'get' and 'value' present
     720, // 15.2.3.6-3-2         Object.defineProperty throws TypeError if desc has 'get' and 'writable' present
     721, // 15.2.3.6-3-3         Object.defineProperty throws TypeError if desc has 'set' and 'value' present
     722, // 15.2.3.6-3-4         Object.defineProperty throws TypeError if desc has 'set' and 'writable' present
     737, // 15.2.3.6-4-18        Object.defineProperty throws TypeError when changing setter of non-configurable accessor properties
     740, // 15.2.3.6-4-20        Object.defineProperty throws TypeError when changing getter (if present) of non-configurable accessor properties
     758, // 15.3.2.1-11-1-s      Duplicate seperate parameter name in Function constructor throws SyntaxError in strict mode
     760, // 15.3.2.1-11-2-s      Duplicate seperate parameter name in Function constructor called from strict mode allowed if body not strict
     761, // 15.3.2.1-11-3-s      Function constructor having a formal parameter named 'eval' throws SyntaxError if function body is strict mode
     763, // 15.3.2.1-11-4-s      Function constructor call from strict code with formal parameter named 'eval' does not throws SyntaxError if function body is not strict mode
     764, // 15.3.2.1-11-5-s      Duplicate combined parameter name in Function constructor throws SyntaxError in strict mode
     766, // 15.3.2.1-11-6-s      Duplicate combined parameter name allowed in Function constructor called in strict mode if body not strict
     767, // 15.3.2.1-11-7-s      Function constructor call from strict code with formal parameter named arguments does not throws SyntaxError if function body is not strict mode
     801, // 15.4.4.14-1-1        Array.prototype.indexOf applied to undefined throws a TypeError
     802, // 15.4.4.14-1-2        Array.prototype.indexOf applied to null throws a TypeError
     803, // 15.4.4.14-10-1       Array.prototype.indexOf returns -1 for elements not present in array
     821, // 15.4.4.14-9-10       Array.prototype.indexOf must return correct index (NaN)
     829, // 15.4.4.14-9-9        Array.prototype.indexOf must return correct index (Sparse Array)
     849, // 15.4.4.15-8-10       Array.prototype.lastIndexOf must return correct index (NaN)
     857, // 15.4.4.15-8-9        Array.prototype.lastIndexOf must return correct index (Sparse Array)
     858, // 15.4.4.15-9-1        Array.prototype.lastIndexOf returns -1 for elements not present
     879, // 15.4.4.16-7-6        Array.prototype.every visits deleted element in array after the call when same index is also present in prototype
     882, // 15.4.4.16-7-c-ii-2   Array.prototype.every - callbackfn takes 3 arguments
     906, // 15.4.4.17-4-9        Array.prototype.some returns -1 if 'length' is 0 (subclassed Array, length overridden with [0]
     922, // 15.4.4.17-7-c-ii-2   Array.prototype.some - callbackfn takes 3 arguments
     925, // 15.4.4.17-8-10       Array.prototype.some - subclassed array when length is reduced
     959, // 15.4.4.18-7-c-ii-1   Array.prototype.forEach - callbackfn called with correct parameters
     997, // 15.4.4.19-8-6        Array.prototype.map visits deleted element in array after the call when same index is also present in prototype
     999, // 15.4.4.19-8-c-ii-1   Array.prototype.map - callbackfn called with correct parameters
    1040, // 15.4.4.20-9-6        Array.prototype.filter visits deleted element in array after the call when same index is also present in prototype
    1042, // 15.4.4.20-9-c-ii-1   Array.prototype.filter - callbackfn called with correct parameters
    1049, // 15.4.4.21-10-3       Array.prototype.reduce - subclassed array of length 1
    1050, // 15.4.4.21-10-4       Array.prototype.reduce - subclassed array with length more than 1
    1052, // 15.4.4.21-10-6       Array.prototype.reduce - subclassed array when initialvalue provided
    1053, // 15.4.4.21-10-7       Array.prototype.reduce - subclassed array with length 1 and initialvalue provided
    1088, // 15.4.4.21-9-6        Array.prototype.reduce visits deleted element in array after the call when same index is also present in prototype
    1094, // 15.4.4.21-9-c-ii-4-s Array.prototype.reduce - null passed as thisValue to strict callbackfn
    1100, // 15.4.4.22-10-3       Array.prototype.reduceRight - subclassed array with length 1
    1101, // 15.4.4.22-10-4       Array.prototype.reduceRight - subclassed array with length more than 1
    1103, // 15.4.4.22-10-6       Array.prototype.reduceRight - subclassed array when initialvalue provided
    1104, // 15.4.4.22-10-7       Array.prototype.reduceRight - subclassed array when length to 1 and initialvalue provided
    1134, // 15.4.4.22-9-1        Array.prototype.reduceRight doesn't consider new elements added beyond the initial length of array after it is called
    1139, // 15.4.4.22-9-6        Array.prototype.reduceRight visits deleted element in array after the call when same index is also present in prototype
    1140, // 15.4.4.22-9-7        Array.prototype.reduceRight stops calling callbackfn once the array is deleted during the call
    1146, // 15.4.4.22-9-c-ii-4-s Array.prototype.reduceRight - null passed as thisValue to strict callbackfn
    1151, // 15.4.5.1-3.d-3       Set array length property to max value 4294967295 (2**32-1,)
    1156, // 15.5.4.20-1-1        String.prototype.trim throws TypeError when string is undefined
    1157, // 15.5.4.20-1-2        String.prototype.trim throws TypeError when string is null
};

void test_main() {
    if (tested_version() != version::es5) {
        return;
    }

    constexpr int num_tests = sizeof(tests)/sizeof(*tests);
    gc_heap h{1<<20};
    int unexpected = 0;
    for (int i = 0; i < num_tests; ++i) {
        const auto& t = tests[i];
        const bool expect_failure = std::find(std::begin(expected_failures), std::end(expected_failures), i) != std::end(expected_failures);

        //std::wcout << std::setw(4) <<  (i+1) << "/" << num_tests << "\r" << std::flush;

        try {
            if (h.use_percentage() > 50) {
                h.garbage_collect();
            }

            auto code = unicode::utf8_to_utf16(helper_functions) + L";" + L"(function(){" + unicode::utf8_to_utf16(t.code) + L"})()";
            auto bs = parse(std::make_unique<source_file>(unicode::utf8_to_utf16(t.id), code, version::es5));

            // Could be optimized to reuse interpreter but take to restore global object
            interpreter interpreter_{h, version::es5};
            auto res = interpreter_.eval(*bs);
            if (res.type() != value_type::boolean || !res.boolean_value()) {
                std::wostringstream woss;
                woss << "Unexpected result: " << to_string(h, res).view();
                throw std::runtime_error(unicode::utf16_to_utf8(woss.str()));
            }
            if (expect_failure) {
                std::wcerr << "Test " << std::setw(4) << i << " passed: " << std::setw(20) << std::left << t.id << std::right << " " <<  t.description << " -- EXPECTED FAILURE\n";
                ++unexpected;
            }
        } catch (const std::exception& e) {
            if (!expect_failure) {
                std::wcerr << "Test " << std::setw(4) << i << " failed: " << std::setw(20) << std::left << t.id << std::right << " " <<  t.description << "\n";
                std::wcerr << e.what() << "\n";
                ++unexpected;
            }
        }
    }
    if (unexpected) {
        std::ostringstream oss;
        oss << unexpected << " unexpected result(s)";
        throw std::runtime_error(oss.str());
    }
}