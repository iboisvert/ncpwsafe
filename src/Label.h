/* Copyright 2022 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"

class Label
{
public:
    Label(WINDOW *win, int y, int x, int width = 0, int attrs = 0, int justification = JUSTIFY_LEFT)
        : m_win(win), m_y(y), m_x(x), m_width(width), m_attrs(attrs), m_justification(justification)
    {
    }

    Label()
    {
    }

    static int Write(
        WINDOW *win, int y, int x, int width, const wchar_t *msg, int attrs = 0, int justification = JUSTIFY_LEFT)
    {
        if (attrs)
            wattron(win, attrs);
        if (justification != JUSTIFY_LEFT) x = CalcX(x, width, wcslen(msg), justification);
        int retval = mvwaddwstr(win, y, x, msg);
        if (attrs)
            wattroff(win, attrs);
        return retval;
    }

    static int Write(
        WINDOW *win, int y, int x, int width, const char *msg, int attrs = 0, int justification = JUSTIFY_LEFT)
    {
        if (attrs)
            wattron(win, attrs);
        if (justification != JUSTIFY_LEFT) x = CalcX(x, width, strlen(msg), justification);
        int retval = mvwaddstr(win, y, x, msg);
        if (attrs)
            wattroff(win, attrs);
        return retval;
    }

    static int WriteJustified(
        WINDOW *win, int y, int x, int width, const wchar_t *msg, int justification)
    {
        return Write(win, y, x, width, msg, /*attrs*/0, justification);
    }

    static int WriteJustified(
        WINDOW *win, int y, int x, int width, const char *msg, int justification)
    {
        return Write(win, y, x, width, msg, /*attrs*/0, justification);
    }

    static int Write(WINDOW *win, int y, int x, const wchar_t *msg)
    {
        return mvwaddwstr(win, y, x, msg);
    }

    static int Write(WINDOW *win, int y, int x, const char *msg)
    {
        return mvwaddstr(win, y, x, msg);
    }

    int Clear(int ch = ' ')
    {
        assert(m_win);
        return mvwhline(m_win, m_y, m_x, ch, m_width);
    }

    int Write(const wchar_t *msg)
    {
        assert(m_win);
        return Label::Write(m_win, m_y, m_x, m_width, msg, m_attrs, m_justification);
    }

    int Write(const char *msg)
    {
        assert(m_win);
        return Label::Write(m_win, m_y, m_x, m_width, msg, m_attrs, m_justification);
    }

    int Rewrite(const wchar_t *msg)
    {
        Clear();
        return Write(msg);
    }

    int Rewrite(const char *msg)
    {
        Clear();
        return Write(msg);
    }

private:
    /**
     * @param x Start column
     * @param width Width of field
     * @param length Length of message
     * @param justification One of one NO_JUSTIFICATION,
       JUSTIFY_RIGHT, JUSTIFY_LEFT, or JUSTIFY_CENTER
    */
    static int CalcX(int x, int width, int length, int justification)
    {
        switch (justification)
        {
        // case JUSTIFY_LEFT:
        case JUSTIFY_RIGHT: {
            x = std::max(x, width - length);
            break;
        }
        case JUSTIFY_CENTER: {
            x = std::max(x, (width + x - length) / 2);
            break;
        }
        }
        return x;
    }
    WINDOW *m_win = nullptr;
    int m_y, m_x;
    int m_width;
    int m_attrs;
    int m_justification;
};