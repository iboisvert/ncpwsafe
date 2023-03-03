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
    void EndTUI();
    /** Input driver */
    DialogResult ProcessInput(KeyHandler handler);

    PWSafeApp &app_;
    
    WINDOW *m_win = nullptr;
    PANEL *m_panel = nullptr;
    int m_saveCursor = 0;
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
