/* Copyright 2023 Ian Boisvert */
#include "Policy.h"
#include <cstring>

static const char * const COMPOSITION_NAME[] {
    "alpha/digit/symbol",
    "alpha/digit",
    "easy-to-read alpha/digit/symbol",
    "digits only",
    "hex digits only",
};

// IMB 2023-01-23 Reverse engineered from LastPass
// ETR==Easy To Read
// ETS==Easy To Say
#define COMP_ALPHA "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define COMP_DIGIT "0123456789"
#define COMP_SYMBOL "!#$%&*@^"
#define COMP_HEX_ALPHA "abcdefABCDEF"
#define COMP_ETR_ALPHA "ACDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz"
#define COMP_ETR_DIGIT "2345679"
#define COMP_ETR_HEX "abcdefABCDEF"
#define COMP_ETS_ALPHA "ABCDEFGHIKLMNOPRSTUVYZabcdefghiklmnoprstuvwxy"

// clang-format off
static constexpr char * const POLICY_CHARSET[] = {
    COMP_ALPHA COMP_DIGIT COMP_SYMBOL,
    COMP_ALPHA COMP_DIGIT,
    COMP_ETR_ALPHA COMP_ETR_DIGIT COMP_SYMBOL,
    COMP_DIGIT,
    COMP_HEX_ALPHA COMP_DIGIT
};
// clang-format on

const char *PasswordPolicy::GetName(PasswordPolicy::Composition composition)
{
    return COMPOSITION_NAME[static_cast<int>(composition)];
}

const char *PasswordPolicy::GetName()
{
    return GetName(composition);
}

std::string PasswordPolicy::MakePassword()
{
    const char * charset = POLICY_CHARSET[static_cast<int>(composition)];
    size_t N = strlen(charset);
    std::string pw(length, 0);
    for (size_t i = 0; i < length; ++i)
    {
        RandomState::result_type n = rand();
        char c = charset[n%N];
        pw[i] = c;
    }
    return pw;
}