#
# FindNcurses
# ---------
#
# Find the ncurses header and library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# none
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ``NCURSES_FOUND``
#   True if ncurses found.
#
# ``NCURSES_INCLUDE_DIRS``
#   Location of ncurses.h.
#
# ``NCURSES_LIBRARIES``
#   List of libraries when using ncurses.
#[=======================================================================[.rst:
FindNcurses
-------

Finds the ncurses library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Foo::Foo``
  The Foo library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``NCURSES_FOUND``
  True if the system has the ncurses library.
``NCURSES_VERSION``
  The version of the ncurses library which was found.
``NCURSES_INCLUDE_DIRS``
  Include directories needed to use ncurses.
``NCURSES_LIBRARIES``
  Libraries needed to link to ncurses.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``NCURSES_INCLUDE_DIR``
  The directory containing ``foo.h``.
``NCURSES_LIBRARY``
  The path to the Foo library.

 Set ``CURSES_NEED_WIDE`` to ``TRUE`` before the
 ``find_package(Curses)`` call if unicode functionality is required.

#]=======================================================================]
include(FindPackageHandleStandardArgs)

if (CURSES_NEED_WIDE)
  set(NCURSES_SUFFIX "w")
endif()

find_package(PkgConfig)
pkg_check_modules(PC_NCURSES QUIET ncurses${NCURSES_SUFFIX})

#
# Locate header 'ncurses.h'
#
find_path(NCURSES_INCLUDE_DIR
  NAMES ncurses.h
  PATHS ${PC_NCURSES_INCLUDE_DIRS}
)

#
# Locate shared library 'libncurses'
#
find_library(NCURSES_LIBRARY
  NAMES ncurses${NCURSES_SUFFIX}
  PATHS ${PC_NCURSES_LIBRARY_DIRS}
)

#
# Components
#
set(_supported_components form panel menu)

foreach(comp ${Ncurses_FIND_COMPONENTS})
  if (NOT ";${_supported_components};" MATCHES ${comp})
    set(NCURSES_FOUND False)
    set(NCURSES_NOT_FOUND_MESSAGE "Unsupported component: ${comp}")
  endif()

  find_library(NCURSES_${comp}_LIBRARY
    NAMES ${comp}${NCURSES_SUFFIX}
    PATHS ${PC_NCURSES_LIBRARY_DIRS}
  )

  find_package_handle_standard_args(Ncurses_${comp}
    REQUIRED_VARS
      NCURSES_${comp}_LIBRARY
  )

  if(Ncurses_${comp}_FOUND)
    list(APPEND NCURSES_LIBRARY "${NCURSES_${comp}_LIBRARY}")
  endif()
endforeach()

set(NCURSES_VERSION ${PC_NCURSES_VERSION})

find_package_handle_standard_args(Ncurses
  REQUIRED_VARS
    NCURSES_LIBRARY
    NCURSES_INCLUDE_DIR
  VERSION_VAR NCURSES_VERSION
  HANDLE_COMPONENTS
)

if(NCURSES_FOUND)
  set(NCURSES_LIBRARIES ${NCURSES_LIBRARY})
  set(NCURSES_INCLUDE_DIRS ${NCURSES_INCLUDE_DIR})
  set(NCURSES_DEFINITIONS ${PC_NCURSES_CFLAGS_OTHER})
endif()

mark_as_advanced(
  NCURSES_INCLUDE_DIR
  NCURSES_LIBRARY
)
