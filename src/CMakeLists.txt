add_library(mjs_lib STATIC
    mjs/array_object.cpp
    mjs/array_object.h
    mjs/gc_function.h
    mjs/gc_heap.cpp
    mjs/gc_heap.h
    mjs/gc_vector.h
    mjs/global_object.cpp
    mjs/global_object.h
    mjs/interpreter.cpp
    mjs/interpreter.h
    mjs/lexer.cpp
    mjs/lexer.h
    mjs/native_object.cpp
    mjs/native_object.h
    mjs/object.cpp
    mjs/object.h
    mjs/parser.cpp
    mjs/parser.h
    mjs/printer.cpp
    mjs/printer.h
    mjs/property_attribute.h
    mjs/string.cpp
    mjs/string.h
    mjs/value.cpp
    mjs/value.h
    mjs/value_representation.cpp
    mjs/value_representation.h
    )
target_include_directories(mjs_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(mjs mjs.cpp)
target_link_libraries(mjs mjs_lib)
