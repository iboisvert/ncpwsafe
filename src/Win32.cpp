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
#include <aclapi.h>
#include <sddl.h>
#include <shlobj.h>

#include <cerrno>
#include <climits>
#include <clocale>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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

static LPTOP_LEVEL_EXCEPTION_FILTER previous_exception_filter = nullptr;

/** Unhandled exception filter: restores the console, then passes the exception on */
static LONG WINAPI RestoreConsoleOnUnhandledException(EXCEPTION_POINTERS *info)
{
    RestoreConsole();
    return previous_exception_filter ? previous_exception_filter(info) : EXCEPTION_CONTINUE_SEARCH;
}

/** `SIGABRT` handler: `abort()` ends the process with `_exit()`, which skips `atexit` */
static void __cdecl RestoreConsoleOnAbort(int)
{
    RestoreConsole();
}

/** Console control handler for Ctrl+C, Ctrl+Break, closing the console, logoff and shutdown */
static BOOL WINAPI RestoreConsoleOnCtrlEvent(DWORD)
{
    RestoreConsole();
    // Not handled, so the default handler ends the process
    return FALSE;
}

void InitConsole()
{
    // The console code pages are a property of the console, not the process,
    // so they are restored on exit to avoid changing the parent shell's console,
    // also when the process ends without running atexit handlers
    saved_input_cp = GetConsoleCP();
    saved_output_cp = GetConsoleOutputCP();
    if (saved_input_cp != 0 || saved_output_cp != 0)
    {
        std::atexit(RestoreConsole);
        std::signal(SIGABRT, RestoreConsoleOnAbort);
        SetConsoleCtrlHandler(RestoreConsoleOnCtrlEvent, TRUE);
        previous_exception_filter = SetUnhandledExceptionFilter(RestoreConsoleOnUnhandledException);
    }
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    // UCRT supports UTF-8 locales since Windows 10 1803.
    // Only LC_CTYPE, as on other platforms, so that e.g. number formats don't change
    std::setlocale(LC_CTYPE, ".UTF-8");
}

/** Unhandled exception filter: ends the process before WER can write a crash dump */
static LONG WINAPI TerminateOnUnhandledException(EXCEPTION_POINTERS *info)
{
    RestoreConsole();
    TerminateProcess(GetCurrentProcess(), info->ExceptionRecord->ExceptionCode);
    return EXCEPTION_EXECUTE_HANDLER;
}

/** Invalid C runtime parameter handler: ends the process instead of reporting to WER */
static void __cdecl TerminateOnInvalidParameter(const wchar_t *, const wchar_t *, const wchar_t *,
                                                unsigned int, uintptr_t)
{
    RestoreConsole();
    // STATUS_INVALID_CRUNTIME_PARAMETER, the exit code the default handler uses
    TerminateProcess(GetCurrentProcess(), 0xC0000417);
}

void DisableCrashDumps()
{
    // Replaces the filter set by InitConsole(), and restores the console itself
    SetUnhandledExceptionFilter(TerminateOnUnhandledException);
    _set_invalid_parameter_handler(TerminateOnInvalidParameter);
    // abort() raises SIGABRT and exits, without the WER fault report
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    // No "ncpwsafe has stopped working" dialog
    SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);
}

/** The SID of the user the process runs as, in SDDL string form, or an empty string */
static std::wstring CurrentUserSid()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return std::wstring();
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<BYTE> buf(size);
    BOOL ok = size > 0 && GetTokenInformation(token, TokenUser, buf.data(), size, &size);
    CloseHandle(token);
    if (!ok)
        return std::wstring();

    std::wstring sid;
    LPWSTR str = nullptr;
    if (ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(buf.data())->User.Sid, &str))
    {
        sid = str;
        LocalFree(str);
    }
    return sid;
}

bool RestrictProcessAccess()
{
    std::wstring user_sid = CurrentUserSid();
    if (user_sid.empty())
        return false;

    // D:P   A DACL that doesn't inherit ACEs
    // SY    SYSTEM: all access
    // user  SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE:
    //       wait for, query and end the process, but not PROCESS_VM_READ etc.
    // OW    OWNER RIGHTS: only READ_CONTROL. Without it the owner implicitly has
    //       WRITE_DAC, and could give itself all access again
    std::wstring sddl = L"D:P(A;;GA;;;SY)(A;;0x00101001;;;" + user_sid + L")(A;;RC;;;OW)";

    PSECURITY_DESCRIPTOR sd = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &sd, nullptr))
        return false;

    DWORD rc = ERROR_INVALID_SECURITY_DESCR;
    BOOL present = FALSE, defaulted = FALSE;
    PACL dacl = nullptr;
    if (GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted) && present)
    {
        // The GetCurrentProcess() pseudo-handle has all access, including WRITE_DAC
        rc = SetSecurityInfo(GetCurrentProcess(), SE_KERNEL_OBJECT,
                             DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                             nullptr, nullptr, dacl, nullptr);
    }
    LocalFree(sd);
    return rc == ERROR_SUCCESS;
}

/** Convert a NUL-terminated UTF-16 string to UTF-8 */
static std::string ToUtf8(const wchar_t *wstr)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1)
        return std::string();
    std::string str(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str.data(), len, nullptr, nullptr);
    str.resize(static_cast<size_t>(len) - 1);  // Without the NUL
    return str;
}

