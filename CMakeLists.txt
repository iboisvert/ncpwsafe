cmake_minimum_required (VERSION 3.22)
project (ncpwsafe 
    VERSION 0.1.0 
    DESCRIPTION "An ncurses terminal program that manages encrypted password databases"
    HOMEPAGE_URL "https://github.com/iboisvert/ncpwsafe"
    LANGUAGES C CXX
)
set(NCPWSAFE_APPNAME "${CMAKE_PROJECT_NAME}")
set(NCPWSAFE_VERSION "${CMAKE_PROJECT_VERSION}")

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

find_package (Ncurses REQUIRED COMPONENTS form menu panel)
include (CheckLibraryExists)
CHECK_LIBRARY_EXISTS(pwsafe pws_db_open "/usr/lib" HAVE_LIBPWSAFE)
if (NOT ${HAVE_LIBPWSAFE})
    message (FATAL_ERROR "Could not find libpwsafe.a")
endif ()
include (CheckIncludeFile)
CHECK_INCLUDE_FILE("sys/prctl.h" HAVE_SYS_PRCTL_H)
CHECK_INCLUDE_FILE("sys/random.h" HAVE_SYS_RANDOM_H)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

enable_testing ()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

# TODO IMB 2023-01-22 Move to preset
add_compile_options(
    -fPIC -Wall -Wextra
    "$<$<CONFIG:DEBUG>:-ggdb;-O0;-fsanitize=address,undefined>"
)
add_compile_definitions(
    "$<$<CONFIG:RELEASE>:NDEBUG>"
)

add_subdirectory(src)