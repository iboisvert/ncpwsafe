/* Copyright 2026 Ian Boisvert */
#include <gtest/gtest.h>
#include "Theme.h"

using Mode = Theme::Mode;

static const Role ALL_ROLES[] = {
    Role::NORMAL, Role::TITLE, Role::HEADING, Role::LABEL, Role::FIELD, Role::FIELD_FOCUS, Role::SELECTION,
    Role::SECRET, Role::MUTED, Role::KEY, Role::KEY_DESC, Role::ERROR, Role::WARNING, Role::SUCCESS,
};

TEST(ThemeTest, SelectModeUsesColourWhenAvailable)
{
    EXPECT_EQ(Mode::COLOUR, Theme::SelectMode(true, nullptr, ""));
    EXPECT_EQ(Mode::COLOUR, Theme::SelectMode(true, nullptr, "default"));
    EXPECT_EQ(Mode::MONO, Theme::SelectMode(false, nullptr, ""));
}

TEST(ThemeTest, SelectModeHonoursNoColor)
{
    EXPECT_EQ(Mode::MONO, Theme::SelectMode(true, "1", ""));
    EXPECT_EQ(Mode::MONO, Theme::SelectMode(true, "anything", ""));
    // https://no-color.org: an empty value doesn't turn colour off
    EXPECT_EQ(Mode::COLOUR, Theme::SelectMode(true, "", ""));
}

TEST(ThemeTest, SelectModeHonoursMonoPref)
{
    EXPECT_EQ(Mode::MONO, Theme::SelectMode(true, nullptr, Theme::THEME_MONO));
}

TEST(ThemeTest, MonoUsesNoColour)
{
    for (Role role : ALL_ROLES)
    {
        EXPECT_EQ(0u, Theme::AttrFor(role, Mode::MONO) & A_COLOR) << static_cast<int>(role);
    }
}

TEST(ThemeTest, ColourIsNeverTheOnlyCue)
{
    for (Role role : ALL_ROLES)
    {
        if (role == Role::NORMAL || role == Role::KEY_DESC)
            continue;
        attr_t attrs = Theme::AttrFor(role, Mode::COLOUR);
        EXPECT_NE(0u, attrs & ~A_COLOR) << static_cast<int>(role);
    }
}

TEST(ThemeTest, SelectionIsReverseVideo)
{
    // Monochrome terminals, and the TUI tests, rely on it
    EXPECT_NE(0u, Theme::AttrFor(Role::SELECTION, Mode::MONO) & A_REVERSE);
    EXPECT_NE(0u, Theme::AttrFor(Role::SELECTION, Mode::COLOUR) & A_REVERSE);
}

TEST(ThemeTest, SymbolsHaveAsciiForms)
{
    for (Symbol symbol : {Symbol::EXPANDED, Symbol::COLLAPSED, Symbol::ELLIPSIS, Symbol::CHECK, Symbol::SEPARATOR})
    {
        const std::string ascii = Theme::SymbolFor(symbol, false);
        EXPECT_EQ(1u, ascii.size());
        EXPECT_LT(static_cast<unsigned char>(ascii[0]), 0x80);
        EXPECT_NE(ascii, Theme::SymbolFor(symbol, true));
    }
    EXPECT_STREQ("\u25be", Theme::SymbolFor(Symbol::EXPANDED, true));
}

TEST(ThemeTest, HeadingStandsOutWithoutColour)
{
    attr_t attrs = Theme::AttrFor(Role::HEADING, Mode::MONO);
    EXPECT_NE(0u, attrs & A_BOLD);
    EXPECT_NE(0u, attrs & A_UNDERLINE);
}
