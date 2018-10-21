cmake_minimum_required(VERSION 3.11)
project(mjs)

if (MSVC)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build configurations" FORCE)

    add_definitions("/W4")
    #add_definitions("/wd4267") # C4267: 'argument': conversion from 'X' to 'Y', possible loss of data
    #add_definitions("/wd4244") # C4244: 'initializing': conversion from 'X' to 'Y', possible loss of data
    #add_definitions("/wd4319") # C4319: '~': zero extending 'X' to 'Y' of greater size
    #add_definitions("/wd4193") # C4193: #pragma warning(pop): no matching '#pragma warning(push)'

    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /permissive- /Zi /Zo /sdl /JMC")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall -Wextra -Werror")
endif()

if (WIN32)
    add_definitions("-DWIN32 -D_WIN32 -DUNICODE -D_UNICODE")
endif()

add_library(mjs_lib STATIC string.cpp string.h value.cpp value.h lexer.cpp lexer.h parser.cpp parser.h global_object.cpp global_object.h interpreter.cpp interpreter.h printer.cpp printer.h)

add_executable(${PROJECT_NAME} mjs.cpp)
target_link_libraries(${PROJECT_NAME} mjs_lib)

enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure --build-config "$<CONFIGURATION>")

macro(mjs_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} mjs_lib)
    add_test(${name} ${name})
endmacro()

mjs_add_test(value_test)
mjs_add_test(interpreter_test)
