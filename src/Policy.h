/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_POLICY_H
#define HAVE_POLICY_H

#include <string>
#include <cassert>
#include "RandUtils.h"

/**
 * Password policy for an account
 */
class PasswordPolicy
{
public:
    // If these enumerators ever become non-consecutive
    // then Composition::COUNT will need to be redefined.
    // To support POLICY field of pwsafe v3+ db, define a map
    // from flags to a specific Composition enumerator
    enum Composition
    {
        ALPHA_DIGIT_SYMBOL,
        ALPHA_DIGIT,
        ETR_ALPHA_DIGIT_SYMBOL,
        DIGIT,
        HEX_DIGIT,
        COUNT
    };

    static const char *GetName(Composition composition);

    PasswordPolicy(Composition composition, size_t length):
        composition(composition), length(length)
    {
        assert(length > 0);
    }

    PasswordPolicy() : PasswordPolicy(Composition::ALPHA_DIGIT_SYMBOL, /*length*/20) {}

    PasswordPolicy &operator=(const PasswordPolicy &src)
    {
        this->composition = src.composition;
        this->length = src.length;
        return *this;
    }

    const char *GetName();

    std::string MakePassword();

private:
    Composition composition;
    size_t length;
    RandomState rand;
};

#endif