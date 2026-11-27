# Chainloaded by vcpkg's toolchain (VCPKG_CHAINLOAD_TOOLCHAIN_FILE in CMakePresets.json)
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)

# /utf-8: source files (e.g. ru.dat) contain UTF-8 string literals
# /wd4068: unknown pragma, the sources use `#pragma GCC diagnostic`
# _CRT_*: silence warnings for POSIX/C-runtime names (strdup, getenv, ...)
# The warning level is in NCPWSAFE_WARNING_FLAGS, a second /W here would make cl warn (D9025)
set(_NCPWSAFE_MSVC_FLAGS "/utf-8 /wd4068 /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS")
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

# Warnings for ncpwsafe's own code only, not the fetched libraries (CMakeLists.txt).
# /external:*: no warnings from headers included with <> (vcpkg's, the Windows SDK's),
# /analyze: the code analysis warnings (C6000-C6999), except in those headers.
# /wd4458 /wd4459: declaration hides a class member or a global, which /W4 warns about;
# as GCC's -Wshadow=local, only locals that hide locals or parameters (C4456, C4457)
set(NCPWSAFE_WARNING_FLAGS /W4 /wd4458 /wd4459 /external:anglebrackets /external:W0 /analyze /analyze:external-)
# With NCPWSAFE_WARNINGS_AS_ERRORS, as in CI. Also makes the /analyze warnings errors
set(NCPWSAFE_WERROR_FLAGS /WX)
