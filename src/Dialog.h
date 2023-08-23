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
    /** \param[in,out] ch The input character, can be modified by the input handler to affect subsequent processing. */
    typedef std::function<bool(Dialog &, int &ch, DialogResult &)> InputHandler;
    typedef std::function<bool(const Dialog &)> Callback;

    /**
     * @param validateCallback A function callback to validate form data when the user saves.
     * if `validateCallback` returns `false`, editing continues.
     * @param discardChangesCallback A function callback to confirm the user wants to discard changes
     * when the user cancels. If `discardChangesCallback` returns false, editing continues.
     * @param inputDelegate Delegate input first to this handler.
     * If `inputDelegate` returns `true` the dialog exits and `Show()` returns;
     * otherwise, input processing continues with the current character.
     * @returns The default return value is `DialogResult::CANCEL`. The return value
     * can be modified by `inputDelegate`.
     */
    Dialog(PWSafeApp &app, const std::vector<DialogField> &fields, bool readOnly = false,
        Callback validateCallback = &DefaultFormValidate, Callback discardChangesCallback = &DefaultDiscardChanges,
        InputHandler inputDelegate = &DefaultInputDelegate)
        : app_(app), dialog_fields_(fields), read_only_(readOnly), input_delegate_(inputDelegate),
          validate_callback_(validateCallback), discard_changes_callback_(discardChangesCallback)
    {
        ConstructFields();
    }

    /** Set the field to be active when the Dialog is initialized. */
    Dialog &SetActiveField(PwsFieldType fieldType);
    /** Gets the active field. */
    const DialogField *GetActiveField() const;

    DialogResult Show(WINDOW *parent, const std::string &title);

    PWSafeApp &GetApp() const
    {
        return app_;
    }

    WINDOW *GetWindow() const
    {
        return win_;
    }

    WINDOW *GetParentWindow() const
    {
        return parent_win_;
    }

    bool IsReadOnly() const
    {
        return read_only_;
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

    PWSafeApp &app_;

    const std::vector<DialogField> &dialog_fields_;
    bool read_only_ = false;
    InputHandler input_delegate_;
    Callback validate_callback_;
    Callback discard_changes_callback_;
    std::map<PwsFieldType, std::string> values_;
    int max_field_width_;

    WINDOW *parent_win_ = nullptr;
    WINDOW *win_ = nullptr;
    PANEL *panel_ = nullptr;
    WINDOW *form_win_ = nullptr;
    FORM *form_ = nullptr;
    std::vector<FIELD *> fields_;
    FIELD *active_field_ = nullptr;
    int save_cursor_ = 0;
};

#endif  //#ifndef HAVE_DIALOG_H