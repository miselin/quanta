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
target_include_directories(quanta PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_PROJECT_SOURCE_DIR}/third_party)

add_executable(quanta_bin
    main.c
)
target_link_libraries(quanta_bin quanta)
set_property(TARGET quanta_bin PROPERTY OUTPUT_NAME quanta)
