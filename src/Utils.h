/* Copyright 2022 Ian Boisvert */
#pragma once

#include "libncurses.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

constexpr char KEY_ESC = 0x1B;
inline constexpr char KEY_CTRL(char c)
{
    return c & 0x1f;
}

/**
 * `text` with each character that isn't printable in the `LC_CTYPE` locale, or isn't
 * valid in its character set, replaced by '?'. An empty `text` gives "?".
 */
std::string PrintableText(const char *text);

/** Returned by `ReadKey()` for a character that isn't ASCII */
constexpr int KEY_WIDE_CHAR = KEY_MAX + 1;

/**
 * Read a key from `win` with `wget_wch()`, so a non-ASCII (multibyte) character is read
 * whole rather than byte by byte.
 * @param[out] wch The character, if `KEY_WIDE_CHAR` is returned
 * @returns A function key or ASCII character as `wgetch()` returns it, `KEY_WIDE_CHAR`
 * for any other character, or `ERR`
 */
int ReadKey(WINDOW *win, wchar_t &wch);

/**
 * Overwrite the 0 buffer of curses fields
 * @param fields A null-terminated array of curses `FIELD`s
 */
void ZeroFieldsBuffer(FIELD **fields);

void ZeroMemory(void *p, size_t len);

/** Overwrite the contents of `str` with zeros, then clear it */
void ZeroString(std::string &str);

/**
 * Read a password from the first line of the file `pathname`.
 * A trailing line ending (LF or CR LF) is removed.
 * @returns `false` if the file can't be read, with `errno` set
 */
bool ReadPasswordFile(const std::string &pathname, std::string &password);

constexpr std::array<char, 5> WS{'\0', ' ', '\n', '\r', '\t'};
template <typename It>
inline It rtrim(It begin, It end)
{
    typedef typename std::iterator_traits<It>::value_type value_type;
    value_type c;
    while (end > begin)
    {
        c = *--end;
        if (c)
        {
            if (std::none_of(WS.begin(), WS.end(), [c](typename decltype(WS)::value_type ws)
                             { return (int)c == (int)ws; }))
            {
                break;
            }
            *end = 0;
        }
    }

    return begin;
}

inline std::string &rtrim(std::string &str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(), [](char ch) {
        return std::none_of(WS.begin(), WS.end(), [&ch](char ws) { 
            return (int)ch == (int)ws; 
        });
    }).base(), str.end());
    return str;
}

/**
 * Set the character type locale (`LC_CTYPE`) to the user's locale if its character set is
 * UTF-8, otherwise to a UTF-8 locale such as C.UTF-8.
 *
 * Account text is UTF-8. ncurses needs a UTF-8 character set to show it, and to edit it
 * in a form without changing it.
 * @returns `false` if no UTF-8 locale is available, `LC_CTYPE` is then the user's locale
 */
bool SetUtf8CtypeLocale();

/** Expand environment variables in the string */
std::string ExpandEnvVars(const std::string &str);

/**
 * Number of characters (code points) in UTF-8 text. A character is assumed to
 * take one column on the terminal, which isn't so for wide (e.g. CJK) characters.
 */
size_t Utf8Length(std::string_view text);

/**
 * Number of terminal columns that text takes in the `LC_CTYPE` locale,
 * e.g. 2 for each CJK character. Characters that can't be converted or
 * printed count as one column, as `PrintableText()` shows them as `?`.
 */
int DisplayWidth(std::string_view text);

/**
 * Wrap text into lines of at most `width` characters (see `Utf8Length`).
 * Lines are broken at newlines and between words, and a word longer than
 * `width` (e.g. a long pathname) is broken across lines.
 * A newline at the end of `text` doesn't start another line.
 */
std::vector<std::string> WrapText(const std::string &text, size_t width);
