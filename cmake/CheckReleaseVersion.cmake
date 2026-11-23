# Check that a release tag is "v" followed by the version in the top-level
# project(), so that a release can't go out with another version in --version
# and the man page. The release workflow runs it before building:
#
#   cmake -DTAG=v0.1.0 -P cmake/CheckReleaseVersion.cmake
#
# It reads the version from CMakeLists.txt without configuring the project, so it
# needs none of the dependencies. Pass -DEXPECT_VERSION=X.Y.Z to only check that
# the version it reads is X.Y.Z (the build's tests do this, see test/CMakeLists.txt),
# or -DPRINT_VERSION=ON to print the version on stdout and nothing else.

cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_LIST_DIR}/ReadVersion.cmake")
read_project_version(version)

if(PRINT_VERSION)
    execute_process(COMMAND ${CMAKE_COMMAND} -E echo "${version}")
    return()
endif()

if(DEFINED EXPECT_VERSION)
    if(NOT version STREQUAL EXPECT_VERSION)
        message(FATAL_ERROR "Read version ${version} from CMakeLists.txt, but project() says ${EXPECT_VERSION}")
    endif()
    message(STATUS "Version ${version}")
    return()
endif()

if(NOT DEFINED TAG OR TAG STREQUAL "")
    message(FATAL_ERROR "Usage: cmake -DTAG=vX.Y.Z -P ${CMAKE_CURRENT_LIST_FILE}")
endif()
if(NOT TAG STREQUAL "v${version}")
    message(FATAL_ERROR "Tag ${TAG} doesn't match the version in CMakeLists.txt, ${version}: "
        "the tag must be v${version}, or change the version in project()")
endif()
message(STATUS "Tag ${TAG} matches the version ${version}")
