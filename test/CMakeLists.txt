enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure --build-config "$<CONFIGURATION>")

macro(mjs_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} mjs_lib)
    target_include_directories(${name} PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/catch2
        )
    add_test(${name} ${name})
    add_dependencies(check ${name})
endmacro()

mjs_add_test(value_test)
mjs_add_test(interpreter_test)
