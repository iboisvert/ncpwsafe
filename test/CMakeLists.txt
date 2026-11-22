add_executable(unittests
    AccountDb-tests.cpp
    AccountList-tests.cpp
    AccountRecord-tests.cpp
    AutoSignOff-tests.cpp
    PWSafeApp-tests.cpp
    Policy-tests.cpp
    TextArea-tests.cpp
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

# The release workflow's check of the tag against the version (cmake/CheckReleaseVersion.cmake)
set(CHECK_RELEASE_VERSION "${PROJECT_SOURCE_DIR}/cmake/CheckReleaseVersion.cmake")
add_test(NAME release.version_read
    COMMAND ${CMAKE_COMMAND} -DEXPECT_VERSION=${PROJECT_VERSION} -P ${CHECK_RELEASE_VERSION})
add_test(NAME release.tag_matches
    COMMAND ${CMAKE_COMMAND} -DTAG=v${PROJECT_VERSION} -P ${CHECK_RELEASE_VERSION})
add_test(NAME release.tag_mismatch
    COMMAND ${CMAKE_COMMAND} -DTAG=v${PROJECT_VERSION}.1 -P ${CHECK_RELEASE_VERSION})
add_test(NAME release.tag_without_v
    COMMAND ${CMAKE_COMMAND} -DTAG=${PROJECT_VERSION} -P ${CHECK_RELEASE_VERSION})
# The output is checked rather than the exit status, so that they don't pass when the script fails another way
set_tests_properties(release.tag_mismatch release.tag_without_v PROPERTIES
    PASS_REGULAR_EXPRESSION "doesn't match the version in CMakeLists.txt")
set_tests_properties(release.version_read release.tag_matches release.tag_mismatch release.tag_without_v
    PROPERTIES LABELS release)

# The release packaging script (cmake/MakePackage.cmake), on the build itself
if(WIN32)
    set(PACKAGE_PLATFORM windows-x64)
else()
    set(PACKAGE_PLATFORM linux-x86_64)
endif()
add_test(NAME release.package
    COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${CMAKE_BINARY_DIR} -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/release-package
        -DPLATFORM=${PACKAGE_PLATFORM} -P ${CMAKE_CURRENT_SOURCE_DIR}/release/CheckPackage.cmake)
set_tests_properties(release.package PROPERTIES LABELS release)
