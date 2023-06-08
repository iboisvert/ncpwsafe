/* Copyright 2022 Ian Boisvert */

#include <algorithm>

#include "Dialog.h"
#include "MessageBox.h"
#include "Utils.h"
#include "Label.h"

Dialog &Dialog::SetActiveField(PwsFieldType ft)
{
    m_activeField = GetField(ft);
    return *this;
}

// FIXME IMB 2022-11-15 Either calculate dialog size from width of fields
//   or calculate field width from dialog size

void Dialog::ConstructFields()
{
    m_fields.clear();
    m_maxFieldWidth = 0;
    // Fields are contained by a subwindow that is
    // moved to align with labels
    int row = 0;
    for (const DialogField &df : m_dialogFields)
    {
        if (df.m_width > m_maxFieldWidth) m_maxFieldWidth = df.m_width;

        FIELD *field = new_field(/*height*/ 1, df.m_width, row++, /*leftcol*/ 0, /*offscreen*/ 0, /*nbuffers*/ 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
        assert(("Unexpected field null", field));
#pragma GCC diagnostic pop
        set_field_userptr(field, const_cast<DialogField *>(&df));
        field_opts_on(field, df.m_fieldOptsOn);
        field_opts_off(field, df.m_fieldOptsOff);
        set_field_back(field, A_UNDERLINE);
        set_field_buffer(field, /*buf*/ 0, df.m_value.c_str());
        m_fields.push_back(field);
    }
    // End of fields marker
    m_fields.push_back(NULL);
}

DialogResult Dialog::Show(WINDOW *parent, const std::string &title)
{
    m_parentWin = parent;

    InitTUI(title);

    touchwin(parent);
    update_panels();
    doupdate();

    DialogResult result = ProcessInput();

    RandomizeBuffers();

    EndTUI();

    touchwin(parent);
    update_panels();
    doupdate();

    return result;
}

void Dialog::InitTUI(const std::string &title)
{
    // Calculate dimensions and coordinates to size and position dialog

    int max_label_width = 0;
    for (const DialogField &field : m_dialogFields)
    {
        // FIXME Correctly count number of characters in current locale, not number of bytes
        max_label_width = std::max(max_label_width, static_cast<int>(field.m_label.size()));
    }

    int max_y, max_x, beg_x, beg_y;
    getmaxyx(m_parentWin, max_y, max_x);
    getbegyx(m_parentWin, beg_y, beg_x);
    int nlines = m_fields.size();
    int ncols = m_maxFieldWidth + max_label_width + 1;
    int begin_y = (max_y - nlines + beg_y) / 2;
    int begin_x = (max_x - ncols + beg_x) / 2;

    // Add 2 to each dimension for border
    m_win = newwin(nlines + 2, ncols + 4, begin_y, begin_x);
    m_panel = new_panel(m_win);

    // Enable keypad so curses interprets function keys
    keypad(m_win, TRUE);
    box(m_win, 0, 0);
    wattron(m_win, A_BOLD);

    Label::WriteJustified(m_win, /*y*/ 0, /*begin_x*/ 2, ncols + 4, title.c_str(), JUSTIFY_CENTER);

    int label_begin_x = 2, form_begin_y = 2, form_begin_x = max_label_width + label_begin_x + 1;
    form_ = new_form(m_fields.data());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
    assert(("Unexpected form null", form_));
#pragma GCC diagnostic pop
    form_opts_off(form_, O_BS_OVERLOAD);
    set_form_win(form_, m_win);
    scale_form(form_, &nlines, &ncols);
    m_formWin = derwin(m_win, nlines, ncols, form_begin_y, form_begin_x);
    set_form_sub(form_, m_formWin);
    post_form(form_);

    int row = 2;
    for (const DialogField &df : m_dialogFields)
    {
        Label::Write(m_win, row++, label_begin_x, df.m_label.c_str());
    }

    wattroff(m_win, A_BOLD);

    if (m_activeField != nullptr)
    {
        set_current_field(form_, m_activeField);
    }

    form_driver(form_, REQ_END_LINE);

    // Reset cursor for input fields
    save_cursor_ = curs_set(1);
}

void Dialog::EndTUI()
{
    unpost_form(form_);
    free_form(form_);
    form_ = nullptr;
    for (FIELD *field : m_fields)
    {
        free_field(field);
    }
    m_fields.clear();

    del_panel(m_panel);
    m_panel = nullptr;
    delwin(m_formWin);
    m_formWin = nullptr;
    delwin(m_win);
    m_win = nullptr;

    curs_set(save_cursor_);
}

static std::string GetFieldValue(const FIELD *field)
{
    char *cbuf = field_buffer(field, /*buffer*/ 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
    assert(("Unexpected field buffer null", cbuf));
#pragma GCC diagnostic pop
    int rows, cols, max;
    dynamic_field_info(field, &rows, &cols, &max);
    std::string buf{cbuf, static_cast<size_t>(rows*cols)};
    rtrim(buf.begin(), buf.end());
    return buf;
}

/** Copy data from form fields to values array */
void Dialog::SaveData()
{
    // Ignore the null terminator field at the end of the field vector
    for (size_t i = 0; i < m_fields.size() - 1; ++i)
    {
        const FIELD *field = m_fields[i];
        const PwsFieldType ft = reinterpret_cast<const DialogField*>(field_userptr(field))->m_fieldType;
        m_values[ft] = GetFieldValue(field);
    }
}

DialogResult Dialog::ProcessInput()
{
    DialogResult result = DialogResult::CANCEL;
    int ch;
    while ((ch = wgetch(m_win)) != ERR)
    {
        if (m_inputDelegate(*this, ch, result))
        {
            SaveData();
            goto done;
        }

        switch (ch)
        {
        // Possibly add "apply" command ^A to apply changes and continue editing
        case KEY_CTRL('S'): {
            if (!m_readOnly)
            {
                // Required to synchronize window to buffer
                form_driver(form_, REQ_VALIDATION);
                SaveData();

                if (ValidateForm())
                {
                    result = DialogResult::OK;
                    goto done;
                }
            }
            break;
        }
        case KEY_CTRL('X'): {
            if (m_readOnly)
            {
                goto done;
            }
            else
            {
                // Required to synchronize window to buffer
                form_driver(form_, REQ_VALIDATION);
                SaveData();

                if (DiscardChanges())
                {
                    goto done;
                }
            }
            break;
        }
        case '\n':
        case '\t':
        case KEY_DOWN: {
            /* Go to next field */
            form_driver(form_, REQ_NEXT_FIELD);
            /* Go to the end of the present buffer */
            /* Leaves nicely at the last character */
            form_driver(form_, REQ_END_LINE);
            break;
        }
        case KEY_BTAB:
        case KEY_UP: {
            /* Go to previous field */
            form_driver(form_, REQ_PREV_FIELD);
            form_driver(form_, REQ_END_LINE);
            break;
        }
        case KEY_LEFT: {
            form_driver(form_, REQ_LEFT_CHAR);
            break;
        }
        case KEY_RIGHT: {
            form_driver(form_, REQ_RIGHT_CHAR);
            break;
        }
        case KEY_HOME: {
            form_driver(form_, REQ_BEG_LINE);
            break;
        }
        case KEY_END: {
            form_driver(form_, REQ_END_LINE);
            break;
        }
        case KEY_BACKSPACE: {
            if (!m_readOnly)
            {
                form_driver(form_, REQ_DEL_PREV);
            }
            break;
        }
        case KEY_DC: {
            if (!m_readOnly)
            {
                form_driver(form_, REQ_DEL_CHAR);
            }
            break;
        }
        default: {
            if (!m_readOnly)
            {
                form_driver(form_, ch);
            }
            break;
        }
        }
    }

done:
    return result;
}

bool Dialog::ValidateForm()
{
    return m_validateCallback ? m_validateCallback(*this) : true;
}

bool Dialog::DiscardChanges()
{
    return m_discardChangesCallback ? m_discardChangesCallback(*this) : true;
}

void Dialog::RandomizeBuffers()
{
    ZeroFieldsBuffer(m_fields.data());
}

FIELD *Dialog::GetField(PwsFieldType ft) const
{
    FIELD *retval = nullptr;
    auto it = std::find_if(m_fields.begin(), m_fields.end(), [ft](FIELD *field){
        return reinterpret_cast<const DialogField *>(field_userptr(field))->m_fieldType == ft;
    });
    if (it != m_fields.end())
    {
        retval = *it;
    }
    return retval;
}

/** Sets the value of a curses `FIELD` for the given `FieldType` */
void Dialog::SetField(PwsFieldType ft, const std::string &value)
{
    FIELD *field = GetField(ft);
    assert(field != nullptr);
    if (field != nullptr)
    {
        set_field_buffer(field, /*buf*/0, value.c_str());
    }
}

const std::string &Dialog::GetValue(PwsFieldType ft) const
{
    return m_values.at(ft);
}
