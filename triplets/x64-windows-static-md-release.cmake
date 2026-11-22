# x64-windows-static-md, release libraries only, for the x64-windows-msvc-release preset.
# vcpkg normally builds every port twice (debug and release); a Release build
# only links the release libraries, so this halves the dependency build time.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)
