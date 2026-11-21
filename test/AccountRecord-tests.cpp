/* Copyright 2026 Ian Boisvert */
#include <string>
#include <gtest/gtest.h>
#include "AccountRecord.h"

TEST(AccountRecordTest, FieldContainsCaseInsensitive)
{
    const AccountRecord rec{{FT_TITLE, "Bookstore"}, {FT_NOTES, "VPN required"}};
    EXPECT_TRUE(rec.FieldContainsCaseInsensitive(FT_TITLE, "book"));
    EXPECT_TRUE(rec.FieldContainsCaseInsensitive(FT_TITLE, "Book"));
    EXPECT_TRUE(rec.FieldContainsCaseInsensitive(FT_TITLE, "BOOK"));
    EXPECT_TRUE(rec.FieldContainsCaseInsensitive(FT_TITLE, "kSt"));
    EXPECT_TRUE(rec.FieldContainsCaseInsensitive(FT_NOTES, "vpn"));
    EXPECT_FALSE(rec.FieldContainsCaseInsensitive(FT_TITLE, "bookshop"));
    // The field isn't set
    EXPECT_FALSE(rec.FieldContainsCaseInsensitive(FT_USER, "book"));
}

TEST(AccountRecordTest, FieldContainsCaseInsensitiveNonAscii)
{
    const AccountRecord rec{{FT_TITLE, "Сберегательный счёт"}, {FT_USER, "Crème brûlée"}};
    EXPECT_TRUE(rec.FieldContainsCaseInsensitive(FT_TITLE, "сбер"));
    EXPECT_TRUE(rec.FieldContainsCaseInsensitive(FT_TITLE, "СЧЁТ"));
    EXPECT_TRUE(rec.FieldContainsCaseInsensitive(FT_USER, "BRÛLÉE"));
    EXPECT_FALSE(rec.FieldContainsCaseInsensitive(FT_USER, "brulee"));
    EXPECT_FALSE(rec.FieldContainsCaseInsensitive(FT_TITLE, "счет"));
}
