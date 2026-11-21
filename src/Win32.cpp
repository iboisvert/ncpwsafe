/* Copyright 2026 Ian Boisvert */
#ifdef _WIN32

#include "Win32.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <climits>
#include <clocale>
#include <cstdint>
#include <cstdlib>

namespace win32
{

static UINT saved_input_cp = 0;
static UINT saved_output_cp = 0;

static void RestoreConsole()
{
    if (saved_input_cp != 0)
        SetConsoleCP(saved_input_cp);
    if (saved_output_cp != 0)
        SetConsoleOutputCP(saved_output_cp);
}

void InitConsole()
{
    // The console code pages are a property of the console, not the process,
    // so they are restored on exit to avoid changing the parent shell's console
    saved_input_cp = GetConsoleCP();
    saved_output_cp = GetConsoleOutputCP();
    if (saved_input_cp != 0 || saved_output_cp != 0)
    {
        std::atexit(RestoreConsole);
    }
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    // UCRT supports UTF-8 locales since Windows 10 1803
    std::setlocale(LC_ALL, ".UTF-8");
}

/** Unhandled exception filter: ends the process before WER can write a crash dump */
static LONG WINAPI TerminateOnUnhandledException(EXCEPTION_POINTERS *info)
{
    TerminateProcess(GetCurrentProcess(), info->ExceptionRecord->ExceptionCode);
    return EXCEPTION_EXECUTE_HANDLER;
}

/** Invalid C runtime parameter handler: ends the process instead of reporting to WER */
static void __cdecl TerminateOnInvalidParameter(const wchar_t *, const wchar_t *, const wchar_t *,
                                                unsigned int, uintptr_t)
{
    // STATUS_INVALID_CRUNTIME_PARAMETER, the exit code the default handler uses
    TerminateProcess(GetCurrentProcess(), 0xC0000417);
}

void DisableCrashDumps()
{
    SetUnhandledExceptionFilter(TerminateOnUnhandledException);
    _set_invalid_parameter_handler(TerminateOnInvalidParameter);
    // abort() raises SIGABRT and exits, without the WER fault report
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    // No "ncpwsafe has stopped working" dialog
    SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);
}

/** Place a DWORD value on the (open) clipboard in a registered format */
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

    // The system owns the memory if SetClipboardData succeeds
    if (SetClipboardData(format, hmem) == nullptr)
    {
        GlobalFree(hmem);
    }
}

/** Zero and free clipboard memory that was not handed to the system */
static void FreeSecret(HGLOBAL hmem, size_t size)
{
    void *p = GlobalLock(hmem);
    if (p != nullptr)
    {
        SecureZeroMemory(p, size);
        GlobalUnlock(hmem);
    }
    GlobalFree(hmem);
}

int CopyTextToClipboard(const std::string &text)
{
    if (text.size() > static_cast<size_t>(INT_MAX))
        return CLIPBOARD_ERR_ENCODING;
    const int text_len = static_cast<int>(text.size());

    int wlen = 0;
    if (text_len > 0)
    {
        wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), text_len, nullptr, 0);
        if (wlen == 0)
            return CLIPBOARD_ERR_ENCODING;
    }

    const size_t size = (static_cast<size_t>(wlen) + 1) * sizeof(wchar_t);
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (hmem == nullptr)
        return CLIPBOARD_ERR_ALLOC;

    wchar_t *wtext = static_cast<wchar_t *>(GlobalLock(hmem));
    if (wtext == nullptr)
    {
        GlobalFree(hmem);
        return CLIPBOARD_ERR_ALLOC;
    }
    if (wlen > 0)
    {
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), text_len, wtext, wlen);
    }
    wtext[wlen] = L'\0';
    GlobalUnlock(hmem);

    // A console process has no window of its own, and a clipboard opened
    // without an owner window cannot reliably be written to,
    // so use a message-only window as the clipboard owner
    HWND hwnd = CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                GetModuleHandleW(nullptr), nullptr);

    // Another process may briefly hold the clipboard open, so retry
    BOOL opened = FALSE;
    for (int attempt = 0; attempt < 10 && !opened; ++attempt)
    {
        opened = OpenClipboard(hwnd);
        if (!opened)
            Sleep(20);
    }
    if (!opened)
    {
        FreeSecret(hmem, size);
        if (hwnd != nullptr)
            DestroyWindow(hwnd);
        return CLIPBOARD_ERR_OPEN;
    }

    int result = CLIPBOARD_OK;
    EmptyClipboard();
    if (SetClipboardData(CF_UNICODETEXT, hmem) == nullptr)
    {
        FreeSecret(hmem, size);
        result = CLIPBOARD_ERR_SET;
    }
    else
    {
        // Keep secrets out of clipboard history, cloud clipboard and clipboard managers
        // https://learn.microsoft.com/windows/win32/dataxchg/clipboard-formats#cloud-clipboard-and-clipboard-history-formats
        SetClipboardDword(L"ExcludeClipboardContentFromMonitorProcessing", 0);
        SetClipboardDword(L"CanIncludeInClipboardHistory", 0);
        SetClipboardDword(L"CanUploadToCloudClipboard", 0);
    }

    CloseClipboard();
    if (hwnd != nullptr)
        DestroyWindow(hwnd);
    return result;
}

}  // namespace win32

#endif  // #ifdef _WIN32
