/* Copyright 2026 Ian Boisvert */
#pragma once

#include <cstdlib>
#include <ctime>

#ifdef _WIN32
/** MinGW-w64 doesn't provide POSIX `setenv()`. Always overwrites. */
inline int setenv(const char *name, const char *value, int /*overwrite*/)
{
    int rc = _putenv_s(name, value);
    // The C runtime only reads TZ when _tzset() is called
    _tzset();
    return rc;
}
#endif
