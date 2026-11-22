add_executable(unittests
    AccountDb-tests.cpp
    AccountList-tests.cpp
    AccountRecord-tests.cpp
    PWSafeApp-tests.cpp
    Policy-tests.cpp
    Theme-tests.cpp
    Utils-tests.cpp
    Win32-tests.cpp
)
target_link_libraries(unittests 
    libncpwsafe
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unittests WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../test")

# add_test(NAME "unit tests" COMMAND unittests WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test")
add_subdirectory(e2e)
