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
      80, // 11.13.1-4-4-s        simple assignment throws TypeError if LeftHandSide is a readonly property in strict mode (Global.length)
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
     520, // 15.2.3.3-4-164       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RegExp.prototype.compile)
     540, // 15.2.3.3-4-183       Object.getOwnPropertyDescriptor returns undefined for non-existent properties on built-ins (Function.arguments)
     641, // 15.2.3.3-4-74        Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (String.prototype.substr)
     673, // 15.2.3.4-4-13        Object.getOwnPropertyNames returns array of property names (RegExp.prototype)
     701, // 15.2.3.4-4-7         Object.getOwnPropertyNames returns array of property names (String.prototype)
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