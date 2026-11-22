# Issues

Known open issues. Severity is one of:

- **Critical**: a bug that loses or corrupts account data, or exposes secrets.
- **Important**: a bug that breaks the build, the tests or a security guarantee, or that gets in the way of normal use.
- **Minor**: a smaller defect, a portability difference or a project-convention violation.

Issues 1–4 were found by trying out the UI brush-up (branch `uiux`, 2026-11-21).

## 2. F1 opens the terminal's help instead of the ncpwsafe help

**Severity:** Important

The help page is bound to `KEY_F(1)` (`src/AccountsWin.cpp:746`, `src/Dialog.cpp:337`), and the command bar ends with `F1 More` (`CommandBarWin::MORE`, `src/CommandBarWin.cpp:10`). Many desktop terminals (GNOME Terminal, Konsole and others) and window managers take F1 for their own help, so the key never reaches ncpwsafe. Change the shortcut to `^H`.

Things to handle with `^H`:

- **Backspace:** terminals send either `^?` (DEL) or `^H` for Backspace. When the terminfo entry says Backspace is `^H` (`kbs=^H`), which includes `TERM=screen` (used by the TUI tests) and the Windows console, ncurses turns `^H` into `KEY_BACKSPACE` because of `keypad()`. Then `^H` can't be told apart from Backspace. In dialogs, `KEY_BACKSPACE` deletes the previous character (`src/Dialog.cpp:408`). Decide what the help key does on such terminals, e.g. accept `KEY_F(1)` as well, and check that Backspace still edits fields.
- Update `CommandBarWin::MORE`, the `KEY_F(1)` cases, `HelpWin`, AGENTS.md (the F1 convention), the README, and the TUI tests that send `F1` (`test/e2e/tui/test_navigation.py:119`, `:135`).

## 3. Closing an unchanged account entry asks to discard changes, and saving it changes the entry

**Severity:** Critical

In a personal database, opening some (not all) account entries and closing them right away with `^X`, without touching any field, shows "The account entry has changed. Discard changes?". The prompt comes from `AccountDetailsDlg::DiscardChanges()` (`src/AccountDetailsDlg.cpp:26`). It copies every form field back into the record with `SaveData()` and compares the result with the stored record. For these entries, a value read back from the form isn't the stored value.

**Cause.** `Dialog::ConstructFields()` puts each value into a one-row form field with `set_field_buffer()` (`src/Dialog.cpp:60`), and ncurses writes it much as it would write to a terminal. `GetFieldValue()` (`src/Dialog.cpp:288`) then reads back what is on that row and drops trailing whitespace with `rtrim()`. A value changes on the way when it holds:

| Stored | Read back |
|---|---|
| `line one\nline two` (multi-line notes) | `line one`: `\n` ends the row and the rest is dropped |
| `line one\r\nline two` (notes with Windows line endings) | empty: `\r` goes back to the start of the row and `\n` ends it |
| `\nafter a blank line` | empty |
| `abc\rdef` | `def` |
| `pw-trail ` (trailing space) | `pw-trail` |
| `a\tb` | `a       b` (tab expanded to spaces) |
| `abc\x1bdef`, `abc\x7fdef` | `abc^[def`, `abc^?def` (caret notation) |

Non-ASCII text (accents, Cyrillic, CJK, emoji, combining marks) and values longer than the field come back unchanged, so the earlier guess about `rows*cols` cutting off multibyte text was wrong. Multi-line notes are the likely cause in a personal database, especially one first written by Password Safe on Windows, which stores `\r\n`.

The same read-back is also what gets saved, which is the worse half of the bug:

- **Saving loses data.** Editing any field of an affected entry and saving with `^S` writes all of the read-back values, so the notes are cut to their first line, notes with `\r\n` are erased, and a password with trailing whitespace loses it. Nothing warns about this. Answering "No" to the discard prompt and then saving has the same effect.
- **Copying from the dialog copies the wrong text.** `^U` and `^P` in the dialog copy `Dialog::GetCurrentValue()`, the same read-back, so a password with a trailing space is copied without it. Copying from the accounts list uses the stored value and is right.
- **What's shown is wrong.** The dialog shows only the first line of multi-line notes, and nothing for notes with `\r\n` (see also issue 4).

Reproduced with the scratch fixture below (built with `e2e-mkdb`, which now accepts `\n`, `\r`, `\t` and `\\` escapes in fixture fields). Opening and closing each entry asked to discard changes for all but `Plain` and `NonAscii`. Editing only the title of `MultiLine`, `CrLf`, `TrailSpace` and `Tab` and saving stored notes `line one`, `` (empty), password `pw-trail` and notes `a       b`.

```
	Plain	alice	pw-plain	One line
	MultiLine	alice	pw-multi	line one\nline two
	CrLf	alice	pw-crlf	line one\r\nline two
	BlankFirst	alice	pw-blank	\nafter a blank line
	TrailSpace	alice	pw-trail 	notes
	Tab	alice	pw-tab	a\tb
	NonAscii	josé	pässwörd	naïve 日本語
```

**Fix:** stop round-tripping values the user didn't edit.

- In `ConstructFields()`, clear each field's status after the first `set_field_buffer()` (`set_field_status(field, FALSE)`). `set_field_buffer()` sets the status, and so do the user's edits through `form_driver()`, so the `^C` password change is already marked as edited. Then have `SaveData()`, `GetCurrentValue()` and `AccountDetailsDlg::SaveData()` keep the stored value for fields whose `field_status()` is still false.
- A field the user does edit still can't hold line breaks, tabs or control characters. The notes field needs real multi-line editing (issue 4): split the stored text into the rows of a multi-row field and join the rows with the stored line ending when saving. Until then, the notes of an entry with line breaks could be shown read-only, so they can't be cut short by an edit.
- Tests: add the fixture above (as `fixtures/roundtrip.tsv`) and TUI tests that open and close each entry without a prompt, and that edit only the title, save, and check with `assertDbContains` that every other field is unchanged. Add a unit test for the field read-back if `Dialog` gets a testable seam.

## 4. The notes field shows only one line

**Severity:** Minor

Every dialog field is created one row high (`new_field(/*height*/ 1, ...)`, `src/Dialog.cpp:47`), so the Notes field of the account dialog (`src/AccountDetailsDlg.cpp:152`) shows a single line. Show 3 lines by default.

**Fix:** add a height to `DialogField` (default 1), use it in `Dialog::ConstructFields()` and in the dialog's row count and geometry, and give the notes field 3 rows. A multi-line field also needs Enter to insert a new line (`REQ_NEW_LINE`) and Up/Down to move between its lines, and how notes with line breaks are stored and shown ties in with issue 3.
