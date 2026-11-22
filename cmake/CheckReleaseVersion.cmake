# Check that a release tag is "v" followed by the version in the top-level
# project(), so that a release can't go out with another version in --version
# and the man page. The release workflow runs it before building:
#
#   cmake -DTAG=v0.1.0 -P cmake/CheckReleaseVersion.cmake
#
# It reads the version from CMakeLists.txt without configuring the project, so it
# needs none of the dependencies. Pass -DEXPECT_VERSION=X.Y.Z to only check that
# the version it reads is X.Y.Z (the build's tests do this, see test/CMakeLists.txt).

cmake_minimum_required(VERSION 3.21)

file(READ "${CMAKE_CURRENT_LIST_DIR}/../CMakeLists.txt" top_level)
# The arguments of project(ncpwsafe ...), up to its closing parenthesis
if(NOT top_level MATCHES "\nproject\\(ncpwsafe([^)]*)\\)")
    message(FATAL_ERROR "No project() in CMakeLists.txt")
endif()
if(NOT CMAKE_MATCH_1 MATCHES "VERSION[ \t\r\n]+([0-9]+\\.[0-9]+\\.[0-9]+)[ \t\r\n]")
    message(FATAL_ERROR "project() in CMakeLists.txt has no MAJOR.MINOR.PATCH version")
endif()
set(version "${CMAKE_MATCH_1}")

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
