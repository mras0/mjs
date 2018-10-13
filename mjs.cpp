#include <iostream>
#include <string>
#include <stdexcept>
#include <cassert>

#include "value.h"
#include "lexer.h"

int main() {
    mjs::lexer l{L"1 + 2 * 3"};
    for (auto t = mjs::eof_token; (t = l.current_token()).type() != mjs::token_type::eof; l.next_token()) {
        std::wcout << t << '\n';
    }
}