# Testing ncpwsafe

There are two kinds of tests, and CTest runs both:

| Kind | Where | What it checks | CTest labels |
|---|---|---|---|
| Unit tests | `test/*-tests.cpp` | Classes in `libncpwsafe`, with GoogleTest | none |
| End-to-end CLI tests | `test/e2e/cli/` | The `--generate-password`, `--export-db`, `--change-password` and `--test-db` commands, run as a user would | `e2e`, `cli` |
| End-to-end TUI tests | `test/e2e/tui/` | The ncurses UI, driven through a detached tmux server | `e2e`, `tui` |

## Requirements

- The build's own dependencies, see the top-level [README](../README.md).
- **GoogleTest** for the unit tests.
- **Python 3** (standard library only) for the end-to-end tests. Without
  it, CMake prints a warning and the end-to-end tests aren't registered.
- **tmux** for the TUI tests. Without it, the TUI tests are reported as
  *skipped*, not failed. They were developed with tmux 3.8, and CI uses
  the tmux package from Ubuntu.

On Ubuntu: `sudo apt-get install libgtest-dev python3 tmux`.

### Windows

The unit tests and the CLI tests run on Windows too, with the
`x64-windows-msvc-*` presets. The CLI tests need Python 3 on the `PATH`
(it's preinstalled on GitHub's Windows runners). The TUI tests are always
reported as skipped on Windows, because tmux can't host a native Windows
console program.

## Running the tests

The build directory must be a direct child of the repository root (for
example `build/`), because the unit tests run with their working directory
set to `build/../test`.

```sh
cmake --preset=x64-linux-gcc-debug
cmake --build --preset=x64-linux-gcc-debug

ctest --preset=x64-linux-gcc-debug                    # everything
ctest --preset=x64-linux-gcc-debug -j8                # in parallel, every test is isolated
ctest --preset=x64-linux-gcc-debug -LE e2e            # unit tests only
ctest --preset=x64-linux-gcc-debug -L cli             # CLI tests only
ctest --preset=x64-linux-gcc-debug -L tui             # TUI tests only
ctest --preset=x64-linux-gcc-debug -R tui.edit        # one test file
ctest --preset=x64-linux-gcc-debug -R tui.edit -V     # ... with each test case's output
```

Each `test_*.py` file is one CTest test, named `<cli|tui>.<file name
without test_>`, e.g. `test/e2e/tui/test_edit.py` is `tui.edit`.

The unit tests can also be run directly:

```sh
./build/test/unittests --gtest_filter='AccountDbTest.*'
```

### Running one end-to-end test case

The scripts read the paths they need from the environment that CTest sets.
To run a script by hand, set them yourself, then use the standard
`unittest` options (`-k` selects test cases by name):

```sh
export NCPWSAFE=$PWD/build/bin/ncpwsafe \
       E2E_MKDB=$PWD/build/test/e2e/e2e-mkdb \
       TMUX_BIN=$(command -v tmux) \
       LANG=C.UTF-8 LC_ALL=C.UTF-8
python3 -X utf8 test/e2e/tui/test_edit.py -k test_add_account
```

On Windows, in PowerShell:

```powershell
$env:NCPWSAFE = "$PWD\build\bin\ncpwsafe.exe"
$env:E2E_MKDB = "$PWD\build\test\e2e\e2e-mkdb.exe"
python -X utf8 test\e2e\cli\test_export.py -k test_export
```

`-X utf8` makes Python read and write files and pipes as UTF-8, which
Windows doesn't do by default. CTest passes it to every script.

To see the UI while a TUI test runs, find its tmux server and attach to
it from another terminal (the socket names are `ncpwsafe-e2e-<pid>-<n>`):

```sh
ls /tmp/tmux-$(id -u)/
tmux -L ncpwsafe-e2e-12345-0 attach -r
```

### Debug-only and sanitizer builds

`--test-db` only exists in Debug builds. `cli.test_db` exits with code 77
in Release builds, and CTest reports it as skipped.

Running the end-to-end tests against a `-DNCPWSAFE_DEBUG_SANITIZE=ON`
build is a good way to find memory errors in full UI sessions. A sanitizer
report makes ncpwsafe exit non-zero, which fails the test.

## How the end-to-end tests work

