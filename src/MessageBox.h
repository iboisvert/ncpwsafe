/* Copyright 2020 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include "Utils.h"

typedef bool (*KeyHandler)(int ch, DialogResult &result);

extern bool DefaultMessageBoxKeyHandler(int ch, DialogResult &result);

class MessageBox
{
public:
    MessageBox(PWSafeApp &app): m_app(app)
    {
    }

    /** 
     * Show the message box.
     * @returns The default return value is `RESULT_CANCEL`. The return value 
     * can be modified by `handler`.
     */
    DialogResult Show(WINDOW *parent, const stringT &msg, KeyHandler handler = &DefaultMessageBoxKeyHandler);

private:
    void InitTUI(WINDOW *parent, const stringT &msg);
    void EndTUI();
    /** Input driver */
    DialogResult ProcessInput(KeyHandler handler);

    PWSafeApp &m_app;
    
    WINDOW *m_win = nullptr;
    PANEL *m_panel = nullptr;
    int m_saveCursor = 0;
};
