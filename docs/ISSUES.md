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

## 2. An empty new database password is asked about, then rejected

**Severity:** Minor

In the Change Account Database Password dialog (`^C` in the accounts list), an empty new password
asks `Password is empty. Are you sure?`. Answering `y` closes the dialog, but
`ChangeDbPasswordCommand::Write()` returns `RC_ERR_INVALID_ARG` for an empty password, so the user
gets `An error occurred changing the account database password.` and the password isn't changed.
`--change-password` rejects an empty password too.

**Fix:** decide whether an empty database password is allowed. If it is, accept it in `Write()`
(keeping `--change-password`'s check if wanted); if not, reject it in the dialog's validation
instead of asking. Add a TUI test for the answer.

## 3. `--export-db` reports success when writing the file fails

**Severity:** Important

`ExportDbCommand::Execute()` in `src/ExportDbCommand.cpp` checks only that `fopen()` succeeds. It
ignores the results of `fprintf()` and of the `fclose()` that flushes the file, and returns
`RC_SUCCESS`. When the disk is full, or the file is on a network file system that fails on close,
`--export-db` exits 0 and leaves a truncated export, which the user may keep as their only copy.

**Fix:** close the file explicitly at the end, check `ferror()` and the result of `fclose()`, and
return an error (and remove the partial file) when either fails. A CLI test can export to
`/dev/full` on Linux.
