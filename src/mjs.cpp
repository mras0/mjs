// TODO: use <filesystem> when supported on windows with GCC
#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <chrono>
#include <fstream>
#include <streambuf>
#include <cstring>

#include <mjs/value.h>
#include <mjs/parser.h>
#include <mjs/interpreter.h>
#include <mjs/printer.h>
#include <mjs/platform.h>
#include <mjs/char_conversions.h>
#include <mjs/function_object.h>

using namespace mjs;

constexpr uint32_t deafult_heap_size = 1<<24;
std::wstring base_dir;

std::shared_ptr<source_file> read_utf8_file(version ver, const std::wstring_view filename) {
    const auto u8fname = unicode::utf16_to_utf8(filename);
#ifdef _MSC_VER
    std::ifstream in(std::wstring{filename});
#else
    std::ifstream in(u8fname);
#endif
    if (!in) throw std::runtime_error("Could not open \"" + u8fname + "\"");
    return std::make_shared<source_file>(
        filename,
        unicode::utf8_to_utf16(std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>())),
        ver
        );
}

std::shared_ptr<source_file> make_source(const std::wstring_view s, version ver) {
    return std::make_shared<source_file>(std::wstring(L"inline code"), std::wstring(s), ver);
}

value load_file(interpreter& i, const std::wstring_view path) {   
#if 0
    std::wcout << "Loading " << path << "\n";
#endif
    auto& global = *i.global();
    std::unique_ptr<block_statement> bs;
    try {
        bs = parse(read_utf8_file(global.language_version(), base_dir + std::wstring{path}), global.strict_mode());
    } catch (const std::exception& e) {
        throw native_error_exception{native_error_type::syntax, global.stack_trace(), e.what()};
    }
    return i.eval(*bs);
}

void add_functions(interpreter& i) {
    auto global = i.global();
    put_native_function(global, global, "load", [&i](const value&, const std::vector<value>& args) {
        if (args.size() != 1 || args[0].type() != value_type::string) {
            throw native_error_exception{native_error_type::eval, i.global()->stack_trace(), "path must be a string"};
        }
        return load_file(i, args[0].string_value().view());
    }, 1);
}

int interpret_file(const std::shared_ptr<source_file>& source) {
    gc_heap heap{deafult_heap_size};
    interpreter i{heap, source->language_version()
#if 0
        , [](const statement& s, const completion& c) {
        std::wcout << s << " ----> " << c << "\n\n";
    }
#endif
    };
    add_functions(i);
    return to_int32(i.eval(*parse(source)));
}

void set_base_dir(const std::wstring_view fname) {
    const wchar_t* last_slash = fname.data();
    for (const auto& ch: fname) {
        if (ch == '/'
#ifdef _WIN32
            || ch == '\\'
#endif
            ) {
            last_slash = &ch;
        }
    }
    base_dir = last_slash == fname.data() ? L"." : std::wstring{fname.data(), last_slash};
#ifdef _WIN32
    std::replace(base_dir.begin(), base_dir.end(), L'\\', L'/');
#endif
    base_dir.push_back(L'/');
}

int main(int argc, char* argv[]) {
    platform_init();
    //base_dir = std::filesystem::current_path();
    try {
        auto ver = version::latest;
        if (argc > 1 && !std::strncmp(argv[1], "-es", 3)) {
            std::istringstream iss{&argv[1][3]};
            int v;
            if (!(iss >> v) || iss.rdbuf()->in_avail() || (v != 1 && v != 3 && v != 5)) {
                throw std::runtime_error(std::string("Invalid version argument: ") + argv[1]);
            }
            if (v == 1) ver = version::es1;
            else if (v == 3) ver = version::es3;
            else if (v == 5) ver = version::es5;
            else assert(false);
            --argc;
            ++argv;
        }

        if (argc > 1) {
            auto fname = unicode::utf8_to_utf16(argv[1]);
            set_base_dir(fname);
            return interpret_file(read_utf8_file(ver, fname));
        }

        gc_heap heap{deafult_heap_size};
        interpreter i{heap, ver};
        add_functions(i);
        for (;;) {
            std::wcout << "> " << std::flush;
            std::wstring line;
            if (!getline(std::wcin, line)) {
                break;
            }
            const value res = i.eval(*parse(make_source(line, ver)));
            debug_print(std::wcout, res, 2);
            std::wcout << "\n";
        }

    } catch (const std::exception& e) {
        std::wcout << unicode::utf8_to_utf16(e.what()) << "\n";
        return 1;
    }
}
