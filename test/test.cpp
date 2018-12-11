#include "test.h"
#include <mjs/parser.h>
#include <mjs/interpreter.h>
#include <mjs/printer.h>

using namespace mjs;

version parser_version = default_version;

void run_test(const std::wstring_view& text, const value& expected) {
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
            bs = parse(std::make_shared<source_file>(L"test", text), parser_version);
        } catch (const std::exception& e) {
            std::wcout << "Parse failed for \"" << text << "\": " << e.what() <<  "\n";
            throw;
        }
        auto pb = [&bs]() {
            print(std::wcout, *bs);
            std::wcout << '\n';
            for (const auto& s: bs->l()) {
                std::wcout << *s << "\n";
            }
        };
        try {
            interpreter i{h, *bs};
            value res = i.eval_program();
            if (res != expected) {
                std::wcout << "Test failed: " << text << " expecting " << debug_string(expected) << " got " << debug_string(res) << "\n";
                pb();
                THROW_RUNTIME_ERROR("Test failed");
            }
        } catch (const std::exception& e) {
            std::wcout << "Test failed: " << text << " uexpected exception thrown: " << e.what() << "\n";
            pb();
            throw;
        }
    }

    h.garbage_collect();
    const auto used_now = h.use_percentage();
    if (used_now) {
        std::wcout << "Leaks! Used now: " << used_now;
        THROW_RUNTIME_ERROR("Leaks");
    }
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