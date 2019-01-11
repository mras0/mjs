#include "test_spec.h"
#include "test.h" // for parser_version
#include <mjs/parser.h>
#include <mjs/interpreter.h>
#include <mjs/printer.h>

#include <sstream>
#include <cmath>

//#define TEST_SPEC_DEBUG

#ifdef TEST_SPEC_DEBUG
#include <iostream>
#include <iomanip>

namespace {
const auto pos_w = std::setw(4);
}

#endif

namespace mjs {

std::string_view trim(const std::string_view& s) {
    size_t pos = 0, len = s.length();
    while (pos < s.length() && isblank(s[pos]))
        ++pos, --len;
    while (len && isblank(s[pos+len-1]))
        --len;
    return s.substr(pos, len);
}

char16_t get_escape_sequence(std::istream& in) {
    char16_t n = 0;
    for (int i = 0; i < 4; ++i) {
        if (!in.rdbuf()->in_avail()) {
            NOT_IMPLEMENTED("Unterminated unicode escape sequence");
        }
        int ch = in.get();
        if (ch >= '0' && ch <= '9') {
            ch -= '0';
        } else if (ch >= 'A' && ch <= 'F') {
            ch -= 'A' - 10;
        } else if (ch >= 'a' && ch <= 'f') {
            ch -= 'a' - 10;
        } else {
            NOT_IMPLEMENTED("Invalid hexdigit");
        }
        n = n*16+static_cast<char16_t>(ch);
    }
    return n;
}

value parse_value(gc_heap& h, const std::string& s) {
    std::istringstream iss{s};
    std::string type;
    if (iss >> type) {
        while (iss.rdbuf()->in_avail() && isblank(iss.peek()))
            iss.get();

        if (type == "undefined") {
            assert(!iss.rdbuf()->in_avail());
            return value::undefined;
        } else if (type == "null") {
            assert(!iss.rdbuf()->in_avail());
            return value::null;
        } else if (type == "boolean") {
            std::string val;
            if ((iss >> val) && (val == "true" || val == "false")) {
                assert(!iss.rdbuf()->in_avail());
                return mjs::value{val == "true"};
            }
        }
        if (type == "number" && iss.rdbuf()->in_avail()) {
            if (isalpha(iss.peek())) {
                std::string name;
                if (iss >> name) {
                    assert(!iss.rdbuf()->in_avail());
                    for (auto& c: name) c = static_cast<char>(tolower(c));
                    if (name == "infinity") return mjs::value{INFINITY};
                    if (name == "nan") return mjs::value{NAN};
                }
            } else {
                double val;
                if (iss >> val) {
                    assert(!iss.rdbuf()->in_avail());
                    return value{val};
                }
            }
        }
        if (type == "string" && iss.get() == '\'') {
            std::wstring res;
            bool escape = false;
            while (iss.rdbuf()->in_avail()) {
                const auto ch = static_cast<uint16_t>(iss.get());
                if (escape)  {
                    switch (ch) {
                    case 'b': res.push_back('\x08'); break;
                    case 't': res.push_back('\t'); break;
                    case 'n': res.push_back('\n'); break;
                    case 'v': res.push_back('\x0b'); break;
                    case 'f': res.push_back('\x0c'); break;
                    case 'r': res.push_back('\r'); break;
                    case 'u': res.push_back(get_escape_sequence(iss)); break;
                    case '\"': [[fallthrough]];
                    case '\'': [[fallthrough]];
                    case '\\': res.push_back(ch); break;
                    default:
                        NOT_IMPLEMENTED("Unhandled escape sequence \\" << (char)ch);
                    }
                    escape = false;
                } else if (ch == '\\') {
                    escape = true;
                } else if (ch == '\'') {
                    assert(!iss.rdbuf()->in_avail());
                    return value{mjs::string{h, res}};
                } else {
                    res.push_back(ch);
                }
            }
        }
    }
    throw std::runtime_error("Invalid test spec value \"" + s + "\"");
}

struct test_spec {
    source_extend extend;
    uint32_t position;
    value expected;
};

class test_spec_runner {
public:
    static size_t run(gc_heap& h, const std::vector<test_spec>& specs, const block_statement& statements) {
#ifdef TEST_SPEC_DEBUG
        std::wcout << "Running test spec for " << statements.extend().file->text() << "\n";
        std::wcout << "Parsed:\n";
        print(std::wcout, statements);
        std::wcout << "Specs:\n";
        for (const auto& s: specs) {
            std::wcout << pos_w << s.position << " " << s.expected.type() << " " << to_string(h, s.expected) << "\n";
        }
        std::wcout << "\n";
#endif
        test_spec_runner tsr{h, specs, statements};
        tsr.i_.eval_program();
        tsr.check_test_spec_done(statements.extend().end);
#ifdef TEST_SPEC_DEBUG
        std::wcout << "\n";
#endif
        return tsr.index_;
    }

private:
    const std::vector<test_spec>& specs_;
    std::shared_ptr<source_file> source_;
    interpreter i_;
    size_t index_ = 0;
    uint32_t last_line_ = 0;
    completion last_result_{};
#ifdef TEST_SPEC_DEBUG
    gc_heap& heap_;
#endif

