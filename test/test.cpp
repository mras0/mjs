#include "test.h"
#include <mjs/parser.h>
#include <mjs/interpreter.h>
#include <mjs/printer.h>
#include <mjs/platform.h>
#include <sstream>

using namespace mjs;

version tested_version_ = version::latest;

mjs::version tested_version() {
    return tested_version_;
}

static const char* run_test_func;
static const char* run_test_file;
static int run_test_line;
void run_test_debug_pos(const char* func, const char* file, int line) {
    run_test_func = func;
    run_test_file = file;
    run_test_line = line;
}

void run_test(const std::wstring_view& text, const value& expected) {
    assert(run_test_func && run_test_file && run_test_line);

    // Use local heap, even if expected lives in another heap
    constexpr uint32_t num_slots = 1<<21;
#ifndef MJS_GC_STRESS_TEST
    // Re-use storage unless we're stress testing
    static uint64_t storage[num_slots];
    gc_heap h{storage, num_slots};
#else
    gc_heap h{num_slots};
#endif

    {
        decltype(parse(nullptr)) bs;
        try {
            bs = parse(std::make_shared<source_file>(L"test", text, tested_version()));
        } catch (const std::exception& e) {
            std::wcout << "Parse failed for \"" << text << "\": " << e.what() <<  "\n";
            throw;
        }
        std::wostringstream error_stream;
        auto pb = [&]() {
            const char * start = "----------------\n";
            const char * end = "----------------\n\n";

            error_stream << "\n";
            error_stream << run_test_file << ":" << run_test_line << " in " << run_test_func << ":\n";

            error_stream << "Test failure in:\n";
            error_stream << start;
            error_stream << text << "\n";
            error_stream << end;

            error_stream << "Parsed as:\n";
            error_stream << start;
            for (const auto& s: bs->l()) {
                error_stream << *s << "\n\n";
            }
            error_stream << end;

            error_stream << "Pretty printed:\n";
            error_stream << start;
            print(error_stream, *bs);
            error_stream << "\n";
            error_stream << end;

            error_stream << "\n";
        };
        completion c;
        try {
            interpreter i{h, tested_version(), *bs};
            c = i.eval(*bs);
        } catch (const std::exception& e) {
            pb();
            error_stream << "Unexpected exception thrown: " << e.what() << "\n";
            THROW_RUNTIME_ERROR(error_stream.str());
        }
        if (c) {
            pb();
            error_stream << "Got unexpected abrupt completion: " << c << "\n";
            THROW_RUNTIME_ERROR(error_stream.str());
        }
        if (c.result != expected) {
            pb();
            error_stream << "Expecting " << debug_string(expected) << " got " << debug_string(c.result) << "\n";
            THROW_RUNTIME_ERROR(error_stream.str());
        }
    }

    h.garbage_collect();
    const auto used_now = h.use_percentage();
    if (used_now) {
        std::wcout << "Leaks! Used now: " << used_now;
        THROW_RUNTIME_ERROR("Leaks");
    }
}

std::string expect_eval_exception(const std::wstring_view& text) {
    decltype(parse(nullptr)) bs;
    try {
        bs = parse(std::make_shared<source_file>(L"test", text, tested_version()));
    } catch (const std::exception& e) {
        std::wcout << "Parse failed for \"" << text << "\": " << e.what() <<  "\n";
        throw;
    }

    try {
        gc_heap h{1<<20}; // Use local heap, even if expected lives in another heap
        interpreter i{h, tested_version(), *bs};
        (void) i.eval_program();
    } catch (const eval_exception& e) {
        return e.what();
    } catch (const std::exception& e) {
        std::wcout << "Unexpected exception thrown: " << e.what() << " while processing\n" << text << "\n"; 
        throw;
    }

    std::wcout << "Exception not thrown in\n" << text << "\n";
    THROW_RUNTIME_ERROR("Expected exception not thrown");
}

namespace mjs {

bool operator==(const token& l, const token& r) {
    if (l.type() != r.type()) {
        return false;
    } else if (l.has_text()) {
        return l.text() == r.text();
    } else if (l.type() == token_type::numeric_literal) {
        return l.dvalue() == r.dvalue();
    }
    return true;
}


}  // namespace mjs

int main() {
    platform_init();
    try {
        for (const auto ver: supported_versions) {
            tested_version_ = ver;
            test_main();
        }
    } catch (const std::exception& e) {
        std::wcerr << e.what() << "\n";
        std::wcerr << "Tested version: " << tested_version() << "\n";
        return 1;
    }
    return 0;
}