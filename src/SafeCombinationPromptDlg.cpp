/* Copyright 2022 Ian Boisvert */

#include <functional>
#include "SafeCombinationPromptDlg.h"
#include "Dialog.h"
#include "MessageBox.h"
#include "PWSafeApp.h"

SafeCombinationPromptDlg::SafeCombinationPromptDlg(PWSafeApp &app) : m_app(app)
{
    m_app.GetCommandBar().Register(this, {
        {L"^S", L"Save and continue", L"Unlock the account database"},
        {L"^X", L"Cancel", L"Cancel opening the account database"}
    });
}

bool SafeCombinationPromptDlg::ValidateForm(const Dialog &dialog)
{
    std::string filename = dialog.GetValue(FT_FILENAME);
    std::string password = dialog.GetValue(FT_PASSWORD);

    if (filename.empty())
    {
        MessageBox(m_app).Show(m_parentWin, L"Account database file is required");
        redrawwin(dialog.GetWindow());
        SetCommandBarWin();
        return false;
    }
    else
    {
        bool exists = pws_os::FileExists(filename.c_str());
        if (exists)
        {
            std::FILE *pfile = pws_os::FOpen(filename, "r");
            ulong64 len = 0;
            if (pfile != nullptr)
            {
                len = pws_os::fileLength(pfile);
                pws_os::FClose(pfile, /*bIsWrite*/false);
            }
            if (pfile == nullptr || len == 0)
            {
                std::string msg(L"An error occurred opening file ");
                msg.append(filename).append(L".\nCheck that the file is valid.");
                MessageBox(m_app).Show(m_parentWin, msg.c_str());
                redrawwin(dialog.GetWindow());
                SetCommandBarWin();
                return false;
            }
        }
        else
        {
            std::string msg(L"File ");
            msg.append(filename).append(L" does not exist");
            MessageBox(m_app).Show(m_parentWin, msg.c_str());
            redrawwin(dialog.GetWindow());
            SetCommandBarWin();
            return false;
        } // !exists
    }

    int status = m_app.GetCore().CheckPasskey(filename, password);
    switch (status)
    {
    case PWScore::WRONG_PASSWORD: {
        std::string msg;
        LoadAString(msg, IDSC_BADPASSWORD);
        MessageBox(m_app).Show(m_parentWin, msg);
        redrawwin(dialog.GetWindow());
        SetCommandBarWin();
        break;
    }
    case PWScore::SUCCESS: {
        break;
    }
    default: {
        MessageBox(m_app).Show(m_parentWin, L"Incorrect passkey, not a PasswordSafe database, or a corrupt database.");
        redrawwin(dialog.GetWindow());
        SetCommandBarWin();
        break;
    }
    }

    return status == PWScore::SUCCESS;
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

    const std::string filename = m_app.GetCore().GetCurFile();
    std::vector<DialogField> fields{
        {FT_FILENAME, L"Database:", filename, /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC},
        {FT_PASSWORD, L"Password:", L"", /*m_width*/ 40, /*m_fieldOptsOn*/ 0, O_STATIC | O_PUBLIC}};

    auto fvalidate = std::bind(&SafeCombinationPromptDlg::ValidateForm, this, _1);
    Dialog dialog(m_app, fields, /*readOnly*/ false, fvalidate);
    dialog.SetActiveField(filename.empty() ? FT_FILENAME : FT_PASSWORD);
    DialogResult result = dialog.Show(parent, L"Unlock Database");
    if (result == DialogResult::OK)
    {
        // Retrieve values from dialog fields
        m_filename = dialog.GetValue(FT_FILENAME);
        m_password = dialog.GetValue(FT_PASSWORD);
    }
    return result;
}
