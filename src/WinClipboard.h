/* Copyright 2026 Ian Boisvert */
#pragma once

#include <string>

#ifdef _WIN32
/**
 * Copy UTF-8 text to the Windows clipboard.
 *
 * The text is excluded from clipboard history and the cloud clipboard,
 * since it is usually an account user name or password.
 *
 * @remark Declared in its own header so that callers don't include
 * `<windows.h>`, whose macros (e.g. `MessageBox`) collide with ncpwsafe names.
 *
 * @returns
 * 0 on success
 * 1 if the text cannot be converted to UTF-16
 * 2 if memory cannot be allocated for the clipboard data
 * 3 if the clipboard cannot be opened
 * 4 if the text cannot be placed on the clipboard
 */
int CopyTextToClipboardWin(const std::string &text);
#endif
