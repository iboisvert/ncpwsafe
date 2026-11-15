# Overlay of the upstream vcpkg ncurses port that adds support for
# native Windows builds with MSVC (cl.exe) and the Windows SDK.
#
# Upstream only supports `!windows | mingw`. ncurses can be built with MSVC
# using its Windows console terminal driver (win32con), which talks to the
# console through the Win32 API instead of a terminfo database. The changes
# from upstream are confined to the VCPKG_TARGET_IS_WINDOWS branches below.

set(NCURSES_TARGET_IS_MSVC OFF)
if(VCPKG_TARGET_IS_WINDOWS AND NOT VCPKG_TARGET_IS_MINGW)
    set(NCURSES_TARGET_IS_MSVC ON)
    # ncurses' libtool-based DLL build is not supported with MSVC
    vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
endif()

vcpkg_download_distfile(
    ARCHIVE_PATH
    URLS
        "https://invisible-mirror.net/archives/ncurses/ncurses-${VERSION}.tar.gz"
        "ftp://ftp.invisible-island.net/ncurses/ncurses-${VERSION}.tar.gz"
        "https://ftp.gnu.org/gnu/ncurses/ncurses-${VERSION}.tar.gz"
    FILENAME "ncurses-${VERSION}.tgz"
    SHA512 fc5a13409d2a530a1325776dcce3a99127ddc2c03999cfeb0065d0eee2d68456274fb1c7b3cc99c1937bc657d0e7fca97016e147f93c7821b5a4a6837db821e8
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE_PATH}"
)

vcpkg_list(SET OPTIONS)

if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    list(APPEND OPTIONS
        --with-cxx-shared
        --with-shared    # "lib model"
        --without-normal # "lib model"
    )
endif()

if(NOT VCPKG_TARGET_IS_WINDOWS)
    list(APPEND OPTIONS
        --enable-mixed-case
    )
endif()

if(VCPKG_TARGET_IS_WINDOWS)
    list(APPEND OPTIONS
        --disable-home-terminfo
        --enable-term-driver
        --disable-termcap
    )
endif()

if(NCURSES_TARGET_IS_MSVC)
    list(APPEND OPTIONS
        --enable-sp-funcs
        --without-cxx-binding
        --without-dlsym
        # Extended (256) colors require tsearch() from <search.h>, which the
        # MSVC C runtime does not provide. The Windows console driver only
        # supports the 16 console colors, so nothing is lost.
        --disable-ext-colors
        # vcpkg configures MSVC builds with a mingw32 host triplet, so ncurses
        # probes `ar` flags (none of which `lib.exe` accepts) and falls back to
        # `unknown`, which the ar-lib wrapper then takes as the archive name.
        # Preset the cached result so only vcpkg's own ARFLAGS are used.
        cf_cv_ar_flags=
    )
endif()

if("check-size" IN_LIST FEATURES)
    list(APPEND OPTIONS
        --enable-check-size
    )
endif()

vcpkg_cmake_get_vars(cmake_vars_file)
include("${cmake_vars_file}")

if(NOT NCURSES_TARGET_IS_MSVC)
    # There are compilation errors on gcc 15. adding `-std=c17` to CFLAGS for workaround.
    # ref: https://gitlab.archlinux.org/archlinux/packaging/packages/ncurses/-/issues/3
    list(APPEND OPTIONS "CFLAGS=\$CFLAGS -std=c17")
endif()

vcpkg_make_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    DEFAULT_OPTIONS_EXCLUDE "^--docdir"
    OPTIONS
        ${OPTIONS}
        --disable-db-install
        --disable-pkg-ldflags
        --disable-rpath-hack
        --enable-pc-files
        --without-ada
        --without-debug # "lib model"
        --without-manpages
        --without-progs
        --without-tack
        --without-tests
        --with-pkg-config-libdir=libdir
)
vcpkg_make_install()

if(NCURSES_TARGET_IS_MSVC)
    # Use MSVC library naming (libncursesw.a -> ncursesw.lib)
    foreach(libdir IN ITEMS "${CURRENT_PACKAGES_DIR}/lib" "${CURRENT_PACKAGES_DIR}/debug/lib")
        file(GLOB archives "${libdir}/lib*.a")
        foreach(archive IN LISTS archives)
            get_filename_component(name "${archive}" NAME_WE)
            string(REGEX REPLACE "^lib" "" name "${name}")
            file(RENAME "${archive}" "${libdir}/${name}.lib")
        endforeach()
    endforeach()

    # Consumers of the static library must not use __declspec(dllimport)
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/ncursesw/ncurses_dll.h"
        "# if defined(NCURSES_STATIC)"
        "# if 1 /* vcpkg: static library, NCURSES_STATIC */"
    )
endif()

vcpkg_fixup_pkgconfig()

# Prefer local files over search path
file(GLOB headers "${CURRENT_PACKAGES_DIR}/include/ncursesw/*.h")
foreach(file IN LISTS headers)
    vcpkg_replace_string("${file}" [[#include <ncursesw/([^>]*)>]] [[#include "\1"]] REGEX IGNORE_UNCHANGED)
endforeach()

vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/tools/ncurses/bin/ncursesw6-config"  "${CURRENT_INSTALLED_DIR}" "\${prefix}")
vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/tools/ncurses/bin/ncursesw6-config"  "\nprefix=\"\${prefix}\"" [=[prefix=$(CDPATH= cd -- "$(dirname -- "$0")"/../../.. && pwd -P)]=])
if(NOT VCPKG_BUILD_TYPE)
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/tools/ncurses/debug/bin/ncursesw6-config"  "${CURRENT_INSTALLED_DIR}" "\${prefix}")
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/tools/ncurses/debug/bin/ncursesw6-config"  "\nprefix=\"\${prefix}/debug\"" [=[prefix=$(CDPATH= cd -- "$(dirname -- "$0")"/../../../.. && pwd -P)/debug]=])
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/tools/ncurses/debug/bin/ncursesw6-config"  "\${prefix}/share" "\${prefix}/../share")
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/bin")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
