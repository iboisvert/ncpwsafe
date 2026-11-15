/* Copyright 2020 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include "Dialog.h"

typedef bool (*KeyHandler)(int ch, DialogResult &result);

/** 
 * Key handler for `MessageBox`. 
 * \param[out] result DialogResult::OK
 * \return `true` if `ch` is newline. 
 */
extern bool DefaultMessageBoxKeyHandler(int ch, DialogResult &result);

class MessageBox
{
public:
    MessageBox(PWSafeApp &app): app_(app)
    {
    }

    /** 
     * Show the message box.
     * @returns The default return value is `RESULT_CANCEL`. The return value 
     * can be modified by `handler`.
     */
    DialogResult Show(WINDOW *parent, const std::string &msg, KeyHandler handler = &DefaultMessageBoxKeyHandler);

private:
    void InitTUI(WINDOW *parent, const std::string &msg);
    /** Create a window for the message box, centered on the parent window */
    WINDOW *NewWindow() const;
    /** Draw the message box border and message */
    void Draw();
    /** Lay out the message box after the terminal has been resized */
    void OnResize();
    void EndTUI();
    /** Input driver */
    DialogResult ProcessInput(KeyHandler handler);

    PWSafeApp &app_;
    
    WINDOW *parent_win_ = nullptr;
    std::string msg_;
    WINDOW *win_ = nullptr;
    PANEL *panel_ = nullptr;
    int save_cursor_ = 0;
};


/** 
 * Key handler for `MessageBox`. 
 * \param[out] result DialogResult::YES if ch == 'y', DialogResult::NO if ch == 'n', 
 *                    otherwise unchanged
 * \return `true` if `ch` is 'y' or 'n', otherwise `false`. 
 */
inline bool YesNoKeyHandler(int ch, DialogResult &result)
{
    ch = tolower(ch);
    if (ch == 'y')
    {
        result = DialogResult::YES;
        return true;
    }
    else if (ch == 'n')
    {
        result = DialogResult::NO;
        return true;
    }
    return false;
}
