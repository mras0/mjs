enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure --build-config "$<CONFIGURATION>")

macro(mjs_add_normal_test name)
    add_executable(${name} ${name}.cpp ${ARGN})
    target_link_libraries(${name} test_lib)
    add_test(${name} ${name})
    add_dependencies(check ${name})
endmacro()

macro(mjs_add_file_test version filename)
    string(REGEX REPLACE "\\." "_" name "${version}_${filename}")
    add_test(NAME ${name} COMMAND mjs -${version} "${CMAKE_CURRENT_SOURCE_DIR}/js/${filename}")
    set_tests_properties(${name} PROPERTIES ${ARGN})
    add_dependencies(check mjs)
endmacro()

add_library(test_lib STATIC test_spec.cpp test_spec.h test.cpp test.h)
target_link_libraries(test_lib mjs_lib)

mjs_add_normal_test(test_value)
mjs_add_normal_test(test_lexer)
mjs_add_normal_test(test_parser)
mjs_add_normal_test(test_interpreter)
mjs_add_normal_test(test_util)
mjs_add_normal_test(test_object_object)
mjs_add_normal_test(test_function_object)
mjs_add_normal_test(test_array_object)
mjs_add_normal_test(test_string_object)
#boolean
#number
#math
mjs_add_normal_test(test_date_object)
mjs_add_normal_test(test_regexp_object)
#error
mjs_add_normal_test(test_json_object)

mjs_add_file_test(es1 array_literal.js WILL_FAIL TRUE)
mjs_add_file_test(es3 array_literal.js PASS_REGULAR_EXPRESSION "OK")
mjs_add_file_test(es3 main.js PASS_REGULAR_EXPRESSION "OK")
mjs_add_file_test(es5 main.js PASS_REGULAR_EXPRESSION "OK")
mjs_add_file_test(es5 test-compat-es5.js PASS_REGULAR_EXPRESSION "All tests OK")

