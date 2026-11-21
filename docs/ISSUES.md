# Issues

## Random number generation (`src/RandUtils.h`, `src/Policy.cpp`)

**Status: fixed.** See [Fix](#fix) below.

Before the fix, `RandomState` wraps a `std::mt19937` engine that is seeded once from
`std::random_device`. `PasswordPolicy::MakePassword` uses it to generate passwords,
and `GenerateTestDbCommand` uses it to generate test data.

### Generated passwords have at most 2^32 possible values

`RandomState` builds a seed of type `std::mt19937::result_type` (`uint_fast32_t`) from
`std::random_device` words and passes it to `mt32.seed()`. `mt19937` is a 32-bit-word
engine, and `seed(value)` keeps only `value mod 2^32`. On platforms where
`uint_fast32_t` is 64 bits (glibc on x86-64), the second `random_device` word is
discarded. On every platform the engine therefore starts from one of only 2^32
states.

A password from a freshly constructed `PasswordPolicy` can therefore be only one of
about 4 billion strings, whatever its length and character set. An attacker who knows
the algorithm can enumerate them all.

### `mt19937` is not a cryptographically secure generator

The internal state of a Mersenne Twister can be reconstructed from 624 consecutive
outputs, after which every later output can be predicted. It shouldn't be used to
generate passwords.

### Modulo bias in `PasswordPolicy::MakePassword`

Characters are chosen with `charset[n % N]`. Unless `N` divides 2^32 evenly, the
characters at the start of the charset are chosen slightly more often than the
others.

### Strict-aliasing violation when filling the seed

The constructor writes `std::random_device::result_type` (`unsigned int`) values
through a `reinterpret_cast` pointer into a `std::mt19937::result_type`. Where those
are different types (`unsigned long` on glibc x86-64), this is undefined behaviour.
`std::memcpy` is the well-defined alternative. On MSVC both types are `unsigned int`,
so this doesn't apply there.

### Unclear divisibility check

The `static_assert` that the size of `std::mt19937::result_type` is an exact multiple
of the size of `std::random_device::result_type` compares the integer quotient with a
floating-point division. `sizeof(A) % sizeof(B) == 0` would state the intent more
clearly.

### Fix

- `PasswordPolicy::MakePassword` no longer uses `RandomState`. It draws characters
  directly from `std::random_device` with `std::uniform_int_distribution<size_t>(0, N - 1)`.
  On MSVC and glibc, `random_device` reads from the operating system's cryptographically
  secure generator, and the distribution removes the modulo bias.
- `RandomState` is now documented as being for test data only. It seeds the whole
  `mt19937` state through a `std::seed_seq` filled with `std::mt19937::state_size`
  `random_device` words, which removes the `reinterpret_cast`, the divisibility
  `static_assert`, and the seed-loop overrun.
- `test/Policy-tests.cpp` checks password length, character sets, that the whole
  charset is used, that passwords don't repeat, and that separate `RandomState`
  instances produce different sequences.
