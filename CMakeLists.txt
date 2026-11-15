cmake_minimum_required(VERSION 3.21)
project(ncpwsafe 
    VERSION 0.1 
    DESCRIPTION "An ncurses terminal program that manages encrypted password databases"
    HOMEPAGE_URL "https://github.com/iboisvert/ncpwsafe"
    LANGUAGES C CXX
)
option(BUILD_TESTING "Enable unit tests" ON)
option(NCPWSAFE_DEBUG_SANITIZE "Enable AddressSanitizer and UBSan, requires full recompile" OFF)
option(USE_GLOG "Enable logging" OFF)
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
if(MSVC)
    add_compile_definitions(
        # Silence warnings for POSIX/C-runtime names (strdup, getenv, ...)
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    add_compile_options(
        # Source files (e.g. ru.dat) contain UTF-8 string literals
        /utf-8
        /W3
        # Unknown pragma, the sources use `#pragma GCC diagnostic`
        /wd4068
        /permissive-
        /Zc:__cplusplus
        # MSVC supports AddressSanitizer only, not UBSan
        "$<$<AND:$<CONFIG:DEBUG>,$<BOOL:${NCPWSAFE_DEBUG_SANITIZE}>>:/fsanitize=address>"
    )
else()
    add_compile_options(
        -fPIC -Wall -Wextra
        "$<$<CONFIG:DEBUG>:-O0>"
        "$<$<AND:$<CONFIG:DEBUG>,$<BOOL:${NCPWSAFE_DEBUG_SANITIZE}>>:-fsanitize=address,undefined>"
    )
    add_link_options(
        -fPIC -Wall -Wextra
        "$<$<AND:$<CONFIG:DEBUG>,$<BOOL:${NCPWSAFE_DEBUG_SANITIZE}>>:-fsanitize=address,undefined>"
    )
endif()

add_subdirectory(src)
if(BUILD_TESTING)
    resolve_test_dependencies()
    enable_testing()
    add_subdirectory(test)
endif()

include(CPack)
