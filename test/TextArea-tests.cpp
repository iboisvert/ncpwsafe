/* Copyright 2026 Ian Boisvert */
#include <gtest/gtest.h>
#include <clocale>
#include <string>
#include "TextArea.h"

/** Runs each test in a UTF-8 LC_CTYPE locale, as ncpwsafe does */
class TextAreaTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const char *ctype = setlocale(LC_CTYPE, nullptr);
        saved_ctype_ = ctype != nullptr ? ctype : "C";
        // ".UTF-8" is the name on Windows
        if (!setlocale(LC_CTYPE, "C.UTF-8") && !setlocale(LC_CTYPE, ".UTF-8"))
            GTEST_SKIP() << "No UTF-8 locale";
    }
    void TearDown() override
    {
        setlocale(LC_CTYPE, saved_ctype_.c_str());
    }

    /** The text of each row of `area` for `width` columns, as it is shown */
    static std::vector<std::string> Rows(const TextArea &area, int width)
    {
        std::vector<std::string> rows;
        for (const TextArea::Row &row : area.Layout(width))
            rows.push_back(area.RowText(row));
        return rows;
    }

    std::string saved_ctype_;
};

TEST_F(TextAreaTest, KeepsTextAsItIs)
{
    for (const char *text : {"", "One line", "line one\nline two", "line one\r\nline two", "abc\rdef",
                             "mixed\r\nline\nbreaks\rhere", "\nafter a blank line", "trailing break\n",
                             "\n\n", "a\tb", "Bell\a", "trailing space ", "naïve 日本語"})
    {
        EXPECT_EQ(text, TextArea(text).Text()) << text;
    }
}

TEST_F(TextAreaTest, SplitsLines)
{
    TextArea area("one\r\ntwo\nthree\rfour");
    ASSERT_EQ(4u, area.LineCount());
    EXPECT_EQ("one", area.Line(0));
    EXPECT_EQ("two", area.Line(1));
    EXPECT_EQ("three", area.Line(2));
    EXPECT_EQ("four", area.Line(3));
    EXPECT_EQ(1u, TextArea("").LineCount());
    EXPECT_EQ(2u, TextArea("end\n").LineCount());
}

TEST_F(TextAreaTest, NewLineUsesTheTextsLineBreak)
{
    TextArea unix_text("ab\ncd");
    unix_text.Last();
    unix_text.NewLine();
    unix_text.Insert("e");
    EXPECT_EQ("ab\ncd\ne", unix_text.Text());

    // Text without a line break gets Password Safe's
    TextArea one_line("ab");
    one_line.End();
    one_line.NewLine();
    one_line.Insert("c");
    EXPECT_EQ("ab\r\nc", one_line.Text());
}

TEST_F(TextAreaTest, NewLineKeepsTheBreakOfTheLineItSplits)
{
    TextArea area("abcd\r\nx\ny");
    area.Right();
    area.Right();
    area.NewLine();
    EXPECT_EQ("ab\r\ncd\r\nx\ny", area.Text());
    EXPECT_EQ(1u, area.CursorLine());
    EXPECT_EQ(0u, area.CursorPos());
}

TEST_F(TextAreaTest, InsertAtCursor)
{
    TextArea area("cafe");
    area.End();
    area.Left();
    area.Insert("é");
    EXPECT_EQ("cafée", area.Text());
    EXPECT_EQ(5u, area.CursorPos());
}

TEST_F(TextAreaTest, BackspaceDeletesACharacter)
{
    TextArea area("café");
    area.End();
    area.Backspace();
    EXPECT_EQ("caf", area.Text());
    area.Home();
    area.Backspace();
    EXPECT_EQ("caf", area.Text());
}

TEST_F(TextAreaTest, BackspaceJoinsLines)
{
    TextArea area("ab\r\ncd\nef");
    area.Right();
    area.Right();
    area.Right();  // The start of "cd"
    area.Backspace();
    EXPECT_EQ("abcd\nef", area.Text());
    EXPECT_EQ(0u, area.CursorLine());
    EXPECT_EQ(2u, area.CursorPos());
}

TEST_F(TextAreaTest, DeleteJoinsLines)
{
    TextArea area("ab\ncd\r\nef");
    area.End();
    area.Delete();
    EXPECT_EQ("abcd\r\nef", area.Text());
    area.Home();
    area.Delete();
    EXPECT_EQ("bcd\r\nef", area.Text());
    area.Last();
    area.Delete();
    EXPECT_EQ("bcd\r\nef", area.Text());
}

