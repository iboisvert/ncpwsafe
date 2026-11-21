# Issues

## 9. `--change-password` exits 0 when it fails

**Status: open.** Test: `test_wrong_password_exit_code` in
`test/e2e/cli/test_change_password.py` (expected failure).

The `CHANGE_DB_PASSWORD` case in `main()` prints "Incorrect account database password"
or "Changing account database password failed", but never sets `result`, so the exit
code is always 0 and scripts can't detect the failure. These messages also go to stdout
rather than stderr, unlike `--export-db`'s errors.

## 10. The delete prompt names the user even when the title is unique

**Status: open.** Test: `test_delete_unique_title_omits_user` in
`test/e2e/tui/test_edit.py` (expected failure).

`AccountsWin::DeleteEntry` is meant to add "with user X" only when another account has
the same title. Its check,
`all_of(records, FieldCompare(FT_TITLE, record, *prec) || &record == prec)`, is true only
when *every* account has the same title, so the user is named almost every time. It
should be `none_of` over the other records having the same title, i.e.
`!FieldCompare(...) || &record == prec` inside `all_of`.

## 11. Cancelling a search that started on a group heading jumps to the first account

**Status: open.** Minor. No test.

`SearchBarWin::Show` saves the starting selection as an account record. When a group
heading is selected, `GetSelection()` returns `nullptr` and the saved position becomes
`records.begin()`, so Esc moves the selection to the first account instead of back to
the heading.

## 12. Crash opening a database with Unicode characters when the locale doesn't support Unicode

**Status: open.** Moved from the README's known issues. Not yet reproduced or tested.

Opening an account database that contains non-ASCII text crashes if the locale doesn't
support Unicode (e.g. `LANG=C`). Note that nothing in `src/` calls `setlocale`, so
ncurses and ICU run in the C locale whatever the environment says. The e2e tests set
`LANG`/`LC_ALL=C.UTF-8` and use ASCII-only fixtures, so they don't cover this. A test
could add a fixture with non-ASCII titles (the `ru` word list is one source) and open it
under `LANG=C`.
