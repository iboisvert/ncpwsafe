/* Copyright 2023 Ian Boisvert */
#include <string>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
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
class ReadPasswordFileTest : public ::testing::Test
{
protected:
    std::string pathname_;

    void SetUp() override
    {
        char tmpl[] = "/tmp/ncpwsafe-pwfile-XXXXXX";
        int fd = mkstemp(tmpl);
        ASSERT_NE(-1, fd);
        close(fd);
        pathname_ = tmpl;
    }
    void TearDown() override
    {
        unlink(pathname_.c_str());
    }
    void Write(const std::string &contents)
    {
        std::FILE *f = std::fopen(pathname_.c_str(), "wb");
        ASSERT_NE(nullptr, f);
        std::fwrite(contents.data(), 1, contents.size(), f);
        std::fclose(f);
    }
};

TEST_F(ReadPasswordFileTest, ReadsFileWithoutLineEnding)
{
    Write("s3cret");
    std::string password;
    ASSERT_TRUE(ReadPasswordFile(pathname_, password));
    EXPECT_EQ("s3cret", password);
}

TEST_F(ReadPasswordFileTest, RemovesLfAndCrLf)
{
    std::string password;
    Write("s3cret\n");
    ASSERT_TRUE(ReadPasswordFile(pathname_, password));
    EXPECT_EQ("s3cret", password);
    Write("s3cret\r\n");
    ASSERT_TRUE(ReadPasswordFile(pathname_, password));
    EXPECT_EQ("s3cret", password);
}

TEST_F(ReadPasswordFileTest, ReadsFirstLineOnly)
{
    Write("s3cret\nsecond line\n");
    std::string password;
    ASSERT_TRUE(ReadPasswordFile(pathname_, password));
    EXPECT_EQ("s3cret", password);
}

TEST_F(ReadPasswordFileTest, KeepsLeadingAndTrailingSpaces)
{
    Write(" s3 cret \n");
    std::string password;
    ASSERT_TRUE(ReadPasswordFile(pathname_, password));
    EXPECT_EQ(" s3 cret ", password);
}

TEST_F(ReadPasswordFileTest, ReadsPasswordLongerThanBuffer)
{
    std::string long_password(1000, 'x');
    Write(long_password + "\n");
    std::string password;
    ASSERT_TRUE(ReadPasswordFile(pathname_, password));
    EXPECT_EQ(long_password, password);
}

TEST_F(ReadPasswordFileTest, EmptyFileGivesEmptyPassword)
{
    Write("");
    std::string password = "old";
    ASSERT_TRUE(ReadPasswordFile(pathname_, password));
    EXPECT_EQ("", password);
}

TEST_F(ReadPasswordFileTest, MissingFileFails)
{
    std::string password = "unchanged";
    EXPECT_FALSE(ReadPasswordFile("/nonexistent/ncpwsafe-password", password));
    EXPECT_EQ(ENOENT, errno);
    EXPECT_EQ("unchanged", password);
}

TEST(UtilTest, TestZeroString)
{
    std::string str("s3cret");
    const char *data = str.data();
    ZeroString(str);
    EXPECT_TRUE(str.empty());
    EXPECT_EQ(0, std::memcmp(data, "\0\0\0\0\0\0", 6));
}

TEST(UtilTest, TestUtf8Length)
{
    EXPECT_EQ(0u, Utf8Length(""));
    EXPECT_EQ(5u, Utf8Length("hello"));
    EXPECT_EQ(5u, Utf8Length("Café!"));
    EXPECT_EQ(4u, Utf8Length("Банк"));
    EXPECT_EQ(3u, Utf8Length("日本語"));
}

using Lines = std::vector<std::string>;

TEST(UtilTest, TestWrapTextFits)
{
    EXPECT_EQ(Lines{}, WrapText("", 10));
    EXPECT_EQ(Lines{"hello"}, WrapText("hello", 10));
    EXPECT_EQ(Lines{"hello"}, WrapText("hello", 5));
    EXPECT_EQ((Lines{"one", "two"}), WrapText("one\ntwo", 10));
    // A newline at the end doesn't add a line, but blank lines in the middle are kept
    EXPECT_EQ(Lines{"one"}, WrapText("one\n", 10));
    EXPECT_EQ((Lines{"one", "", "two"}), WrapText("one\n\ntwo", 10));
}

TEST(UtilTest, TestWrapTextBetweenWords)
{
    EXPECT_EQ((Lines{"Delete", "account", "Bookstore?"}), WrapText("Delete account Bookstore?", 10));
    EXPECT_EQ((Lines{"one two", "three"}), WrapText("one two three", 7));
    EXPECT_EQ((Lines{"one two", "three"}), WrapText("one two three", 8));
    EXPECT_EQ((Lines{"a b c", "d"}), WrapText("a b c d", 5));
}

TEST(UtilTest, TestWrapTextLongWord)
{
    EXPECT_EQ((Lines{"abcd", "efgh", "ij"}), WrapText("abcdefghij", 4));
    EXPECT_EQ((Lines{"File", "/tmp/a/b/c/", "d/e/f.dat", "does not", "exist"}),
              WrapText("File /tmp/a/b/c/d/e/f.dat does not exist", 11));
    // Lines are counted and broken in characters, not bytes
    EXPECT_EQ((Lines{"Сбер", "егат", "ельн", "ый"}), WrapText("Сберегательный", 4));
    EXPECT_EQ((Lines{"Café", "Crème"}), WrapText("Café Crème", 5));
    // A width of 0 is taken as 1
    EXPECT_EQ((Lines{"a", "b"}), WrapText("ab", 0));
}
