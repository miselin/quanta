project(third_party C)

add_library(clog STATIC clog.c)
target_include_directories(clog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
