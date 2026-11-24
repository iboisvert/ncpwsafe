/* Copyright 2026 Ian Boisvert */
#ifndef HAVE_AUTOSIGNOFF_H
#define HAVE_AUTOSIGNOFF_H

#include <cstdint>
#include <exception>
#include <functional>
#include <string>

/**
 * Auto sign-off: ncpwsafe quits after a period without input, discarding unsaved
 * changes, so a database isn't left open on an unattended computer.
 *
 * `ReadKey()` reads keys through the `IdleTimer` set with `SetKeyIdleTimer()`.
 * When the timer expires it throws `AutoSignOff`, which unwinds every open
 * window and dialog and is caught by `PWSafeApp::Show()`.
 */

/** Exit status after an auto sign-off, the status GNU timeout(1) uses for a timeout */
constexpr int EXIT_AUTO_SIGN_OFF = 124;

/** Default period without input before signing off, in seconds */
constexpr long DEFAULT_AUTO_SIGN_OFF_SECONDS = 5 * 60;

/**
 * Milliseconds on a clock that keeps counting while the computer is suspended,
 * so that sleep counts as time without input: `CLOCK_BOOTTIME` where it exists,
 * otherwise `CLOCK_MONOTONIC`; `GetTickCount64()` on Windows.
 */
int64_t IdleClockMs();

class IdleTimer
{
public:
    /** Returns the current time in milliseconds */
    typedef std::function<int64_t()> Clock;

    /**
     * @param period_seconds Time without input before the timer expires, 0 to turn it off
     * @param clock The clock, for tests
     */
    explicit IdleTimer(long period_seconds = 0, Clock clock = IdleClockMs);

    /** Set the period, in seconds, 0 to turn the timer off. Restarts the timer */
    void SetPeriod(long period_seconds);

    long Period() const
    {
        // The period was set in seconds as a long, so it fits (long is 32 bits on Windows)
        return static_cast<long>(period_ms_ / 1000);
    }

    bool Enabled() const
    {
        return period_ms_ > 0;
    }

    /** Restart the period, e.g. when a key is pressed */
    void Touch();

    /** `true` if the timer is enabled and the period has passed since the last `Touch()` */
    bool Expired() const;

    /** Milliseconds until the timer expires, 0 if it has expired or is off */
    int64_t RemainingMs() const;

    /** The current time on the timer's clock */
    int64_t Now() const
    {
        return clock_();
    }

private:
    Clock clock_;
    int64_t period_ms_ = 0;
    int64_t last_input_ms_ = 0;
};

/** Thrown by `ReadKey()` when the key idle timer expires */
class AutoSignOff : public std::exception
{
public:
    const char *what() const noexcept override
    {
        return "Auto sign-off";
    }
};

/**
 * Parse the value of the `auto-sign-off` preference: a number with an optional unit,
 * `s` (seconds), `m` (minutes) or `h` (hours). A number without a unit is minutes.
 * `0` turns auto sign-off off.
 * @param[out] seconds The period in seconds, if the value is valid
 * @returns `false` if the value isn't valid, e.g. empty, negative, or an unknown unit
 */
bool ParseAutoSignOff(const std::string &value, long &seconds);

/** A period for messages, e.g. "5 minutes", "90 seconds", "1 hour" */
std::string FormatDuration(long seconds);

/**
 * Set the timer `ReadKey()` uses, or `nullptr` for none.
 * The timer must outlive its use by `ReadKey()`.
 */
void SetKeyIdleTimer(IdleTimer *timer);

/** The timer `ReadKey()` uses, or `nullptr` */
IdleTimer *GetKeyIdleTimer();

#endif  // #ifndef HAVE_AUTOSIGNOFF_H
