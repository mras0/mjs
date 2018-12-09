enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure --build-config "$<CONFIGURATION>")

macro(mjs_add_test name)
    add_executable(${name} ${name}.cpp ${ARGN})
    target_link_libraries(${name} test_lib)
    add_test(${name} ${name})
    add_dependencies(check ${name})
endmacro()

add_library(test_lib STATIC test_spec.cpp test_spec.h test.cpp test.h)
target_link_libraries(test_lib mjs_lib)

mjs_add_test(value_test)
mjs_add_test(lexer_test)
mjs_add_test(parser_test)
mjs_add_test(interpreter_test)
