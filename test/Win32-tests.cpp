/* Copyright 2026 Ian Boisvert */
#ifdef _WIN32

#include <windows.h>
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

/** `true` if `SeDebugPrivilege` is enabled, which bypasses the process DACL */
static bool DebugPrivilegeEnabled()
{
    LUID luid;
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid))
        return false;
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    PRIVILEGE_SET privileges{1, PRIVILEGE_SET_ALL_NECESSARY, {{luid, 0}}};
    BOOL enabled = FALSE;
    PrivilegeCheck(token, &privileges, &enabled);
    CloseHandle(token);
    return enabled != FALSE;
}

// Changes the DACL of the test process; CTest runs each test in its own process
TEST(RestrictProcessAccessTest, DeniesMemoryAccessToTheUser)
{
    if (DebugPrivilegeEnabled()) GTEST_SKIP() << "SeDebugPrivilege is enabled";

    ASSERT_TRUE(win32::RestrictProcessAccess());

    // Opening the process by ID is access checked, unlike the GetCurrentProcess() pseudo-handle
    const DWORD pid = GetCurrentProcessId();
    HANDLE h = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    DWORD error = GetLastError();
    EXPECT_EQ(nullptr, h);
    EXPECT_EQ(static_cast<DWORD>(ERROR_ACCESS_DENIED), error);
    if (h) CloseHandle(h);

    h = OpenProcess(WRITE_DAC, FALSE, pid);
    EXPECT_EQ(nullptr, h) << "the owner can change the DACL back";
    if (h) CloseHandle(h);

    h = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    EXPECT_NE(nullptr, h);
    if (h) CloseHandle(h);
}

#endif  // #ifdef _WIN32
