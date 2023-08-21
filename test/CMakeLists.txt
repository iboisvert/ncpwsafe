resolve_test_dependencies()

add_executable(unittests
    AccountDb-tests.cpp
    PWSafeApp-tests.cpp
    Utils-tests.cpp
)
target_link_libraries(unittests 
    libncpwsafe
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unittests WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../test")

# add_test(NAME "unit tests" COMMAND unittests WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test")