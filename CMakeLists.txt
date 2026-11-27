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
    VERSION 0.1.0
    DESCRIPTION "An ncurses terminal program that manages encrypted password databases"
    HOMEPAGE_URL "https://github.com/iboisvert/ncpwsafe"
    LANGUAGES C CXX
)
option(BUILD_TESTING "Enable unit tests" ON)
option(NCPWSAFE_DEBUG_SANITIZE "Enable AddressSanitizer and UBSan, requires full recompile" OFF)
option(NCPWSAFE_WARNINGS_AS_ERRORS "Fail the build on compiler warnings in ncpwsafe's own code, as in CI" OFF)
option(NCPWSAFE_CLANG_TIDY "Run clang-tidy on ncpwsafe's own code as it compiles" ON)
option(USE_GLOG "Enable logging" OFF)
option(NCPWSAFE_STATIC_DEPS
    "Link ncurses, ICU, OpenSSL and the GCC runtimes statically, for an executable that runs on other distros (Linux)"
    OFF)
if(NCPWSAFE_STATIC_DEPS AND (WIN32 OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    message(FATAL_ERROR "NCPWSAFE_STATIC_DEPS is for Linux builds; Windows builds link their vcpkg dependencies statically")
endif()

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
if(NOT DEFINED NCPWSAFE_WERROR_FLAGS)
    set(NCPWSAFE_WERROR_FLAGS "$<IF:$<CXX_COMPILER_ID:MSVC>,/WX,-Werror>")
endif()

# After resolve_dependencies(), so that the fetched libraries (libpwsafe, libconfini)
# don't get these: their warnings aren't ours to fix
add_compile_options(${NCPWSAFE_WARNING_FLAGS})
if(NCPWSAFE_WARNINGS_AS_ERRORS)
    add_compile_options(${NCPWSAFE_WERROR_FLAGS})
endif()

# clang-tidy (.clang-tidy) on each of ncpwsafe's source files as it compiles. Also after
# resolve_dependencies(): CMAKE_CXX_CLANG_TIDY only applies to the targets created after it.
# Its findings are warnings and never fail the build, because the existing code isn't clean
if(NCPWSAFE_CLANG_TIDY)
    find_program(NCPWSAFE_CLANG_TIDY_EXE clang-tidy DOC "clang-tidy, for NCPWSAFE_CLANG_TIDY")
    if(NCPWSAFE_CLANG_TIDY_EXE)
        # The compile commands carry GCC's or MSVC's warning flags, some of which clang
        # doesn't know (-Wshadow=local)
        set(CMAKE_CXX_CLANG_TIDY "${NCPWSAFE_CLANG_TIDY_EXE}" --quiet --extra-arg=-Wno-unknown-warning-option)
    else()
        message(WARNING "clang-tidy not found, building without it. Install clang-tidy, "
            "or configure with -DNCPWSAFE_CLANG_TIDY=OFF to hide this warning")
    endif()
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
add_subdirectory(man)
if(BUILD_TESTING)
    resolve_test_dependencies()
    enable_testing()
    add_subdirectory(test)
endif()
