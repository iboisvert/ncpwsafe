# Issues

Known open issues. Severity is one of:

- **Critical**: a bug that loses or corrupts account data, or exposes secrets.
- **Important**: a bug that breaks the build, the tests or a security guarantee, or that gets in the way of normal use.
- **Minor**: a smaller defect, a portability difference or a project-convention violation.

Issues 1–4 were found by trying out the UI brush-up (branch `uiux`, 2026-11-21).

## 4. The notes field shows only one line, and editing multi-line notes loses lines

**Severity:** Critical

Every dialog field is created one row high (`new_field(/*height*/ 1, ...)`, `src/Dialog.cpp:47`), so the Notes field of the account dialog (`src/AccountDetailsDlg.cpp:152`) shows a single line. Show 3 lines by default.

A field the user edits is still saved as it shows, so editing the notes of an account with line breaks in them keeps only the first line, and the rest is lost when the account is saved. Fields that aren't edited keep their stored value.

**Fix:** add a height to `DialogField` (default 1), use it in `Dialog::ConstructFields()` and in the dialog's row count and geometry, and give the notes field 3 rows. A multi-line field also needs Enter to insert a new line (`REQ_NEW_LINE`) and Up/Down to move between its lines, and how notes with line breaks are stored and shown must keep the stored line endings.
