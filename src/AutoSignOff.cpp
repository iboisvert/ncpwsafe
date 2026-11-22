/* Copyright 2026 Ian Boisvert */
#include "AutoSignOff.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <time.h>
#endif

int64_t IdleClockMs()
{
#ifdef _WIN32
    // Counts while the computer sleeps or hibernates
    return static_cast<int64_t>(GetTickCount64());
#else
#ifdef CLOCK_BOOTTIME
    // Unlike CLOCK_MONOTONIC on Linux, counts while the computer is suspended
    const clockid_t clock_id = CLOCK_BOOTTIME;
#else
    const clockid_t clock_id = CLOCK_MONOTONIC;
#endif
    struct timespec ts;
    if (clock_gettime(clock_id, &ts) != 0)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
    }
    return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
#endif
}

IdleTimer::IdleTimer(long period_seconds, Clock clock) : clock_(std::move(clock))
{
    SetPeriod(period_seconds);
}

void IdleTimer::SetPeriod(long period_seconds)
{
    period_ms_ = period_seconds > 0 ? static_cast<int64_t>(period_seconds) * 1000 : 0;
    Touch();
}

void IdleTimer::Touch()
{
    last_input_ms_ = clock_();
}

bool IdleTimer::Expired() const
{
    return Enabled() && clock_() - last_input_ms_ >= period_ms_;
}

int64_t IdleTimer::RemainingMs() const
{
    if (!Enabled())
    {
        return 0;
    }
    return std::max<int64_t>(0, period_ms_ - (clock_() - last_input_ms_));
}

bool ParseAutoSignOff(const std::string &value, long &seconds)
{
    // Trim spaces
    const size_t begin = value.find_first_not_of(" \t");
    if (begin == std::string::npos)
    {
        return false;
    }
    const size_t end = value.find_last_not_of(" \t") + 1;
    const std::string text = value.substr(begin, end - begin);

    size_t ndigits = 0;
    while (ndigits < text.size() && std::isdigit(static_cast<unsigned char>(text[ndigits])))
    {
        ++ndigits;
    }
    if (ndigits == 0 || ndigits > 9)
    {
        // No number, a sign, or a number too large to be useful
        return false;
    }
    const long number = std::strtol(text.substr(0, ndigits).c_str(), nullptr, 10);

    // An optional unit, which may follow a space
    size_t unit_pos = text.find_first_not_of(" \t", ndigits);
    long multiplier = 60;
    if (unit_pos != std::string::npos)
    {
        if (unit_pos + 1 != text.size())
        {
            return false;
        }
        switch (std::tolower(static_cast<unsigned char>(text[unit_pos])))
        {
        case 's':
            multiplier = 1;
            break;
        case 'm':
            multiplier = 60;
            break;
        case 'h':
            multiplier = 60 * 60;
            break;
        default:
            return false;
        }
    }
    if (number > LONG_MAX / multiplier)
    {
        return false;
    }
    seconds = number * multiplier;
    return true;
}

std::string FormatDuration(long seconds)
{
    auto plural = [](long n, const char *unit) {
        return std::to_string(n) + " " + unit + (n == 1 ? "" : "s");
    };
    if (seconds > 0 && seconds % 3600 == 0)
    {
        return plural(seconds / 3600, "hour");
    }
    if (seconds > 0 && seconds % 60 == 0)
    {
        return plural(seconds / 60, "minute");
    }
    return plural(seconds, "second");
}

static IdleTimer *key_idle_timer = nullptr;

void SetKeyIdleTimer(IdleTimer *timer)
{
    key_idle_timer = timer;
}

IdleTimer *GetKeyIdleTimer()
{
    return key_idle_timer;
}
