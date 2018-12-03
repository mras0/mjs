#ifndef MJS_STRING_H
#define MJS_STRING_H
#include <iosfwd>
#include <string>
#include <string_view>
#include "gc_heap.h"

namespace mjs {

class gc_string {
public:
    template<typename CharT>
    static gc_heap_ptr<gc_string> make(gc_heap& h, const std::basic_string_view<CharT>& s) {
        return h.allocate_and_construct<gc_string>(sizeof(gc_string) + s.length() * sizeof(wchar_t), s);
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

    explicit gc_string(const std::string_view& s) : length_(static_cast<uint32_t>(s.length())) {
        for (uint32_t i = 0; i < length_; ++i) {
            data()[i] = s[i];
        }
    }

    explicit gc_string(const std::wstring_view& s) : length_(static_cast<uint32_t>(s.length())) {
        std::memcpy(data(), s.data(), s.length() * sizeof(wchar_t));
    }

    explicit gc_string(gc_string&& other) noexcept : length_(other.length_) {
        std::memcpy(data(), other.data(), other.length_ * sizeof(wchar_t));
    }
};

class string : private gc_heap_ptr<gc_string> {
public:
    string(const gc_heap_ptr<gc_string>& s) : gc_heap_ptr<gc_string>(s) {}
    explicit string(gc_heap& h, const std::string_view& s) : gc_heap_ptr<gc_string>(gc_string::make(h, s)) {}
    explicit string(gc_heap& h, const std::wstring_view& s) : gc_heap_ptr<gc_string>(gc_string::make(h, s)) {}

    using gc_heap_ptr<gc_string>::heap;

    std::wstring_view view() const { return get()->view(); }
    const gc_heap_ptr<gc_string>& unsafe_raw_get() const { return *this; }
};
std::ostream& operator<<(std::ostream& os, const string& s);
std::wostream& operator<<(std::wostream& os, const string& s);
inline bool operator==(const string& l, const string& r) { return l.view() == r.view(); }
inline string operator+(const string& l, const string& r) {
    // TODO: Optimize this
    return string{l.heap(), std::wstring{l.view()} + std::wstring{r.view()}};
}

double to_number(const string& s);

} // namespace mjs

#endif
