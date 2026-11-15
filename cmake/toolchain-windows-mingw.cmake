# Toolchain for building on Windows with MinGW-w64 gcc.
# Chain-loaded by the vcpkg toolchain, see the x64-windows-mingw presets in CMakePresets.json.
#
# MSVC is not supported: ncurses, which provides the form and menu libraries
# required by ncpwsafe, does not build with MSVC.

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_C_FLAGS_DEBUG_INIT "-ggdb -O0 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-ggdb -O0 -fno-omit-frame-pointer")

# Link the C and C++ runtimes statically so that executables
# don't depend on the MinGW-w64 runtime DLLs being on the PATH
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")
