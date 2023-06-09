cmake_minimum_required(VERSION 3.22)
project(ncpwsafe 
    VERSION 0.1.0 
    DESCRIPTION "An ncurses terminal program that manages encrypted password databases"
    HOMEPAGE_URL "https://github.com/iboisvert/ncpwsafe"
    LANGUAGES C CXX
)
option(BUILD_TESTING "Enable unit tests" ON)
set(NCPWSAFE_CONFIG_FILE "$HOME/.config/ncpwsafe/ncpwsafe.ini" CACHE FILEPATH "Config file default pathname")

set(NCPWSAFE_APPNAME "${CMAKE_PROJECT_NAME}")
set(NCPWSAFE_VERSION "${CMAKE_PROJECT_VERSION}")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(ResolveDependencies)

CHECK_INCLUDE_FILE("sys/prctl.h" HAVE_SYS_PRCTL_H)
CHECK_INCLUDE_FILE("sys/random.h" HAVE_SYS_RANDOM_H)

resolve_dependencies()

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# TODO IMB 2023-01-22 Move to preset
add_compile_definitions(
    "$<$<CONFIG:RELEASE>:NDEBUG>"
)
add_compile_options(
    -fPIC -Wall -Wextra
    -Wall -Wextra
    "$<$<CONFIG:DEBUG>:-g;-O0;-fsanitize=address,undefined>"
)
add_link_options(
    -fPIC -Wall -Wextra
    -Wall -Wextra
    "$<$<CONFIG:DEBUG>:-g;-O0;-fsanitize=address,undefined>"
)

add_subdirectory(src)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
