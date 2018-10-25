add_library(mjs_lib STATIC
    mjs/string.cpp
    mjs/string.h
    mjs/value.cpp
    mjs/value.h
    mjs/lexer.cpp
    mjs/lexer.h
    mjs/parser.cpp
    mjs/parser.h
    mjs/global_object.cpp
    mjs/global_object.h
    mjs/interpreter.cpp
    mjs/interpreter.h
    mjs/printer.cpp
    mjs/printer.h
    )
target_include_directories(mjs_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(mjs mjs.cpp)
target_link_libraries(mjs mjs_lib)
