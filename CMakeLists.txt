cmake_minimum_required(VERSION 3.10)
project(quanta C)

include(CTest)
include(FetchContent)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(COVERAGE "enable code coverage" OFF)

set(ASAN OFF CACHE BOOL "Enable ASAN for memory debugging")

if (ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif ()

add_subdirectory(third_party)

add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-function -Wshadow)

if (COVERAGE)
    add_compile_options(-coverage)
    add_link_options(-coverage)
endif ()

add_subdirectory(src)

enable_testing()

add_subdirectory(tests)
