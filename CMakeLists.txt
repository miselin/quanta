cmake_minimum_required(VERSION 3.10)
project(quanta C)

include(FetchContent)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -Werror -Wno-unused-function")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3 -ggdb -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O2 -g")

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_subdirectory(third_party)

add_library(quanta STATIC
    atom.c
    read.c
    intern.c
    print.c
    eval.c
    env.c
    primitive.c
    special.c
    source.c
    gc.c
    lex.c
    log.c
)
target_link_libraries(quanta PUBLIC PkgConfig::deps clog)
target_include_directories(quanta PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(quanta_bin
    main.c
)
target_link_libraries(quanta_bin quanta)
set_property(TARGET quanta_bin PROPERTY OUTPUT_NAME quanta)

enable_testing()

add_subdirectory(tests)