std::string RoamingAppDataDir()
{
    const char *appdata = std::getenv("APPDATA");
    if (appdata && *appdata)
        return appdata;

    std::string dir;
    PWSTR wdir = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &wdir)))
    {
        dir = ToUtf8(wdir);
    }
    // Must be freed even if SHGetKnownFolderPath fails
    CoTaskMemFree(wdir);
    return dir;
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

/** The console input that `ReadConsolePassword()` turned echo off for, and its mode before */
static HANDLE password_input = INVALID_HANDLE_VALUE;
static DWORD password_input_mode = 0;

/**
 * Console control handler while a password is read: turns echo back on,
 * then lets the next handler (in the end, the default one) end the process
 */
static BOOL WINAPI RestoreEchoOnCtrlEvent(DWORD)
{
    HANDLE input = password_input;
    if (input != INVALID_HANDLE_VALUE)
        SetConsoleMode(input, password_input_mode);
    return FALSE;
}

/** Overwrite the contents of `str` with zeros, then clear it */
static void ZeroWideString(std::wstring &str)
{
    if (!str.empty())
        SecureZeroMemory(&str[0], str.size() * sizeof(wchar_t));
    str.clear();
}

/** Append `wc` to `str`. When `str` has to grow, its old buffer is zeroed rather than left on the heap */
static void AppendSecret(std::wstring &str, wchar_t wc)
{
    if (str.size() == str.capacity())
    {
        std::wstring larger;
        larger.reserve(str.capacity() * 2);
        larger.append(str);
        ZeroWideString(str);
        str.swap(larger);
    }
    str.push_back(wc);
}

/** Open a console buffer (`CONIN$` or `CONOUT$`) for reading and writing */
static HANDLE OpenConsole(const wchar_t *name)
{
    return CreateFileW(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                       OPEN_EXISTING, 0, nullptr);
}

PasswordPromptResult ReadConsolePassword(const char *prompt, std::string &password)
{
    HANDLE input = OpenConsole(L"CONIN$");
    if (input == INVALID_HANDLE_VALUE)
        return PasswordPromptResult::NO_TERMINAL;
    DWORD mode = 0;
    HANDLE output = INVALID_HANDLE_VALUE;
    if (!GetConsoleMode(input, &mode) || (output = OpenConsole(L"CONOUT$")) == INVALID_HANDLE_VALUE)
    {
        CloseHandle(input);
        return PasswordPromptResult::NO_TERMINAL;
    }

    password_input_mode = mode;
    password_input = input;
    // Called before the handlers set earlier, such as InitConsole()'s
    SetConsoleCtrlHandler(RestoreEchoOnCtrlEvent, TRUE);

    PasswordPromptResult result = PasswordPromptResult::FAILED;
    int saved_errno = EIO;
    std::wstring line;
    // Most passwords fit without growing, see AppendSecret()
    line.reserve(256);
    const DWORD noecho = (mode | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT) & ~ENABLE_ECHO_INPUT;
    if (SetConsoleMode(input, noecho))
    {
        // Discard what was typed before the prompt
        FlushConsoleInputBuffer(input);
        DWORD written = 0;
        WriteConsoleA(output, prompt, static_cast<DWORD>(std::strlen(prompt)), &written, nullptr);

        wchar_t buf[64];
        bool eol = false;
        while (!eol)
        {
            DWORD n = 0;
            if (!ReadConsoleW(input, buf, static_cast<DWORD>(sizeof(buf) / sizeof(buf[0])), &n, nullptr))
                break;
            if (n == 0)
            {
                // Ctrl+C or Ctrl+Break: the control handlers end the process
                saved_errno = EINTR;
                break;
            }
            // A line ends with CR LF. The rest of the line, past the end of `buf`,
            // is returned by the next read, so read up to the LF
            for (DWORD i = 0; i < n && !eol; ++i)
            {
                if (buf[i] == L'\n')
                    eol = true;
                else if (buf[i] != L'\r')
                    AppendSecret(line, buf[i]);
            }
        }
        SecureZeroMemory(buf, sizeof(buf));
        // Enter wasn't echoed either
        WriteConsoleA(output, "\r\n", 2, &written, nullptr);
        SetConsoleMode(input, mode);

        if (eol && line == L"\x1a")
        {
            // Ctrl+Z, end of input
            result = PasswordPromptResult::CANCELLED;
        }
        else if (eol && line.size() <= static_cast<size_t>(INT_MAX))
        {
            const int wlen = static_cast<int>(line.size());
            int len = wlen == 0 ? 0 : WideCharToMultiByte(CP_UTF8, 0, line.data(), wlen, nullptr, 0, nullptr, nullptr);
            if (wlen == 0 || len > 0)
            {
                std::string value(static_cast<size_t>(len), '\0');
                if (len > 0)
                    WideCharToMultiByte(CP_UTF8, 0, line.data(), wlen, &value[0], len, nullptr, nullptr);
                password.swap(value);
                // Now the previous value of `password`
                if (!value.empty())
                    SecureZeroMemory(&value[0], value.size());
                result = PasswordPromptResult::OK;
            }
        }
    }

    SetConsoleCtrlHandler(RestoreEchoOnCtrlEvent, FALSE);
    password_input = INVALID_HANDLE_VALUE;
    ZeroWideString(line);
    CloseHandle(output);
    CloseHandle(input);
    if (result == PasswordPromptResult::FAILED)
        errno = saved_errno;
    return result;
}

}  // namespace win32

#endif  // #ifdef _WIN32
