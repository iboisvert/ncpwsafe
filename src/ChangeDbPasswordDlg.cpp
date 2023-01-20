/* Copyright 2022 Ian Boisvert */
#include "ChangeDbPasswordDlg.h"
#include "Dialog.h"
#include "MessageBox.h"
#include "Utils.h"

static CItem::FieldType NEW_PASSWORD = static_cast<CItem::FieldType>(CItem::LAST_USER_FIELD+1);
static CItem::FieldType NEW_PASSWORD_CONFIRM = static_cast<CItem::FieldType>(CItem::LAST_USER_FIELD+2);

ChangeDbPasswordDlg::ChangeDbPasswordDlg(PWSafeApp &app)
    : m_app(app)
{
    m_app.GetCommandBar().Register(this, {
        {L"^S", L"Save and close", L"Save the new password and close"},
        {L"^X", L"Cancel", L"Cancel changing password"},
    });
}

/** Validate form field values */
bool ChangeDbPasswordDlg::ValidateForm(const Dialog &dialog)
{
    WINDOW *win = dialog.GetParentWindow();
    PWScore &core = m_app.GetCore();
    if (core.CheckPasskey(core.GetCurFile(), dialog.GetValue(CItem::PASSWORD)) != PWScore::SUCCESS)
    {
        const wchar_t *msg = L"Account database password is incorrect";
        MessageBox(m_app).Show(win, msg);
        return false;
    }
    if (dialog.GetValue(NEW_PASSWORD) != dialog.GetValue(NEW_PASSWORD_CONFIRM))
    {
        const wchar_t *msg = L"New passwords do not match";
        MessageBox(m_app).Show(win, msg);
        return false;
    }
    if (dialog.GetValue(NEW_PASSWORD).empty())
    {
        m_app.GetCommandBar().Show(CommandBarWin::YES_NO);
        const wchar_t *msg = L"Password is empty. Are you sure?";
        if (MessageBox(m_app).Show(win, msg, &YesNoKeyHandler) != DialogResult::YES)
            return false;
    }

    return true;
}

DialogResult ChangeDbPasswordDlg::Show(WINDOW *parent)
{
    using std::placeholders::_1;

    std::vector<DialogField> fields{
        {CItem::PASSWORD, L"Current password:", L"", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC},
        {NEW_PASSWORD, L"New password:", L"", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC},
        {NEW_PASSWORD_CONFIRM, L"Confirm password:", L"", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC}};

    auto f_validate = std::bind(&ChangeDbPasswordDlg::ValidateForm, this, _1);
    Dialog dialog(m_app, fields, /*readOnly*/ false, f_validate);
    DialogResult result = dialog.Show(parent, L"Change Account Database Password");
    if (result == DialogResult::OK)
    {
        m_password = dialog.GetValue(CItem::PASSWORD);
        m_newPassword = dialog.GetValue(NEW_PASSWORD);
    }

    return result;
}
