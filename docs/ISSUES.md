# Issues

Known open issues. Severity is one of:

- **Important**: a bug that breaks the build, the tests or a security guarantee.
- **Minor**: a smaller defect, a portability difference or a project-convention violation.

Issues 1–9 were found by a code review of the Windows MSVC port (branch `dev/build-win-msvc`, 2026-11-21). Issue 10 is the part of the crash-dump issue (3) that its fix did not cover.

## 4. `InitConsole()` sets every locale category

**Severity:** Minor

`win32::InitConsole()` calls `setlocale(LC_ALL, ".UTF-8")` (`src/Win32.cpp:46`). On other platforms `main()` deliberately sets only `LC_CTYPE`, so that number formats and collation don't change. On Windows, `LC_NUMERIC`, `LC_COLLATE` and `LC_TIME` switch to the user's locale. A fr-FR user would get a comma decimal separator in `printf`/`strtod`.

**Fix:** `setlocale(LC_CTYPE, ".UTF-8")`.

## 7. Windows-specific code outside `Win32.{h,cpp}`

**Severity:** Minor

AGENTS.md says "Windows-specific code lives in `src/Win32.{h,cpp}`", but there are `#ifdef _WIN32` blocks in:

- `src/Utils.cpp:116`: `USERPROFILE` fallback for `HOME` in `ExpandEnvVars`
- `src/ProgArgs.cpp:14`: `APPDATA` default config path
- `src/PWSafeApp.cpp:402`: `localtime_s`
- `src/main.cpp:25`, `:41`, `:556`: includes, `ProgramName()`, `win32::InitConsole()`

**Fix:** move these into `Win32.{h,cpp}` helpers (e.g. `win32::LocalTime`, `win32::DefaultConfigPath`), or reword the rule to allow small `#ifdef _WIN32` blocks.

## 9. `ReadPasswordFileTest` temp file is not created exclusively

**Severity:** Minor

`ReadPasswordFileTest::SetUp()` (`test/Utils-tests.cpp:82`) replaced `mkstemp()` with a name made from `std::random_device` and `fopen(..., "wb")`. This truncates an existing file instead of failing, and creates the file with default permissions, where `mkstemp()` created it atomically with mode 0600. On a name collision the test overwrites a file, and `TearDown()` then deletes a file it did not create.

**Fix:** open with `"wbx"` (supported by C11 and MSVC) and retry with a new name if the file exists.

## 10. Other processes can read ncpwsafe's memory on Windows

**Severity:** Minor

On Linux, `prctl(PR_SET_DUMPABLE, 0)` also stops other processes of the same user from attaching with ptrace. `win32::DisableCrashDumps()` (`src/Win32.cpp`) only keeps Windows Error Reporting from writing crash dumps. Any process of the same user can still open ncpwsafe with `PROCESS_VM_READ` and read decrypted passwords, for example with Task Manager's "Create memory dump file" or a debugger. Fail-fast exceptions (`__fastfail`, e.g. `/GS` stack buffer overruns) also bypass the handlers and are still reported to WER, with a dump if LocalDumps is enabled.

**Fix:** set a restrictive DACL on the process at startup that denies `PROCESS_VM_READ`, `PROCESS_VM_WRITE` and `PROCESS_CREATE_THREAD` to everyone but SYSTEM (as KeePass does). This does not stop an administrator or a process with `SeDebugPrivilege`. Test it with the console host and Windows Terminal before turning it on.