```
test/e2e/
  CMakeLists.txt     registers each test_*.py with CTest, builds e2e-mkdb
  mkdb.cpp           e2e-mkdb: builds an account database from a .tsv spec
  e2elib.py          shared helpers and the E2ETestCase/TuiTestCase base classes
  tmuxdriver.py      TmuxApp: start the app in tmux, send keys, read the screen
  fixtures/
    small.tsv        the accounts in the "small" test database
    small.pw         its password
    empty.tsv        a database with no accounts
    sparse.tsv       accounts with fields left out
    unicode.tsv      accounts with non-ASCII groups and titles
    control-chars.tsv  an account with a control character in its title
    roundtrip.tsv    accounts with line breaks, tabs and trailing spaces in their values,
                     and notes longer than the notes field
  cli/test_*.py      CLI tests
  tui/test_*.py      TUI tests
```

### Fixtures

Test databases aren't committed. Each test builds the one it needs with
`e2e-mkdb`, from a tab-separated spec in `fixtures/` (group, title, user,
password, notes, one account per line). In a field, `\n`, `\r`, `\t` and
`\\` stand for a line feed, carriage return, tab and backslash; `read_spec()`
in `e2elib.py` reads them the same way. `e2e-mkdb` writes the file with
libpwsafe, the same code ncpwsafe uses, and it works in Release builds,
where `--test-db` doesn't exist.

Tests check results by exporting the database with `--export-db` and
comparing the rows, without UUIDs, to the spec. `assertDbContains` in
`e2elib.py` does this.

### Isolation

Every test case gets:

- a new temporary directory, removed afterwards, holding its databases,
  password files and exports. It is under `/tmp` on Linux and under
  `%TEMP%` on Windows;
- its own `$HOME` in that directory (on Windows, also `%USERPROFILE%` and
  `%APPDATA%`), so the preferences file
  (`~/.config/ncpwsafe/ncpwsafe.ini`, or
  `%APPDATA%\ncpwsafe\ncpwsafe.ini` on Windows) and database backups
  never touch your real home directory;
- for TUI tests, its own tmux server (`tmux -L <unique name> -f /dev/null`),
  so tests can run in parallel and never see your own tmux sessions or
  configuration.

### Driving the UI

`TuiTestCase.start()` runs ncpwsafe in a 100x30 tmux window with
`TERM=screen`, and returns a `TmuxApp`:

