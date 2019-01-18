cmake_minimum_required(VERSION 3.7)
project(mjs CXX)

# Just hack the settings for now

# Note: To use clang-cl on windows: set CXX=clang-cl && cmake -G "NMake Makefiles" <path>

if (MSVC)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build configurations" FORCE)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /permissive- /Zi /Zo /sdl /W4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4324") # C4324: Structure was padded due to alignment specifier
    if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /JMC")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall -Wextra -g")
    if (NOT WIN32) # Too bad :(
        set(use_asan FALSE CACHE BOOL "Use address sanitizer")
        set(use_usan FALSE CACHE BOOL "Use undefined behavior sanitizer")
        set(coverage_analysis FALSE CACHE BOOL "Enable coverage analysis")
        if (use_asan)
            message("Using address sanitizer")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
        endif()
        if (use_usan)
            message("Using undefined behavior sanitizer")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
        endif()
        if (coverage_analysis)
            message("Coverage analysis enabled")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
            add_custom_target(coverage
                COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR} --output-file coverage_all.info --base-directory ${CMAKE_SOURCE_DIR}
		COMMAND lcov --remove coverage_all.info '/usr/include/*' --output-file coverage.info
                COMMAND genhtml coverage.info --output-directory ${CMAKE_BINARY_DIR}/coverage
                )

        endif()
    endif()
endif()


if (WIN32)
    add_definitions("-DWIN32 -D_WIN32 -DUNICODE -D_UNICODE")
endif()

set(enable_gc_stress_test FALSE CACHE BOOL "Stress test garbage collector by running after every evaluated statement")
if (enable_gc_stress_test)
    add_definitions("-DMJS_GC_STRESS_TEST")
endif()

add_subdirectory(src)
add_subdirectory(test)
