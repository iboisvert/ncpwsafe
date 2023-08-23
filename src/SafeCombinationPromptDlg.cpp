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

bool SafeCombinationPromptDlg::ValidateForm(const Dialog &dialog)
{
    std::string db_pathname = dialog.GetValue(FT_FILEPATH);
    std::string password = dialog.GetValue(FT_PASSWORD);

    if (db_pathname.empty())
    {
        MessageBox(app_).Show(parent_win_, "Account database file is required");
        redrawwin(dialog.GetWindow());
        SetCommandBarWin();
        return false;
    }
    else
    {
        bool exists = fs::Exists(db_pathname);
        if (exists)
        {
            uintmax_t len = fs::FileSize(db_pathname);
            if (len == 0)
            {
                std::string msg("An error occurred opening file ");
                msg.append(db_pathname).append(".\nCheck that the file is valid.");
                MessageBox(app_).Show(parent_win_, msg.c_str());
                redrawwin(dialog.GetWindow());
                SetCommandBarWin();
                return false;
            }
        }
        else
        {
            std::string msg("File ");
            msg.append(db_pathname).append(" does not exist");
            MessageBox(app_).Show(parent_win_, msg.c_str());
            redrawwin(dialog.GetWindow());
            SetCommandBarWin();
            return false;
        } // !exists
    }

    AccountDb &db = app_.GetDb();
    db.DbPathname() = db_pathname;
    db.Password() = password;

    int rc;
    if (!app_.GetDb().CheckPassword(&rc))
    {
        if (rc == RC_ERR_INCORRECT_PASSWORD)
        {
            MessageBox(app_).Show(parent_win_, "Incorrect password");
            redrawwin(dialog.GetWindow());
            SetCommandBarWin();
        }
        else
        {
            MessageBox(app_).Show(parent_win_, "Incorrect passkey, not a PasswordSafe database, or a corrupt database.");
            redrawwin(dialog.GetWindow());
            SetCommandBarWin();
        }
    }

    return rc == PRC_SUCCESS;
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
