set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_C_FLAGS_INIT "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG_INIT "-ggdb -O0 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-ggdb -O0 -fno-omit-frame-pointer")
set(NCPWSAFE_SANITIZE_FLAGS "-fsanitize=address,undefined")
