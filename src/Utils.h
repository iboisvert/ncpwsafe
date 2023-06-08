/* Copyright 2022 Ian Boisvert */
#pragma once

#include "libncurses.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <cassert>

constexpr char KEY_ESC = 0x1B;
inline constexpr char KEY_CTRL(char c)
{
    return c & 0x1f;
}

/**
 * Overwrite the 0 buffer of curses fields
 * @param fields A null-terminated array of curses `FIELD`s
 */
void ZeroFieldsBuffer(FIELD **fields);

void ZeroMemory(void *p, size_t len);

constexpr std::array<char, 4> WS{' ', '\n', '\r', '\t'};
template <typename T> inline T rtrim(T begin, T end)
{
    typedef typename std::iterator_traits<T>::value_type value_type;
    value_type c;
    while (end > begin && (c = *--end))
    {
        if (std::any_of(WS.begin(), WS.end(), [c](typename decltype(WS)::value_type ws) { return (int)c == (int)ws; }))
        {
            *end = 0;
        }
    }
        
    return begin;
}

/** Expand environment variables in the string */
std::string ExpandEnvVars(const std::string &str);