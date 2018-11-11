#ifndef MJS_STRING_H
#define MJS_STRING_H
#include <iosfwd>
#include <string>
#include <string_view>
#include "gc_heap.h"

namespace mjs {

class gc_string {
public:
    static gc_heap_ptr<gc_string> make(gc_heap& h, const std::wstring_view& s) {
        return gc_heap::construct<gc_string>(h.allocate(sizeof(gc_string) + s.length() * sizeof(wchar_t)), s);
    }

    std::wstring_view view() const {
        return std::wstring_view(const_cast<gc_string&>(*this).data(), length_);
    }

private:
    friend gc_type_info_registration<gc_string>;

    uint32_t length_; // TODO: Get from allocation header

    wchar_t* data() {
        return reinterpret_cast<wchar_t*>(reinterpret_cast<std::byte*>(this) + sizeof(*this));
    }

    explicit gc_string(const std::wstring_view& s) : length_(static_cast<uint32_t>(s.length())) {
        std::memcpy(data(), s.data(), s.length() * sizeof(wchar_t));
    }

    gc_heap_ptr_untyped move(gc_heap& new_heap) const {
        return make(new_heap, view());
    }
};

// TODO: Try to eliminate (or lessen) use of local heap
class string {
public:
    explicit string() : string(L"") {}
    // TODO: Could optimize this constructor by providing appropriate overloads in gc_string
    explicit string(const char* str) : string(std::wstring(str, str+std::strlen(str))) {}
    explicit string(const std::wstring_view& s) : s_(gc_string::make(gc_heap::local_heap(),s)) {}
    std::wstring_view view() const { return s_->view(); }
private:
    gc_heap_ptr<gc_string> s_;
};
std::ostream& operator<<(std::ostream& os, const string& s);
std::wostream& operator<<(std::wostream& os, const string& s);
inline bool operator==(const string& l, const string& r) { return l.view() == r.view(); }
inline string operator+(const string& l, const string& r) {
    // TODO: Optimize this
    return string{std::wstring{l.view()} + std::wstring{r.view()}};
}

double to_number(const string& s);

} // namespace mjs

namespace std {
template<> struct hash<::mjs::string> {
    size_t operator()(const ::mjs::string& s) const {
        return hash<std::wstring_view>()(s.view());
    }
};
} // namespace std

#endif
