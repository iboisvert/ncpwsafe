# read_project_version(<var>): set <var> to the MAJOR.MINOR.PATCH version in
# project(ncpwsafe ...) in the top-level CMakeLists.txt, without configuring the
# project. For the release scripts, which run with cmake -P.
function(read_project_version var)
    file(READ "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../CMakeLists.txt" top_level)
    # The arguments of project(ncpwsafe ...), up to its closing parenthesis
    if(NOT top_level MATCHES "\nproject\\(ncpwsafe([^)]*)\\)")
        message(FATAL_ERROR "No project(ncpwsafe ...) in CMakeLists.txt")
    endif()
    if(NOT CMAKE_MATCH_1 MATCHES "VERSION[ \t\r\n]+([0-9]+\\.[0-9]+\\.[0-9]+)[ \t\r\n]")
        message(FATAL_ERROR "project() in CMakeLists.txt has no MAJOR.MINOR.PATCH version")
    endif()
    set(${var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()
