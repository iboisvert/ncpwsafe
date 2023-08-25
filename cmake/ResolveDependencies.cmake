macro(resolve_dependencies)
    include(FetchContent)
    find_package(PkgConfig)

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

    pkg_check_modules(NCURSES REQUIRED ncursesw form menu panel)
    pkg_check_modules(ICU REQUIRED icu-uc)
    pkg_check_modules(GLOG libglog)
    add_compile_definitions($<$<BOOL:${GLOG_FOUND}>:HAVE_GLOG>)
endmacro()

macro(resolve_test_dependencies)
    find_package(GTest REQUIRED)
endmacro()