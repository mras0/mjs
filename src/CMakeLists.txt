add_library(mjs_core STATIC
    mjs/char_conversions.cpp
    mjs/char_conversions.h
    mjs/platform.cpp
    mjs/platform.h
)

add_library(mjs_gc STATIC
    mjs/gc_heap.cpp
    mjs/gc_heap.h
)

add_library(mjs_parser STATIC
    mjs/lexer.cpp
    mjs/lexer.h
    mjs/parser.cpp
    mjs/parser.h
    mjs/regexp.cpp
    mjs/regexp.h
    mjs/version.cpp
    mjs/version.h
    mjs/unicode_data.h
)
target_link_libraries(mjs_parser mjs_core)

add_library(mjs_global STATIC
    mjs/array_object.cpp
    mjs/array_object.h
    mjs/boolean_object.cpp
    mjs/boolean_object.h
    mjs/date_object.cpp
    mjs/date_object.h
    mjs/error_object.cpp
    mjs/error_object.h
    mjs/function_object.cpp
    mjs/function_object.h
    mjs/gc_function.h
    mjs/gc_vector.h
    mjs/global_object.cpp
    mjs/global_object.h
    mjs/native_object.cpp
    mjs/native_object.h
    mjs/number_object.cpp
    mjs/number_object.h
    mjs/number_to_string.cpp
    mjs/number_to_string.h
    mjs/object.cpp
    mjs/object.h
    mjs/object_object.cpp
    mjs/object_object.h
    mjs/property_attribute.cpp
    mjs/property_attribute.h
    mjs/regexp_object.cpp
    mjs/regexp_object.h
    mjs/string.cpp
    mjs/string.h
    mjs/string_object.cpp
    mjs/string_object.h
    mjs/value.cpp
    mjs/value.h
    mjs/value_representation.cpp
    mjs/value_representation.h
)
target_link_libraries(mjs_global mjs_parser)

add_library(mjs_lib STATIC
    mjs/interpreter.cpp
    mjs/interpreter.h
    mjs/printer.cpp
    mjs/printer.h
)
target_include_directories(mjs_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mjs_lib mjs_gc mjs_global)

add_executable(mjs mjs.cpp)
target_link_libraries(mjs mjs_lib)
