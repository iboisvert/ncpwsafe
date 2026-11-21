# Issues

Known open issues. Severity is one of:

- **Important**: a bug that breaks the build, the tests or a security guarantee.
- **Minor**: a smaller defect, a portability difference or a project-convention violation.

Issues 1–9 were found by a code review of the Windows MSVC port (branch `dev/build-win-msvc`, 2026-11-21). Issue 10 is the part of the crash-dump issue (3) that its fix did not cover.

## 4. `InitConsole()` sets every locale category

**Severity:** Minor

`win32::InitConsole()` calls `setlocale(LC_ALL, ".UTF-8")` (`src/Win32.cpp:46`). On other platforms `main()` deliberately sets only `LC_CTYPE`, so that number formats and collation don't change. On Windows, `LC_NUMERIC`, `LC_COLLATE` and `LC_TIME` switch to the user's locale. A fr-FR user would get a comma decimal separator in `printf`/`strtod`.

**Fix:** `setlocale(LC_CTYPE, ".UTF-8")`.

## 5. Console code page not restored after a crash

**Severity:** Minor

`win32::InitConsole()` sets the console input and output code pages to UTF-8 (`src/Win32.cpp:42`). These belong to the console, which is shared with the parent shell, not to the process. They are restored only from an `atexit` handler (line 40), which does not run on a crash, `abort()` or Ctrl+Break. The parent `cmd.exe` then stays on code page 65001, and legacy console programs run from it afterwards show garbled text.

**Fix:** also restore from a `SetConsoleCtrlHandler` handler and an unhandled-exception filter, or accept and document it.

## 6. `Win32.cpp` is not in the `SRC` list

**Severity:** Minor

`src/CMakeLists.txt:59` adds `Win32.cpp` with a conditional `target_sources(libncpwsafe PRIVATE Win32.cpp)`, not through the `SRC` list as AGENTS.md requires for new `.cpp` files. The file is already wrapped in `#ifdef _WIN32`, so the condition is not needed.

**Fix:** add `Win32.cpp` to `SRC`.

## 7. Windows-specific code outside `Win32.{h,cpp}`

**Severity:** Minor

AGENTS.md says "Windows-specific code lives in `src/Win32.{h,cpp}`", but there are `#ifdef _WIN32` blocks in:

- `src/Utils.cpp:116`: `USERPROFILE` fallback for `HOME` in `ExpandEnvVars`
- `src/ProgArgs.cpp:14`: `APPDATA` default config path
- `src/PWSafeApp.cpp:402`: `localtime_s`
- `src/main.cpp:25`, `:41`, `:556`: includes, `ProgramName()`, `win32::InitConsole()`

**Fix:** move these into `Win32.{h,cpp}` helpers (e.g. `win32::LocalTime`, `win32::DefaultConfigPath`), or reword the rule to allow small `#ifdef _WIN32` blocks.

## 8. Default config path when `APPDATA` is not set

**Severity:** Minor

On Windows the default config file is `ExpandEnvVars("${APPDATA}/ncpwsafe/ncpwsafe-config.ini")` (`src/ProgArgs.cpp:15`). If `APPDATA` is not set (a service, a scheduled task, a minimal environment), it expands to `/ncpwsafe/ncpwsafe-config.ini` at the root of the current drive. Preferences are then read from `C:\ncpwsafe\`, or saving them fails with access denied. There is no fallback like the `HOME`→`USERPROFILE` one in `ExpandEnvVars`.

**Fix:** fall back to `%USERPROFILE%\AppData\Roaming`, or use `SHGetKnownFolderPath(FOLDERID_RoamingAppData)` in `Win32.cpp`.

## 9. `ReadPasswordFileTest` temp file is not created exclusively

**Severity:** Minor

`ReadPasswordFileTest::SetUp()` (`test/Utils-tests.cpp:82`) replaced `mkstemp()` with a name made from `std::random_device` and `fopen(..., "wb")`. This truncates an existing file instead of failing, and creates the file with default permissions, where `mkstemp()` created it atomically with mode 0600. On a name collision the test overwrites a file, and `TearDown()` then deletes a file it did not create.

**Fix:** open with `"wbx"` (supported by C11 and MSVC) and retry with a new name if the file exists.

## 10. Other processes can read ncpwsafe's memory on Windows

**Severity:** Minor

On Linux, `prctl(PR_SET_DUMPABLE, 0)` also stops other processes of the same user from attaching with ptrace. `win32::DisableCrashDumps()` (`src/Win32.cpp`) only keeps Windows Error Reporting from writing crash dumps. Any process of the same user can still open ncpwsafe with `PROCESS_VM_READ` and read decrypted passwords, for example with Task Manager's "Create memory dump file" or a debugger. Fail-fast exceptions (`__fastfail`, e.g. `/GS` stack buffer overruns) also bypass the handlers and are still reported to WER, with a dump if LocalDumps is enabled.

**Fix:** set a restrictive DACL on the process at startup that denies `PROCESS_VM_READ`, `PROCESS_VM_WRITE` and `PROCESS_CREATE_THREAD` to everyone but SYSTEM (as KeePass does). This does not stop an administrator or a process with `SeDebugPrivilege`. Test it with the console host and Windows Terminal before turning it on.