TEST_F(TextAreaTest, LeftAndRightCrossLines)
{
    TextArea area("ab\nc");
    area.End();
    area.Right();
    EXPECT_EQ(1u, area.CursorLine());
    EXPECT_EQ(0u, area.CursorPos());
    area.Left();
    EXPECT_EQ(0u, area.CursorLine());
    EXPECT_EQ(2u, area.CursorPos());
    area.First();
    area.Left();
    EXPECT_EQ(0u, area.CursorPos());
}

TEST_F(TextAreaTest, LayoutWrapsLongLines)
{
    EXPECT_EQ((std::vector<std::string>{"abcd", "efgh", "ij", ""}), Rows(TextArea("abcdefghij\n"), 4));
    // A line that fills a row takes one row
    EXPECT_EQ((std::vector<std::string>{"abcd"}), Rows(TextArea("abcd"), 4));
    EXPECT_EQ((std::vector<std::string>{""}), Rows(TextArea(""), 4));
}

TEST_F(TextAreaTest, RowTextShowsTabsAndControlCharacters)
{
    EXPECT_EQ((std::vector<std::string>{"a b", "Bell?"}), Rows(TextArea("a\tb\nBell\a"), 10));
}

#ifndef _WIN32
// MSVC has no wcwidth(), every character takes one column there
TEST_F(TextAreaTest, LayoutWrapsWideCharacters)
{
    // A wide character that doesn't fit starts the next row
    EXPECT_EQ((std::vector<std::string>{"a日", "本語"}), Rows(TextArea("a日本語"), 4));
    int row, col;
    TextArea area("ab\n日本");
    area.Right();
    area.Down(10);
    area.CursorPosition(10, row, col);
    // Column 1 is inside 日, the cursor goes before it
    EXPECT_EQ(1, row);
    EXPECT_EQ(0, col);
}
#endif

TEST_F(TextAreaTest, UpAndDownMoveByRow)
{
    TextArea area("abcdefgh\nxy");
    const int width = 4;  // Rows "abcd", "efgh", "xy"
    EXPECT_TRUE(area.Down(width));
    EXPECT_EQ(0u, area.CursorLine());
    EXPECT_EQ(4u, area.CursorPos());
    EXPECT_TRUE(area.Down(width));
    EXPECT_EQ(1u, area.CursorLine());
    EXPECT_EQ(0u, area.CursorPos());
    EXPECT_FALSE(area.Down(width));

    area.End();  // Column 2 of "xy"
    EXPECT_TRUE(area.Up(width));
    EXPECT_EQ(0u, area.CursorLine());
    EXPECT_EQ(6u, area.CursorPos());
    EXPECT_TRUE(area.Up(width));
    EXPECT_EQ(2u, area.CursorPos());
    EXPECT_FALSE(area.Up(width));
    EXPECT_EQ(2u, area.CursorPos());
}

TEST_F(TextAreaTest, UpStaysOnTheRowAboveAtItsEnd)
{
    TextArea area("abcdefgh");
    area.End();  // Column 4 of "efgh"
    int row, col;
    area.CursorPosition(4, row, col);
    EXPECT_EQ(1, row);
    EXPECT_EQ(4, col);
    EXPECT_TRUE(area.Up(4));
    // The end of "abcd" is the start of "efgh", the cursor stays on "abcd"
    area.CursorPosition(4, row, col);
    EXPECT_EQ(0, row);
    EXPECT_EQ(3u, area.CursorPos());
}

TEST_F(TextAreaTest, ScrollTopFollowsTheCursor)
{
    TextArea area("1\n2\n3\n4\n5");
    EXPECT_EQ(0, area.ScrollTop(10, 3));
    area.Last();
    EXPECT_EQ(2, area.ScrollTop(10, 3));
    area.Up(10);
    area.Up(10);
    EXPECT_EQ(2, area.ScrollTop(10, 3));
    area.Up(10);
    EXPECT_EQ(1, area.ScrollTop(10, 3));
    // Text shorter than the height doesn't scroll
    TextArea short_text("1\n2");
    short_text.Last();
    EXPECT_EQ(0, short_text.ScrollTop(10, 3));
}

TEST_F(TextAreaTest, Clear)
{
    TextArea area("secret\nnotes");
    area.Last();
    area.Clear();
    EXPECT_EQ("", area.Text());
    EXPECT_EQ(1u, area.LineCount());
    EXPECT_EQ(0u, area.CursorLine());
    EXPECT_EQ(0u, area.CursorPos());
}
