/* Copyright 2026 Ian Boisvert */
#ifndef HAVE_THEME_H
#define HAVE_THEME_H

#include <string>
#include "libncurses.h"

/**
 * What a piece of text is, independent of how it is drawn.
 * Windows draw text with `Theme::Attr(Role)` rather than with raw attributes
 * or colour pairs, so the look can change in one place.
 */
enum class Role
{
    NORMAL,
    /** Application and dialog titles */
    TITLE,
    /** Group headings in the accounts list, section headings in dialogs */
    HEADING,
    /** Dialog field labels */
    LABEL,
    /** Editable input fields */
    FIELD,
    /** The input field that has the cursor */
    FIELD_FOCUS,
    /** The selected item in a list */
    SELECTION,
    /** Passwords */
    SECRET,
    /** Secondary text: user names in the list, counts, hints */
    MUTED,
    /** A key in the command bar */
    KEY,
    /** The description of a key in the command bar */
    KEY_DESC,
    ERROR,
    WARNING,
    SUCCESS,
};

/** Symbols that have a Unicode and an ASCII form */
enum class Symbol
{
    /** Marks an expanded group heading */
    EXPANDED,
    /** Marks a collapsed group heading */
    COLLAPSED,
    /** Ends text that was cut to fit */
    ELLIPSIS,
    /** Marks a success message */
    CHECK,
    /** Between a group and an account title */
    SEPARATOR,
};

/**
 * Text attributes for each `Role`.
 *
 * With colour, roles use the 8 basic colours over the terminal's own
 * background (`use_default_colors()`), which every colour terminal supports,
 * including the Linux console and the Windows console driver.
 * Without colour (no colour support, `NO_COLOR` set, or the `theme = mono` preference)
 * roles use only bold, underline, dim and reverse.
 *
 * Every role keeps a non-colour cue, so that colour is never the only signal.
 * The selection is always drawn in reverse video.
 */
class Theme
{
public:
    enum class Mode
    {
        MONO,
        COLOUR,
    };

    /** Preference value of `Prefs::THEME` that turns colour off */
    static constexpr const char *THEME_MONO = "mono";

    /**
     * The mode to use.
     * @param has_colors The terminal supports colour, see `has_colors()`
     * @param no_color The value of the `NO_COLOR` environment variable, or `nullptr`.
     *        Any non-empty value turns colour off (https://no-color.org).
     * @param theme_pref The value of the `theme` preference, `"mono"` turns colour off
     */
    static Mode SelectMode(bool has_colors, const char *no_color, const std::string &theme_pref);

    /** The attributes for `role` in `mode`, for a terminal with or without default colours */
    static attr_t AttrFor(Role role, Mode mode);

    /** The Unicode or the ASCII form of a symbol */
    static const char *SymbolFor(Symbol symbol, bool unicode);

    /**
     * Select the mode and, for colour, initialize the colour pairs.
     * Call after `initscr()`.
     */
    void Init(const std::string &theme_pref);

    Mode GetMode() const
    {
        return mode_;
    }

    attr_t Attr(Role role) const
    {
        return AttrFor(role, mode_);
    }

    /** A symbol in Unicode if the `LC_CTYPE` locale is UTF-8, else in ASCII */
    const char *Sym(Symbol symbol) const
    {
        return SymbolFor(symbol, unicode_);
    }

private:
    Mode mode_ = Mode::MONO;
    bool unicode_ = false;
};

/**
 * Set the attributes of a window, including a colour pair in `attrs`.
 * Unlike `wattr_set(win, attrs, 0, nullptr)`, which drops the colour pair.
 */
inline void SetAttrs(WINDOW *win, attr_t attrs)
{
    wattr_set(win, attrs & ~A_COLOR, static_cast<short>(PAIR_NUMBER(attrs)), nullptr);
}

/** Turns on the attributes of a role in a window for the lifetime of the object */
class ScopedAttr
{
public:
    ScopedAttr(WINDOW *win, attr_t attrs) : win_(win), attrs_(attrs)
    {
        wattr_on(win_, attrs_, nullptr);
    }
    ScopedAttr(WINDOW *win, const Theme &theme, Role role) : ScopedAttr(win, theme.Attr(role))
    {
    }
    ~ScopedAttr()
    {
        wattr_off(win_, attrs_, nullptr);
    }
    ScopedAttr(const ScopedAttr &) = delete;
    ScopedAttr &operator=(const ScopedAttr &) = delete;

private:
    WINDOW *win_;
    attr_t attrs_;
};

#endif  // #ifndef HAVE_THEME_H
