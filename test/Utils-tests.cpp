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
        char *result = rtrim(buf, buf+strlen(buf));
        ASSERT_STREQ("", result);
    }
    {
        char buf[] = "abc  ";
        char *result = rtrim(buf, buf+strlen(buf));
        ASSERT_STREQ("abc", result);
    }
}