cmake_minimum_required (VERSION 3.22)
project (ncpwsafe 
    VERSION 0.1.0 
    DESCRIPTION "An ncurses terminal program that manages encrypted password databases"
    HOMEPAGE_URL "https://github.com/iboisvert/ncpwsafe"
)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

find_package (Ncurses REQUIRED COMPONENTS form menu panel)
include (CheckLibraryExists)
CHECK_LIBRARY_EXISTS(pwsafe pws_db_open "/usr/lib" HAVE_LIBPWSAFE)
if (NOT ${HAVE_LIBPWSAFE})
    message (FATAL_ERROR "Could not find libpwsafe.a")
endif ()

enable_testing ()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra")
# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -O0 -fsanitize=address,undefined")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -O0 -fsanitize=address,undefined")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -O0")

add_subdirectory(src)