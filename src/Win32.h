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
 * Sets the console input and output code pages to UTF-8 and selects the
 * UTF-8 C runtime locale so that multibyte strings are interpreted
 * as UTF-8 by curses. The original console code pages are restored
 * when the process exits.
 */
void InitConsole();

/**
 * Keep Windows Error Reporting from writing crash dumps, which could
 * expose decrypted passwords.
 *
 * Unhandled exceptions, `abort()` and invalid C runtime parameters end the
 * process immediately instead of being reported to WER. Fail-fast exceptions
 * (e.g. `/GS` stack buffer overruns) can't be intercepted and are still
 * reported. This does not stop another process of the same user from
 * reading this process's memory (e.g. Task Manager's "Create memory dump file").
 */
void DisableCrashDumps();

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
