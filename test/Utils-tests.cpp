/* Copyright 2023 Ian Boisvert */
#include <string>
#include <cstdlib>
#include <gtest/gtest.h>
#include "Utils.h"


TEST(UtilTest, TestExpandEnvVars)
{
    setenv("FOO", "foo", 1);
    setenv("BAR", "bar", 1);
    ASSERT_STREQ("foo", ExpandEnvVars("${FOO}").c_str());
    ASSERT_STREQ("abcfooxyz", ExpandEnvVars("abc${FOO}xyz").c_str());
    ASSERT_STREQ("abcfoobarxyz", ExpandEnvVars("abc${FOO}${BAR}xyz").c_str());
    ASSERT_STREQ("abcfooxyzbar123", ExpandEnvVars("abc${FOO}xyz${BAR}123").c_str());
    ASSERT_STREQ("abc${FOOxyz", ExpandEnvVars("abc${FOOxyz").c_str());
}

TEST(UtilTest, TestRtrim)
{
    {
        char buf[] = "";
        char *result = rtrim(buf, buf);
        ASSERT_STREQ("", result);
    }
    {
        char buf[] = "";
        char *result = rtrim(buf, buf+1);
        ASSERT_STREQ("", result);
    }
    {
        char buf[] = "  ";
        char *result = rtrim(buf, buf+sizeof(buf));
        ASSERT_STREQ("", result);
    }
    {
        char buf[] = "abc  ";
        char *result = rtrim(buf, buf+sizeof(buf));
        ASSERT_STREQ("abc", result);
    }
    {
        std::string buf{"/workspaces/libpwsafe/test/data/test-v1.dat                "};
        rtrim(buf.begin(), buf.end());
        ASSERT_STREQ("/workspaces/libpwsafe/test/data/test-v1.dat", buf.c_str());
        ASSERT_EQ(43, strlen(buf.c_str()));
        // std::string length is not correct if rtrim is used on iterators
        ASSERT_NE(43, buf.length());
    }
    {
        std::string buf{"/workspaces/libpwsafe/test/data/test-v1.dat                "};
        rtrim(buf);
        ASSERT_STREQ("/workspaces/libpwsafe/test/data/test-v1.dat", buf.c_str());
        ASSERT_EQ(43, strlen(buf.c_str()));
        ASSERT_EQ(43, buf.length());
    }
    {
        char buf[] = " two words \t\n  ";
        char *result = rtrim(buf, buf+sizeof(buf));
        ASSERT_STREQ(" two words", result);
    }
}