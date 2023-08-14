/* Copyright 2022 Ian Boisvert */
#include "ChangePasswordDlg.h"
#include "GeneratePasswordDlg.h"
#include "Dialog.h"
#include "MessageBox.h"
#include "Utils.h"

static PwsFieldType PASSWORD_CONFIRM = static_cast<PwsFieldType>(FT_END+1);

ChangePasswordDlg::ChangePasswordDlg(PWSafeApp &app)
    : app_(app)
{
    app_.GetCommandBar().Register(this, {
        {"^S", "Save and close", "Save the new password and close"},
        {"^X", "Cancel", "Cancel changing password"},
        {"^P", "Generate", "Randomly generate a new password"}
    });
}

void ChangePasswordDlg::SetCommandBarWin()
{
    app_.GetCommandBar().Show(this);
}

/** Validate form field values */
bool ChangePasswordDlg::ValidateForm(const Dialog &dialog)
{
    bool valid = dialog.GetValue(FT_PASSWORD) == dialog.GetValue(PASSWORD_CONFIRM);
    if (!valid)
    {
        const char *msg = "Passwords do not match";
        MessageBox(app_).Show(dialog.GetParentWindow(), msg);

        SetCommandBarWin();
    }
    else if (dialog.GetValue(FT_PASSWORD).empty())
    {
        app_.GetCommandBar().Show(CommandBarWin::YES_NO);
        const char *msg = "Password is empty. Are you sure?";
        valid = MessageBox(app_).Show(dialog.GetParentWindow(), msg, &YesNoKeyHandler) == DialogResult::YES;

        SetCommandBarWin();
    }

    return valid;
}

bool ChangePasswordDlg::InputHandler(Dialog &dialog, int ch, DialogResult &result)
{
    int retval = false;

    if (ch == KEY_CTRL('P'))
    {
        GeneratePasswordDlg dlg(app_);
        WINDOW *win = dialog.GetParentWindow();
        result = dlg.Show(win);
        if (result == DialogResult::OK)
        {
            const std::string &newPassword = dlg.GetPassword();
            dialog.SetField(FT_PASSWORD, newPassword);
            dialog.SetField(PASSWORD_CONFIRM, newPassword);
            retval = true;
        }

        SetCommandBarWin();
    }

    return retval;
}

DialogResult ChangePasswordDlg::Show(WINDOW *parent)
{
    using std::placeholders::_1, std::placeholders::_2, std::placeholders::_3;

    SetCommandBarWin();

    std::vector<DialogField> fields{
        {FT_PASSWORD, "New password:", "", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC},
        {PASSWORD_CONFIRM, "Confirm password:", "", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC}};

    auto f_validate = std::bind(&ChangePasswordDlg::ValidateForm, this, _1);
    auto f_inputHandler = std::bind(&ChangePasswordDlg::InputHandler, this, _1, _2, _3);
    Dialog dialog(app_, fields, /*readOnly*/ false, f_validate, nullptr, f_inputHandler);
    DialogResult result = dialog.Show(parent, "Change Password");
    if (result == DialogResult::OK)
    {
        password_ = dialog.GetValue(FT_PASSWORD);
    }

    return result;
}
