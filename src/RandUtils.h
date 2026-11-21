/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_RANDUTILS_H
#define HAVE_RANDUTILS_H

#include <algorithm>
#include <array>
#include <functional>
#include <random>

/**
 * Fast, non-cryptographic pseudo-random number generator, for test data only.
 * Use `std::random_device` for anything that must be unpredictable, such as passwords.
 */
class RandomState
{
    std::mt19937 mt32;

public:
    typedef std::mt19937::result_type result_type;

    RandomState()
    {
        // Seed the whole engine state, not just the single 32-bit word that
        // `std::mt19937::seed(result_type)` uses
        std::random_device rd;
        std::array<std::random_device::result_type, std::mt19937::state_size> seed_data;
        std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
        std::seed_seq seq(seed_data.begin(), seed_data.end());
        mt32.seed(seq);
    }
    RandomState(const RandomState &) = delete;
    RandomState &operator=(const RandomState &) = delete;

    result_type operator()()
    {
        return mt32();
    }
};

#endif
