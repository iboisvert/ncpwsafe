/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_DIALOG_H
#define HAVE_DIALOG_H

#include <functional>
#include <map>
#include <string>
#include "libpwsafe.h"
#include "libncurses.h"

class PWSafeApp;

enum class DialogResult
{
    OK = 0,
    CANCEL,
    CONTINUE,
    YES,
    NO,
};

struct DialogField
{
    PwsFieldType m_fieldType;
    std::string m_label;
    std::string m_value;
    int m_width;
    int m_fieldOptsOn;
    int m_fieldOptsOff;
};

class Dialog;

/** @returns `false` */
inline bool DefaultInputDelegate(Dialog & /*dialog*/, int /*ch*/, DialogResult & /*result*/)
{
    return false;
}
/** @returns `true` */
inline bool DefaultFormValidate(const Dialog & /*dialog*/)
{
    return true;
}
/** @returns `true` */
inline bool DefaultDiscardChanges(const Dialog & /*dialog*/)
{
    return true;
}

class Dialog
{
public:
    typedef std::function<bool(Dialog &, int, DialogResult &)> InputHandler;
    typedef std::function<bool(const Dialog &)> Callback;

    /**
     * @param validateCallback A function callback to validate form data when the user saves.
     * if `validateCallback` returns `false`, editing continues.
     * @param discardChangesCallback A function callback to confirm the user wants to discard changes
     * when the user cancels. If `discardChangesCallback` returns false, editing continues.
     * @param inputDelegate Delegate input first to this handler.
     * If `inputDelegate` returns `true` the dialog exits and `Show()` returns.
     * @returns The default return value is `DialogResult::CANCEL`. The return value
     * can be modified by `inputDelegate`.
     */
    Dialog(PWSafeApp &app, const std::vector<DialogField> &fields, bool readOnly = false,
        Callback validateCallback = &DefaultFormValidate, Callback discardChangesCallback = &DefaultDiscardChanges,
        InputHandler inputDelegate = &DefaultInputDelegate)
        : m_app(app), m_dialogFields(fields), m_readOnly(readOnly), m_inputDelegate(inputDelegate),
          m_validateCallback(validateCallback), m_discardChangesCallback(discardChangesCallback)
    {
        ConstructFields();
    }

    /** Set the field to be active when the Dialog is initialized. */
    Dialog &SetActiveField(PwsFieldType fieldType);

    DialogResult Show(WINDOW *parent, const std::string &title);

    PWSafeApp &GetApp() const
    {
        return m_app;
    }

    WINDOW *GetWindow() const
    {
        return m_win;
    }

    WINDOW *GetParentWindow() const
    {
        return m_parentWin;
    }

    bool IsReadOnly() const
    {
        return m_readOnly;
    }

    /** Gets the curses `FIELD` for the given `FieldType` */
    FIELD *GetField(PwsFieldType ft) const;
    /** Sets the value of a curses `FIELD` for the given `FieldType` */
    void SetField(PwsFieldType ft, const std::string &value);
    /** Gets the value of the field with the given `FieldType` */
    const std::string &GetValue(PwsFieldType ft) const;

private:
    friend int DefaultInputHandler(Dialog &);

    void ConstructFields();
    void InitTUI(const std::string &title);
    void EndTUI();
    /** Input driver */
    DialogResult ProcessInput();
    /** Copy data from form fields to values array */
    void SaveData();
    /** Validate form field values */
    bool ValidateForm();
    /** If field value changed, ask for confirmation to quit */
    bool DiscardChanges();
    /** Scramble curses field buffers */
    void RandomizeBuffers();

    PWSafeApp &m_app;

    const std::vector<DialogField> &m_dialogFields;
    bool m_readOnly = false;
    InputHandler m_inputDelegate;
    Callback m_validateCallback;
    Callback m_discardChangesCallback;
    std::map<PwsFieldType, std::string> m_values;
    int m_maxFieldWidth;

    WINDOW *m_parentWin = nullptr;
    WINDOW *m_win = nullptr;
    PANEL *m_panel = nullptr;
    WINDOW *m_formWin = nullptr;
    FORM *form_ = nullptr;
    std::vector<FIELD *> m_fields;
    FIELD *m_activeField = nullptr;
    int m_saveCursor = 0;
};

#endif  //#ifndef HAVE_DIALOG_H