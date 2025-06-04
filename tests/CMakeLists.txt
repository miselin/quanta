project(tests C CXX)

include(GoogleTest)

add_executable(quanta_tests
    test_main.cc
    parse_failures_test.cc
    atoms_test.cc
    comment_test.cc
    arithmetic_test.cc
    cond_test.cc
    define_test.cc
    let_test.cc
    eval_test.cc
)
target_link_libraries(quanta_tests quanta GTest::gtest)
gtest_discover_tests(quanta_tests)