| Method | Does |
|---|---|
| `send("C-a")`, `send("Down", "Enter")` | Send keys, using tmux key names (`Enter`, `Escape`, `BSpace`, `DC`, `Up`, `Home`, `NPage`, `C-x`, ...) |
| `type("text")` | Type literal text |
| `screen()`, `text()` | The screen as lines, or as one string. Line drawing shows as the ACS letters `lqk`, `x`, `mqj`, also when ncurses draws it with Unicode box drawing characters |
| `highlighted()` | The names of the rows in reverse video, i.e. the selection: the first column of the row (an account's title, or a group heading without its `▾`/`▸` marker, rule and count). The command bar is left out |
| `wait_for(text)`, `wait_gone(text)`, `wait_highlighted(text)`, `wait_until(fn, what)` | Poll until the screen matches, or fail with a dump of the screen after 10 s |
| `resize(cols, rows)` | Resize the window. ncpwsafe gets `SIGWINCH`/`KEY_RESIZE` |
| `wait_exit()` | Wait for ncpwsafe to exit and return its exit code |
| `pid()` | The process id of ncpwsafe, e.g. to inspect `/proc/<pid>` |

Always wait for something on screen after sending keys, rather than
sleeping. When a key should do nothing, send a key that does something
visible after it and wait for that, then check that nothing else changed
(see `test_readonly.py`). The exception is a test of time itself:
`test_auto_sign_off.py` sleeps to check that ncpwsafe is still open after
some time, and keeps its periods to a few seconds.

Exit codes: 0 after `^S` (save and exit), 1 (`e2elib.PRC_ERR_FAIL`) after
leaving with `^X` or cancelling the password prompt, 124 after an auto
sign-off. Every TUI test runs with the default auto sign-off of 5 minutes;
to test it, write a short `auto-sign-off` to `self.config_file` before
starting ncpwsafe.

### Clipboard

`TuiTestCase` points `NCPWSAFE_XCLIP` at a stub script that writes what it
is given to `clipboard.txt` in the test directory, and sets `DISPLAY` so
ncpwsafe tries to copy. ncpwsafe runs xclip with only `DISPLAY` and
`XAUTHORITY` in its environment, so the output path is written into the
stub instead of being passed through the environment.
`clipboard_stub(exit_code=1)` makes a stub that fails.

### Known bugs

Tests for behaviour that is currently broken are marked
`@unittest.expectedFailure`, with a comment explaining the bug. They pass
while the bug is there. `e2elib.expected_failure_if(condition)` does the
same only when `condition` is true, e.g. `IS_DEBUG_BUILD` for a bug that only
Debug build assertions catch. Once the bug is fixed, the test reports an
"unexpected success", which fails the run, so remove the decorator in the
same change as the fix.

```sh
grep -rn -A4 expectedFailure test/e2e
```

## Choosing a kind of test

Test each behaviour at the lowest level that can check it, and add a
higher-level test only for what the lower levels can't reach.

**Unit test (GoogleTest)** when the behaviour lives in one class or
function and can be checked without a terminal: parsing, validation,
password generation, sorting and merging records, backup rotation,
preferences. Unit tests are the default for edge cases and error paths
(bad input, I/O errors through the `Filesystem` mock), because they are
cheap to list exhaustively. If the logic is tangled with ncurses, split it
into a function that doesn't use ncurses, so it can be unit-tested.

**CLI end-to-end test** for anything observable from the command line:
options and the checks in `ValidateArgs`, exit codes, messages, and the
effect on files (what was written, what an export contains, backups). Most
of this is in `main.cpp`, which the unit tests don't link.

**TUI end-to-end test** for user-visible flows that only work when several
components are put together: key bindings, dialogs and their validation,
what ends up saved, read-only mode, the clipboard, resize. Cover the main
path and the failures that matter, and leave the exhaustive edge cases to
unit tests. TUI tests are the slowest and the most sensitive to wording
and layout changes. They catch wiring bugs that unit tests miss, such as
a stale reference between the accounts list and the records (see
`docs/ISSUES.md`).

**Fixing a bug**: add a test that reproduces it at the lowest level that
can, and an end-to-end test as well if the bug was only visible through
the CLI or the UI. To record a bug before it is fixed, add the test with
`@unittest.expectedFailure` (see [Known bugs](#known-bugs)) and an entry
in `docs/ISSUES.md`.

**Changing behaviour**: update the tests that cover it in the same change.
UI changes (keys, messages, dialog titles and fields, command bar entries)
usually break TUI tests that wait for that text.

**Still manual**: the terminal matrix (8 and 256 colours, `TERM` values,
`NO_COLOR`, light and dark themes) and suspend and resume. The TUI tests
only run under tmux with `TERM=screen`. See the manual checklists in
`docs/dev-plan.md`.

## Adding tests

- **A unit test**: add a `*-tests.cpp` file to the `unittests` list in
  `test/CMakeLists.txt`. A test that needs private members needs a matching
  `FRIEND_TEST` declaration in the class header.
- **An end-to-end test case**: add a method to the right `test_*.py` file.
- **An end-to-end test file**: create `test/e2e/cli/test_<name>.py` or
  `test/e2e/tui/test_<name>.py`, following an existing one. It needs the
  `sys.path` line, `import e2elib`, `e2elib.require_tmux()` for TUI tests,
  and `e2elib.main()` at the end. Re-run CMake configure so CTest picks it
  up (the glob uses `CONFIGURE_DEPENDS`, so a build also does this).
- **A fixture**: add `fixtures/<name>.tsv` and use `self.make_db("<name>")`.
- A script that can't run in the current environment should exit with
  `e2elib.SKIP_EXIT_CODE` (77), so CTest reports it as skipped.
- CLI tests must run on Linux and Windows. Build paths with `pathlib`, not
  `/` strings; pass `encoding="utf-8"` (or rely on `-X utf8`) when reading
  files; check crashes with `assertNotCrashed()` rather than a negative exit
  code; and use `e2elib.IS_WINDOWS` to skip a test that can't apply there,
  such as one that depends on POSIX permission bits.
