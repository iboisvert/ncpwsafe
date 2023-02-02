/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_RANDUTILS_H
#define HAVE_RANDUTILS_H

#include <cstddef>
#include <random>
#include <cstring>

class RandomState
{
    static constexpr size_t R = sizeof(typename std::mt19937::result_type)/sizeof(typename std::random_device::result_type);
    static_assert(static_cast<float>(R) == static_cast<float>(sizeof(typename std::mt19937::result_type))/static_cast<float>(sizeof(typename std::random_device::result_type)));

    std::random_device rd;
    std::mt19937 mt32;

public:
    typedef std::mt19937::result_type result_type;

    RandomState()
    {
        typename std::mt19937::result_type seed;
        for (size_t i = 0; i <= R; ++i)
        {
            *(reinterpret_cast<typename std::random_device::result_type *>(&seed)+i) = rd();
        }
        mt32.seed(seed);
    }
    RandomState(const RandomState &) = delete;
    RandomState &operator=(const RandomState &) = delete;

    typename std::mt19937::result_type operator()()
    {
        return mt32();
    }
};

#endif
