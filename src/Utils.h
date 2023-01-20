/* Copyright 2022 Ian Boisvert */
#pragma once

#include "libncurses.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <cassert>

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
