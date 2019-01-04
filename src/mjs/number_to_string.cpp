#include "number_to_string.h"
#include <sstream>
#include <cassert>
#include <cmath>
#include <cstring>

namespace mjs {

namespace {

struct ecvt_result {
    int  k;     // number of decimal digits in s
    int  n;     // position of decimal point in s
    char s[18]; // NUL-terminated array of length k
};

ecvt_result do_ecvt(double m, int k) {
    assert(k >= 1); // k is the number of decimal digits in the representation

    ecvt_result res;
    res.k = k;
#if 1
    char temp[32];
    snprintf(temp, sizeof(temp), "%1.*e", k-1, m);
    res.s[0] = temp[0];
    if (k > 1) {
        assert(temp[1] == '.');
        std::memcpy(&res.s[1], &temp[2], k-1);
    }
    const char* exp = &temp[k+(k>1)+1];
    assert(exp[-1] == 'e' && (exp[0] == '+' || exp[0] == '-'));
    res.n = 0;
    for (int i = 1; exp[i]; ++i) {
        res.n = res.n*10 + exp[i] - '0';
    }
    if (exp[0] == '-') {
        res.n = -res.n;
    }
    ++res.n; // Adjust for "1." format part
#else
    int sign;                   // sign is set if the value is negative (never true since to_string handles that)
#ifdef _MSC_VER
    char s[_CVTBUFSIZE + 1];    // s is the decimal representation of the number
    _ecvt_s(s, m, k, &res.n, &sign);
#else
    const char* s = ecvt(m, k, &res.n, &sign);
#endif
    assert(sign == 0);
    assert((int)std::strlen(s) == k);
    std::memcpy(res.s, s, k);
#endif
    res.s[k] = '\0';
    return res;
}

} // unnamed namespace

std::wstring do_format_double(double m, int k) {
    auto [k_, n, s] = do_ecvt(m, k); (void)k_;

    std::wostringstream woss;
    if (k <= n && n <= 21) {
        // 6. If k <= n <= 21, return the string consisting of the k digits of the decimal
        // representation of s (in order, with no leading zeroes), followed by n - k
        // occurences of the character ‘0’
        woss << s << std::wstring(n-k, '0');
    } else if (0 < n && n <= 21) {
        // 7. If 0 < n <= 21, return the string consisting of the most significant n digits
        // of the decimal representation of s, followed by a decimal point ‘.’, followed
        // by the remaining k - n digits of the decimal representation of s.
        woss << std::wstring(s, s + n) << '.' << std::wstring(s + n, s + std::strlen(s));
    } else if (-6 < n && n <= 0) {
        // 8. If -6 < n <= 0, return the string consisting of the character ‘0’, followed
        // by a decimal point ‘.’, followed by -n occurences of the character ‘0’, followed
        // by the k digits of the decimal representation of s.
        woss << "0." << std::wstring(-n, '0') << s;
    } else if (k == 1) {
        // 9.  Otherwise, if k = 1, return the string consisting of the single digit of s,
        // followed by lowercase character ‘e’, followed by a plus sign ‘+’ or minus sign
        // ‘-’ according to whether n - 1 is positive or negative, followed by the decimal
        // representation of the integer abs(n - 1) (with no leading zeros).
        woss << s << 'e' << (n-1>=0?'+':'-') << std::abs(n-1);
    } else {
        // 10. Return the string consisting of the most significant digit of the decimal
        // representation of s, followed by a decimal point ‘.’, followed by the remaining
        // k - 1 digits of the decimal representation of s , followed by the lowercase character
        // ‘e’, followed by a plus sign ‘+’ or minus sign ‘-’ according to whether n - 1 is positive
        // or negative, followed by the decimal representation of the integer abs(n - 1)
        // (with no leading zeros)
        woss << s[0] << '.' << s+1 << 'e' << (n-1>=0?'+':'-') << std::abs(n-1);
    }
    return woss.str();
}

std::wstring number_to_string(double m) {
    // Handle special cases
    if (std::isnan(m)) {
        return L"NaN";
    }
    if (m == 0) {
        return L"0";
    }
    if (m < 0) {
        return L"-" + number_to_string(-m);
    }
    if (std::isinf(m)) {
        return L"Infinity";
    }

    assert(std::isfinite(m) && m > 0);

    // 9.8.1 ToString Applied to the Number Type    

    // Use really slow method to determine shortest representation of m
    for (int k = 1; k <= 17; ++k) {
        std::ostringstream woss;
        woss.precision(k);
        woss << std::defaultfloat << m;
        if (double t; std::istringstream{woss.str()} >> t && t == m) {
            return do_format_double(m, k);
        }
    }
    // More than 17 digits should never happen
    assert(false);
    throw std::runtime_error("Internal error");
}

std::wstring number_to_fixed(double x, int f) {
    // TODO: Implement actual rules from ES3, 15.7.4.5
    assert(std::isfinite(x) && std::fabs(x) < 1e21 && f >= 0 && f <= 20);
    std::wstringstream wss;
    if (x < 0) {
        x = -x;
        wss << L'-';
    }
    wss.precision(f);
    wss.flags(std::ios::fixed);
    wss << x;
    return wss.str();
}

std::wstring number_to_exponential(double x, int f) {
    // TODO: Implement actual rules from ES3, 15.7.4.6
    assert(std::isfinite(x) && f >= 0 && f <= 20);
    std::wstringstream wss;
    if (x < 0) {
        x = -x;
        wss << L'-';
    }
    wss.precision(f ? f : 18);
    wss.flags(std::ios::scientific);
    wss << x;
    auto s = wss.str();

    // Remove unnecessary digits (yuck)
    if (f == 0) {
        auto e_pos = s.find_last_of(L'e');
        assert(e_pos != std::wstring::npos && e_pos > 0);
        for (;;) {
            --e_pos;
            if (s[e_pos] == '.') {
                s.erase(e_pos, 1);
                break;
            }
            if (s[e_pos] != '0') {
                break;
            }
            s.erase(e_pos, 1);
        }
    }

    // Fix up the exponent to match ECMAscript format (yuck)
    auto e_pos = s.find_last_of(L'e');
    assert(e_pos != std::wstring::npos && e_pos+2 < s.length() && (s[e_pos+1] == '+' || s[e_pos+1] == '-'));
    e_pos += 2;

    // Remove leading zeros in exponent
    while (e_pos + 1 < s.length() && s[e_pos] == L'0') {
        s.erase(e_pos, 1);
    }

    return s;
}

std::wstring number_to_precision(double x, int p) {
    // HACK HACK
    assert(std::isfinite(x) && p >= 1 && p <= 21);
    std::wstring s = L"";
    if (x < 0) {
        s = L"-";
        x = -x;
    }

    auto [k, e, digits] = do_ecvt(x, p);
    std::wstring m = std::wstring{digits, digits+k};
    e--;
    if (e < -6 || e >= p) {
        if (m.size() > 1) {
            m.insert(m.begin() + 1, L'.');
        }
        if (e == 0) {
            m += L"e+0";
        } else if (e > 0) {
            m += L"e+" + std::to_wstring(e);
        } else {
            m += L"e-" + std::to_wstring(-e);
        }
    } else if (e == p - 1) {
    } else if (e >= 0) {
        m.insert(m.begin() + e + 1, L'.');
    } else {
        m = L"0." + std::wstring(-(e+1), L'0') + m;
    }

    return s + m;
}

} // namespace mjs