macro(resolve_dependencies)
    include(FetchContent)
    if(NOT MSVC)
        find_package(PkgConfig REQUIRED)
    endif()

    FetchContent_Declare(
        libpwsafe
        GIT_REPOSITORY https://github.com/iboisvert/libpwsafe.git
        GIT_TAG HEAD
        # URL https://github.com/iboisvert/libpwsafe/archive/refs/heads/main.zip
        # SOURCE_DIR /workspaces/libpwsafe
    )
    set(LIBPWSAFE_BUILD_DOCS OFF CACHE INTERNAL "Disable libpwsafe docs")
    set(LIBPWSAFE_BUILD_TESTS OFF CACHE INTERNAL "Disable libpwsafe tests")
    set(LIBPWSAFE_BUILD_SHARED OFF CACHE INTERNAL "Disable libpwsafe shared lib")
    set(LIBPWSAFE_DEBUG_SANITIZE ${NCPWSAFE_DEBUG_SANITIZE})

    FetchContent_Declare(
        libconfini
        GIT_REPOSITORY https://github.com/iboisvert/libconfini.git
        GIT_TAG HEAD
    )

    FetchContent_MakeAvailable(libpwsafe libconfini)

    # libpwsafe enables clang-tidy with `if(DEFINED CLANG_TIDY_EXE)`, which is
    # also true when clang-tidy was not found (CLANG_TIDY_EXE-NOTFOUND),
    # and the build then fails trying to run it. This is always the case on Windows.
    if(TARGET objlib AND NOT CLANG_TIDY_EXE)
        set_property(TARGET objlib PROPERTY C_CLANG_TIDY "")
    endif()

    if(MSVC)
        resolve_msvc_dependencies()
    else()
        # The wide character libraries are needed to show non-ASCII (UTF-8) text:
        # the others take each byte of a multibyte character as a character of its own
        if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
            set(CURSES_NEED_WIDE TRUE)
            find_package(Curses REQUIRED COMPONENTS form menu panel)
        else()
            pkg_check_modules(NCURSES REQUIRED ncursesw formw menuw panelw)
        endif()
        pkg_check_modules(ICU REQUIRED icu-uc)
        if(USE_GLOG)
            pkg_check_modules(GLOG REQUIRED libglog)
        endif()
    endif()
    add_compile_definitions($<$<BOOL:${GLOG_FOUND}>:HAVE_GLOG>)
endmacro()

# Find a library built by vcpkg in both release and debug configurations,
# and set `<var>` to a list suitable for target_link_libraries()
function(find_release_debug_library var)
    include(SelectLibraryConfigurations)
    set(prefix "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
    find_library(${var}_LIBRARY_RELEASE NAMES ${ARGN}
        PATHS "${prefix}/lib" NO_DEFAULT_PATH)
    find_library(${var}_LIBRARY_DEBUG NAMES ${ARGN}
        PATHS "${prefix}/debug/lib" NO_DEFAULT_PATH)
    select_library_configurations(${var})
    if(NOT ${var}_LIBRARY)
        message(FATAL_ERROR "Library not found: ${ARGN}")
    endif()
    set(${var}_LIBRARY "${${var}_LIBRARY}" PARENT_SCOPE)
endfunction()

# Dependencies for native Windows builds with MSVC and the Windows SDK.
# These are expected to be provided by vcpkg (see vcpkg.json),
# pkg-config is not used.
macro(resolve_msvc_dependencies)
    if(NOT DEFINED VCPKG_INSTALLED_DIR OR NOT DEFINED VCPKG_TARGET_TRIPLET)
        message(FATAL_ERROR
            "MSVC builds require vcpkg, set CMAKE_TOOLCHAIN_FILE to "
            "<vcpkg-root>/scripts/buildsystems/vcpkg.cmake or use a windows CMake preset")
    endif()

    # ncurses, wide character build with the Windows console driver,
    # from the overlay port in ports/ncurses
    find_path(NCURSES_INCLUDE_DIR NAMES ncursesw/ncurses.h REQUIRED)
    set(NCURSES_INCLUDE_DIRS "${NCURSES_INCLUDE_DIR}/ncursesw")
    set(NCURSES_LINK_LIBRARIES)
    # Order matters for static linking: form, menu, and panel depend on ncurses
    foreach(lib IN ITEMS formw menuw panelw ncursesw)
        find_release_debug_library(NCURSES_${lib} ${lib})
        list(APPEND NCURSES_LINK_LIBRARIES ${NCURSES_${lib}_LIBRARY})
    endforeach()

    # getopt_long()
    find_package(getopt CONFIG REQUIRED)
    if(TARGET getopt::getopt_shared)
        set(GETOPT_LINK_LIBRARIES getopt::getopt_shared)
    else()
        set(GETOPT_LINK_LIBRARIES getopt::getopt_static)
    endif()

    find_package(ICU REQUIRED COMPONENTS uc data)
    set(ICU_LINK_LIBRARIES ICU::uc ICU::data)

    if(USE_GLOG)
        find_package(glog CONFIG REQUIRED)
        set(GLOG_FOUND TRUE)
        set(GLOG_LINK_LIBRARIES glog::glog)
    endif()
endmacro()

macro(resolve_test_dependencies)
    find_package(GTest REQUIRED)
endmacro()
