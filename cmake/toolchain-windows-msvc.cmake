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

# The C runtime must match the vcpkg libraries' (VCPKG_CRT_LINKAGE of the triplet):
# static (/MT) for x64-windows-static*, the release preset's, so the executable needs
# no Visual C++ Redistributable; otherwise the DLL (/MD), as for x64-windows-static-md
if(VCPKG_TARGET_TRIPLET MATCHES "-static" AND NOT VCPKG_TARGET_TRIPLET MATCHES "-static-md")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
# libconfini's cmake_minimum_required() predates CMAKE_MSVC_RUNTIME_LIBRARY (policy
# CMP0091), which it would otherwise ignore, keeping the default /MD
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

# MSVC supports AddressSanitizer only, not UBSan
set(NCPWSAFE_SANITIZE_FLAGS "/fsanitize=address")
