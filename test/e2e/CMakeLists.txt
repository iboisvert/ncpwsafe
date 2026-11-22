# End-to-end tests: run the real ncpwsafe binary.
#   CLI tests (label "cli") run the non-interactive commands.
#   TUI tests (label "tui") drive the ncurses UI inside a detached tmux server.
# Both are Python 3 (standard library only) and carry the "e2e" label.
# The CLI tests run on Linux and Windows, the TUI tests only on Linux.
# See test/README.md.

find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_Interpreter_FOUND)
    message(WARNING "Python 3 not found, end-to-end tests are disabled")
    return()
endif()
find_program(TMUX_EXECUTABLE tmux)
if(NOT TMUX_EXECUTABLE)
    message(STATUS "tmux not found, TUI end-to-end tests will be skipped")
endif()

# Builds account databases with known contents for the tests
add_executable(e2e-mkdb mkdb.cpp)
target_link_libraries(e2e-mkdb libncpwsafe)

set(E2E_NCPWSAFE "$<TARGET_FILE:ncpwsafe>")

set(E2E_ENV
    "NCPWSAFE=${E2E_NCPWSAFE}"
    "E2E_MKDB=$<TARGET_FILE:e2e-mkdb>"
    "E2E_FIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    "NCPWSAFE_MAN_PAGE=${CMAKE_BINARY_DIR}/man/ncpwsafe.1"
    "NCPWSAFE_VERSION=${PROJECT_VERSION}"
    "TMUX_BIN=${TMUX_EXECUTABLE}"
    "PYTHONDONTWRITEBYTECODE=1"
    "LANG=C.UTF-8"
    "LC_ALL=C.UTF-8"
)

# Register one CTest test per test_*.py file, named <label>.<file stem>
function(add_e2e_tests label timeout)
    file(GLOB scripts CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${label}/test_*.py")
    foreach(script ${scripts})
        get_filename_component(stem ${script} NAME_WE)
        string(SUBSTRING ${stem} 5 -1 name)  # strip "test_"
        add_test(NAME ${label}.${name}
            # -X utf8: UTF-8 file and pipe I/O on every platform (Windows defaults to the ANSI code page)
            COMMAND ${Python3_EXECUTABLE} -B -X utf8 ${script}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        set_tests_properties(${label}.${name} PROPERTIES
            LABELS "e2e;${label}"
            ENVIRONMENT "${E2E_ENV}"
            # A script exits 77 when a prerequisite (tmux, a Debug-only
            # command) is unavailable
            SKIP_RETURN_CODE 77
            TIMEOUT ${timeout})
    endforeach()
endfunction()

add_e2e_tests(cli 60)
add_e2e_tests(tui 180)
