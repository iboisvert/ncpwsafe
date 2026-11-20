# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ncpwsafe is a C++17 ncurses TUI (plus a few non-interactive CLI commands) for Password Safe V1/V2 database files. The actual file format/crypto lives in the external `libpwsafe` C library (`pwsafe.h`, `pws_*` functions, `PwsDbRecord`/`PwsFieldType`), not in this repo. `xclip` is required at runtime for clipboard copy.

## Build & test

```sh
cmake --preset=x64-linux-gcc-debug            # or x64-linux-clang-*, *-release; extra options: -DBUILD_TESTING=OFF, -DNCPWSAFE_DEBUG_SANITIZE=ON (ASan+UBSan, Debug only), -DUSE_GLOG=ON
cmake --build --preset=x64-linux-gcc-debug
./build/bin/ncpwsafe [DATABASE_FILE]          # executable goes to build/bin (README's build/src path is out of date)
./build/bin/ncpwsafe --help                   # CLI commands: --generate-password, --export-db, --change-password, --test-db (Debug only)

ctest --preset=x64-linux-gcc-debug            # all tests
ctest --preset=x64-linux-gcc-debug -R AppTest # tests matching a regex
./build/test/unittests --gtest_filter='AccountDbTest.*'   # run the gtest binary directly
```

- Presets (`CMakePresets.json`) use the Unix Makefiles generator and all write to `./build`, so switching compiler or toolchain needs `--fresh`. The compiler, warning flags and debug flags come from `cmake/toolchain-linux-{gcc,clang}.cmake`, not from `CMakeLists.txt`. This mirrors the libpwsafe layout.

- System deps via pkg-config: ncurses (+form, menu, panel), icu-uc, nettle, GTest (and libglog if `USE_GLOG`). `libpwsafe` and `libconfini` are pulled with `FetchContent` from the author's GitHub forks at `HEAD` (see `cmake/ResolveDependencies.cmake`), so configuring needs network access. To develop against a local libpwsafe checkout, swap in the commented `SOURCE_DIR` line there.
- `gtest_discover_tests` sets the working directory to `${CMAKE_BINARY_DIR}/../test`, so tests expect the build directory to be a direct child of the repo root (e.g. `build/`).
- CI (`.github/workflows/build.yml`) runs the `x64-linux-gcc-release` and `x64-linux-clang-release` presets on Ubuntu (configure, build, ctest).
- Some CMake artifacts (`CMakeCache.txt`, `Makefile`, `config.h`, ...) may be sitting in the repo root from an in-source build. They're gitignored; use `build/`.
- `config.h` is generated from `config.h.in` into the build dir (app name/version, default config path `$HOME/.config/ncpwsafe/ncpwsafe.ini`, `HAVE_SYS_*` checks).

## Architecture

- **Build layout**: every source file except `main.cpp` is compiled into the `libncpwsafe` OBJECT library, which links into both the `ncpwsafe` executable and the `unittests` gtest binary. New `.cpp` files must be added to the `SRC` list in `src/CMakeLists.txt`; new test files go in `test/CMakeLists.txt`.
- **Entry point** (`src/main.cpp`): parses `getopt_long` options into `InputProgArgs` (all `std::optional`), validates them, then `PWSafeApp::Init` turns them into a `ProgArgs` with defaults filled in. It then switches on `Operation`: `OPEN_DB` runs the TUI (`PWSafeApp::Show`), and the others run a `*Command` class (`ExportDbCommand`, `ChangeDbPasswordCommand`, `GeneratePasswordCommand`, `GenerateTestDbCommand`), each with `Execute()`. Adding a CLI option means touching the `options[]` table, `ParseArgs`, `ValidateArgs`, `Usage`, `InputProgArgs`, `ProgArgs` and the `DEFAULT_*` values in `ProgArgs.cpp`.
- **`PWSafeApp`** is the central object, passed by reference to every window, dialog and command. It owns `ProgArgs`, the `AccountDb`, the root/accounts/command-bar ncurses windows, and database backup (rotation is controlled by the `Prefs` keys `backup-before-save` and `backup-count`).
- **Data model**: `AccountDb` (pathname, password, read-only flag) wraps `AccountRecords`, a collection of `AccountRecord` maps keyed by `PwsFieldType` with dirty tracking. `ReadDb`/`WriteDb` convert to and from libpwsafe's linked-list `PwsDbRecord`. Return codes use `ResultCode` (which extends libpwsafe's `PRC_*` codes) through an optional `int *rc` out-param plus `SetResultCode()`.
- **TUI**: built on ncurses `panel`, `menu` and `form`. `AccountsWin` is the main two-column menu of accounts, backed by `AccountRecord*` stored in each menu item's userptr. `Dialog` is a generic form dialog built from a `std::vector<DialogField>`, with validate/discard/input-delegate callbacks; specific dialogs (`AccountDetailsDlg`, `ChangePasswordDlg`, `SafeCombinationPromptDlg`, ...) build on it. `MessageBox`, `CommandBarWin` and `SearchBarWin` complete the set. Each component runs its own modal `wgetch` loop.
- **Terminal resize**: every modal loop must handle `KEY_RESIZE` by calling `app_.HandleResize()`, and must register its own relayout for the length of the loop with `ScopedResizeHandler resize_handler(app_, [this]() { OnResize(); });`. Handlers form a stack and run bottom-to-top so stacked panels lay out in order. Follow this pattern in any new window or dialog.
- **Prefs**: a `Prefs::Instance()` singleton backed by an INI file (libconfini), with typed `Get<T>`/`Set<T>`.
- **Secrets hygiene**: `Dialog::RandomizeBuffers()` scrambles form buffers and `Utils.cpp` zeroes memory through a volatile `memset` pointer. Keep these when touching password-handling code. `main.cpp` calls `prctl(PR_SET_DUMPABLE, 0)` to block ptrace and core dumps; this is currently under `#ifndef NDEBUG`, so it applies to Debug builds.

## Testing conventions

- GoogleTest. To reach private members, classes declare `FRIEND_TEST(...)` under `#ifdef FRIEND_TEST` (see `PWSafeApp.h`, `AccountDb.h`). A new test that needs private access must add a matching `FRIEND_TEST` declaration in the class header.
- Filesystem access goes through the virtual `Filesystem` struct (`Filesystem::instance` by default), which is injected into `PWSafeApp`'s constructor so tests can pass a mock (see `FilesystemMock` in `test/PWSafeApp-tests.cpp`). Prefer this over calling `std::filesystem` directly in code that needs testing.
- `src/en_CA.dat` and `src/ru.dat` are word lists written as C string literals and `#include`d directly into `GenerateTestDbCommand.cpp`. `--test-db` uses them to generate test databases (`--test-db-lang=en-CA|ru`).

`TODO.md` tracks known bugs and planned features.
