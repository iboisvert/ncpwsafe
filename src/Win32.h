/* Copyright 2026 Ian Boisvert */
#ifndef HAVE_WIN32_H
#define HAVE_WIN32_H

/**
 * Windows-specific platform support implemented with the Windows SDK.
 *
 * The implementation is kept in its own translation unit so that
 * `<windows.h>` is never included alongside the curses headers
 * (both define `KEY_EVENT`, `MOUSE_MOVED`, ...) or alongside this project's
 * `MessageBox` class (`<windows.h>` defines a `MessageBox` macro).
 */
#ifdef _WIN32

#include <string>

namespace win32
{

/**
 * Configure the console and C runtime for UTF-8.
 *
 * Sets the console input and output code pages to UTF-8 and selects a
 * UTF-8 `LC_CTYPE` C runtime locale so that multibyte strings are interpreted
 * as UTF-8 by curses. The original console code pages are restored
 * when the process exits, also after a crash, `abort()` or a console control
 * event (Ctrl+C, Ctrl+Break, closing the console). They are not restored
 * after a fail-fast exception, or if the process is killed.
 */
void InitConsole();

/**
 * Keep Windows Error Reporting from writing crash dumps, which could
 * expose decrypted passwords.
 *
 * Unhandled exceptions, `abort()` and invalid C runtime parameters end the
 * process immediately instead of being reported to WER. Fail-fast exceptions
 * (e.g. `/GS` stack buffer overruns) can't be intercepted and are still
 * reported, with a dump only if an administrator has enabled WER LocalDumps.
 */
void DisableCrashDumps();

/**
 * Keep other processes from reading or changing this process's memory,
 * which could expose decrypted passwords, the counterpart of
 * `prctl(PR_SET_DUMPABLE, 0)` on Linux.
 *
 * Replaces the process's DACL so that the current user can only wait for,
 * query and end the process (e.g. a debugger, or Task Manager's
 * "Create memory dump file", can't open it), and the owner can't change the
 * DACL back. Only SYSTEM keeps full access. This does not stop an
 * administrator using `SeDebugPrivilege`.
 *
 * @returns `true` on success
 */
bool RestrictProcessAccess();

/**
 * The directory for per-user application data (`%APPDATA%`), UTF-8 encoded.
 *
 * The `APPDATA` environment variable is used if it is set, so that it can
 * be overridden (the end-to-end tests do). Otherwise the folder comes
 * from `SHGetKnownFolderPath(FOLDERID_RoamingAppData)`, e.g. when ncpwsafe
 * runs with a minimal environment.
 *
 * @returns The directory, or an empty string if it can't be found
 */
std::string RoamingAppDataDir();

/** Result codes for `CopyTextToClipboard` */
enum ClipboardResult
{
    CLIPBOARD_OK = 0,
    CLIPBOARD_ERR_ENCODING = 1,  ///< Text is not valid UTF-8
    CLIPBOARD_ERR_ALLOC = 2,     ///< Unable to allocate clipboard memory
    CLIPBOARD_ERR_OPEN = 3,      ///< Unable to open the clipboard
    CLIPBOARD_ERR_SET = 4,       ///< Unable to set clipboard data
};

/**
 * Copy UTF-8 text to the Windows clipboard.
 *
 * The clipboard content is marked so that it is excluded from
 * clipboard history (Win+V), cloud clipboard sync, and clipboard monitors,
 * since it is typically a password.
 *
 * @returns One of `ClipboardResult`
 */
int CopyTextToClipboard(const std::string &text);

}  // namespace win32

#endif  // #ifdef _WIN32

#endif  // #ifndef HAVE_WIN32_H
