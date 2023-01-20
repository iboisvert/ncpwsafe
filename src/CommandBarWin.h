/* Copyright 2020 Ian Boisvert */
#ifndef HAVE_COMMANDBARWIN_H
#define HAVE_COMMANDBARWIN_H

#include <map>
#include <vector>
#include <string>

class PWSafeApp;

struct Action
{
    static const Action YES;
    static const Action NO;

    /**
     * Mask to determine if this action will be displayed.
     * If `opts & m_mask == 0` the action will not be displayed.
     */
    int m_mask;
    std::string m_key;
    std::string m_name;
    std::string m_description;

    Action(int mask, std::string key, std::string name, std::string description)
        : m_mask(mask), m_key(key), m_name(name), m_description(description)
    {
    }
    Action(std::string key, std::string name, std::string description) : Action(-1, key, name, description)
    {
    }
    Action(int mask, std::string key, std::string name) : Action(mask, key, name, "")
    {
    }
    Action(std::string key, std::string name) : Action(key, name, "")
    {
    }
};

class CommandBarWin
{
public:
    static const std::vector<Action> YES_NO;

    CommandBarWin(PWSafeApp &app, WINDOW *win) : m_app(app), m_win(win)
    {
    }

    /** Assign command bar actions to object `p` */
    void Register(void *p, std::vector<Action> actions)
    {
        m_actions[p] = actions;
    }

    /**
     * Activate the command bar registered to `p`.
     * @param opts Flags identifying the actions that will be displayed.
     *             Actions where `opts & action.m_mask == 0` will not be displayed.
     * @see CommandBarWin::Register()
     */
    void Show(void *p, int opts = -1);

    /** Update the command bar with the give actions */
    void Show(std::vector<Action> actions);

    // Generate help screen from list of actions
    // void ShowHelp()

    static void ShowActions(WINDOW *win, const std::vector<Action> &actions, int opts = -1);

private:
    PWSafeApp &m_app;
    std::map<void *, std::vector<Action>> m_actions;

    WINDOW *m_win = nullptr;
};

#endif