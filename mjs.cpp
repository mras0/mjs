#include <iostream>
#include <string>
#include <stdexcept>
#include <cassert>

#include "value.h"

int main() {
    std::wcout << mjs::value{mjs::string("Test!")} << '\n';
}