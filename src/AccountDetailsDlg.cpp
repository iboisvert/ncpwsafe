/* Copyright 2020 Ian Boisvert */

#include <functional>
#include "libpwsafe.h"

#include "AccountDetailsDlg.h"
#include "Utils.h"
#include "ChangePasswordDlg.h"
#include "MessageBox.h"
#include "Dialog.h"
#include "AccountsWin.h"

AccountDetailsDlg::AccountDetailsDlg(PWSafeApp &app, const CItemData &item)
    : m_app(app), m_item(item), m_itemOrig(item)
{
    m_app.GetCommandBar().Register(this, {
        {~CBOPTS_READONLY, "^S", "Save and close"},
        {CBOPTS_READONLY, "^X", "Exit"},
        {~CBOPTS_READONLY, "^X", "Cancel"},
        {"^V", "View password", "Show or hide the account password"},
        {~CBOPTS_READONLY, "^C", "Change password", "Change the account password"},
        {"^U", "Copy user", "Copy the account user name to the clipboard"},
        {"^P", "Copy password", "Copy the account password to the clipboard"},
    });
}

/** Handle dialog cancel */
bool AccountDetailsDlg::DiscardChanges(const Dialog &dialog)
{
    bool retval = true;

    bool readOnly = dialog.IsReadOnly();
    if (!readOnly)
    {
        m_app.GetCommandBar().Show(CommandBarWin::YES_NO);

        SaveData(dialog);
        if (m_item != m_itemOrig)
        {
            // Ask for confirmation
            const char *msg = "The account entry has changed. Discard changes?";
            WINDOW *win = dialog.GetParentWindow();
            DialogResult result = MessageBox(m_app).Show(win, msg, &YesNoKeyHandler);
            retval = result == DialogResult::YES;

            SetCommandBarWin(readOnly);
        }
    }
    return retval;
}

/** Validate form field values */
bool AccountDetailsDlg::ValidateForm(const Dialog &dialog)
{
    const std::string &title = dialog.GetValue(FT_TITLE);
    if (title.empty())
    {
        WINDOW *win = dialog.GetParentWindow();
        MessageBox(m_app).Show(win, "Account title is required");

        SetCommandBarWin(dialog.IsReadOnly());
    }
    return (!title.empty());
}

bool AccountDetailsDlg::InputHandler(Dialog &dialog, int ch, DialogResult &/*result*/)
{
    WINDOW *win = dialog.GetParentWindow();
    const bool readOnly = dialog.IsReadOnly();

    if (!readOnly && ch == KEY_CTRL('C'))
    {
        ChangePasswordDlg pwprompt(m_app);
        if (pwprompt.Show(win) == DialogResult::OK)
        {
            const std::string &newPassword = pwprompt.GetPassword();
            FIELD *field = dialog.GetField(FT_PASSWORD);
            set_field_buffer(field, /*buf*/ 0, newPassword.c_str());
        }

        SetCommandBarWin(readOnly);
    }
    else if (ch == KEY_CTRL('V'))
    {
        FIELD *field = dialog.GetField(FT_PASSWORD);
        set_field_opts(field, field_opts(field) ^ O_PUBLIC);
    }
    else if (ch == KEY_CTRL('U'))
    {
        const std::string &str = m_item.GetUser();
        CopyTextToClipboard(m_app, dialog.GetParentWindow(), str);
    }
    else if (ch == KEY_CTRL('P'))
    {
        const std::string &str = m_item.GetPassword();
        CopyTextToClipboard(m_app, dialog.GetParentWindow(), str);
    }
    return false;
}

void AccountDetailsDlg::SetCommandBarWin(bool readOnly)
{
    const unsigned char opts = readOnly ? CBOPTS_READONLY : ~CBOPTS_READONLY;
    m_app.GetCommandBar().Show(this, opts);
}

/** 
 * Copy data from the dialog input controls back
 * into the account entry
 */
void AccountDetailsDlg::SaveData(const Dialog &dialog)
{
    m_item.SetGroup(dialog.GetValue(FT_GROUP));
    m_item.SetTitle(dialog.GetValue(FT_TITLE));
    m_item.SetUser(dialog.GetValue(FT_USER));
    m_item.SetPassword(dialog.GetValue(FT_PASSWORD));
    m_item.SetURL(dialog.GetValue(FT_URL));
    m_item.SetEmail(dialog.GetValue(FT_EMAIL));
    m_item.SetNotes(dialog.GetValue(FT_NOTES));
}

/** Show the account details */
DialogResult AccountDetailsDlg::Show(WINDOW *parent, bool readOnly)
{
    using std::placeholders::_1, std::placeholders::_2, std::placeholders::_3;

    SetCommandBarWin(readOnly);

    std::vector<DialogField> fields{
        {FT_GROUP, "Group:", m_item.GetGroup(), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_TITLE, "Title:", m_item.GetTitle(), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_USER, "User:", m_item.GetUser(), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_PASSWORD, "Password:", m_item.GetPassword(), /*m_width*/56, /*m_fieldOptsOn*/0, O_STATIC | O_PUBLIC | O_EDIT | O_ACTIVE},
        {FT_URL, "URL:", m_item.GetURL(), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_EMAIL, "Email:", m_item.GetEmail(), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_NOTES, "Notes:", m_item.GetNotes(), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC}
    };

    auto f_validate = std::bind(&AccountDetailsDlg::ValidateForm, this, _1);
    auto f_discardChanges = std::bind(&AccountDetailsDlg::DiscardChanges, this, _1);
    auto f_inputHandler = std::bind(&AccountDetailsDlg::InputHandler, this, _1, _2, _3);
    Dialog dialog(m_app, fields, readOnly, f_validate, f_discardChanges, f_inputHandler);
    DialogResult result = dialog.Show(parent, "View Account");
    if (result == DialogResult::OK)
    {
        SaveData(dialog);
    }
    return result;
}
