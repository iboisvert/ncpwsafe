/* Copyright 2023 Ian Boisvert */

#include "FileUtils.h"
#include <sys/stat.h>

bool FileExists(const char *pathname)
{
#ifndef _WINDOWS
    struct stat statbuf;
    int ret = stat(pathname, &statbuf);
    return ret == 0;
#else
    #error FileExists() not implemented
#endif
}

size_t FileLength(const char *pathname)
{
#ifndef _WINDOWS
    struct stat statbuf;
    int ret = stat(pathname, &statbuf);
    if (ret == 0)
    {
        return statbuf.st_size;
    }

    return 0;
#else
    #error FileExists() not implemented
#endif
}
