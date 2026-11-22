# x64-windows-static, release libraries only, for the x64-windows-msvc-release preset.
# The libraries and the C runtime are linked statically (/MT), so the release
# executable runs on a stock Windows without the Visual C++ Redistributable;
# cmake/toolchain-windows-msvc.cmake builds ncpwsafe with the same runtime.
# vcpkg normally builds every port twice (debug and release); a Release build
# only links the release libraries, so this halves the dependency build time.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)
