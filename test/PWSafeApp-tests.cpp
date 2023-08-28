/* Copyright 2023 Ian Boisvert */
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <stdlib.h>

#include "PWSafeApp.h"
#include "Filesystem.h"

struct FilesystemMock : public Filesystem
{
    std::filesystem::path canonical_retval;
    mutable std::filesystem::path canonical_arg;
    std::filesystem::path Canonical(const std::filesystem::path &arg) const
    {
        canonical_arg = arg;
        return canonical_retval;
    }

    mutable std::vector<std::tuple<std::filesystem::path, std::filesystem::path> > copy_args;
    void Copy(const std::filesystem::path &arg1, const std::filesystem::path &arg2) const
    {
        copy_args.push_back(std::make_tuple(arg1, arg2));
        // Empty
    }

    bool exists_retval;
    mutable std::filesystem::path exists_arg;
    bool Exists(const std::filesystem::path &arg) const
    {
        exists_arg = arg;
        return exists_retval;
    }
};

// IMB 2023-07-02 These tests will probably fail on Windows

TEST(AppTest, TestBackupDb_DstDoesntExist)
{
    setenv("TZ", "UTC0", 1);

    FilesystemMock fs_mock;
    std::string db_file("/foo/pwsafe.dat");

    PWSafeApp app(fs_mock);
    app.args_.database_ = db_file;

    fs_mock.canonical_retval = db_file;
    fs_mock.exists_retval = false;

    ASSERT_EQ(ResultCode::RC_SUCCESS, app.BackupDb());
    ASSERT_EQ(0, strncmp("/foo/pwsafe-", std::get<1>(fs_mock.copy_args[0]).string().c_str(), 7));
    ASSERT_EQ(0, strncmp(".dat", std::get<1>(fs_mock.copy_args[0]).string().c_str()+27, 4));
}

TEST(AppTest, TestBackupDb_DstDoesExist)
{
    setenv("TZ", "UTC0", 1);

    FilesystemMock fs_mock;
    std::string db_file("/foo/pwsafe.dat");

    PWSafeApp app(fs_mock);
    app.args_.database_ = db_file;

    fs_mock.canonical_retval = db_file;
    fs_mock.exists_retval = true;

    ASSERT_EQ(ResultCode::RC_SUCCESS, app.BackupDb());
    ASSERT_EQ(0, strncmp("/foo/pwsafe-", std::get<1>(fs_mock.copy_args[0]).string().c_str(), 7));
    ASSERT_EQ(0, strncmp(".dat", std::get<1>(fs_mock.copy_args[0]).string().c_str()+27, 4));
}