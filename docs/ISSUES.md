# Issues

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
