/* Copyright 2026 Ian Boisvert */
#include <cctype>
#include <cstring>
#include <set>
#include <string>
#include <gtest/gtest.h>
#include "Policy.h"
#include "RandUtils.h"

namespace
{
const char *const SYMBOLS = "!#$%&*@^";

bool IsSymbol(char c)
{
    return c != '\0' && strchr(SYMBOLS, c) != nullptr;
}

std::string MakePassword(PasswordPolicy::Composition composition, size_t length)
{
    PasswordPolicy policy{composition, length};
    return policy.MakePassword();
}
} // namespace

TEST(PolicyTest, TestMakePassword_Length)
{
    for (int i = 0; i < PasswordPolicy::Composition::COUNT; ++i)
    {
        auto composition = static_cast<PasswordPolicy::Composition>(i);
        ASSERT_EQ(1u, MakePassword(composition, 1).length());
        ASSERT_EQ(20u, MakePassword(composition, 20).length());
        ASSERT_EQ(500u, MakePassword(composition, 500).length());
    }
}

TEST(PolicyTest, TestMakePassword_Charset)
{
    const size_t LEN = 2000;
    for (char c : MakePassword(PasswordPolicy::ALPHA_DIGIT_SYMBOL, LEN))
    {
        ASSERT_TRUE(isalnum(static_cast<unsigned char>(c)) || IsSymbol(c)) << c;
    }
    for (char c : MakePassword(PasswordPolicy::ALPHA_DIGIT, LEN))
    {
        ASSERT_TRUE(isalnum(static_cast<unsigned char>(c))) << c;
    }
    for (char c : MakePassword(PasswordPolicy::ETR_ALPHA_DIGIT_SYMBOL, LEN))
    {
        ASSERT_TRUE(isalnum(static_cast<unsigned char>(c)) || IsSymbol(c)) << c;
        // Easy-to-read passwords leave out characters that look alike
        ASSERT_EQ(nullptr, strchr("BILOilo018", c)) << c;
    }
    for (char c : MakePassword(PasswordPolicy::DIGIT, LEN))
    {
        ASSERT_TRUE(isdigit(static_cast<unsigned char>(c))) << c;
    }
    for (char c : MakePassword(PasswordPolicy::HEX_DIGIT, LEN))
    {
        ASSERT_TRUE(isxdigit(static_cast<unsigned char>(c))) << c;
    }
}

TEST(PolicyTest, TestMakePassword_UsesWholeCharset)
{
    // The chance that a 2000-character password misses one of 10 digits is about 10 * 0.9^2000
    std::string pw = MakePassword(PasswordPolicy::DIGIT, 2000);
    std::set<char> seen(pw.begin(), pw.end());
    ASSERT_EQ(10u, seen.size());
}

TEST(PolicyTest, TestMakePassword_Unique)
{
    // Passwords from separate policies, and from the same policy, must differ.
    // A seeded engine with a weak seed would make repeats likely.
    std::set<std::string> passwords;
    PasswordPolicy policy;
    for (int i = 0; i < 1000; ++i)
    {
        passwords.insert(policy.MakePassword());
        passwords.insert(PasswordPolicy{}.MakePassword());
    }
    ASSERT_EQ(2000u, passwords.size());
}

TEST(RandomStateTest, TestSeparateInstancesDiffer)
{
    RandomState a, b;
    bool differ = false;
    for (int i = 0; i < 8 && !differ; ++i)
    {
        differ = a() != b();
    }
    ASSERT_TRUE(differ);
}
