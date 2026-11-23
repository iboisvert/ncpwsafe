# Make a release package of a Release build: install it into a staging directory
# with cmake --install, add the README, the licences and (on Windows) the man page
# rendered to HTML, and archive it. The release workflow runs it after the tests:
#
#   cmake -DBUILD_DIR=build -DPLATFORM=linux-x86_64 \
#         "-DLICENSES=ncurses=/usr/share/doc/libncurses-dev/copyright;..." \
#         -P cmake/MakePackage.cmake
#
# Variables:
#   BUILD_DIR   The build directory (required)
#   PLATFORM    linux-x86_64 (makes a .tar.gz) or windows-x64 (makes a .zip) (required)
#   OUTPUT_DIR  Where the package goes, default ${BUILD_DIR}/package
#   LICENSES    The licences of the libraries linked into the executable, as a list of
#               Name=file: each file is copied into THIRD-PARTY-LICENSES under its name.
#               libpwsafe and libconfini are added from the build's sources.
#   MAN_HTML    The man page rendered to HTML (mandoc -Thtml), required for windows-x64,
#               which has no man command
#
# The package is ncpwsafe-<version>-<platform>.tar.gz or .zip, holding a directory
# ncpwsafe/ with the installed files (bin/ncpwsafe and share/man/man1/ncpwsafe.1 on
# Linux, ncpwsafe.exe on Windows), README.md, LICENSE and THIRD-PARTY-LICENSES.

cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_LIST_DIR}/ReadVersion.cmake")
get_filename_component(source_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
read_project_version(version)

if(NOT DEFINED BUILD_DIR OR NOT EXISTS "${BUILD_DIR}/CMakeCache.txt")
    message(FATAL_ERROR "BUILD_DIR must be a configured and built build directory")
endif()
get_filename_component(BUILD_DIR "${BUILD_DIR}" ABSOLUTE)
if(PLATFORM STREQUAL "linux-x86_64")
    set(archive_ext tar.gz)
    set(archive_mode cfz)
    set(archive_format)
elseif(PLATFORM STREQUAL "windows-x64")
    set(archive_ext zip)
    set(archive_mode cf)
    set(archive_format --format=zip)
    if(NOT DEFINED MAN_HTML OR NOT EXISTS "${MAN_HTML}")
        message(FATAL_ERROR "MAN_HTML must name the man page rendered to HTML for ${PLATFORM}, not \"${MAN_HTML}\"")
    endif()
else()
    message(FATAL_ERROR "PLATFORM must be linux-x86_64 or windows-x64, not \"${PLATFORM}\"")
endif()
if(NOT DEFINED OUTPUT_DIR)
    set(OUTPUT_DIR "${BUILD_DIR}/package")
endif()
get_filename_component(OUTPUT_DIR "${OUTPUT_DIR}" ABSOLUTE)

set(stage "${OUTPUT_DIR}/stage")
set(root "${stage}/ncpwsafe")
set(package "${OUTPUT_DIR}/ncpwsafe-${version}-${PLATFORM}.${archive_ext}")
file(REMOVE_RECURSE "${stage}")
file(REMOVE "${package}")

function(run)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE status)
    if(NOT status EQUAL 0)
        list(JOIN ARGN " " command)
        message(FATAL_ERROR "Failed (${status}): ${command}")
    endif()
endfunction()

# The installed files. Stripped on Linux: the symbols aren't needed to run it
if(PLATFORM STREQUAL "linux-x86_64")
    set(strip --strip)
endif()
run(${CMAKE_COMMAND} --install "${BUILD_DIR}" --prefix "${root}" --config Release ${strip})

file(COPY "${source_dir}/README.md" "${source_dir}/LICENSE" DESTINATION "${root}")
if(DEFINED MAN_HTML)
    file(COPY_FILE "${MAN_HTML}" "${root}/ncpwsafe.1.html")
endif()

# THIRD-PARTY-LICENSES: the forks, then the libraries named in LICENSES
set(notices "ncpwsafe ${version} includes the following libraries, linked statically into the
executable. ncpwsafe itself is under the GNU General Public License version 3,
see LICENSE. The source of this version of ncpwsafe is at
https://github.com/iboisvert/ncpwsafe/tree/v${version}
")
foreach(fork IN ITEMS libpwsafe libconfini)
    set(fork_dir "${BUILD_DIR}/_deps/${fork}-src")
    execute_process(COMMAND git -C "${fork_dir}" rev-parse HEAD
        OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE status)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Can't read the commit of ${fork} in ${fork_dir}")
    endif()
    string(APPEND notices "
================================================================================
${fork}
================================================================================

Under the GNU General Public License version 3, the same licence as ncpwsafe
(see LICENSE). Source: https://github.com/iboisvert/${fork}/tree/${commit}
")
endforeach()
foreach(entry IN LISTS LICENSES)
    if(NOT entry MATCHES "^([^=]+)=(.+)$")
        message(FATAL_ERROR "LICENSES entries are Name=file, not \"${entry}\"")
    endif()
    set(name "${CMAKE_MATCH_1}")
    set(file "${CMAKE_MATCH_2}")
    if(NOT EXISTS "${file}")
        message(FATAL_ERROR "Licence file of ${name} not found: ${file}")
    endif()
    file(READ "${file}" text)
    string(APPEND notices "
================================================================================
${name}
================================================================================

${text}")
endforeach()
file(WRITE "${root}/THIRD-PARTY-LICENSES" "${notices}")

run(${CMAKE_COMMAND} -E chdir "${stage}" ${CMAKE_COMMAND} -E tar ${archive_mode} "${package}" ${archive_format} ncpwsafe)
file(REMOVE_RECURSE "${stage}")
message(STATUS "Package: ${package}")
