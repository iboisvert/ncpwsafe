# Issues

## 1. README build instructions point to the wrong binary path

**Status: open.**

`README.md` says to run `./build/src/ncpwsafe`, but `src/CMakeLists.txt` sets
`RUNTIME_OUTPUT_DIRECTORY` to `${CMAKE_BINARY_DIR}/bin`. The binary is actually at
`./build/bin/ncpwsafe`.

## 2. Core-dump/ptrace protection is enabled in Debug builds, not Release

**Status: open.**

In `src/main.cpp`, `DisableCoreDump()` calls `prctl(PR_SET_DUMPABLE, 0)` only inside
`#ifndef NDEBUG`. That turns it on in Debug builds and leaves it off in Release builds.
The comment at the call site ("Don't allow ptrace or gdump on release build") says the
opposite. As written, Release builds can still be ptraced and can write core dumps that
may contain decrypted passwords. Debug builds block debugger attach.

## 3. Leftover in-source CMake build artifacts in the repo root

**Status: partly cleaned up.** `CMakeCache.txt` and most of the root files are gone,
which removes the risk to `build/`. Still left: `CMakeFiles/` and `config.h` in the
root, and `CMakeFiles/`, `Makefile`, `cmake_install.cmake`, `CTestTestfile.cmake` and
`cmake_test_discovery_*.json` under `test/`.

The repo root contained files from an in-source CMake configure (`CMakeCache.txt`,
`CMakeFiles/`, `Makefile`, `cmake_install.cmake`, `CTestTestfile.cmake`,
`CPackConfig.cmake`, `CPackSourceConfig.cmake`, `config.h`), along with similar files
under `test/`. They are gitignored, but a stale `CMakeCache.txt` in the source dir can
interfere with out-of-source builds in `build/`. The rest can be deleted.

## 4. Message boxes wider than the terminal are not shown

**Status: open.**

`MessageBox::InitTUI` sizes the window to fit the longest line of the message. If that
is wider than the terminal (e.g. "File <long pathname> does not exist"), `newwin` fails,
`wgetch(NULL)` returns `ERR`, and `ProcessInput` returns immediately, so the message is
never seen. Long lines should be wrapped or truncated to fit the terminal. Also,
`ProcessInput` treats any `ERR` from `wgetch` as closing the box.

## 5. Editing an account's group or title deletes a different account

**Status: open.** Test: `test_edit_account_moves_group` in `test/e2e/tui/test_edit.py`
(expected failure).

In `AccountsWin::ProcessInput`, Enter on an account passes `*record`, a reference into
the `AccountRecords` vector, to `UpdateMenu` as `old_record`. `UpdateMenu` calls
`records.Save(new_record)`, which re-sorts the vector. When the group or title changes,
the record moves, so `old_record` now refers to whichever account took its slot. The
UUID comparison then fails, and `records.Delete(old_record)` deletes that other account.
Saving writes the loss to the file. For example, with the e2e `small` fixture, moving
Bookstore from Shopping to Banking deletes Work mail. Copy the old record (or its UUID)
before calling `Save`.

## 7. Search doesn't match queries containing capital letters

**Status: open.** Test: `test_search_mixed_case` in `test/e2e/tui/test_navigation.py`
(expected failure).

`AccountRecord::FieldContainsCaseInsensitive` (`src/AccountRecord.cpp`) lowercases the
field value but not the query, so "book" finds Bookstore but "Book" finds nothing.
Lowercase `sub` too, or use ICU case folding on both.

## 8. `--password-policy` by name selects the wrong policy, and 0 is accepted

**Status: open.** Tests: `test_policy_by_name` and `test_policy_zero_rejected` in
`test/e2e/cli/test_generate_password.py` (expected failures).

- When the policy is given by name, `ParseArgs` stores the 0-based index of the matching
  name. `main()` treats `--password-policy` as 1-based (`args.password_policy_ - 1`), so
  a name selects the policy listed before it. `alpha/digit/symbol` gives index -1, which
  is out of bounds for `COMPOSITION_NAME` and `POLICY_CHARSET`. Store `i + 1`.
- `ValidateArgs` only rejects values `> PasswordPolicy::Composition::COUNT`, so
  `--password-policy=0` is accepted and also indexes at -1. Reject values `< 1` too.

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
