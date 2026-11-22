/* Copyright 2026 Ian Boisvert */
#include "Theme.h"
#include <cstdlib>
#include "Utils.h"

// Colour pairs. Only the 8 basic colours are used, over the terminal's background
enum : short
{
    PAIR_TITLE = 1,  // White on blue
    PAIR_CYAN,       // Headings, the selection (in reverse video)
    PAIR_FOCUS,      // Black on yellow
    PAIR_GREEN,      // Secrets, success
    PAIR_RED,        // Errors
    PAIR_YELLOW,     // Warnings
};

Theme::Mode Theme::SelectMode(bool has_colors, const char *no_color, const std::string &theme_pref)
{
    if (!has_colors || (no_color != nullptr && *no_color != '\0') || theme_pref == THEME_MONO)
    {
        return Mode::MONO;
    }
    return Mode::COLOUR;
}

attr_t Theme::AttrFor(Role role, Mode mode)
{
    const bool colour = mode == Mode::COLOUR;
    switch (role)
    {
    case Role::NORMAL:
        return A_NORMAL;
    case Role::TITLE:
        return colour ? COLOR_PAIR(PAIR_TITLE) | A_BOLD : A_BOLD | A_REVERSE;
    case Role::HEADING:
        return colour ? COLOR_PAIR(PAIR_CYAN) | A_BOLD : A_BOLD | A_UNDERLINE;
    case Role::LABEL:
        return A_DIM;
    case Role::FIELD:
        return A_UNDERLINE;
    case Role::FIELD_FOCUS:
        return colour ? COLOR_PAIR(PAIR_FOCUS) | A_BOLD : A_REVERSE;
    case Role::SELECTION:
        // Always reverse video, which is also how the TUI tests find the selection
        return colour ? COLOR_PAIR(PAIR_CYAN) | A_REVERSE | A_BOLD : A_REVERSE;
    case Role::SECRET:
        return colour ? COLOR_PAIR(PAIR_GREEN) | A_BOLD : A_BOLD;
    case Role::MUTED:
        return A_DIM;
    case Role::KEY:
        return A_REVERSE | A_BOLD;
    case Role::KEY_DESC:
        return A_NORMAL;
    case Role::ERROR:
        return colour ? COLOR_PAIR(PAIR_RED) | A_BOLD : A_BOLD;
    case Role::WARNING:
        return colour ? COLOR_PAIR(PAIR_YELLOW) | A_BOLD : A_BOLD;
    case Role::SUCCESS:
        return colour ? COLOR_PAIR(PAIR_GREEN) | A_BOLD : A_BOLD;
    }
    return A_NORMAL;
}

const char *Theme::SymbolFor(Symbol symbol, bool unicode)
{
    switch (symbol)
    {
    case Symbol::EXPANDED:
        return unicode ? "\u25be" : "v";  // ▾
    case Symbol::COLLAPSED:
        return unicode ? "\u25b8" : ">";  // ▸
    case Symbol::ELLIPSIS:
        return unicode ? "\u2026" : "~";  // …
    case Symbol::CHECK:
        return unicode ? "\u2713" : "*";  // ✓
    case Symbol::SEPARATOR:
        return unicode ? "\u203a" : ">";  // ›
    case Symbol::BULLET:
        return unicode ? "\u2022" : "*";  // •
    }
    return "?";
}

void Theme::Init(const std::string &theme_pref)
{
    unicode_ = IsUtf8Ctype();
    mode_ = SelectMode(has_colors(), std::getenv("NO_COLOR"), theme_pref);
    if (mode_ != Mode::COLOUR)
    {
        return;
    }
    if (start_color() == ERR)
    {
        mode_ = Mode::MONO;
        return;
    }
    // -1 is the terminal's own background, so the theme works on light and dark backgrounds
    const short bg = use_default_colors() == CURSES_OK ? -1 : COLOR_BLACK;
    init_pair(PAIR_TITLE, COLOR_WHITE, COLOR_BLUE);
    init_pair(PAIR_CYAN, COLOR_CYAN, bg);
    init_pair(PAIR_FOCUS, COLOR_BLACK, COLOR_YELLOW);
    init_pair(PAIR_GREEN, COLOR_GREEN, bg);
    init_pair(PAIR_RED, COLOR_RED, bg);
    init_pair(PAIR_YELLOW, COLOR_YELLOW, bg);
}
