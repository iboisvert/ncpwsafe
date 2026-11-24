set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_C_FLAGS_INIT "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG_INIT "-ggdb -O0 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-ggdb -O0 -fno-omit-frame-pointer")
set(NCPWSAFE_SANITIZE_FLAGS "-fsanitize=address,undefined")

# Warnings for ncpwsafe's own code only, not the fetched libraries (CMakeLists.txt).
# -Wshadow=local: a local shadowing a local or parameter, not members or globals
# (such as the curses FIELD type)
set(NCPWSAFE_WARNING_FLAGS -Wshadow=local -Wconversion -Wnull-dereference)
# With NCPWSAFE_WARNINGS_AS_ERRORS, as in CI
set(NCPWSAFE_WERROR_FLAGS -Werror)
