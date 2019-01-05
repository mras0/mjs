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

std::shared_ptr<mjs::source_file> read_ascii_file(mjs::version ver, const char* filename) {
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("Could not open " + std::string(filename));
    return std::make_shared<mjs::source_file>(
        std::wstring(filename, filename+std::strlen(filename)), 
        std::wstring((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()),
        ver
        );
}

std::shared_ptr<mjs::source_file> make_source(const std::wstring_view& s, mjs::version ver) {
    return std::make_shared<mjs::source_file>(std::wstring(L"inline code"), std::wstring(s), ver);
}

int interpret_file(const std::shared_ptr<mjs::source_file>& source) {
    mjs::gc_heap heap{1<<24}; // TODO: Do something sane
    auto bs = mjs::parse(source);
    mjs::interpreter i{heap, source->language_version(), *bs};
    return to_int32(i.eval_program());
}

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
void stream_init() {
    if (_isatty(_fileno(stdout))) {
        _setmode(_fileno(stdin), _O_U16TEXT);
        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);
    }
}
#else
void stream_init() {}
#endif

int main(int argc, char* argv[]) {
    stream_init();
    try {
        auto ver = mjs::default_version;
        if (argc > 1 && !std::strncmp(argv[1], "-es", 3)) {
            std::istringstream iss{&argv[1][3]};
            int v;
            if (!(iss >> v) || iss.rdbuf()->in_avail() || (v != 1 && v != 3)) {
                throw std::runtime_error(std::string("Invalid version argument: ") + argv[1]);
            }
            if (v == 1) ver = mjs::version::es1;
            else if (v == 3) ver = mjs::version::es3;
            else assert(false);
            --argc;
            ++argv;
        }

        if (argc > 1) {
            return interpret_file(read_ascii_file(ver, argv[1]));
        }

        mjs::gc_heap heap{1<<22}; // TODO: Do something sane
        mjs::interpreter i{heap, ver, *mjs::parse(make_source(L"", ver))};
        for (;;) {
            std::wcout << "> " << std::flush;
            std::wstring line;
            if (!getline(std::wcin, line)) {
                break;
            }
            mjs::value res{};
            auto bs = mjs::parse(make_source(line, ver));
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
