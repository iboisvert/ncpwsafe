/* Copyright 2026 Ian Boisvert */
#ifndef HAVE_TESTUTILS_H
#define HAVE_TESTUTILS_H

#include <cstdlib>
#include <ctime>

/** Portable `setenv(name, value, 1)` */
inline int SetEnv(const char *name, const char *value)
{
#ifdef _WIN32
    return _putenv_s(name, value);
#else
    return setenv(name, value, /*overwrite*/ 1);
#endif
}

/** Set the `TZ` environment variable and reinitialize the C runtime time zone */
inline void SetTimeZone(const char *tz)
{
    SetEnv("TZ", tz);
#ifdef _WIN32
    _tzset();
#else
    tzset();
#endif
}

#endif  // #ifndef HAVE_TESTUTILS_H