    explicit test_spec_runner(gc_heap& h, const std::vector<test_spec>& specs, const block_statement& statements)
        : specs_(specs)
        , source_(statements.extend().file)
        , i_(h, tested_version(), statements, [this](const statement& s, const completion& res) {
#ifdef TEST_SPEC_DEBUG
            std::wcout << pos_w << s.extend().start << "-" << pos_w << s.extend().end << ": ";
            print(std::wcout, s);
            std::wcout << " ==> " << debug_string(res.result) << "\n";
#endif
            if (s.extend().file == source_ && s.extend().start > last_line_) {
                check_test_spec_done(s.extend().start);
                last_result_ = res;
                last_line_ = s.extend().start;
            }
#if 0 // The stress test should be enabled, but this might still be relevant later on when debugging the GC
            h.garbage_collect(); // Run garbage collection after each statement to help catch bugs
#endif
        })
#ifdef TEST_SPEC_DEBUG
        , heap_(h)
#endif
    {
    }

    void check_test_spec_done(uint32_t check_pos) {
        if (index_ == specs_.size()) {
            return;
        }
        const auto& s = specs_[index_];

        if (check_pos > s.position) {
#ifdef TEST_SPEC_DEBUG
            std::wcout << pos_w << check_pos << ": Checking pos " << pos_w << s.position << " at " << s.extend << " expecting " << debug_string(s.expected) << "\n";
#endif
            if (last_result_) {
                std::wostringstream oss;
                oss << source_->filename() << " failed: " << last_result_.type << " " << debug_string(last_result_.result) << " at " << s.extend << "\n" << s.extend.source_view();
                THROW_RUNTIME_ERROR(oss.str());
            }
            if (last_result_.result != specs_[index_].expected) {
                std::wostringstream oss;
                oss << "Expecting\n" << debug_string(specs_[index_].expected) << "\ngot\n" << debug_string(last_result_.result) << "\nat " << s.extend << "\n" << s.extend.source_view();
                THROW_RUNTIME_ERROR(oss.str());
            }
            ++index_;
        }
    }
};

void run_test_spec(const std::string_view& source_text, const std::string_view& name) {
    constexpr const char delim[] = "//$";
    constexpr const int delim_len = sizeof(delim)-1;


    gc_heap heap{1<<20};

    {
        std::vector<test_spec> specs;
        auto file = std::make_shared<source_file>(std::wstring(name.begin(), name.end()), std::wstring(source_text.begin(), source_text.end()), tested_version());

        for (size_t pos = 0, next_pos; pos < source_text.length(); pos = next_pos + 1) {
            size_t delim_pos = source_text.find(delim, pos);
            if (delim_pos == std::string_view::npos) {
                break;
            }
            next_pos = source_text.find("\n", delim_pos);
            if (next_pos == std::string_view::npos) {
                throw std::runtime_error("Invalid test spec." + std::string(name) + ": line terminator missing.");
            }

            delim_pos += delim_len;

            source_extend extend{file, static_cast<uint32_t>(pos), static_cast<uint32_t>(next_pos)};

            specs.push_back(test_spec{extend, static_cast<uint32_t>(delim_pos), parse_value(heap, std::string(trim(source_text.substr(delim_pos, next_pos - delim_pos))))});
        }

        if (specs.empty()) {
            throw std::runtime_error("Invalid test spec." + std::string(name) + ": No specs found");
        }

        auto bs = parse(file);
        const auto index = test_spec_runner::run(heap, specs, *bs);
        if (index != specs.size()) {
            throw std::runtime_error("Invalid test spec." + std::string(name) + ": Only " + std::to_string(index) + " of " + std::to_string(specs.size()) + " specs ran");
        }
    }

    heap.garbage_collect();
    if (heap.use_percentage()) {
        std::wostringstream oss;
        oss << "Leaks in test spec: " << heap.use_percentage() << "%\n" << std::wstring(source_text.begin(), source_text.end());
        THROW_RUNTIME_ERROR(oss.str());
    }

}

} // namespace mjs
