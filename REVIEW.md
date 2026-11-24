# Review guidelines

How to review a change to ncpwsafe, by a person or an agent, before it is pushed
(see "Code review" in `AGENTS.md`) and on a pull request. Review the diff against
`main`, and run only the rules whose trigger the diff matches.

If the diff changes this file, review the change with the version on `main`
(`git show origin/main:REVIEW.md`), so that a change can't weaken the rules that judge it.

## Findings

Report each finding as one of:

- **Important**: a likely bug, a secret exposed or left in memory, account data lost or
  corrupted, a broken build, test or documented behaviour. Important findings block: fix
  them, or say in the pull request why not.
- **Nit**: a minor problem or a project-convention slip that doesn't change behaviour.
- **Pre-existing**: a problem in lines the change didn't touch. Don't block on it. If it is
  worth keeping, record it in `docs/ISSUES.md`, numbered and with its severity, in a commit
  of its own.

Give each finding the file and line, what goes wrong, and when (the input or state that
triggers it). Don't report what a check already enforces: compiler warnings, the unit and
end-to-end tests that CI runs on Linux and Windows, `cli.man_page` (every option, key and
preference documented, and the page lint-free), and the sanitizers in a
`-DNCPWSAFE_DEBUG_SANITIZE=ON` build.

## Always check

- **Changed checks.** A diff that edits this file, `AGENTS.md`, `.github/workflows/`, the
  warning flags in `cmake/toolchain-*.cmake`, the test helpers (`test/e2e/e2elib.py`,
  `tmuxdriver.py`), or `cli.man_page`: flag it for a human to review, whatever it changes.
- **Tests match the change.** A change of behaviour comes with a test that fails without
  it, at the lowest level that can check it (`test/README.md`, "Choosing a kind of test").
  A bug fix comes with a test that reproduces the bug. Flag tests that were weakened,
  deleted or skipped, assertions made looser, and sleeps added to TUI tests (they wait for
  the screen instead).
- **Known bugs.** A fix removes the bug's `@unittest.expectedFailure` and its entry in
  `docs/ISSUES.md` in the same change. A new `expectedFailure` has a comment and an
  `ISSUES.md` entry.
- **Docs in the same change.** A new or changed option, key, screen, message, preference,
  environment variable or exit status updates `ncpwsafe.1.in` and `--help` (`cli.man_page`
  only checks that they're listed, not that they're still right). A changed test layout or
  helper updates `test/README.md`. A UI change that makes the README screenshots out of date
  takes them again (`docs/images/make-screenshots.py`).

## When the diff touches ... (trigger → check)

### Passwords and other secrets

Trigger: a `std::string` or buffer that holds a password, account field or key; a
dialog with a password field; `*Command` classes; `main.cpp` arguments.

- Every copy is zeroed (`ZeroString()`, `ZeroMemory()`) before it is freed. Look for the
  copies that are easy to miss: parameters taken by value and copied into a member, a
  shorter value assigned over a longer one (`ChangeDbPasswordCommand::SetDbPassword()`
  zeroes first), `std::string` growth, temporaries, and dialog members (#16: the password
  dialogs kept passwords in members that were never zeroed).
- Secrets are released when `AutoSignOff` unwinds through the code (destructors, or catch
  and rethrow as `Dialog::Show()` does).
