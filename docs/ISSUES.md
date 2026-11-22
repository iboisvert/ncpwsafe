# Issues

Known open issues. Severity is one of:

- **Important**: a bug that breaks the build, the tests or a security guarantee.
- **Minor**: a smaller defect, a portability difference or a project-convention violation.

Issues 1–9 were found by a code review of the Windows MSVC port (branch `dev/build-win-msvc`, 2026-11-21). Issue 10 is the part of the crash-dump issue (3) that its fix did not cover.

## 10. Other processes can read ncpwsafe's memory on Windows

**Severity:** Minor

On Linux, `prctl(PR_SET_DUMPABLE, 0)` also stops other processes of the same user from attaching with ptrace. `win32::DisableCrashDumps()` (`src/Win32.cpp`) only keeps Windows Error Reporting from writing crash dumps. Any process of the same user can still open ncpwsafe with `PROCESS_VM_READ` and read decrypted passwords, for example with Task Manager's "Create memory dump file" or a debugger. Fail-fast exceptions (`__fastfail`, e.g. `/GS` stack buffer overruns) also bypass the handlers and are still reported to WER, with a dump if LocalDumps is enabled.

**Fix:** set a restrictive DACL on the process at startup that denies `PROCESS_VM_READ`, `PROCESS_VM_WRITE` and `PROCESS_CREATE_THREAD` to everyone but SYSTEM (as KeePass does). This does not stop an administrator or a process with `SeDebugPrivilege`. Test it with the console host and Windows Terminal before turning it on.
