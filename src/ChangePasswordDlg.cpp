/* Copyright 2022 Ian Boisvert */
#include "ChangePasswordDlg.h"
#include "GeneratePasswordDlg.h"
#include "Dialog.h"
#include "MessageBox.h"
#include "Utils.h"

static CItem::FieldType PASSWORD_CONFIRM = static_cast<CItem::FieldType>(CItem::LAST_USER_FIELD+1);

ChangePasswordDlg::ChangePasswordDlg(PWSafeApp &app)
    : m_app(app)
{
    m_app.GetCommandBar().Register(this, {
        {L"^S", L"Save and close", L"Save the new password and close"},
        {L"^X", L"Cancel", L"Cancel changing password"},
        {L"^P", L"Generate", L"Randomly generate a new password"}
    });
}

void ChangePasswordDlg::SetCommandBarWin()
{
    m_app.GetCommandBar().Show(this);
}

/** Validate form field values */
bool ChangePasswordDlg::ValidateForm(const Dialog &dialog)
{
    bool valid = dialog.GetValue(CItem::PASSWORD) == dialog.GetValue(PASSWORD_CONFIRM);
    if (!valid)
    {
        const wchar_t *msg = L"Passwords do not match";
        MessageBox(m_app).Show(dialog.GetParentWindow(), msg);

        SetCommandBarWin();
    }
    else if (dialog.GetValue(CItem::PASSWORD).empty())
    {
        m_app.GetCommandBar().Show(CommandBarWin::YES_NO);
        const wchar_t *msg = L"Password is empty. Are you sure?";
        valid = MessageBox(m_app).Show(dialog.GetParentWindow(), msg, &YesNoKeyHandler) == DialogResult::YES;

        SetCommandBarWin();
    }

    return valid;
}

bool ChangePasswordDlg::InputHandler(Dialog &dialog, int ch, DialogResult &result)
{
    int retval = false;

    if (ch == KEY_CTRL('P'))
    {
        GeneratePasswordDlg GeneratePasswordDlgDlg(m_app);
        WINDOW *win = dialog.GetParentWindow();
        result = GeneratePasswordDlgDlg.Show(win);
        if (result == DialogResult::OK)
        {
            const StringX &newPassword = GeneratePasswordDlgDlg.GetPassword();
            dialog.SetField(CItem::PASSWORD, newPassword);
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
        {CItem::PASSWORD, L"New password:", L"", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC},
        {PASSWORD_CONFIRM, L"Confirm password:", L"", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC}};

    auto f_validate = std::bind(&ChangePasswordDlg::ValidateForm, this, _1);
    auto f_inputHandler = std::bind(&ChangePasswordDlg::InputHandler, this, _1, _2, _3);
    Dialog dialog(m_app, fields, /*readOnly*/ false, f_validate, nullptr, f_inputHandler);
    DialogResult result = dialog.Show(parent, L"Change Password");
    if (result == DialogResult::OK)
    {
        m_password = dialog.GetValue(CItem::PASSWORD);
    }

    return result;
}
