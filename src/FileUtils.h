/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_FILEUTILS_H
#define HAVE_FILEUTILS_H

extern bool FileExists(const char *pathname);
bool FileExists(const std::string &pathname)
{
    return FileExists(pathname.c_str());
}
extern size_t FileLength(const char *pathname);
extern bool CopyFile(const char *src_pathname, const char* dst_pathname);
extern bool MoveFile(const char *src_pathname, const char* dst_pathname);

#endif