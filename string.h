#ifndef MJS_STRING_H
#define MJS_STRING_H
#include <iosfwd>
#include <string>
#include <string_view>

namespace mjs {

class string {
public:
    explicit string() : s_() {}
    explicit string(const char* str);
    explicit string(const std::wstring_view& s) : s_(s) {}
    explicit string(std::wstring&& s) : s_(std::move(s)) {}
    string(const string& s) : s_(s.s_) {}
    string(string&& s) : s_(std::move(s.s_)) {}
    string& operator=(const string& s) { s_ = s.s_; return *this; }
    string& operator=(const string&& s) { s_ = std::move(s.s_); return *this; }

    string& operator+=(const string& rhs) {
        s_ += rhs.s_;
        return *this;
    }

    std::wstring_view view() const { return s_; }
    const std::wstring& str() const { return s_; }
private:
    std::wstring s_;
};
std::ostream& operator<<(std::ostream& os, const string& s);
std::wostream& operator<<(std::wostream& os, const string& s);
inline bool operator==(const string& l, const string& r) { return l.view() == r.view(); }
inline string operator+(const string& l, const string& r) { auto res = l; return res += r; }

double to_number(const string& s);

} // namespace mjs

namespace std {
template<> struct hash<::mjs::string> {
    size_t operator()(const ::mjs::string& s) const {
        return hash<std::wstring>()(s.str());
    }
};
} // namespace std

#endif
