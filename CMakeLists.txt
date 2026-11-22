cmake_minimum_required(VERSION 3.21)

# An in-source build leaves CMakeCache.txt, Makefiles etc. all over the source
# tree, and a stale CMakeCache.txt there interferes with builds in build/
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported, use a preset or -B build. "
        "Remove CMakeCache.txt and CMakeFiles/ from the source directory.")
endif()
# vcpkg installs the manifest's dependencies at project(), so its features are chosen here
if(USE_GLOG)
    list(APPEND VCPKG_MANIFEST_FEATURES "glog")
endif()
project(ncpwsafe 
    VERSION 0.1 
    DESCRIPTION "An ncurses terminal program that manages encrypted password databases"
    HOMEPAGE_URL "https://github.com/iboisvert/ncpwsafe"
    LANGUAGES C CXX
)
option(BUILD_TESTING "Enable unit tests" ON)
option(NCPWSAFE_DEBUG_SANITIZE "Enable AddressSanitizer and UBSan, requires full recompile" OFF)
option(USE_GLOG "Enable logging" OFF)

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
CHECK_INCLUDE_FILE("langinfo.h" HAVE_LANGINFO_H)

resolve_dependencies()

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Compiler warnings and debug flags live in cmake/toolchain-*.cmake,
# selected via CMAKE_TOOLCHAIN_FILE in CMakePresets.json
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT DEFINED NCPWSAFE_SANITIZE_FLAGS)
    set(NCPWSAFE_SANITIZE_FLAGS "-fsanitize=address,undefined")
endif()

add_compile_definitions(
    "$<$<CONFIG:RELEASE>:NDEBUG>"
)
if(MSVC)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()
add_compile_options(
    "$<$<AND:$<CONFIG:DEBUG>,$<BOOL:${NCPWSAFE_DEBUG_SANITIZE}>>:${NCPWSAFE_SANITIZE_FLAGS}>"
)
if(NOT MSVC)
    # MSVC adds the ASan runtime from the /fsanitize compile option, link.exe doesn't take it
    add_link_options(
        "$<$<AND:$<CONFIG:DEBUG>,$<BOOL:${NCPWSAFE_DEBUG_SANITIZE}>>:${NCPWSAFE_SANITIZE_FLAGS}>"
    )
elseif(NCPWSAFE_DEBUG_SANITIZE)
    # CMake's default MSVC Debug flags have /RTC1, which cl rejects with /fsanitize=address (D8016),
    # and link.exe ignores /INCREMENTAL with ASan. Only affects this directory and below,
    # the dependencies are not built with ASan
    foreach(lang C CXX)
        string(REGEX REPLACE "(^| )[/-]RTC[1csu]*" "" CMAKE_${lang}_FLAGS_DEBUG "${CMAKE_${lang}_FLAGS_DEBUG}")
    endforeach()
    string(REGEX REPLACE "(^| )[/-]INCREMENTAL(:[A-Za-z]+)?" "" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /INCREMENTAL:NO")
endif()

add_subdirectory(src)
if(BUILD_TESTING)
    resolve_test_dependencies()
    enable_testing()
    add_subdirectory(test)
endif()

include(CPack)
