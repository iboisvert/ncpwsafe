/* Copyright 2022 Ian Boisvert */
#include "ChangeDbPasswordDlg.h"
#include "Dialog.h"
#include "MessageBox.h"
#include "Utils.h"

static PwsFieldType NEW_PASSWORD = static_cast<PwsFieldType>(FT_END+1);
static PwsFieldType NEW_PASSWORD_CONFIRM = static_cast<PwsFieldType>(FT_END+2);

ChangeDbPasswordDlg::ChangeDbPasswordDlg(PWSafeApp &app)
    : m_app(app)
{
    m_app.GetCommandBar().Register(this, {
        {"^S", "Save and close", "Save the new password and close"},
        {"^X", "Cancel", "Cancel changing password"},
    });
}

/** Validate form field values */
bool ChangeDbPasswordDlg::ValidateForm(const Dialog &dialog)
{
    WINDOW *win = dialog.GetParentWindow();
    AccountDb &db = m_app.GetDb();
    if (!db.CheckPassword(dialog.GetValue(FT_PASSWORD)))
    {
        const char *msg = "Account database password is incorrect";
        MessageBox(m_app).Show(win, msg);
        return false;
    }
    if (dialog.GetValue(NEW_PASSWORD) != dialog.GetValue(NEW_PASSWORD_CONFIRM))
    {
        const char *msg = "New passwords do not match";
        MessageBox(m_app).Show(win, msg);
        return false;
    }
    if (dialog.GetValue(NEW_PASSWORD).empty())
    {
        m_app.GetCommandBar().Show(CommandBarWin::YES_NO);
        const char *msg = "Password is empty. Are you sure?";
        if (MessageBox(m_app).Show(win, msg, &YesNoKeyHandler) != DialogResult::YES)
            return false;
    }

    return true;
}

DialogResult ChangeDbPasswordDlg::Show(WINDOW *parent)
{
    using std::placeholders::_1;

    std::vector<DialogField> fields{
        {FT_PASSWORD, "Current password:", "", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC},
        {NEW_PASSWORD, "New password:", "", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC},
        {NEW_PASSWORD_CONFIRM, "Confirm password:", "", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC}};

    auto f_validate = std::bind(&ChangeDbPasswordDlg::ValidateForm, this, _1);
    Dialog dialog(m_app, fields, /*readOnly*/ false, f_validate);
    DialogResult result = dialog.Show(parent, "Change Account Database Password");
    if (result == DialogResult::OK)
    {
        m_password = dialog.GetValue(FT_PASSWORD);
        m_newPassword = dialog.GetValue(NEW_PASSWORD);
    }

    return result;
}
