# Resolve curses, ICU, and glog on Windows, where pkg-config is usually not available.
# Dependencies are expected to be provided by vcpkg, see the x64-windows-mingw presets in CMakePresets.json
macro(_resolve_windows_dependencies)
    if(MSVC)
        message(FATAL_ERROR
            "MSVC is not supported: the ncurses form and menu libraries required by ncpwsafe "
            "do not build with MSVC. Use a MinGW-w64 toolchain, see README.md")
    endif()

    if(VCPKG_TARGET_TRIPLET MATCHES "-static$")
        set(_ncpwsafe_static_deps_default ON)
    else()
        set(_ncpwsafe_static_deps_default OFF)
    endif()
    option(NCPWSAFE_STATIC_DEPENDENCIES "Dependencies are static libraries" ${_ncpwsafe_static_deps_default})
    if(NCPWSAFE_STATIC_DEPENDENCIES)
        # Otherwise the ncurses and ICU headers declare their API __declspec(dllimport)
        add_compile_definitions(NCURSES_STATIC U_STATIC_IMPLEMENTATION)
    endif()

    # Wide-character ncurses. Headers are included without a directory prefix,
    # e.g. <form.h>, so the include directory must be the one containing them
    find_path(NCURSES_INCLUDE_DIRS NAMES form.h PATH_SUFFIXES ncursesw REQUIRED)
    set(NCURSES_LINK_LIBRARIES)
    # Static libraries must be linked in dependency order
    foreach(_ncpwsafe_lib IN ITEMS panel menu form ncurses)
        string(TOUPPER "${_ncpwsafe_lib}" _ncpwsafe_LIB)
        find_library(NCURSES_${_ncpwsafe_LIB}_LIBRARY NAMES ${_ncpwsafe_lib}w ${_ncpwsafe_lib} REQUIRED)
        list(APPEND NCURSES_LINK_LIBRARIES "${NCURSES_${_ncpwsafe_LIB}_LIBRARY}")
    endforeach()

    find_package(ICU REQUIRED COMPONENTS uc data)
    set(ICU_LINK_LIBRARIES ICU::uc ICU::data)

    if(USE_GLOG)
        find_package(glog CONFIG REQUIRED)
        set(GLOG_LINK_LIBRARIES glog::glog)
        set(GLOG_FOUND TRUE)
    endif()
endmacro()

macro(resolve_dependencies)
    include(FetchContent)
    if(NOT WIN32)
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

    # libpwsafe enables clang-tidy when CLANG_TIDY_EXE is defined,
    # which it always is, even if clang-tidy wasn't found
    if(TARGET objlib AND NOT CLANG_TIDY_EXE)
        set_target_properties(objlib PROPERTIES C_CLANG_TIDY "")
    endif()

    if(WIN32)
        _resolve_windows_dependencies()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
        find_package(Curses REQUIRED COMPONENTS form menu panel)
    else()
        pkg_check_modules(NCURSES REQUIRED ncurses form menu panel)
    endif()
    if(NOT WIN32)
        pkg_check_modules(ICU REQUIRED icu-uc)
        if(USE_GLOG)
            pkg_check_modules(GLOG REQUIRED libglog)
        endif()
        pkg_check_modules(NETTLE REQUIRED nettle)
    endif()
    add_compile_definitions($<$<BOOL:${GLOG_FOUND}>:HAVE_GLOG>)
endmacro()

macro(resolve_test_dependencies)
    find_package(GTest REQUIRED)
endmacro()
