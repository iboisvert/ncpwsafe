/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_FILEUTILS_H
#define HAVE_FILEUTILS_H

#include <string>
#include <cstdlib>

extern bool FileExists(const char *pathname);
inline bool FileExists(const std::string &pathname)
{
    return FileExists(pathname.c_str());
}
extern size_t FileLength(const char *pathname);

#endif