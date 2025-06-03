project(tests C CXX)

include(GoogleTest)

add_executable(quanta_tests
    test_main.cc
    parse_failures_test.cc
    atoms_test.cc
)
target_link_libraries(quanta_tests quanta GTest::gtest)
gtest_discover_tests(quanta_tests)
