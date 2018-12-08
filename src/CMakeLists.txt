add_library(mjs_lib STATIC
    mjs/string.cpp
    mjs/string.h
    mjs/value.cpp
    mjs/value.h
    mjs/object.cpp
    mjs/object.h
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
    mjs/gc_heap.cpp
    mjs/gc_heap.h
    mjs/gc_function.h
    mjs/value_representation.cpp
    mjs/value_representation.h
    mjs/property_attribute.h
    mjs/gc_vector.h
    )
target_include_directories(mjs_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(mjs mjs.cpp)
target_link_libraries(mjs mjs_lib)
