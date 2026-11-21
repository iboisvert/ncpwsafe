# Issues

## README build instructions point to the wrong binary path

**Status: open.**

`README.md` says to run `./build/src/ncpwsafe`, but `src/CMakeLists.txt` sets
`RUNTIME_OUTPUT_DIRECTORY` to `${CMAKE_BINARY_DIR}/bin`. The binary is actually at
`./build/bin/ncpwsafe`.

## Core-dump/ptrace protection is enabled in Debug builds, not Release

**Status: open.**

In `src/main.cpp`, `DisableCoreDump()` calls `prctl(PR_SET_DUMPABLE, 0)` only inside
`#ifndef NDEBUG`. That turns it on in Debug builds and leaves it off in Release builds.
The comment at the call site ("Don't allow ptrace or gdump on release build") says the
opposite. As written, Release builds can still be ptraced and can write core dumps that
may contain decrypted passwords. Debug builds block debugger attach.

## Leftover in-source CMake build artifacts in the repo root

**Status: partly cleaned up.** `CMakeCache.txt` and most of the root files are gone,
which removes the risk to `build/`. Still left: `CMakeFiles/` and `config.h` in the
root, and `CMakeFiles/`, `Makefile`, `cmake_install.cmake`, `CTestTestfile.cmake` and
`cmake_test_discovery_*.json` under `test/`.

The repo root contained files from an in-source CMake configure (`CMakeCache.txt`,
`CMakeFiles/`, `Makefile`, `cmake_install.cmake`, `CTestTestfile.cmake`,
`CPackConfig.cmake`, `CPackSourceConfig.cmake`, `config.h`), along with similar files
under `test/`. They are gitignored, but a stale `CMakeCache.txt` in the source dir can
interfere with out-of-source builds in `build/`. The rest can be deleted.

## Message boxes wider than the terminal are not shown

**Status: open.**

`MessageBox::InitTUI` sizes the window to fit the longest line of the message. If that
is wider than the terminal (e.g. "File <long pathname> does not exist"), `newwin` fails,
`wgetch(NULL)` returns `ERR`, and `ProcessInput` returns immediately, so the message is
never seen. Long lines should be wrapped or truncated to fit the terminal. Also,
`ProcessInput` treats any `ERR` from `wgetch` as closing the box.
