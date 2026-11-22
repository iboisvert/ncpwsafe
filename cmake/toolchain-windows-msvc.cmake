# Chainloaded by vcpkg's toolchain (VCPKG_CHAINLOAD_TOOLCHAIN_FILE in CMakePresets.json)
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)

# /utf-8: source files (e.g. ru.dat) contain UTF-8 string literals
# /wd4068: unknown pragma, the sources use `#pragma GCC diagnostic`
# _CRT_*: silence warnings for POSIX/C-runtime names (strdup, getenv, ...)
set(_NCPWSAFE_MSVC_FLAGS "/W3 /utf-8 /wd4068 /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS")
set(CMAKE_C_FLAGS_INIT "${_NCPWSAFE_MSVC_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${_NCPWSAFE_MSVC_FLAGS} /permissive- /Zc:__cplusplus")
unset(_NCPWSAFE_MSVC_FLAGS)

# MSVC supports AddressSanitizer only, not UBSan
set(NCPWSAFE_SANITIZE_FLAGS "/fsanitize=address")
