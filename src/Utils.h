/* Copyright 2022 Ian Boisvert */
#pragma once

#include "core/StringX.h"
#include "core/UTF8Conv.h"
#include "libncurses.h"
#include <algorithm>
#include <array>
#include <iterator>

enum class DialogResult
{
    OK = 0,
    CANCEL,
    CONTINUE,
    YES,
    NO,
};

constexpr char KEY_ESC = 0x1B;
inline constexpr char KEY_CTRL(char c)
{
    return c & 0x1f;
}

char *WideToMultibyteString(const wchar_t *src, char *dst, size_t dst_len);
/**
 * Convert wide to multibyte character string.
 * Caller must `delete` allocated character string.
 */
char *WideToMultibyteString(const wchar_t *src, char **pdst);
template <class T> inline char *WideToMultibyteString(const T &src, char **pdst)
{
    return WideToMultibyteString(src.data(), pdst);
}
void WideToMultibyteString(const wchar_t *src, cstringT &dst);
template <class T> inline void WideToMultibyteString(const T &src, cstringT &dst)
{
    WideToMultibyteString(src.c_str(), dst);
}
template <class T> inline cstringT WideToMultibyteString(const T &src)
{
    cstringT dst;
    WideToMultibyteString(src, dst);
    return dst;
}

template <class T> inline T MultibyteToWideString(const char *src)
{
    assert(src);
    size_t wclen = mbstowcs(NULL, src, 0);
    T dst;
    dst.resize(wclen);
    __attribute__((unused)) size_t count = mbstowcs(dst.data(), src, wclen);
    assert(count == wclen);
    return dst;
}

extern StringX Utf8ToWideString(const char *src);
inline StringX Utf8ToWideString(const cstringT &src)
{
    return Utf8ToWideString(src.c_str());
}
extern bool Utf8ToWideString(const char *src, unsigned src_len, StringX &dst);
extern bool Utf8ToWideString(const cstringT &src, StringX &dst);

/**
 * Randomize the 0 buffer of curses fields
 * @param fields A null-terminated array of curses `FIELD`s
 */
void RandomizeFieldsBuffer(FIELD **fields);

/** Key handler for `MessageBox`. */
inline bool YesNoKeyHandler(int ch, DialogResult &result)
{
    ch = tolower(ch);
    if (ch == 'y')
    {
        result = DialogResult::YES;
        return true;
    }
    else if (ch == 'n')
    {
        result = DialogResult::NO;
        return true;
    }
    return false;
}

constexpr std::array<char, 4> WS{' ', '\n', '\r', '\t'};
template <typename T> inline T rtrim(T begin, T end)
{
    typedef typename std::iterator_traits<T>::value_type value_type;
    value_type c;
    while (end >= begin && (c = *--end),
        std::any_of(WS.begin(), WS.end(), [c](typename decltype(WS)::value_type ws) { return (int)c == (int)ws; }))
        *end = 0;
    return begin;
}
