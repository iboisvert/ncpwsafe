/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_FILESYSTEM_H
#define HAVE_FILESYSTEM_H

#include <filesystem>

namespace fs
{
inline std::filesystem::path Canonical(const std::filesystem::path &pathname)
{
    return std::filesystem::canonical(pathname);
}
inline void Copy(const std::filesystem::path &src, const std::filesystem::path &dst)
{
    std::filesystem::copy(src, dst);
}
inline bool Exists(const std::filesystem::path &pathname)
{
    return std::filesystem::exists(pathname);
}
inline uintmax_t FileSize(const std::filesystem::path &pathname)
{
    return std::filesystem::file_size(pathname);
}
}

// This class exists so it can be mocked in tests
struct Filesystem
{
    static Filesystem instance;

    virtual std::filesystem::path Canonical(const std::filesystem::path &pathname) const
    {
        return fs::Canonical(pathname);
    }
    virtual void Copy(const std::filesystem::path &src, const std::filesystem::path &dst) const
    {
        fs::Copy(src, dst);
    }
    virtual bool Exists(const std::filesystem::path &pathname) const
    {
        return fs::Exists(pathname);
    }
    virtual uintmax_t FileSize(const std::filesystem::path &pathname) const
    {
        return fs::FileSize(pathname);
    }
};

#endif //#define HAVE_FILESYSTEM_H