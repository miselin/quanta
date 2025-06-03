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

add_executable(quanta
    main.c
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
)
target_link_libraries(quanta PkgConfig::deps clog)
