/* Copyright 2023 Ian Boisvert */
#include <gtest/gtest.h>
#include "TestUtils.h"

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

TEST(AppTest, TestBackupDb_DstDoesntExist)
{
    SetTimeZone("UTC0");

    FilesystemMock fs_mock;
    std::string db_file("/foo/pwsafe.dat");

    PWSafeApp app(fs_mock);
    app.args_.database_ = db_file;

    fs_mock.canonical_retval = db_file;
    fs_mock.exists_retval = false;

    ASSERT_EQ(ResultCode::RC_SUCCESS, app.BackupDb());
    // generic_string(): MSVC's std::filesystem joins paths with '\\'
    std::string backup = std::get<1>(fs_mock.copy_args[0]).generic_string();
    ASSERT_EQ(0, strncmp("/foo/pwsafe-", backup.c_str(), 12));
    ASSERT_EQ(0, strncmp(".dat", backup.c_str()+27, 4));
}

TEST(AppTest, TestBackupDb_DstDoesExist)
{
    SetTimeZone("UTC0");

    FilesystemMock fs_mock;
    std::string db_file("/foo/pwsafe.dat");

    PWSafeApp app(fs_mock);
    app.args_.database_ = db_file;

    fs_mock.canonical_retval = db_file;
    fs_mock.exists_retval = true;

    ASSERT_EQ(ResultCode::RC_SUCCESS, app.BackupDb());
    // generic_string(): MSVC's std::filesystem joins paths with '\\'
    std::string backup = std::get<1>(fs_mock.copy_args[0]).generic_string();
    ASSERT_EQ(0, strncmp("/foo/pwsafe-", backup.c_str(), 12));
    ASSERT_EQ(0, strncmp(".dat", backup.c_str()+27, 4));
}