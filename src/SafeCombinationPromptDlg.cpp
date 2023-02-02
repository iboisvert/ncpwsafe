/* Copyright 2022 Ian Boisvert */

#include <functional>
#include "SafeCombinationPromptDlg.h"
#include "Dialog.h"
#include "MessageBox.h"
#include "PWSafeApp.h"
#include "FileUtils.h"

SafeCombinationPromptDlg::SafeCombinationPromptDlg(PWSafeApp &app) : m_app(app)
{
    m_app.GetCommandBar().Register(this, {
        {"^S", "Save and continue", "Unlock the account database"},
        {"^X", "Cancel", "Cancel opening the account database"}
    });
}

bool SafeCombinationPromptDlg::ValidateForm(const Dialog &dialog)
{
    std::string filename = dialog.GetValue(FT_FILEPATH);
    std::string password = dialog.GetValue(FT_PASSWORD);

    if (filename.empty())
    {
        MessageBox(m_app).Show(m_parentWin, "Account database file is required");
        redrawwin(dialog.GetWindow());
        SetCommandBarWin();
        return false;
    }
    else
    {
        bool exists = FileExists(filename.c_str());
        if (exists)
        {
            size_t len = FileLength(filename.c_str());
            if (len == 0)
            {
                std::string msg("An error occurred opening file ");
                msg.append(filename).append(".\nCheck that the file is valid.");
                MessageBox(m_app).Show(m_parentWin, msg.c_str());
                redrawwin(dialog.GetWindow());
                SetCommandBarWin();
                return false;
            }
        }
        else
        {
            std::string msg("File ");
            msg.append(filename).append(" does not exist");
            MessageBox(m_app).Show(m_parentWin, msg.c_str());
            redrawwin(dialog.GetWindow());
            SetCommandBarWin();
            return false;
        } // !exists
    }

    PwsResultCode rc;
    if (!m_app.GetDb().CheckPassword(filename, password, &rc))
    {
        if (rc == PRC_ERR_INCORRECT_PW)
        {
            std::string msg;
            LoadAString(msg, IDSC_BADPASSWORD);
            MessageBox(m_app).Show(m_parentWin, msg);
            redrawwin(dialog.GetWindow());
            SetCommandBarWin();
        }
        else
        {
            MessageBox(m_app).Show(m_parentWin, "Incorrect passkey, not a PasswordSafe database, or a corrupt database.");
            redrawwin(dialog.GetWindow());
            SetCommandBarWin();
        }
    }

    return rc == PRC_SUCCESS;
}

void SafeCombinationPromptDlg::SetCommandBarWin()
{
    m_app.GetCommandBar().Show(this);
}

DialogResult SafeCombinationPromptDlg::Show(WINDOW *parent)
{
    using std::placeholders::_1;

    m_parentWin = parent;

    SetCommandBarWin();

    const std::string &filename = m_app.GetDb().DbPathName();
    std::vector<DialogField> fields{
        {FT_FILEPATH, "Database:", filename, /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC},
        {FT_PASSWORD, "Password:", "", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC}};

    auto fvalidate = std::bind(&SafeCombinationPromptDlg::ValidateForm, this, _1);
    Dialog dialog(m_app, fields, /*readOnly*/ false, fvalidate);
    dialog.SetActiveField(filename.empty() ? FT_FILEPATH : FT_PASSWORD);
    DialogResult result = dialog.Show(parent, "Unlock Database");
    if (result == DialogResult::OK)
    {
        // Retrieve values from dialog fields
        m_filename = dialog.GetValue(FT_FILEPATH);
        m_password = dialog.GetValue(FT_PASSWORD);
    }
    return result;
}
