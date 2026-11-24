set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_FLAGS_INIT "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG_INIT "-g -O0 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -O0 -fno-omit-frame-pointer")
set(NCPWSAFE_SANITIZE_FLAGS "-fsanitize=address,undefined")

# Warnings for ncpwsafe's own code only, not the fetched libraries (CMakeLists.txt).
# Clang has no -Wshadow=local (GCC's), its -Wshadow is the closest: it doesn't report
# constructor parameters that shadow members, or names that shadow types
set(NCPWSAFE_WARNING_FLAGS -Wshadow -Wconversion -Wnull-dereference)
# With NCPWSAFE_WARNINGS_AS_ERRORS, as in CI
set(NCPWSAFE_WERROR_FLAGS -Werror)
