#include "test.h"
#include <mjs/char_conversions.h>
#include <mjs/platform.h>

using namespace mjs;
using namespace mjs::unicode;

void test_utf8_length() {
    const struct {
        uint8_t end;
        unsigned expected_length;
    } cases[] = {
        { 0b0111'1111, 1 },
        { 0b1011'1111, 0 } ,
        { 0b1101'1111, 2 } ,
        { 0b1110'1111, 3 } ,
        { 0b1111'0111, 4 } ,
        { 0b1111'1111, 0 } ,
    };

    unsigned first = 0;
    for (const auto& c: cases) {
        for (unsigned cp = first; cp <= c.end; ++cp) {
            REQUIRE_EQ(utf8_length_from_lead_code_unit(static_cast<uint8_t>(cp)), c.expected_length);
        }
        first = c.end + 1;
    }
}

void test_utf8_conversion() {
    const struct {
        const char* in;
        char32_t out;
    } single_char_cases[] = {
        { "$"                , 0x00024 },
        { "\xC2\xA2"         , 0x000A2 },
        { "\xE0\xA4\xB9"     , 0x00939 },
        { "\xE2\x82\xAC"     , 0x020AC },
        { "\xF0\x90\x8D\x88" , 0x10348 },
    };
    char buffer[utf8_max_length];

    REQUIRE_EQ(utf8_to_utf32("\0", 1).length, 1U);
    REQUIRE_EQ(utf8_to_utf32("\0", 1).code_point, 0U);
    for (const auto& c: single_char_cases) {
        const auto len = static_cast<unsigned>(std::strlen(c.in));
        const auto res = utf8_to_utf32(c.in, len);
        REQUIRE_EQ(res.length, len);
        REQUIRE_EQ(res.code_point, c.out);

        REQUIRE_EQ(utf8_code_unit_count(c.out), len);
        REQUIRE_EQ(utf32_to_utf8(c.out, buffer), len);
        REQUIRE(!strncmp(buffer, c.in, len));
    }
    REQUIRE_EQ(utf32_to_utf8(max_code_point-1, buffer), 4U);
    REQUIRE(!strncmp(buffer, "\xf4\x8f\xbf\xbf", 4));
    REQUIRE_EQ(utf32_to_utf8(max_code_point, buffer), 0U);
    REQUIRE_EQ(utf32_to_utf8(UINT32_MAX, buffer), 0U);
}

void test_utf16_conversion() {
    const struct {
        uint32_t code_point;
        wchar_t  chars[2];
    } cases[] = {
        { 0x00000, { 0x0000, 0x0000 } },
        { 0x00024, { 0x0024, 0x0000 } },
        { 0x020AC, { 0x20AC, 0x0000 } },
        { 0x10437, { 0xD801, 0xDC37 } },
        { 0x24B62, { 0xD852, 0xDF62 } },
    };

    wchar_t buffer[utf16_max_length];

    for (const auto& c: cases) {
        const unsigned len = c.chars[1] != 0 ? 2 : 1;
        REQUIRE_EQ(utf16_length_from_lead_code_unit(c.chars[0]), len);
        const auto res = utf16_to_utf32(c.chars, len);
        REQUIRE_EQ(res.length, len);
        REQUIRE_EQ(res.code_point, c.code_point);

        REQUIRE_EQ(utf16_code_unit_count(c.code_point), len);

        REQUIRE_EQ(utf32_to_utf16(c.code_point, buffer), len);
        REQUIRE_EQ(buffer[0], c.chars[0]);
        if (len>1) {
            REQUIRE_EQ(buffer[1], c.chars[1]);
        }
    }

    REQUIRE_EQ(utf16_length_from_lead_code_unit(0xdc00), 0U);
    REQUIRE_EQ(utf16_length_from_lead_code_unit(0xd800), 2U);
    REQUIRE_EQ(utf16_to_utf32(L"\xdc00", 2).length, 0U);
    REQUIRE_EQ(utf16_to_utf32(L"\xd800", 1).length, 0U);

    REQUIRE_EQ(utf32_to_utf16(max_code_point, buffer), 0U);

    REQUIRE_EQ(utf16_code_unit_count(utf16_high_surrogate), 0U);
    REQUIRE_EQ(utf16_code_unit_count(utf16_last_surrogate), 0U);
    REQUIRE_EQ(utf16_code_unit_count(max_code_point), 0U);
}

void test_utf8_to_utf16() {
    REQUIRE_EQ(utf8_to_utf16(""), L"");
    REQUIRE_EQ(utf8_to_utf16("abc"), L"abc");

    REQUIRE_EQ(utf8_to_utf16("$"                ) , L"\x0024" );
    REQUIRE_EQ(utf8_to_utf16("\xC2\xA2"         ) , L"\x00A2" );
    REQUIRE_EQ(utf8_to_utf16("\xE0\xA4\xB9"     ) , L"\x0939" );
    REQUIRE_EQ(utf8_to_utf16("\xE2\x82\xAC"     ) , L"\x20AC" );
    REQUIRE_EQ(utf8_to_utf16("\xF0\x90\x8D\x88" ) , L"\xD800\xDF48" );
    REQUIRE_EQ(utf8_to_utf16("\xf4\x8f\xbf\xbf" ) , L"\xDBFF\xDFFF" );

    try { 
        utf8_to_utf16("\xf8\x00\x00\x00\x00");
        REQUIRE(false);
    } catch (const std::exception& e) {
        REQUIRE_EQ(std::wstring(e.what(), e.what() + std::strlen(e.what())), L"Unicode conversion failed");
    }

}

void test_main() {
    test_utf8_length();
    test_utf8_conversion();
    test_utf16_conversion();
    test_utf8_to_utf16();
}
