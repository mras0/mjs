#include "test_es5_conformance.h"

#include "test.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

#include <mjs/char_conversions.h>
#include <mjs/parser.h>
#include <mjs/interpreter.h>

using namespace mjs;

constexpr int expected_failures[] = {
     116, // 11.4.1-5-1-s         delete operator throws ReferenceError when deleting a direct reference to a var in strict mode
     118, // 11.4.1-5-2-s         delete operator throws ReferenceError when deleting a direct reference to a function argument in strict mode
     120, // 11.4.1-5-3-s         delete operator throws ReferenceError when deleting a direct reference to a function name in strict mode
     149, // 12.14-13             catch introduces scope - updates are based on scope
     520, // 15.2.3.3-4-164       Object.getOwnPropertyDescriptor returns data desc for functions on built-ins (RegExp.prototype.compile)
     673, // 15.2.3.4-4-13        Object.getOwnPropertyNames returns array of property names (RegExp.prototype)
     906, // 15.4.4.17-4-9        Array.prototype.some returns -1 if 'length' is 0 (subclassed Array, length overridden with [0]
    1094, // 15.4.4.21-9-c-ii-4-s Array.prototype.reduce - null passed as thisValue to strict callbackfn
    1140, // 15.4.4.22-9-7        Array.prototype.reduceRight stops calling callbackfn once the array is deleted during the call
    1146, // 15.4.4.22-9-c-ii-4-s Array.prototype.reduceRight - null passed as thisValue to strict callbackfn
};

void test_main() {
    if (tested_version() != version::es5) {
        return;
    }

    constexpr int num_tests = sizeof(tests)/sizeof(*tests);
    gc_heap h{1<<20};
    int unexpected = 0;
    const auto helper_code = unicode::utf8_to_utf16(helper_functions) + L";";
    for (int i = 0; i < num_tests; ++i) {
        const auto& t = tests[i];
        const bool expect_failure = std::find(std::begin(expected_failures), std::end(expected_failures), i) != std::end(expected_failures);

        //std::wcout << std::setw(4) <<  (i+1) << "/" << num_tests << "\r" << std::flush;

        std::chrono::high_resolution_clock::time_point t0, t1, t2;

        try {
            if (h.use_percentage() > 50) {
                h.garbage_collect();
            }

            t0 = std::chrono::high_resolution_clock::now();

            auto code = unicode::utf8_to_utf16(t.prelude) +  helper_code + L"(function(){" + unicode::utf8_to_utf16(t.code) + L"})()";
            auto bs = parse(std::make_unique<source_file>(unicode::utf8_to_utf16(t.id), code, version::es5));

            t1 = std::chrono::high_resolution_clock::now();

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
            }
            if (!expect_failure) {
                ++unexpected;
            }
        }

        t2 = std::chrono::high_resolution_clock::now();

        const auto d0 = std::chrono::duration<double>(t1-t0).count();
        const auto d1 = std::chrono::duration<double>(t2-t1).count();
        if (d1>1) {
            std::wcout << std::setw(4) << i << " " << std::setw(10) << d0*1000 << " " << std::setw(10) << d1*1000 << " " << t.id << " is slow\n";
        }
    }
    if (unexpected) {
        std::ostringstream oss;
        oss << unexpected << " unexpected result(s)";
        throw std::runtime_error(oss.str());
    }
}