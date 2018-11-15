#include "test_spec.h"
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

value parse_value(const std::string& s) {
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
                    case '\\': [[fallthrough]];
                    case '\'': res.push_back(ch); break;
                    case 'n': res.push_back('\n'); break;
                    default:
                        NOT_IMPLEMENTED("Unhandled escape sequence \\" << (char)ch);
                    }
                    escape = false;
                } else if (ch == '\\') {
                    escape = true;
                } else if (ch == '\'') {
                    assert(!iss.rdbuf()->in_avail());
                    return value{mjs::string{res}};
                } else {
                    res.push_back(ch);
                }
            }
        }
    }
    throw std::runtime_error("Invalid test spec value \"" + s + "\"");
}

struct test_spec {
    uint32_t position;
    value expected;
};

class test_spec_runner {
public:
    static size_t run(const std::vector<test_spec>& specs, const block_statement& statements) {
#ifdef TEST_SPEC_DEBUG
        std::wcout << "Running test spec for " << statements.extend().file->text << "\nSpecs:\n";
        for (const auto& s: specs) {
            std::wcout << pos_w << s.position << " " << s.expected.type() << " " << to_string(s.expected) << "\n";
        }
        std::wcout << "\n";
#endif
        test_spec_runner tsr{specs, statements};
        tsr.i_.eval(statements);
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

    explicit test_spec_runner(const std::vector<test_spec>& specs, const block_statement& statements)
        : specs_(specs)
        , source_(statements.extend().file)
        , i_(statements, [this](const statement& s, const completion& res) {
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
            //gc_heap::local_heap().garbage_collect();
        }) {
    }

    void check_test_spec_done(uint32_t check_pos) {
        if (index_ == specs_.size()) {
            return;
        }

        if (check_pos > specs_[index_].position) {
#ifdef TEST_SPEC_DEBUG
            std::wcout << pos_w << check_pos << ": Checking at pos " << pos_w << specs_[index_].position << " expecting " << specs_[index_].expected.type() << " " << to_string(specs_[index_].expected) << "\n";
#endif
            if (last_result_) {
                std::wostringstream oss;
                oss << source_->filename << " failed: " << last_result_.type << " " << debug_string(last_result_.result);
                THROW_RUNTIME_ERROR(oss.str());
            }
            if (last_result_.result != specs_[index_].expected) {
                std::wostringstream oss;
                oss << "Expecting " << debug_string(specs_[index_].expected) << " got " <<  debug_string(last_result_.result) << " while evaluating " << source_->filename;
                THROW_RUNTIME_ERROR(oss.str());
            }
            ++index_;
        }
    }
};

void run_test_spec(const std::string_view& source_text, const std::string_view& name) {
    constexpr const char delim[] = "//$";
    constexpr const int delim_len = sizeof(delim)-1;


    scoped_gc_heap heap{1<<20};

    {
        std::vector<test_spec> specs;

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
            specs.push_back(test_spec{static_cast<uint32_t>(delim_pos), parse_value(std::string(trim(source_text.substr(delim_pos, next_pos - delim_pos))))});
        }

        if (specs.empty()) {
            throw std::runtime_error("Invalid test spec." + std::string(name) + ": No specs found");
        }

        auto bs = parse(std::make_shared<source_file>(std::wstring(name.begin(), name.end()), std::wstring(source_text.begin(), source_text.end())));
        const auto index = test_spec_runner::run(specs, *bs);
        if (index != specs.size()) {
            throw std::runtime_error("Invalid test spec." + std::string(name) + ": Only " + std::to_string(index) + " of " + std::to_string(specs.size()) + " specs ran");
        }
    }

    heap.garbage_collect();
    if (heap.calc_used()) {
        std::wostringstream oss;
        oss << "Leaks in test spec: " << heap.calc_used() << "\n" << std::wstring(source_text.begin(), source_text.end());
        oss << "\n";
        heap.debug_print(oss);
        THROW_RUNTIME_ERROR(oss.str());
    }

}

} // namespace mjs
