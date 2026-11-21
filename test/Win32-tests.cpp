/* Copyright 2026 Ian Boisvert */
#ifdef _WIN32

#include <cstdlib>
#include <optional>
#include <string>
#include <gtest/gtest.h>
#include "TestUtils.h"
#include "Win32.h"

/** Puts back the `APPDATA` environment variable after each test */
class RoamingAppDataDirTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const char *appdata = std::getenv("APPDATA");
        if (appdata) saved_appdata_ = appdata;
    }
    void TearDown() override
    {
        // An empty value removes the variable
        SetEnv("APPDATA", saved_appdata_ ? saved_appdata_->c_str() : "");
    }

    std::optional<std::string> saved_appdata_;
};

TEST_F(RoamingAppDataDirTest, UsesAppDataVariable)
{
    SetEnv("APPDATA", "C:\\ncpwsafe-test\\AppData\\Roaming");
    EXPECT_EQ("C:\\ncpwsafe-test\\AppData\\Roaming", win32::RoamingAppDataDir());
}

TEST_F(RoamingAppDataDirTest, FallsBackToKnownFolderWithoutAppDataVariable)
{
    SetEnv("APPDATA", "");
    ASSERT_EQ(nullptr, std::getenv("APPDATA"));
    std::string dir = win32::RoamingAppDataDir();
    EXPECT_FALSE(dir.empty());
    EXPECT_NE(std::string::npos, dir.find("Roaming")) << dir;
}

#endif  // #ifdef _WIN32
