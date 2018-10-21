#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <chrono>

#include "value.h"
#include "parser.h"
#include "interpreter.h"
#include "printer.h"

#include <fstream>
#include <streambuf>
#include <cstring>
std::shared_ptr<mjs::source_file> read_ascii_file(const char* filename) {
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("Could not open " + std::string(filename));
    return std::make_shared<mjs::source_file>(std::wstring(filename, filename+std::strlen(filename)), std::wstring((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
}

int main() {
    try {
        auto source = read_ascii_file("../js-performance-test.js");
        auto bs = mjs::parse(source);
        mjs::interpreter i{*bs};
        for (const auto& s: bs->l()) {
            if constexpr (false) {
                std::wcout << "> ";
                print(std::wcout, *s);
                std::wcout << "\n";
                std::wcout << i.eval(*s).result << "\n";
            } else {
                (void)i.eval(*s);
            }
        }

    } catch (const std::exception& e) {
        std::wcout << e.what() << "\n";
        return 1;
    }
#ifndef NDEBUG
    mjs::object::garbage_collect({});
    assert(!mjs::object::object_count());
#endif
}
