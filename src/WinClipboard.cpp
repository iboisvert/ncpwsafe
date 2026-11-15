/* Copyright 2026 Ian Boisvert */
#ifdef _WIN32

#include "WinClipboard.h"

#include <climits>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/** Zero and free global memory that was not given to the clipboard */
static void FreeGlobal(HGLOBAL hmem)
{
    if (void *p = GlobalLock(hmem))
    {
        SecureZeroMemory(p, GlobalSize(hmem));
        GlobalUnlock(hmem);
    }
    GlobalFree(hmem);
}

/**
 * Place a DWORD value on the clipboard in the registered format `format_name`.
 * These formats are hints to clipboard history and cloud clipboard,
 * so errors are ignored.
 */
static void SetClipboardDword(const wchar_t *format_name, DWORD value)
{
    UINT format = RegisterClipboardFormatW(format_name);
    if (format == 0)
        return;

    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
    if (hmem == nullptr)
        return;
    DWORD *p = static_cast<DWORD *>(GlobalLock(hmem));
    if (p == nullptr)
    {
        GlobalFree(hmem);
        return;
    }
    *p = value;
    GlobalUnlock(hmem);

    if (SetClipboardData(format, hmem) == nullptr)
    {
        GlobalFree(hmem);
    }
}

/** Another application may have the clipboard open briefly, retry before giving up */
static bool OpenClipboardRetry(HWND hwnd)
{
    constexpr int ATTEMPTS = 5;
    constexpr DWORD RETRY_DELAY_MS = 20;
    for (int i = 0; i < ATTEMPTS; ++i)
    {
        if (OpenClipboard(hwnd))
            return true;
        Sleep(RETRY_DELAY_MS);
    }
    return false;
}

/**
 * Convert `text` to a null-terminated UTF-16 string in global memory.
 * Text is expected to be UTF-8; if it isn't, it is converted from the ANSI code page,
 * which is the encoding of fields in older Password Safe databases.
 * @returns `nullptr` on error, and sets `rc`
 */
static HGLOBAL ToUtf16Global(const std::string &text, int &rc)
{
    if (text.size() > static_cast<size_t>(INT_MAX))
    {
        rc = 1;
        return nullptr;
    }
    const int len = static_cast<int>(text.size());

    UINT codepage = CP_UTF8;
    int wlen = 0;
    if (len > 0)
    {
        wlen = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, text.data(), len, nullptr, 0);
        if (wlen == 0)
        {
            codepage = CP_ACP;
            wlen = MultiByteToWideChar(codepage, 0, text.data(), len, nullptr, 0);
        }
        if (wlen == 0)
        {
            rc = 1;
            return nullptr;
        }
    }

    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, (static_cast<SIZE_T>(wlen) + 1) * sizeof(wchar_t));
    if (hmem == nullptr)
    {
        rc = 2;
        return nullptr;
    }
    wchar_t *buf = static_cast<wchar_t *>(GlobalLock(hmem));
    if (buf == nullptr)
    {
        GlobalFree(hmem);
        rc = 2;
        return nullptr;
    }
    if (wlen > 0 && MultiByteToWideChar(codepage, 0, text.data(), len, buf, wlen) != wlen)
    {
        GlobalUnlock(hmem);
        FreeGlobal(hmem);
        rc = 1;
        return nullptr;
    }
    buf[wlen] = L'\0';
    GlobalUnlock(hmem);

    return hmem;
}

int CopyTextToClipboardWin(const std::string &text)
{
    int rc = 0;
    HGLOBAL hmem = ToUtf16Global(text, rc);
    if (hmem == nullptr)
        return rc;

    // The clipboard must be opened with a window for SetClipboardData() to succeed
    // after EmptyClipboard(). A console program has no window of its own, use a
    // message-only window. Data placed on the clipboard remains after it is destroyed.
    HWND hwnd = CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);

    if (!OpenClipboardRetry(hwnd))
    {
        FreeGlobal(hmem);
        rc = 3;
    }
    else
    {
        if (!EmptyClipboard() || SetClipboardData(CF_UNICODETEXT, hmem) == nullptr)
        {
            // The clipboard didn't take ownership of the memory
            FreeGlobal(hmem);
            rc = 4;
        }
        else
        {
            // Keep the text out of clipboard history (Win+V) and the cloud clipboard, see
            // https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats#cloud-clipboard-and-clipboard-history-formats
            SetClipboardDword(L"ExcludeClipboardContentFromMonitorProcessing", 0);
            SetClipboardDword(L"CanIncludeInClipboardHistory", 0);
            SetClipboardDword(L"CanUploadToCloudClipboard", 0);
        }
        CloseClipboard();
    }

    if (hwnd != nullptr)
    {
        DestroyWindow(hwnd);
    }

    return rc;
}

#endif // #ifdef _WIN32
