/* Copyright 2022 Ian Boisvert */
#include <functional>

#include "SafeCombinationPromptDlg.h"
#include "Dialog.h"
#include "MessageBox.h"
#include "Filesystem.h"
#include "PWSafeApp.h"
#include "Utils.h"

SafeCombinationPromptDlg::SafeCombinationPromptDlg(PWSafeApp &app) : app_(app)
{
    app_.GetCommandBar().Register(this, {
        {"^S", "Save and continue", "Unlock the account database"},
        {"^X", "Cancel", "Cancel opening the account database"}
    });
}

bool SafeCombinationPromptDlg::InputHandler(Dialog &dialog, int &ch, DialogResult &/*result*/)
{
    if (const DialogField *pfield = dialog.GetActiveField(); pfield && pfield->m_fieldType == FT_PASSWORD && ch == '\n')
    {
        ch = KEY_CTRL('S');
    }
    return false;
}

std::string SafeCombinationPromptDlg::CheckDatabase(AccountDb &db, const std::string &db_pathname,
    const std::string &password, const char *incorrect_password_msg)
{
    if (db_pathname.empty())
    {
        return "Account database file is required";
    }
    if (!fs::Exists(db_pathname))
    {
        return std::string("File ").append(db_pathname).append(" does not exist");
    }
    if (fs::FileSize(db_pathname) == 0)
    {
        return std::string("An error occurred opening file ").append(db_pathname).append(".\nCheck that the file is valid.");
    }

    db.DbPathname() = db_pathname;
    db.Password() = password;

    int rc = PRC_SUCCESS;
    if (!db.CheckPassword(&rc))
    {
        if (rc == RC_ERR_INCORRECT_PASSWORD)
        {
            return incorrect_password_msg;
        }
        return "Incorrect passkey, not a PasswordSafe database, or a corrupt database.";
    }
    return "";
}

bool SafeCombinationPromptDlg::ValidateForm(const Dialog &dialog)
{
    std::string msg = CheckDatabase(app_.GetDb(), dialog.GetValue(FT_FILEPATH), dialog.GetValue(FT_PASSWORD));
    if (!msg.empty())
    {
        MessageBox(app_).Show(parent_win_, msg.c_str());
        redrawwin(dialog.GetWindow());
        SetCommandBarWin();
        return false;
    }
    return true;
}

void SafeCombinationPromptDlg::SetCommandBarWin()
{
    app_.GetCommandBar().Show(this);
}

DialogResult SafeCombinationPromptDlg::Show(WINDOW *parent)
{
    using std::placeholders::_1, std::placeholders::_2, std::placeholders::_3;

    parent_win_ = parent;

    SetCommandBarWin();

    const std::string &db_pathname = app_.GetDb().DbPathname();
    std::vector<DialogField> fields{
        {FT_FILEPATH, "Database:", db_pathname, /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC},
        {FT_PASSWORD, "Password:", "", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC}};

    auto fvalidate = std::bind(&SafeCombinationPromptDlg::ValidateForm, this, _1);
    auto finputHandler = std::bind(&SafeCombinationPromptDlg::InputHandler, this, _1, _2, _3);
    Dialog dialog(app_, fields, /*readOnly*/ false, fvalidate, DefaultDiscardChanges, finputHandler);
    dialog.SetActiveField(db_pathname.empty() ? FT_FILEPATH : FT_PASSWORD);
    DialogResult result = dialog.Show(parent, "Unlock Database");
    if (result == DialogResult::OK)
    {
        // Retrieve values from dialog fields
        m_filename = dialog.GetValue(FT_FILEPATH);
        password_ = dialog.GetValue(FT_PASSWORD);
    }
    return result;
}
