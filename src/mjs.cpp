#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <chrono>

#include <mjs/value.h>
#include <mjs/parser.h>
#include <mjs/interpreter.h>
#include <mjs/printer.h>

#include <fstream>
#include <streambuf>
#include <cstring>

std::shared_ptr<mjs::source_file> read_ascii_file(const char* filename) {
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("Could not open " + std::string(filename));
    return std::make_shared<mjs::source_file>(std::wstring(filename, filename+std::strlen(filename)), std::wstring((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
}

std::shared_ptr<mjs::source_file> make_source(const std::wstring_view& s) {
    return std::make_shared<mjs::source_file>(std::wstring(L"inline code"), std::wstring(s));
}

int interpret_file(mjs::version ver, const std::shared_ptr<mjs::source_file>& source) {
    mjs::gc_heap heap{1<<24}; // TODO: Do something sane
    auto bs = mjs::parse(source, ver);
    mjs::interpreter i{heap, ver, *bs};
    return to_int32(i.eval_program());
}

int main(int argc, char* argv[]) {
    try {
        const auto ver = mjs::default_version;
        if (argc > 1) {
            return interpret_file(ver, read_ascii_file(argv[1]));
        }

        mjs::gc_heap heap{1<<22}; // TODO: Do something sane
        mjs::interpreter i{heap, ver, *mjs::parse(make_source(L""), ver)};
        for (;;) {
            std::wcout << "> " << std::flush;
            std::wstring line;
            if (!getline(std::wcin, line)) {
                break;
            }
            mjs::value res{};
            auto bs = mjs::parse(make_source(line), ver);
            for (const auto& s: bs->l()) {
                auto c = i.eval(*s);
                if (c) {
                    std::wcout << "abrupt completion: " << c << "\n";
                    continue;
                }
                res = c.result;
            }
            mjs::debug_print(std::wcout, res, 2);
            std::wcout << "\n";
        }

    } catch (const std::exception& e) {
        std::wcout << e.what() << "\n";
        return 1;
    }
}
