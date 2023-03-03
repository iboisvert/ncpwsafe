cmake_minimum_required(VERSION 3.22)
project(ncpwsafe 
    VERSION 0.1.0 
    DESCRIPTION "An ncurses terminal program that manages encrypted password databases"
    HOMEPAGE_URL "https://github.com/iboisvert/ncpwsafe"
    LANGUAGES C CXX
)

set(NCPWSAFE_APPNAME "${CMAKE_PROJECT_NAME}")
set(NCPWSAFE_VERSION "${CMAKE_PROJECT_VERSION}")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

find_package(Ncurses REQUIRED COMPONENTS form menu panel)
find_package(ICU REQUIRED COMPONENTS uc)
CHECK_INCLUDE_FILE_CXX("cpptoml.h" HAVE_CPPTOML_H)
if(NOT HAVE_CPPTOML_H)
    message(SEND_ERROR "Could not find cpptoml.h")
endif()
CHECK_INCLUDE_FILE("sys/prctl.h" HAVE_SYS_PRCTL_H)
CHECK_INCLUDE_FILE("sys/random.h" HAVE_SYS_RANDOM_H)

set(PWSAFE_BUILD_DOCS OFF CACHE INTERNAL "Disable libpwsafe docs")
set(PWSAFE_BUILD_TESTS OFF CACHE INTERNAL "Disable libpwsafe tests")
FetchContent_Declare(
    libpwsafe
    # GIT_REPOSITORY https://github.com/iboisvert/libpwsafe.git
    # GIT_TAG HEAD
    URL /workspaces/ncpwsafe/libpwsafe-0.1.0-Source.tar.gz
)
FetchContent_MakeAvailable(libpwsafe)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

enable_testing()

# TODO IMB 2023-01-22 Move to preset
add_compile_definitions(
    "$<$<CONFIG:RELEASE>:NDEBUG>"
)
add_compile_options(
    -fPIC -Wall -Wextra
    "$<$<CONFIG:DEBUG>:-ggdb;-O0;-fsanitize=address,undefined>"
)
add_link_options(
    -fPIC -Wall -Wextra
    "$<$<CONFIG:DEBUG>:-ggdb;-O0;-fsanitize=address,undefined>"
)

add_subdirectory(src)