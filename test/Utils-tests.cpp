/* Copyright 2023 Ian Boisvert */
#include <string>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <filesystem>
#include <optional>
#include <random>
#include <system_error>
#ifndef _WIN32
#include <langinfo.h>
#endif
#include <gtest/gtest.h>
#include "TestUtils.h"
#include "Utils.h"


TEST(UtilTest, TestExpandEnvVars)
{
    SetEnv("FOO", "foo");
    SetEnv("BAR", "bar");
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
        // Portable replacement for mkstemp(), which MSVC doesn't have.
        // "x" fails if the file exists, so an existing file is never truncated
        // (or removed by TearDown()); try again with a new name
        std::random_device rd;
        const std::filesystem::path tmpdir = std::filesystem::temp_directory_path();
        for (int attempt = 0; attempt < 100; ++attempt)
        {
            std::string name = "ncpwsafe-pwfile-" + std::to_string(rd()) + "-" + std::to_string(rd());
            std::string pathname = (tmpdir / name).string();
            errno = 0;
            std::FILE *f = std::fopen(pathname.c_str(), "wbx");
            if (f)
            {
                std::fclose(f);
                pathname_ = pathname;
                // Like mkstemp(), only the owner can read the password file
                std::filesystem::permissions(pathname_, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
                return;
            }
            ASSERT_EQ(EEXIST, errno) << "Unable to create " << pathname << ": " << std::strerror(errno);
        }
        FAIL() << "Unable to create a unique file in " << tmpdir;
    }
    void TearDown() override
    {
        if (!pathname_.empty())
        {
            std::error_code ec;
            std::filesystem::remove(pathname_, ec);
        }
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

// The locale tests use POSIX setenv() and nl_langinfo(); on Windows
// main() sets a UTF-8 locale with win32::InitConsole() instead
#ifndef _WIN32
/** Sets LC_ALL for a test, and puts back the environment and LC_CTYPE afterwards */
class Utf8LocaleTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const char *lc_all = getenv("LC_ALL");
        if (lc_all) saved_lc_all_ = lc_all;
        saved_ctype_ = setlocale(LC_CTYPE, nullptr);
    }
    void TearDown() override
    {
        if (saved_lc_all_) setenv("LC_ALL", saved_lc_all_->c_str(), 1);
        else unsetenv("LC_ALL");
        setlocale(LC_CTYPE, saved_ctype_.c_str());
    }
    static std::string Codeset()
    {
        return nl_langinfo(CODESET);
    }

    std::optional<std::string> saved_lc_all_;
    std::string saved_ctype_;
};

TEST_F(Utf8LocaleTest, KeepsUtf8Locale)
{
    if (!setlocale(LC_CTYPE, "en_US.UTF-8")) GTEST_SKIP() << "en_US.UTF-8 locale is not installed";
    setenv("LC_ALL", "en_US.UTF-8", 1);
    ASSERT_TRUE(SetUtf8CtypeLocale());
    EXPECT_EQ("UTF-8", Codeset());
    EXPECT_STREQ("en_US.UTF-8", setlocale(LC_CTYPE, nullptr));
}

TEST_F(Utf8LocaleTest, ReplacesCLocale)
{
    setenv("LC_ALL", "C", 1);
    if (!SetUtf8CtypeLocale()) GTEST_SKIP() << "no UTF-8 locale is installed";
    EXPECT_EQ("UTF-8", Codeset());
}

TEST_F(Utf8LocaleTest, ReplacesNonUtf8Locale)
{
    if (!setlocale(LC_CTYPE, "en_US.ISO-8859-15")) GTEST_SKIP() << "en_US.ISO-8859-15 locale is not installed";
    setenv("LC_ALL", "en_US.ISO-8859-15", 1);
    if (!SetUtf8CtypeLocale()) GTEST_SKIP() << "no UTF-8 locale is installed";
    EXPECT_EQ("UTF-8", Codeset());
}

TEST_F(Utf8LocaleTest, PrintableTextUtf8)
{
    if (!setlocale(LC_CTYPE, "C.UTF-8")) GTEST_SKIP() << "C.UTF-8 locale is not installed";
    EXPECT_EQ("Router", PrintableText("Router"));
    EXPECT_EQ("Café", PrintableText("Café"));
    EXPECT_EQ("Bell?é", PrintableText("Bell\aé"));
    EXPECT_EQ("a?b", PrintableText("a\tb"));
    // Not valid UTF-8
    EXPECT_EQ("a?b", PrintableText("a\xff" "b"));
    EXPECT_EQ("?", PrintableText(""));
}

TEST_F(Utf8LocaleTest, PrintableTextCLocale)
{
    setlocale(LC_CTYPE, "C");
    EXPECT_EQ("Router", PrintableText("Router"));
    // One '?' for each UTF-8 character, not for each byte
    EXPECT_EQ("Caf?", PrintableText("Café"));
    EXPECT_EQ("????", PrintableText("Банк"));
    EXPECT_EQ("Bell??", PrintableText("Bell\aé"));
}

#endif  // #ifndef _WIN32
