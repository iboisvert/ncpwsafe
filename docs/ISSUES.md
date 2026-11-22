# Issues

Known open issues. Severity is one of:

- **Critical**: a bug that loses or corrupts account data, or exposes secrets.
- **Important**: a bug that breaks the build, the tests or a security guarantee, or that gets in the way of normal use.
- **Minor**: a smaller defect, a portability difference or a project-convention violation.

## 1. A comment or section in the preferences file aborts Debug builds

**Severity:** Important

`IniHandler()` in `src/Prefs.cpp` handles every node that libconfini dispatches, and asserts that
it is a key:

```cpp
assert(dispatch->type == IniNodeType::INI_KEY);
```

libconfini also dispatches comments, sections and other nodes. So a preferences file with a comment
or a section, such as

```ini
# a comment
backup-count = 2

[theme]
heading = bold
```

aborts a Debug build at startup (`Assertion 'dispatch->type == IniNodeType::INI_KEY' failed`, exit
134), even for `--generate-password`. A Release build doesn't abort, but stores every node's text as
a key and value, so comments and section names become preferences, and the TUI writes them back as
keys when it exits (`Prefs::WritePrefs()` writes the whole map).

The README already suggests editing the file by hand (`theme = mono`), and the planned templates
(`docs/dev-plan.md` §1.4) live in sections of this file.

**Fix:** take only `INI_KEY` nodes, and use the node's section (`append_to`) to tell top-level keys
from keys in sections. Add a unit test that reads a file with comments, blank lines and sections.

## 4. The save-error Retry? prompt can't be cancelled with ^X, and declining exits 0

**Severity:** Important

When saving fails, `AccountsWin::Save()` asks `Retry?` and shows `^X Cancel` in the command bar, but `YesNoCancelKeyHandler` cancels on `^C`, so `^X` does nothing. Answering `n` sets `retval = true`, so the program exits with `DialogResult::OK` and status 0 although nothing was saved. ncpwsafe(1) says status 0 means the changes were saved.

**Fix:** make `YesNoCancelKeyHandler` accept `^X` (or show the key it really accepts), and return failure when the user declines to retry, so the exit status is not 0. Add a TUI test that makes the save fail (for example a read-only directory) and checks both keys and the exit status.

## 5. Changing the database password loses unsaved changes and writes a mix of saved and unsaved data

**Severity:** Critical

In the accounts list, `^C` (Change Account Database Password) runs `ChangeDbPasswordCommand::Execute()`, which calls `AccountDb::ReadDb()` before `WriteDb()`. `ReadDb()` doesn't clear the records in memory: it passes each record in the file to `AccountRecords::Save()`, which overwrites the in-memory record with the same UUID and re-inserts records that were deleted. The file is then written with the new password. So, for changes not yet saved with `^S`:

- edits to existing accounts are lost, in memory and in the file;
- deleted accounts come back and are written;
- added accounts are kept and written.

`AccountsWin` doesn't call `AccountList::Build()` afterwards, so the list's rows still point to the records' old places, which `Save()` has inserted into and re-sorted. The list shows accounts under the wrong groups and leaves out the added ones; if the vector reallocated, the rows are dangling pointers. `^X` then still asks to discard changes, although the file has been written.

**Fix:** when changing the password from the TUI, don't re-read the file: set the new password and write the records in memory (or ask to save first). Call `list_.Build()` afterwards, and clear the dirty state only if everything in memory was written. `--change-password` still has to read the file, since it has nothing in memory. Add a TUI test that makes an edit, a deletion and an addition without saving, changes the password, and checks the list and `--export-db`.