- No password is taken as an option value, where other users can see it: it comes from
  `--password-file` or `PromptForPassword()` (#20). No secret is written to stderr, logs
  (glog) or the preferences file.
- A password prompt restores the terminal on every path out, including signals.

### Modal loops and keys

Trigger: a loop that reads keys in a window or dialog; a new or changed key; a
`CommandBarWin` `Action` list.

- Keys are read with `ReadKey()`, never `wgetch()` (#11: non-ASCII text was garbled, and the
  auto sign-off timer doesn't run in `wgetch()`).
- `KEY_RESIZE` calls `app_.HandleResize()`, and the loop holds a `ScopedResizeHandler` for
  its relayout (#9).
- The key that the command bar shows is the key the handler acts on (#16: the save error
  prompt showed `^X Cancel` but cancelled on `^C`). Every key is registered with a
  description, and `^G` shows the help in loops that own a command bar.
- Each answer exits with the status `ncpwsafe(1)` gives for it (#16: answering `n` at the
  save error prompt exited 0, which means saved).

### Account records and the accounts list

Trigger: `AccountRecords`, `AccountRecord`, `AccountDb::ReadDb()`/`WriteDb()`, `AccountList`,
`AccountsWin`.

- `AccountList::Build()` runs after the records change, since rows point to records
  (#11: editing an account's group or title deleted another account; #16: the rows pointed
  at records that had moved).
- Reading the file doesn't silently discard unsaved changes, and dirty state is cleared
  only once they are written (#16: changing the database password undid unsaved edits).
- Code that acts on the selection handles an empty list and a group heading being selected
  (#11: crashes with no account selected).
- A failed write leaves the database file as it was, and the backup is made before saving.
  Read-only mode can't write.

### Text on screen

Trigger: drawing text, measuring or cutting it, attributes and colours.

- Widths come from `DisplayWidth()`, `FitText()` or `Utf8Length()`, not `size()`: account
  text is UTF-8, and a CJK character takes two columns (#11).
- Text is drawn with `Theme` roles (`Attr(Role::...)`, `ScopedAttr`), not raw attributes or
  colour pairs, and the selection stays in reverse video, which the TUI tests rely on.
- One-row form fields keep a value they can't show until it is edited (`field_status()`);
  multi-line values go through `TextArea`.

### Command line

Trigger: `options[]`, `ParseArgs()`, `ValidateArgs()`, `InputProgArgs`, `ProgArgs`, a
`*Command::Execute()`.

- A `std::optional` argument is read only when it is set (#11: `--export-db` without `-o`
  read an empty optional).
- An option that doesn't apply to the command is rejected, and arguments are checked
  before anything is read, written or prompted for.
- Exit codes match EXIT STATUS in `ncpwsafe.1.in` (#11: `--change-password` exited 0 when it
  failed).

### Undefined behaviour

Trigger: casts to enums, integer arithmetic on sizes and counts, raw buffers.

- No value outside an enum's range is cast to it (#16: dialog field ids 256 and 257 cast to
  `PwsFieldType`, reported by UBSan). Dialog fields use `DialogFieldId`.
- Sizes and lengths don't wrap or go negative (`size_t` minus a larger value), and
  `assert()`s have no side effects, since Release builds drop them.

### Preferences

Trigger: `Prefs`, `IniHandler()`, a new preference key.

- Reading the file copes with every kind of INI node, not just keys: comments, blank lines
  and sections (`docs/ISSUES.md` #1).
- A new key has a default that matches the man page, and an invalid value falls back to it
  rather than failing.

### Platform code

Trigger: `#ifdef _WIN32`, `Win32.{h,cpp}`, POSIX calls (`termios`, signals, `prctl`),
paths and line endings, `NDEBUG`.

- `<windows.h>` is included only in `Win32.cpp`, never with the curses headers.
- Both sides of a platform `#ifdef` build. CI builds and tests Windows, but the TUI and
  pseudo-terminal tests are skipped there, so say in the pull request what was run on
  Windows by hand.
- Release-only code (`NDEBUG`: `prctl(PR_SET_DUMPABLE)`, `DisableCrashDumps()`) is still
  correct when it is compiled, and Debug-only options (`--test-db`) don't leak into Release.
- CLI tests stay portable: `pathlib`, `e2elib.IS_WINDOWS`, `assertNotCrashed`.

## Do not flag

- Code fetched or generated at build time: `build*/`, `_deps/` (libpwsafe, libconfini),
  `config.h`.
- The word lists `src/en_CA.dat` and `src/ru.dat`.
- Formatting and naming preferences that aren't in `AGENTS.md`.
- Problems in lines the change didn't touch, beyond labelling them Pre-existing.
- Defensive checks for states the code can't reach, unless the reason it can't is fragile.

## Escalate

If the same finding comes back after it was answered, for three rounds, stop and ask a
person to decide.

## Keeping this file useful

- Each rule has a trigger that a diff shows, a check, and the bug behind it (a pull request
  or `docs/ISSUES.md` number). A rule without a trigger can't be applied selectively.
- When a bug gets past review, and a diff would have shown it, and it has happened before,
  add a rule for it.
- When a rule can be checked mechanically, make it a test or a compiler option and delete
  it here, as `cli.man_page` did for documenting options and keys.
- Delete rules that haven't produced a useful finding in a long time.
