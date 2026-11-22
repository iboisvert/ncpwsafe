/* Copyright 2026 Ian Boisvert */
#include <gtest/gtest.h>
#include "AccountDb.h"
#include "AutoSignOff.h"

/** A clock the test moves by hand */
struct FakeClock
{
    int64_t now_ms = 1000000;
    IdleTimer::Clock Get()
    {
        return [this]() { return now_ms; };
    }
};

TEST(IdleTimerTest, ExpiresAfterPeriod)
{
    FakeClock clock;
    IdleTimer timer(300, clock.Get());
    EXPECT_TRUE(timer.Enabled());
    EXPECT_EQ(timer.Period(), 300);
    EXPECT_FALSE(timer.Expired());
    EXPECT_EQ(timer.RemainingMs(), 300000);

    clock.now_ms += 299999;
    EXPECT_FALSE(timer.Expired());
    EXPECT_EQ(timer.RemainingMs(), 1);

    clock.now_ms += 1;
    EXPECT_TRUE(timer.Expired());
    EXPECT_EQ(timer.RemainingMs(), 0);
}

TEST(IdleTimerTest, TouchRestartsPeriod)
{
    FakeClock clock;
    IdleTimer timer(10, clock.Get());
    clock.now_ms += 9000;
    timer.Touch();
    clock.now_ms += 9000;
    EXPECT_FALSE(timer.Expired());
    EXPECT_EQ(timer.RemainingMs(), 1000);
    clock.now_ms += 1000;
    EXPECT_TRUE(timer.Expired());
}

TEST(IdleTimerTest, ZeroPeriodNeverExpires)
{
    FakeClock clock;
    IdleTimer timer(0, clock.Get());
    EXPECT_FALSE(timer.Enabled());
    clock.now_ms += 365LL * 24 * 3600 * 1000;
    EXPECT_FALSE(timer.Expired());
    EXPECT_EQ(timer.RemainingMs(), 0);
}

TEST(IdleTimerTest, ClockJumpExpiresAtOnce)
{
    // A suspend: the boot-time clock jumps forward by the time asleep
    FakeClock clock;
    IdleTimer timer(300, clock.Get());
    clock.now_ms += 8LL * 3600 * 1000;
    EXPECT_TRUE(timer.Expired());
}

TEST(IdleTimerTest, SetPeriodRestarts)
{
    FakeClock clock;
    IdleTimer timer(10, clock.Get());
    clock.now_ms += 9000;
    timer.SetPeriod(20);
    EXPECT_EQ(timer.Period(), 20);
    EXPECT_EQ(timer.RemainingMs(), 20000);
    timer.SetPeriod(-5);
    EXPECT_FALSE(timer.Enabled());
}

TEST(IdleTimerTest, IdleClockAdvances)
{
    const int64_t t0 = IdleClockMs();
    EXPECT_GT(t0, 0);
    EXPECT_GE(IdleClockMs(), t0);
}

static long Parse(const std::string &value)
{
    long seconds = -1;
    EXPECT_TRUE(ParseAutoSignOff(value, seconds)) << "\"" << value << "\"";
    return seconds;
}

static bool Invalid(const std::string &value)
{
    long seconds = -42;
    const bool valid = ParseAutoSignOff(value, seconds);
    // Not changed when invalid
    EXPECT_EQ(seconds, -42) << "\"" << value << "\"";
    return !valid;
}

TEST(ParseAutoSignOffTest, NumberWithoutUnitIsMinutes)
{
    EXPECT_EQ(Parse("5"), 300);
    EXPECT_EQ(Parse("1"), 60);
}

TEST(ParseAutoSignOffTest, Units)
{
    EXPECT_EQ(Parse("90s"), 90);
    EXPECT_EQ(Parse("5m"), 300);
    EXPECT_EQ(Parse("1h"), 3600);
    EXPECT_EQ(Parse("2H"), 7200);
    EXPECT_EQ(Parse("10 m"), 600);
    EXPECT_EQ(Parse("  15s "), 15);
}

TEST(ParseAutoSignOffTest, ZeroTurnsOff)
{
    EXPECT_EQ(Parse("0"), 0);
    EXPECT_EQ(Parse("0s"), 0);
}

TEST(ParseAutoSignOffTest, InvalidValues)
{
    EXPECT_TRUE(Invalid(""));
    EXPECT_TRUE(Invalid("   "));
    EXPECT_TRUE(Invalid("-1"));
    EXPECT_TRUE(Invalid("+5"));
    EXPECT_TRUE(Invalid("abc"));
    EXPECT_TRUE(Invalid("5x"));
    EXPECT_TRUE(Invalid("5 minutes"));
    EXPECT_TRUE(Invalid("5ms"));
    EXPECT_TRUE(Invalid("1.5m"));
    EXPECT_TRUE(Invalid("m"));
    EXPECT_TRUE(Invalid("9999999999h"));
}

TEST(FormatDurationTest, Units)
{
    EXPECT_EQ(FormatDuration(300), "5 minutes");
    EXPECT_EQ(FormatDuration(60), "1 minute");
    EXPECT_EQ(FormatDuration(90), "90 seconds");
    EXPECT_EQ(FormatDuration(1), "1 second");
    EXPECT_EQ(FormatDuration(3600), "1 hour");
    EXPECT_EQ(FormatDuration(7200), "2 hours");
    EXPECT_EQ(FormatDuration(5400), "90 minutes");
}

TEST(AccountDbTest, WipeRemovesPasswordAndRecords)
{
    AccountDb db;
    db.DbPathname() = "accounts.dat";
    db.Password() = "secret";
    db.Records().Save(AccountRecord{{FT_TITLE, "Bank"}, {FT_PASSWORD, "hunter2"}});
    ASSERT_TRUE(db.IsDirty());

    db.Wipe();
    EXPECT_TRUE(db.Password().empty());
    EXPECT_EQ(db.Records().begin(), db.Records().end());
    EXPECT_FALSE(db.IsDirty());
    EXPECT_EQ(db.DbPathname(), "accounts.dat");
}
