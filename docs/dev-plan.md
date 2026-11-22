# ncpwsafe development plan

This plan covers six features, plus the project's other planned work:

1. [Password templates](#1-password-templates): named, reusable password generation settings.
2. [UI brush-up](#2-ui-brush-up): visual grouping, highlighting, and navigation improvements.
3. [Auto sign-off](#3-auto-sign-off): quit after a period without input, to protect a database
   left open on an unattended computer.
4. [Man page](#4-man-page): an `ncpwsafe(1)` manual page for the user interface, its keys, the
   command line, and the preferences file.
5. [New database format](#5-new-database-format): an encrypted SQLite-based format, in a new
   library, replacing Password Safe V2 as the data store, with import from V1/V2 files.
6. [Product releases](#6-product-releases): versioned Linux and Windows executables, with the man
   page, published to GitHub releases from version tags.
7. [Other work](#7-other-work): known bugs, features, and backlog items that aren't part of the
   features above. This replaces the old `TODO.md`.

Each feature section describes the current state, the proposed design, and the work broken into
milestones that can be merged on their own. [Open questions](#8-open-questions) at the end
need a decision before or during implementation. Detailed bug write-ups are in
[`ISSUES.md`](ISSUES.md), which has no open issues at the moment.

---

## 1. Password templates

### 1.1 Current state

- `PasswordPolicy` (`src/Policy.{h,cpp}`) has five fixed `Composition` values (alpha/digit/symbol,
  alpha/digit, easy-to-read, digits, hex) and a length. You cannot change the character sets.
- `GeneratePasswordDlg` steps through the compositions (Space) and lengths 4–56 in steps of 4 (`<`/`>`).
- The CLI command `--generate-password` takes `--password-count`, `--password-length` and
  `--password-policy`, given as a number `1..5` or as the policy's exact name (fixed in #11: a name
  used to select the policy before it, and `0` was accepted).
- No settings are saved. Each account's policy is lost when the dialog closes (requested
  items: "Save password policy to account entry", "Diceware passwords", "Fix password policies").
- **Security issue (fixed, milestone P1):** `MakePassword()` used `RandomState` (`src/RandUtils.h`),
  a `std::mt19937` with a 32-bit seed, and picked characters with `n % N`. It now draws from
  `std::random_device` through `std::uniform_int_distribution`, and `RandomState` is only used for test
  data. See `ISSUES.md` history for details.

### 1.2 Concepts

A **password template** is a named set of generation settings. There are two kinds:

| Kind         | Produces                                         | Example                               |
|--------------|--------------------------------------------------|---------------------------------------|
| `passphrase` | Words drawn from a word list, joined by a separator | `Crumpet-Velvet-Absorb-Tidal7`     |
| `random`     | Random characters from a configured character set   | `q7#Lm2!vZx9@Rt4k`                 |

Both kinds can limit the length. A passphrase sets a word count, and can also set a minimum and
maximum total character count. A random string sets a character count.

Templates can be:
- picked in the Generate Password dialog,
- linked to an account, so "generate" on that account uses its template by default,
- selected by name on the CLI to print lists of passwords.

### 1.3 Template settings

Settings marked **(requested)** come from the feature request. The rest are suggested additions.

#### Common to both kinds

| Setting | Type | Default | Notes |
|---|---|---|---|
| `name` | string | — | **(requested)** Unique key, also used on the CLI. |
| `kind` | `passphrase` \| `random` | — | **(requested)** |
| `description` | string | `""` | Shown in the template picker. |
| `min_length` / `max_length` | int | — / — | **(requested)** Limits on total length. For a passphrase, a result outside the limits is regenerated. Many sites cap passwords at a maximum length, so this is useful. |
| `min_entropy_bits` | int | none | Refuse to save a template, or warn, if its estimated entropy is below this value. |

#### `random` kind

| Setting | Type | Default | Notes |
|---|---|---|---|
| `length` | int | 20 | **(requested)** |
| `lowercase` | bool | true | **(requested)** "letters" |
| `uppercase` | bool | true | **(requested)** "caps" |
| `digits` | bool | true | **(requested)** |
| `symbols` | bool | true | **(requested)** Uses the default symbol set (`!#$%&*@^`, the current `COMP_SYMBOL`). |
| `custom_symbols` | string | `""` | **(requested)** Replaces the default symbol set when not empty. |
| `exclude_ambiguous` | bool | false | Leaves out look-alike characters: `0O o 1lI \| 5S 2Z`. This replaces the current "easy-to-read" composition. |
| `exclude_chars` | string | `""` | Characters a site rejects, for example `'"\`;`. |
| `require_each_class` | bool | true | Each enabled class appears at least once. Done by rejection sampling: regenerate until it passes, rather than forcing characters into place, so the distribution stays uniform. |
| `min_per_class` | map class→int | `{}` | For example `{ "digits": 2 }` for sites that need two digits. |
| `no_leading_trailing_symbol` | bool | false | Some systems reject a symbol in the first or last position. |
| `max_consecutive_repeats` | int | 0 (off) | For example, 2 rejects `aaa`. |
| `charset_preset` | enum | none | Shortcuts: `hex`, `pin` (digits only), `base32` (RFC 4648). These keep today's "digits" and "hex" options working. |

#### `passphrase` kind

| Setting | Type | Default | Notes |
|---|---|---|---|
| `word_count` | int | 5 | **(requested)** Length limit in words. |
| `wordlist` | string | `eff-large` | A built-in name (`eff-large`, `eff-short`) or the path to a file. Files may have one word per line or diceware format (`11111<TAB>word`). |
| `separator` | string | `-` | Can be empty. A `random` option picks from a set of separators for each gap. |
| `capitalization` | enum | `first` | `none`, `first` (first letter of each word), `upper`, `random` (randomly per word). |
| `append_digit` / `append_symbol` | bool | false / false | Adds one digit or symbol at a random word boundary, for sites that require them. |
| `min_word_length` / `max_word_length` | int | none | Filters the word list. Entropy must then be calculated from the filtered list size. |
| `no_repeat_words` | bool | true | |

#### Other per-template metadata (suggested)

| Setting | Type | Notes |
|---|---|---|
| `max_age_days` | int | Marks accounts linked to this template as stale after N days. Shown in the account list (see U-4), using the new format's `password_modified_at` (§5.5). Accounts imported from V1/V2 have no such time until their password next changes. |
| `clipboard_clear_seconds` | int | Override how long until the clipboard is cleared, for high-value accounts. Depends on adding a clipboard timeout feature. |

#### Built-in templates

Ship read-only built-ins that match today's compositions, so `--password-policy=1..5` keeps working
as an alias:

| Built-in name | Equivalent |
|---|---|
| `strong` | random, 20, all classes |
| `alnum` | random, 20, no symbols |
| `easy-to-read` | random, 20, `exclude_ambiguous` |
| `pin` | random, 6, digits only |
| `hex` | random, 32, `charset_preset: hex` |
| `diceware` | passphrase, 6 words, EFF large list, `-` separator |

The built-ins can't be changed or replaced. To vary one, define a user template with its settings
under another name (§1.4).

### 1.4 Storage

Templates belong to the user, not to a database. They are defined by hand in the configuration file,
`ncpwsafe.ini`, with the preferences, and an account refers to its template by name.

#### Configuration files

ncpwsafe reads **one** configuration file: the user's file if it exists, otherwise the system-wide
file if that exists, otherwise neither (built-in defaults only).

| Platform | User file | System-wide file |
|---|---|---|
| Linux, BSD | `$XDG_CONFIG_HOME/ncpwsafe/ncpwsafe.ini` (`~/.config/ncpwsafe/ncpwsafe.ini` when `XDG_CONFIG_HOME` is unset) | `SYSCONFDIR/ncpwsafe.ini`: `/etc/ncpwsafe.ini` for a `/usr` install (Linux packages), `/usr/local/etc/ncpwsafe.ini` for a `/usr/local` install (FreeBSD ports, builds from source) |
| Windows | `%APPDATA%\ncpwsafe\ncpwsafe.ini` (`FOLDERID_RoamingAppData`, as now) | `%ProgramData%\ncpwsafe\ncpwsafe.ini` (`FOLDERID_ProgramData`) |

- `SYSCONFDIR` comes from CMake's `GNUInstallDirs` (`CMAKE_INSTALL_FULL_SYSCONFDIR`), which gives
  `/etc` for the `/usr` prefix and `PREFIX/etc` otherwise, so each platform's packaging gets the right
  place without special cases. It goes into `config.h`.
- The files aren't merged. When the user file exists, the system file is ignored completely, so what
  the user file says is all that applies.
- An administrator can provide templates (for example the company's password rules) and defaults in
  the system file for users who have no file of their own. Nothing in it is enforced.
- `-c/--config=PATH` names the one file to read and write; neither default file is used.
- Errors are reported with the file name and line number.
- The TUI and CLI tests point the system file at their temporary directory, so a
  `/etc/ncpwsafe.ini` on the test machine can't change the results. Use an environment variable read
  only for this, such as `NCPWSAFE_SYSCONFDIR`.

#### Writing the configuration file

ncpwsafe writes a few settings itself: the last database opened (`db-pathname`) and the list of
backups (`backup.N`). Today `Prefs::WritePrefs()` rewrites the whole file from its key/value map,
which drops comments, sections such as the templates (`[template.*]`, below) and `[theme]`, and the
order of the keys. Instead, rewrite only the keys ncpwsafe owns:

- On save, read the file again (so edits the user made while ncpwsafe was running are kept), remove
  every `db-pathname` and `backup.N` key, and write their current values back. There is no tracking
  of which keys changed in the session: the owned keys are always rewritten, which costs nothing and
  keeps the code simple.
- Everything else (comments, blank lines, other keys, sections, their order) is kept as it is.
- Write to a temporary file in the same directory and `rename()` it over the old file, keeping the
  old file's mode (new files get `0600`), so a crash never leaves half a file.
- **No user file yet.** The first write creates the user file. Because the system file is ignored
  from then on, start the new user file as a copy of the system file, if there is one, and rewrite
  the owned keys in the copy. Otherwise the user would lose the system file's templates and defaults
  the first time ncpwsafe saved `db-pathname`. The copy doesn't follow later changes to the system
  file; the man page says so.
- ncpwsafe never writes the system file.

**Library: extend the libconfini fork.** ncpwsafe uses the author's fork of libconfini
(https://github.com/iboisvert/libconfini), pulled with `FetchContent`. libconfini only parses: it
edits its buffer in place while dispatching nodes, and `IniDispatch` says nothing about where a node
was in the file. So keeping comments while writing needs new API in the fork rather than
line-editing code in ncpwsafe. The fork's plan is in its `internal-docs/README.md`. In short:

- The parser reports each node's physical line range in the file (a new loading function, so
  existing callers and the `IniDispatch` layout are unchanged).
- A new editing module (`ini_editor_*`) built on that: load a file, set or delete keys by section
  and name (including deleting every key that matches, for `backup.N`), quote values safely, and save
  atomically, keeping comments, blank lines, order, line endings and any BOM.
- With it, ncpwsafe's save is: load the file into an editor, delete `db-pathname` and every
  `backup.N`, set their current values, save.

Then in ncpwsafe:

- Pin the fork to a release tag in `cmake/ResolveDependencies.cmake` instead of `GIT_TAG HEAD`, so a
  change in the fork can't silently change ncpwsafe's builds.
- Fix `Prefs`' read handler (`IniHandler` in `src/Prefs.cpp`). It asserts that every node is a key,
  so today a comment or a section in `ncpwsafe.ini` aborts a Debug build, and a Release build stores
  comments and section names as if they were keys (`docs/ISSUES.md`). Take only `INI_KEY` nodes,
  and use the node's section (`append_to`) to tell top-level preferences from `[theme]` and
  `[template.NAME]` keys.

Alternatives, if extending the fork turns out to be too much work: mINI
(https://github.com/metayeti/mINI, MIT, header-only), whose README says it "only writes changes and
updates and preserves custom formatting and comments", or SimpleIni
(https://github.com/brofield/simpleini), which "preserves comments ... where possible". Either would
replace libconfini rather than extend it. A different format isn't simpler: TOML libraries
re-serialize their value tree (toml++ doesn't mention keeping comments; toml11 keeps comments
attached to values but not the rest of the layout), and JSON has no comments.

#### Templates in the configuration file

```ini
# Template used when an account has none
default-template = memorable

[template.work-sso]
kind = random
description = Corporate SSO: 16-64 chars, no quotes
length = 24
custom_symbols = !@#$%^&*-_=+
exclude_chars = '"`

[template.memorable]
kind = passphrase
word_count = 5
separator = -
capitalization = first
append_digit = true
max_length = 40
```

- One section per template, `[template.NAME]`. The keys are the settings in §1.3, with the defaults
  in §1.3 for keys left out. `min_per_class` uses one key per class (`min_per_class.digits = 2`).
  Other sections (such as `[theme]`, §2.3) and top-level keys are the preferences.
- The built-in templates (§1.3) are always there. A section with a built-in's name is an error, so a
  built-in always means the same thing.
- Errors (an unknown kind or key, an invalid value, a template that can't be satisfied, for example a
  length of 8 with 3+3+3 required characters) are reported with the file and line number. The
  template with the error is left out, and the others still load.
- The file holds no secrets, so it isn't encrypted.

#### Links from accounts

An account refers to its template by name, in the `template` column of the new database format
(§5.5). The column is empty for accounts that use the default.

- A name rather than a UUID, because the user writes the names, can read them in the TUI and in an
  export, and the same name can mean a template on each of their computers (or in the system file). A UUID would survive a rename, but would have to be written into the file by hand.
- Renaming a template leaves accounts pointing at the old name. An account whose template isn't
  defined uses the default, and the account details show "work-sso (not found, using the default)",
  so the problem can be seen and fixed by renaming back or linking the account again.
- Accounts in V1/V2 files have no `template` field; they use the default template until they are
  imported (§5.8).
- Generated passwords are never stored anywhere but in the account itself.

### 1.5 Word lists

- Embed the EFF large list (7,776 words, about 12.9 bits per word) and the EFF short list (1,296 words,
  about 10.3 bits per word) in the source, the same way `en_CA.dat` and `ru.dat` are compiled in with
  `#include`. The untracked `eff_large_wordlist.txt` in the repo root (diceware format) is the source;
  add a small CMake step or script that turns it into a `.dat` file. EFF publishes the lists under
  CC BY 3.0 US, so add attribution to the README.
- Load user word lists from a file path, removing duplicate words. Warn when fewer than 1,000 unique
  words remain.

### 1.6 CLI

Keep the existing options working, and add:

```
--generate-password [--template=NAME] [--password-count=N] [--password-length=N]
--list-templates                 List built-in and user templates with entropy estimates
--show-template=NAME             Print a template's settings, as a `[template.NAME]` section
```

- `--template` and `--password-policy` cannot be used together. Map `--password-policy=N` to the
  matching built-in.
- `--password-length` overrides the template's `length`, or its `word_count` for a passphrase. Reject it
  if the result breaks `min_length` or `max_length`.
- Without `--template`, use `default-template` from the configuration files, or `strong`.
- None of these need a database: templates come from the built-ins and the configuration files.
- Print one password per line to stdout and errors to stderr, so output can be piped.
- Optional `--entropy` flag: print the estimated entropy in bits on stderr.
- These options go through the usual path: `options[]`, `ParseArgs`, `ValidateArgs`, `Usage`,
  `InputProgArgs`, `ProgArgs`, and `DEFAULT_*` in `ProgArgs.cpp`.

### 1.7 TUI

- **Generate Password dialog** (`GeneratePasswordDlg`), rebuilt:
  - The top shows the template picker (`Tab`/`Shift-Tab` or a dropdown), with the template's description
    and estimated entropy (for example, "≈ 104 bits, very strong").
  - Below it, the generated password in bold, centered. `Enter` regenerates it.
  - `<`/`>` still change the length or word count for this one generation only; they do not save.
  - `^T` links the selected template to the current account.
- **No template editor or manager.** Templates are written by hand in the configuration file (§1.4).
  The Generate Password dialog shows errors from the files, and `--list-templates` checks them.
- **Account details dialog**: add a read-only "Template:" row showing the linked template,
  "(default)" when none is linked, or "(not found, using the default)" for a name that isn't defined.

### 1.8 Code structure

| New or changed | Responsibility |
|---|---|
| `src/SecureRandom.{h,cpp}` (replaces `RandUtils.h`) | `getrandom()` when `HAVE_SYS_RANDOM_H` is set, otherwise `/dev/urandom` or nettle's `yarrow256`. `uniform(n)` uses rejection sampling, which has no modulo bias. |
| `src/PasswordTemplate.{h,cpp}` | Plain data struct with `Validate()`, `EstimateEntropyBits()` and `Generate(SecureRandom&)`. Replaces `PasswordPolicy`. |
| `src/WordList.{h,cpp}` | Built-in and file-based word lists. |
| `src/PasswordTemplateStore.{h,cpp}` | Reads the `[template.*]` sections of the system or user configuration file with libconfini, through `Filesystem` so tests can mock it, and merges them with the built-ins. Looks up templates by name. Owned by `PWSafeApp`. |
| `Prefs` | Picks the user or the system file, and reads only key nodes, by section. On save rewrites only the keys ncpwsafe owns through the fork's `ini_editor_*` API (§1.4), copying the system file when it creates the user file. |
| `GeneratePasswordCommand`, `GeneratePasswordDlg` | Use the store and templates. |

Zero generated-password buffers after use (the volatile memset in `Utils.cpp`), the same as other
password-handling code.

### 1.9 Testing

- `SecureRandom::uniform`: run a chi-square check of the distribution over many samples. Make the
  random source injectable so tests can use a fixed sequence.
- `PasswordTemplate`: every constraint (class requirements, exclusions, length bounds, repeats,
  passphrase capitalization and separators), plus `Validate()` rejecting templates that can't be
  satisfied.
- Entropy estimates checked against known values (for example, 6 EFF words is about 77.5 bits).
- Configuration: the user file when it exists (the system file ignored), the system file when it
  doesn't, neither, `-c`, and `XDG_CONFIG_HOME`.
- Reading: comments, sections and `[template.NAME]` keys don't become preferences (the current
  assertion failure).
- Saving: `db-pathname` and every `backup.N` rewritten (old backup entries removed), comments,
  blank lines, other keys, sections and their order unchanged, other keys edited by the user during
  the session kept, the file's mode kept, and the user file created as a copy of the system file.
- `PasswordTemplateStore`: reading every setting, defaults for keys left out, errors with file and line (one bad template doesn't stop the others),
  a built-in's name refused, and links to names that aren't defined.
- CLI: `--template` together with `--password-policy`, unknown template names, and that each existing
  `--password-policy=N` matches its built-in.

### 1.10 Milestones

| # | Milestone | Depends on |
|---|---|---|
| P1 | **Done.** `MakePassword()` uses `std::random_device` instead of `RandomState`, which fixes the RNG weakness. | — |
| P2 | `PasswordTemplate` (random kind) and built-ins; `GeneratePasswordCommand` uses them; `--password-policy` still works. | P1 |
| P3 | Passphrase kind and embedded EFF word lists. | P2 |
| C1 | Configuration files: the user or the system file, `XDG_CONFIG_HOME`, the read handler fix, and rewriting only the owned keys with the libconfini editor. | libconfini fork: line ranges and `ini_editor_*` |
| P4 | Templates in the configuration files, `--template`, `--list-templates`, `--show-template`. | P2, C1 |
| P5 | Generate Password dialog rebuilt with the template picker. | P4, U-1 for styling |
| P6 | Account links (`template` column and `^T`); the details dialog shows the template. | P4, P5, D2 |

---

## 2. UI brush-up

### 2.1 Current state

The first milestones (U-1, U-2/U-4, U-5, U-6, and parts of U-9 and U-10) were done in #12. The
status of each milestone is in §2.8.

- **Theme.** `Theme` (`src/Theme.{h,cpp}`) maps semantic roles to attributes (§2.3). Colour uses the 8
  basic colours over the terminal's own background (`use_default_colors()`). `NO_COLOR`, a terminal
  without colour, or the `theme = mono` pref draws in monochrome. Every role keeps a non-colour cue,
  and the selection is always reverse video. Symbols (`▾ ▸ … ✓ › •`) fall back to ASCII
  (`v > ~ * > *`) when the `LC_CTYPE` locale isn't UTF-8.
- **Accounts list.** One column, drawn by `AccountsWin` without the `menu` library; `AccountList`
  holds the rows, the collapsed groups, the selection and the scrolling, and has no ncurses code.
  - Each group has a heading with a rule and its number of accounts. Accounts without a group come
    first, under "(no group)"; with no groups at all there are no headings.
  - Accounts are indented beneath their heading, with the user name in a muted second column. Long
    titles and user names are cut with an ellipsis.
  - Left on an account goes to its heading, Left on a heading collapses the group, Right expands it
    and then goes to its first account, and Enter or Space toggles a heading. Collapsed groups are
    forgotten when the app exits.
  - Group names are flat: `a.b.c` is one group, not a tree.
- **Title bar.** The top border shows the application title, the file name followed by `*` when there
  are unsaved changes or `[read-only]` (in the warning style), and at the right the selection with its
  position (`Banking › Chequing  3/7`) or the selected group.
- **Command bar.** Keys are drawn as key caps. When the commands don't all fit, the bar ends with
  `^G More`. Commands that don't apply to a read-only database are hidden (`CBOPTS_READONLY`).
- **Help.** `^G` in the accounts list or a dialog opens `HelpWin`, which lists every key of the
  context with its description, built from the `Action` list the window registers with the command
  bar. Search (`/`), Space, Left and Right are registered too. The navigation keys (arrows,
  Home/End, PgUp/PgDn) and the field-movement keys in dialogs (`Tab`, `Shift-Tab`, `Enter`) aren't.
  The overlay doesn't scroll: rows that don't fit are cut off without a sign. Any key closes it.
  `^G` replaced F1, which many terminals and window managers keep for their own help.
- **Dialogs.** Built on `Dialog`: a title chip, dimmed labels, fields that can start a section (View
  Account has Account, Credentials and Notes), and the focused field highlighted (black on yellow,
  or reverse video). A hidden password shows as 8 bullets whatever its length, and `^V` shows it in
  the secret style. The notes field is 3 rows and edits multi-line text (`TextArea`); text that isn't
  edited, line breaks included, is kept as it was stored. `GeneratePasswordDlg` is still drawn by
  hand rather than with `Dialog`; it is rebuilt in P5.
- **Clipboard.** Copying with `^U` or `^P` is confirmed in the bottom border ("✓ Copied the password
  of Chequing", never the copied text) until the next key. Nothing clears the clipboard afterwards.
- **Search.** `/` opens the search bar. Each key moves to the next account whose title, name, user
  name or notes contain the query (case-insensitive, wrapping around), expanding its group if it is
  collapsed. `^L` goes to the next match, Enter keeps the selection, and Esc goes back to where the
  search started. Group and URL aren't searched, there is no previous match, and matches aren't
  highlighted.
- **Message boxes.** `MessageBox` wraps its text to the terminal and stays centered, but uses no theme
  roles: errors, warnings and confirmations all look the same. `y`/`n` are handled by a key handler
  passed in by each caller (`YesNoKeyHandler`, `YesNoCancelKeyHandler` in `AccountsWin.cpp`).
- **Resizing.** Every window and dialog lays out again on `KEY_RESIZE` (#9). Below
  `PWSafeApp::MIN_TERM_LINES`×`MIN_TERM_COLS` (6×20) the windows are left as ncurses truncated them.
- **Text.** Non-ASCII account text is shown and edited correctly (#11), and `FitText()` and
  `DisplayWidth()` measure it in terminal columns.

### 2.2 Goals

1. Make structure easy to see: group headings, section headings, and focus should stand out at a glance.
2. Make navigation faster, especially in databases with hundreds of accounts.
3. Keep it usable on monochrome terminals, `TERM=linux`, and under `NO_COLOR`.

### 2.3 Theme system

Code draws text with `Theme::Attr(Role)` or `ScopedAttr`, never with raw attributes or colour pairs.
`SetAttrs()` sets a window's attributes without dropping the colour pair. The roles as built:

| Role | Colour | Monochrome | Used for |
|---|---|---|---|
| `TITLE` | bold, white on blue | bold + reverse | Application, dialog and help titles |
| `HEADING` | bold, cyan | bold + underline | Group headings, dialog sections |
| `LABEL` | dim | dim | Field labels |
| `FIELD` | underline | underline | Input fields |
| `FIELD_FOCUS` | bold, black on yellow | reverse | The field with the cursor |
| `SELECTION` | bold, cyan, reverse | reverse | The selected row |
| `SECRET` | bold, green | bold | Shown passwords, the generated password, the bullets |
| `MUTED` | dim | dim | User names in the list, counts, rules |
| `KEY` / `KEY_DESC` | bold + reverse / normal | the same | Command bar and help keys |
| `ERROR` / `WARNING` / `SUCCESS` | bold red / yellow / green | bold | `[read-only]` and `*` (warning), copy confirmation (success); `ERROR` is unused |

Remaining theme work:

- **Message boxes and the search bar** don't use roles yet (U-11). `ERROR` is meant for error
  messages.
- **`[theme]` overrides** in the INI prefs, for example `heading = bold cyan` or
  `selection = black on cyan`, and a built-in `light` theme for light backgrounds, selected with
  `theme = light`. With `use_default_colors()` the current colours already work on most light
  backgrounds, so check whether `light` is still needed before building it; yellow (focus, warning)
  is the most likely problem.
- **`--no-color`** on the command line, the same as `NO_COLOR` or `theme = mono`. Optional, since
  both of those already work.
- Roles that were planned but aren't needed yet: `field_readonly` (read-only dialogs make the fields
  uneditable but draw them the same), `selection_inactive` (only one list exists), and
  `search_match` (U-7). Add them with the feature that needs them.
- The plan to define the colour pairs again after `KEY_RESIZE` turned out not to be needed so far; do
  it only if a terminal is found that resets them.

### 2.4 Remaining usability improvements

Ordered from most to least useful for the effort. Done parts are left out; see §2.1.

**U-2. Grouped list: what's left.**
- Show nested groups (`a.b.c`, the Password Safe convention) as a tree, with a heading per level.
- Collapsed groups are not remembered between sessions (Q6): saving them would put group names in
  the prefs file in plaintext.

**U-3. Detail pane.** On wide terminals (≥ 120 columns), show the selected account's details (group,
title, user, URL, email, notes preview; password masked) in a read-only pane on the right, so you
don't have to open a dialog just to look. It updates as the selection moves. The single-column list
leaves room for it.

**U-4. Richer rows: what's left.** Show the URL host when there is room. Optionally mark accounts
linked to a template that has `max_age_days` set (§1.3).

**U-6. Help: what's left.**
- Scroll the help overlay when its rows don't fit, or at least show that some are cut off.
- List the navigation and field-movement keys too, by registering them as actions that the command
  bar leaves out (a flag on `Action`), so the help and the man page (§4) can list every key.

**U-7. Better search.**
- Filter as you type: show only matching accounts (and their headings) instead of moving to the next
  match.
- Highlight the matched text with a new `search_match` role, and show the match count in the title
  bar status ("2 of 5").
- Search the group and URL too. Fuzzy matching is optional.
- A previous-match key. Shift-F3 doesn't reach ncpwsafe on `TERM=screen` (see the note in
  `PWSafeApp::InitTUI`), so use another `^` key next to `^L`.

**U-8. Keyboard navigation polish.**
- No new plain letter keys in the accounts list for now (Q7): no type-to-jump, Vim keys or
  single-letter copy keys. Navigation stays with the arrows, Home/End, PgUp/PgDn, Left/Right and `/`.
- Optional: copy the URL with a control key, alongside `^U` and `^P`.

**U-9. Password reveal and clipboard: what's left.** Masking (8 bullets) and `^V` to show the password
are done, as is the copy confirmation. Left: hide a shown password again after a while or when the
focus leaves the field, and clear the clipboard after a timeout with a countdown in the confirmation
("Password copied, clears in 30 s"). The clipboard timeout also serves §1.3's
`clipboard_clear_seconds` and auto sign-off's S3.

**U-10. Read-only: what's left.** `*` and `[read-only]` in the title bar are done. Left: dim the
editing commands in the command bar and the help instead of hiding them, so you can see that they
exist. `CommandBarWin` would need a "disabled" state for actions, drawn in `MUTED`.

**U-11. Confirmations and messages.** Give `MessageBox` a kind (error, warning, confirmation,
information) drawn with the `ERROR`/`WARNING`/`SUCCESS` roles: a coloured border or a prefix. Move the
`y`/`n`/Esc handling from the callers' key handlers into `MessageBox`, with a default button shown in
`FIELD_FOCUS`. Show errors from the libpwsafe `ResultCode` as readable text.

**U-12. Mouse support (optional).** `mousemask()` for clicking list rows and headings, scroll-wheel
scrolling, and clicking command bar entries. Off by default; enabled with the `mouse = true` pref.

**U-13. Minimum-size handling.** Below a minimum layout (for example 40×10), show a centered message
("Terminal too small: 32×8, need 40×10") instead of truncated windows. Today's limit is 6×20, below
which `HandleResize()` leaves the windows as ncurses truncated them.

**U-14. Help on first run.** When there is no prefs file, show a short message pointing to `^G`.

### 2.5 Accessibility and compatibility

- Never use colour as the only signal: every role in §2.3 keeps a bold, underline, reverse or symbol
  cue. Keep this for new roles.
- Test with 8 and 256 colours, `TERM=linux`, `TERM=screen`/tmux, the Windows console, `NO_COLOR=1`,
  and light and dark backgrounds. The TUI tests cover only tmux with `TERM=screen`.
- ASCII symbols are used when the `LC_CTYPE` locale isn't UTF-8 (`Theme::Sym`). Lines use ncurses
  ACS characters, which fall back on their own.

### 2.6 Code structure

| File | State | Responsibility |
|---|---|---|
| `src/Theme.{h,cpp}` | Exists | Roles, colour pairs, symbols, `ScopedAttr`, `SetAttrs`. To add: `[theme]` overrides and `light`. |
| `src/AccountList.{h,cpp}` | Exists | Rows, collapsing, selection and scrolling, without ncurses. To add: nested groups, filtering (U-7). |
| `src/HelpWin.{h,cpp}` | Exists | The `^G` overlay. To add: scrolling. |
| `src/TextArea.{h,cpp}` | Exists | Multi-line notes. |
| `AccountsWin` | Changed | Draws the list, the title bar status and the copy confirmation. To add: the detail pane layout. |
| `CommandBarWin` | Changed | Key caps, `^G More`. To add: disabled actions (U-10), actions hidden from the bar but shown in the help (U-6). |
| `Dialog` | Changed | Sections, focus highlight, masks, text areas. |
| `MessageBox` | To change | Kinds and roles, built-in `y`/`n`/Esc, default button (U-11). |
| `SearchBarWin` | To change | Filtering, match count, previous match (U-7). |
| `DetailPaneWin` | New | Read-only detail view (U-3). |
| `Prefs` | Changed | `theme`. To add: `[theme]` overrides, `mouse`. |

### 2.7 Testing

- Exists: `Theme-tests.cpp` (mode selection and attributes), `AccountList-tests.cpp` (rows,
  collapsing, selection, scrolling), `TextArea-tests.cpp`, and TUI tests for the list, the help, the
  title bar and the dialogs.
- To add with the remaining work: parsing `[theme]` overrides (valid and invalid), nested groups and
  filtering in `AccountList`, the help overlay scrolling, and message box kinds and keys.
- Manual checklist for each milestone: the terminal matrix in §2.5, resizing in every window,
  read-only mode, and an empty database.

### 2.8 Milestones

| # | Milestone | Depends on | Status |
|---|---|---|---|
| U-1 | `Theme` and roles, colour enabled, existing windows switched to roles. | — | Done in #12, except `[theme]` overrides, `light` and `--no-color` (§2.3) |
| U-5 | Command bar overhaul. | U-1 | Done in #12: key caps and `^G More`. The status went to the title bar instead. |
| U-6 | `^G` help overlay. | U-5 | Done in #12, except scrolling and listing the navigation keys |
| U-2 / U-4 | Grouped accounts list with richer rows. | U-1 | Done in #12, except nested groups and URL hosts |
| U-9 / U-10 | Masking and indicators. | U-1 | Masking, `^V`, sections, focus, `*`/`[read-only]` and copy confirmation done in #12. Left: re-hiding, clipboard timeout, dimmed read-only commands |
| U-11 | Message boxes. | U-1 | To do |
| U-7 / U-8 | Search and navigation polish. | U-2 | To do. Search already expands collapsed groups; U-8 adds no plain letter keys (Q7). |
| U-3 | Detail pane. | U-2 | To do |
| U-12 / U-13 / U-14 | Mouse, minimum-size handling, first-run help. | — | To do |

---

## 3. Auto sign-off

Protects a user who walks away from the computer with ncpwsafe running: after a period with no
input, ncpwsafe quits and discards any unsaved changes. The default period is 5 minutes, and it can
be changed in the preferences file. This replaces the old `TODO.md` item "Close db after timeout".
S1 was done in #13. §3.7 plans keeping the unsaved changes in an encrypted recovery file instead of
discarding them, for the user to restore or throw away the next time they open the database.

### 3.1 Current state

S1 was done in #13, mostly as designed below. Where it differs:

- The timer, the `AutoSignOff` exception and the preference parsing are in `src/AutoSignOff.{h,cpp}`
  rather than `src/IdleTimer.{h,cpp}`. Every modal loop reads keys with `ReadKey()` (`wget_wch`),
  which throws `AutoSignOff` when the timer expires.
- The exit status is `EXIT_AUTO_SIGN_OFF` (124, the status GNU `timeout(1)` uses), not a new
  `ResultCode`. `main()` prints the message.
- The clock is `CLOCK_BOOTTIME` where it exists (checked with `#ifdef`, not in CMake), otherwise
  `CLOCK_MONOTONIC`, and `GetTickCount64()` on Windows.
- `AccountDb::Wipe()` wipes the database password and the records when `PWSafeApp::Show()` catches
  the sign-off.
- Tests: `IdleTimer` and preference parsing in `test/AutoSignOff-tests.cpp`, and
  `tui/test_auto_sign_off.py`. The `auto-sign-off` preference is documented in the man page (#14).

Left: the countdown warning (S2), clearing the clipboard (S3) and the recovery file (S4).

### 3.2 Behaviour

- After the configured period without a keystroke, in any window or dialog, ncpwsafe signs off:
  1. It discards unsaved changes without asking. It doesn't save them to the database, because a
     half-finished edit may not be what the user wants written. S4 (§3.7) saves them to a recovery
     file instead, for the user to review.
  2. It wipes the secrets it holds (see §3.3).
  3. It clears the screen, so the last screen (which may show a password) isn't left in the terminal,
     then restores the terminal with `endwin()`.
  4. It prints `ncpwsafe: signed off after 5 minutes without input` to stderr, followed by
     `Unsaved changes were discarded.` when the database had changes.
  5. It exits with a new, distinct `ResultCode` (for example `RC_AUTO_SIGN_OFF`), so a wrapper
     script can tell a sign-off from a normal exit or an error.
- Only key presses count as activity. `KEY_RESIZE` and read timeouts don't reset the timer.
- The timer runs for the whole TUI session (`Operation::OPEN_DB`), including the unlock prompt, where
  signing off loses nothing. The CLI commands are not affected.
- Read-only databases are signed off in the same way.
- Sign-off works from any depth: a message box over a dialog over the accounts list, or the search bar.
- **Sleep and suspend count as idle time.** Measure time with `CLOCK_BOOTTIME`, which keeps counting
  while the computer is suspended. `CLOCK_MONOTONIC` stops during suspend on Linux, and the wall
  clock jumps when the time is set. After a resume the first timer check signs off, so a laptop that
  is closed and reopened later doesn't show the database. Fall back to `CLOCK_MONOTONIC` where
  `CLOCK_BOOTTIME` doesn't exist (check for it in CMake, like the `HAVE_SYS_*` checks in `config.h`).

### 3.3 Design

| New or changed | Responsibility |
|---|---|
| `src/IdleTimer.{h,cpp}` (new) | `Touch()`, `Expired()`, `Remaining()`. Takes the period and a clock function, so tests can pass a fake clock. A period of 0 never expires. |
| `PWSafeApp` | Owns the `IdleTimer`, set from prefs in `Init()`. Provides one key-reading function for every modal loop (below). Catches the sign-off and cleans up in `Show()`. |
| `Utils` `ReadKey()`, and the loops in `AccountsWin`, `MessageBox`, `HelpWin`, `GeneratePasswordDlg` | Move every `wgetch()` call to the shared key reader. |
| `Prefs` | New `AUTO_SIGN_OFF` key with a doc comment and a default in `DEFAULTS`. |
| `AccountDb` | Wipe the password and secret fields (`FT_PASSWORD`, and probably `FT_NOTES`) with the volatile `memset` in `Utils.cpp`. |
| `ResultCode` | `RC_AUTO_SIGN_OFF`. |

- **Shared key reader.** Something like `int PWSafeApp::ReadKey(WINDOW *win, wchar_t &wch)` that sets
  `wtimeout(win, …)` to the time left (at most 1 s), reads with `wget_wch()`, and loops on a timeout.
  It returns only real keys, after calling `Touch()`, so the loops that stop on `ERR` keep working.
  When the timer has expired it starts the sign-off.
- **Leaving the loops.** Throw an `AutoSignOff` exception from the key reader and catch it in
  `PWSafeApp::Show()`. Unwinding runs the destructors of every open window and dialog, including
  `ScopedResizeHandler`. Before relying on this, check that each window and dialog frees its ncurses
  objects (forms, menus, panels, windows) in its destructor, and that `Dialog` scrambles its buffers
  with `RandomizeBuffers()` in its destructor and not only on the close paths. The alternative, a new
  `DialogResult::SIGN_OFF` returned through every loop, touches more code and is easy to miss in
  one loop.
- Don't use `SIGALRM`: a signal handler can't safely call ncurses or unwind the stack.
- Write the preferences as on a normal exit (last database, backup list); they hold no secrets.

### 3.4 Preference

```ini
# Quit after this long without input, discarding unsaved changes. 0 turns auto sign-off off.
auto-sign-off = 5m
```

- The value is a number with an optional unit: `s` (seconds), `m` (minutes) or `h` (hours). A number
  without a unit is minutes, so `auto-sign-off = 5` also works. The default is `5m`.
- Seconds let the end-to-end tests use a short period in Release builds, which is what CI runs,
  without a hidden test-only setting.
- An invalid or negative value uses the default and logs a warning. It doesn't turn the feature off.
- Because the default is on, users with an existing preferences file get auto sign-off without
  changing it. Mention this in the release notes.

### 3.5 Optional follow-ups

- **Warning before sign-off.** For the last 30 seconds (or less, for short periods), show a countdown
  in the command bar: "Signing off in 30 s, press any key to stay". The key that cancels the warning
  is discarded rather than acted on, so pressing `^D` to stay doesn't delete an account.
- **Clear the clipboard.** A password copied with `^P` stays on the clipboard after sign-off. Clear
  the clipboard on sign-off if ncpwsafe copied a secret during the session. This shares code with the
  clipboard timeout idea in §1.3 and U-9.

### 3.6 Testing

- `IdleTimer` unit tests with a fake clock: expiry, `Touch()` resetting the period, a period of 0
  never expiring, and a clock that jumps forward (a suspend) expiring at once.
- Preference parsing: `5`, `5m`, `90s`, `1h`, `0`, `-1`, `abc`, an empty value, and a missing key.
- TUI test `tui/test_auto_sign_off.py`, with `auto-sign-off = 2s` written to the preferences file in
  the test's temporary `$HOME`:
  - The app exits by itself (`wait_exit()`) with the new exit code, and the message is shown.
  - After an unsaved edit, the database is unchanged (`assertDbContains`).
  - It signs off with the account details dialog or a message box open.
  - Key presses keep it open: use a longer period (for example 4 s) and send a harmless key every
    second, so the test isn't sensitive to timing.
- Manual check: suspend the computer for longer than the period, resume, and check that ncpwsafe has
  signed off.

### 3.7 Recovery file for unsaved changes

Today a sign-off loses every unsaved change. With this feature, ncpwsafe saves the changes to an
encrypted recovery file next to the database, and offers them back the next time the database is
opened. The recovery file records which fields of which accounts were changed, so restoring merges
only those fields into the database. A sign-off still never writes the database itself, so a change
the user didn't save never reaches it without the user's say-so.

This needs the new format (§5): the recovery file is a `.ncpwsafe` file, and the format stores the
change state of every field (§5.5). It works the same whatever the database's own format, so a V2
database gets a `.ncpwsafe` recovery file too.

#### Change tracking

Today `AccountRecord` has one dirty flag per account, and `AccountRecords` one for the collection
(set when an account is deleted). That says that something changed, not what. Track instead:

- for each account, which fields were changed since the database was opened or last saved;
- which accounts were added;
- the UUIDs of the accounts that were deleted, since a deleted account is no longer in `AccountRecords`.

A field set back to its saved value is still counted as changed; comparing with the saved value isn't
worth keeping a second copy of every secret in memory. `ClearDirty()` (on save) clears all of it.
`IsDirty()` keeps its meaning: anything tracked.

#### On sign-off

- If the database has unsaved changes, write them to the recovery file before wiping memory (§3.3),
  then wipe as today. A database without changes, or a read-only one, writes nothing.
- The recovery file holds:
  - each added or changed account, as a whole row, so that it can be put back if it has been deleted
    from the database since;
  - its changes: for each account, whether it was added, changed or deleted, and for a changed account
    the fields that were changed (the `changes` table, §5.5);
  - in `meta`: `kind = recovery`, the pathname of the database it belongs to, and the time of the
    sign-off.
- It's encrypted with the database's password, so it is as safe on disk as the database. It holds
  only changes the user made, never what was typed in an open dialog and not yet submitted.
- **Name.** The database's pathname with the extension `.ncpwsafe-tmp`: `accounts.dat` gives
  `accounts.ncpwsafe-tmp`. Replacing the extension makes `accounts.dat` and `accounts.psafe` in one
  directory share a recovery file, so consider appending instead (`accounts.dat.ncpwsafe-tmp`);
  see Q9.
- Saved like a database (§5.4): to a temporary name in the same directory, then renamed into place,
  owner-only (`0600`). A recovery file left from an earlier sign-off (the user chose Skip) is
  overwritten: the user was offered its changes when the database was opened and chose to leave
  them.
- If the write fails (disk full, directory not writable), the changes are discarded as today, and the
  message says so.
- The message on stderr becomes `Unsaved changes were saved to accounts.ncpwsafe-tmp.` (or
  `Unsaved changes could not be saved and were discarded.`). The exit status stays 124.
- A sign-off mustn't prompt or wait for input. Writing the file adds one key derivation (Argon2id,
  about a second, §5.3); reuse the database's derived key if the library allows it, so the sign-off
  stays quick.

#### On opening the database

- When the TUI opens a database (`Operation::OPEN_DB`), it checks for that database's recovery file.
  It doesn't scan the directory for other files that match the pattern.
- The unlock prompt is shown as usual, and the password unlocks both the database and the recovery
  file. If the recovery file doesn't open with it (the database password was changed by another
  program since), ask once for the recovery file's password; Cancel is treated as Skip.
- ncpwsafe reads the changes from the recovery file, with no comparison of the two files needed, and
  shows them in a dialog over the accounts list:
  - accounts added, deleted and changed, each by group and title;
  - for a changed account, the fields that were changed (`Password`, `Notes`, `User name`, ...), with
    the value from the recovery file and the value in the database now, for fields that aren't secret.
    Password values are never shown, only "changed";
  - a mark on each change that the database no longer matches: a changed account that has been deleted
    from the database since (restoring puts it back as `Title (RESTORED)`, see below), or an added
    account whose UUID is now in the database (restoring treats it as changed in every field).
  - The list scrolls, so a long one fits on any terminal (the help window's scrolling, U-6).
- The dialog asks **Restore unsaved changes?**:

| Key | Answer | Result |
|---|---|---|
| `y` | Yes | Merge the changes into the database: add the added accounts, delete the deleted ones, and for each changed account set only the changed fields. Back up the database as before any save (`backup-before-save`, `backup-count`), save it, and only then delete the recovery file. |
| `n` | No | Delete the recovery file and open the database as it is. Ask "Delete the unsaved changes?" first, since this can't be undone. |
| `s`, `Esc` | Skip | Leave the recovery file and open the database as it is. The question comes back the next time the database is opened. |

- **An account deleted from the database since.** If the recovery file has changes to an account that
  is no longer in the database, restoring puts the whole account back from the recovery file, not
  only its changed fields, and changes its title to `{title} (RESTORED)`, for example
  `Chequing (RESTORED)`. The new title shows the user that the account had been deleted, so they can
  check it and delete it again if they meant to. The account keeps its UUID, so its other links (the
  template, §1.4) still work.
- Merging fields rather than replacing the file keeps changes made to the database after the sign-off
  (by another program or another copy of ncpwsafe), as long as they are to other fields or accounts.
  For a field changed in both, the recovery file wins; the dialog shows the current value next to the
  restored one, so the user sees what will be overwritten.
- The merge is saved straight away, so that deleting the recovery file can't lose the changes. If the
  save fails, the recovery file is kept, and the merged changes stay in the open database as unsaved
  changes.
- If the recovery file is damaged, isn't a recovery file (`meta.kind`), or belongs to another
  database (`meta` pathname), say so and treat it like Skip, leaving it for the user to look at.
- The CLI commands don't offer to restore. `--export-db` and `--change-password` print a warning to
  stderr when a recovery file exists, because they work on the database without its unsaved changes.
  `--change-password` doesn't change the recovery file's password.

#### Code

| New or changed | Responsibility |
|---|---|
| `AccountRecord`, `AccountRecords` | Per-field change state, added accounts and deleted UUIDs, in place of the dirty flags. Unit-tested. |
| `RecoveryFile` (new) | The recovery file's pathname for a database; writing the changed accounts and their change state through `libncpwsafe` (temporary name, rename, `0600`); reading them back; deleting it. Uses `Filesystem`, so it can be tested with a mock. |
| `AccountDb::Merge()` (new) | Applies a set of changes to the records: add, delete, set fields, and put back a changed account that has been deleted as `{title} (RESTORED)`. Unit-tested, including accounts that no longer match. |
| `RecoveryDlg` (new) | The scrolling list of changes and the Yes/No/Skip question. Reads keys with `ReadKey()`, so auto sign-off works while it's open, and handles `KEY_RESIZE`. |
| `PWSafeApp::SignOff()` | Writes the recovery file before `db_.Wipe()`, and records the result for the message in `main.cpp`. |
| `PWSafeApp::OpenAndEdit()` | After unlocking, checks for a recovery file and shows `RecoveryDlg`. |
| Man page (§4) | The recovery file under FILES, the new sign-off messages, and the CLI warnings. |

#### Testing

- Change tracking unit tests: editing a field marks only that field, adding and deleting accounts,
  deleting an added account (nothing to record), and `ClearDirty()` on save.
- `AccountDb::Merge()` unit tests: each kind of change, a field changed in both, a changed account
  deleted from the database (put back whole, titled `{title} (RESTORED)`, with every field from the
  recovery file), an added account whose UUID exists.
- `RecoveryFile` unit tests: the pathname for names with and without an extension, a round trip of
  the change state, overwriting an existing recovery file, a failed write leaving no file behind.
- TUI tests (a new `tui/test_recovery.py`), with `auto-sign-off = 2s`:
  - Sign-off with unsaved changes writes the recovery file and leaves the database unchanged
    (`assertDbContains`); the message names the file. Without changes, no file is written.
  - Opening the database again lists the changed fields, without any password values.
  - Yes: the database has the changes, other fields are as they were, a backup was made, and the
    recovery file is gone. With the database changed by another program after the sign-off, its
    changes to other fields are kept, and a changed account it deleted comes back as
    `{title} (RESTORED)`.
  - No: the database is unchanged and the recovery file is gone.
  - Skip: both are unchanged, and the question comes back the next time.
  - A recovery file with a different password, a damaged one, and one for another database.
- CLI tests: `--export-db` warns when a recovery file exists.

### 3.8 Milestones

| # | Milestone | Depends on | Status |
|---|---|---|---|
| S1 | `IdleTimer`, the `auto-sign-off` preference, the shared key reader, sign-off by exception, wiping secrets, the exit code and message, and the tests. Document the preference in the man page if it exists by then (M1). | — | Done in #13; documented in the man page in #14 |
| S2 | Countdown warning in the command bar. | S1 | |
| S3 | Clear the clipboard on sign-off. | S1; shares code with a clipboard timeout | |
| S4 | Recovery file: per-field change tracking, saving the changes on sign-off, and merging them into the database when it is opened (§3.7). | S1, D2 (the recovery file is a `.ncpwsafe` file with the `changes` table); uses the help window's scrolling (U-6) | |

---

## 4. Man page

An `ncpwsafe(1)` manual page that describes the user interface, the keys for navigating it and for
each of its functions, the command-line arguments, and the user preferences file. M1 was done in
#14; §4.1 lists what's left.

### 4.1 Current state

M1 was done in #14:

- `man/ncpwsafe.1.in`, written by hand in mdoc(7), with the sections in §4.2, including the
  `auto-sign-off` preference and exit status 124 (§3).
- `man/CMakeLists.txt` fills in the version with `configure_file()` (to `build/man/ncpwsafe.1`) and
  installs the page to `share/man/man1`, except on Windows.
- `cli/test_man_page.py` (`cli.man_page`) checks the page against `--help`, the command bar `Action`
  lists and the key constants in `src/Prefs.h`, and lints it with mandoc, or groff when mandoc isn't
  installed. It's documented in `test/README.md`.
- `AGENTS.md` says to update the page with any change to an option, key, screen, preference,
  environment variable or exit status.
- `-h` is now accepted, `-h`/`--help` print the usage to stdout and exit 0, and the `--help` typos
  are fixed.

Left:

- There is still no `install(TARGETS ncpwsafe)` rule, so `cmake --install` installs the page but
  not the executable.
- Shipping the page with the release executables waits for the release pipeline (§6, R2).

The rest of this section is the design M1 followed.

### 4.2 Contents

One page, `ncpwsafe(1)`, with these sections:

| Section | Contents |
|---|---|
| NAME, SYNOPSIS | One synopsis line for opening a database and one for each command. |
| DESCRIPTION | What ncpwsafe does, the V1/V2 Password Safe formats, read-only mode, backups before saving, and the need for `xclip` to copy to the clipboard. |
| COMMANDS | Opening a database (the default), `--generate-password`, `--export-db`, `--change-password`. `--test-db` only in Debug builds, so leave it out of the page or mark it as such. |
| OPTIONS | Every option in `options[]`, grouped as in `--help`, with defaults. Mark `-P/--password` and `--new-password` as deprecated and explain why, pointing to `--password-file` and `--new-password-file`. |
| USER INTERFACE | The screens and what each is for: the unlock prompt, the accounts list (collapsible groups, the title bar status, the copy confirmation), the account details dialog (sections, masked password, multi-line notes), change account password, generate password, change database password, the search bar, the `^G` help, message boxes and confirmations. What changes in read-only mode. Resizing the terminal. |
| KEYS | A table of keys for each screen: the command bar keys from each window's `Action` list, plus the keys the command bar doesn't show (navigation, `/`, field movement, `y`/`n` in confirmations). |
| PREFERENCES | Which file is read (§1.4), what ncpwsafe writes and how, the `-c/--config` override, the INI format, the `[template.NAME]` sections, and each key with its type, default and meaning: `db-pathname`, `backup-before-save`, `backup-count`, `theme`, `auto-sign-off` (§3), and later the template settings and theme overrides (§1, §2.3). How backup files are named, and which keys ncpwsafe writes itself (`db-pathname`, `backup.N`). |
| FILES | The user and system configuration files (§1.4), the default database, backup files, the `.lock` file, and the database format (§5). |
| ENVIRONMENT | `HOME`; `LC_ALL`/`LC_CTYPE`/`LANG` and the fallback to a UTF-8 `LC_CTYPE`; `TERM`; `NO_COLOR`; `DISPLAY` for `xclip`; `NCPWSAFE_XCLIP`. |
| EXIT STATUS | 0 on success, the `ResultCode` values a user can see, and the auto sign-off code (§3). |
| SECURITY | Passwords on the command line can be seen by other users; use `--password-file` (with `<(pass show …)` as an example). Core dumps and ptrace are blocked in Release builds. Auto sign-off. Copied passwords stay on the clipboard. |
| EXAMPLES | Opening read-only, exporting to a file, generating ten passwords, changing the database password from files. |
| SEE ALSO, BUGS, AUTHORS | `xclip(1)`, Password Safe; `docs/ISSUES.md` and the issue tracker. |

Keep the preferences file in a section of `ncpwsafe(1)` rather than in its own `ncpwsafe.ini(5)`
page, so everything is in one place. Split it out later if it grows, for example once templates and
theme overrides add their settings.

### 4.3 Format and build

- Write the page by hand in mdoc(7), as `man/ncpwsafe.1.in`. mdoc has semantic macros (options,
  arguments, files, environment variables), and `mandoc -Tlint` can check it. groff and mandoc both
  render it, so building needs no new dependency. Generating the page from Markdown with pandoc or
  scdoc would add a build dependency, and the generated output is harder to get right in the details.
- `configure_file()` the page into the build directory, filling in the version, so the page always
  matches the build. The default preferences paths are fixed in `src/ProgArgs.cpp`, so the page can
  state them directly.
- Add install rules: `include(GNUInstallDirs)`, `install(TARGETS ncpwsafe)`, and
  `install(FILES …/ncpwsafe.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)`.
- Preview with `man -l build/man/ncpwsafe.1`.
- Ship the page with the release executables (§6).
- Fix the `--help` typos at the same time, and keep `--help` and the page's OPTIONS in the same order
  and wording.

### 4.4 Keeping it current

- CLI test `cli/test_man_page.py`:
  - Every long option in the `--help` output appears in the page's OPTIONS section, and every option
    in the page appears in `--help`.
  - Every key constant in `src/Prefs.h` (except the `backup.N` entries) appears in the PREFERENCES
    section.
  - Every key in the `Action` lists in `src/*.cpp` appears in the KEYS section. This one reads the
    sources and is the most brittle; drop it if it gets in the way.
- Lint the page with `mandoc -Tlint` (or `groff -mdoc -ww -z`) in a test that exits 77, and so is
  skipped, when neither is installed.
- Add a line to `AGENTS.md`: changing an option, a key, a screen or a preference means updating the
  man page in the same change. Document the new test in `test/README.md`.
- The `^G` help overlay (U-6) and the KEYS section describe the same keys; keep their wording the
  same. Once U-6 registers the navigation keys too, the consistency test can check the KEYS section
  against the `Action` lists alone.

### 4.5 Milestones

| # | Milestone | Depends on | Status |
|---|---|---|---|
| M1 | The page for the current application, `configure_file()` and install rules, the lint and consistency tests, the `AGENTS.md` note, and the `--help` typo fixes. | — | Done in #14, except `install(TARGETS ncpwsafe)` |

After M1, each feature updates the page in its own change (S1's `auto-sign-off` was added with M1; D3
adds `--import`, P4 adds `--template`, theme overrides add `[theme]`), rather than in a separate milestone.

---

## 5. New database format

Replace the Password Safe V2 format as ncpwsafe's data store with a new encrypted format: an SQLite
database, encrypted and authenticated as a whole. Support for the format is built as a separate C
library, like libpwsafe, which ncpwsafe imports with `FetchContent`. A new import command, in the CLI
and the TUI, converts a Password Safe V1 or V2 file to the new format.

The library is `libncpwsafe` (C prefix `ncpw_`), and its files use the extension `.ncpwsafe` (Q8).

### 5.1 Why replace V2

- **Weak cryptography.** V1/V2 use Blowfish (64-bit blocks) and a SHA-1 based key derivation with a small, fixed work factor, so the password can be guessed quickly from a stolen file. The file has no
  authentication: changes to the ciphertext aren't detected.
- **Fixed, small set of fields.** No field for templates (§1), timestamps, password history, or
  anything else. libpwsafe can only add fields the V2 format already has.
- **Every feature has to work around it.** There is nowhere to store an account's template (§1.4).
- V3, Password Safe's current format, fixes some of this (Twofish, SHA-256 key stretching, HMAC) but
  is still a fixed record format, and libpwsafe doesn't support it.

### 5.2 Goals

1. Strong, current cryptography: a memory-hard password KDF and authenticated encryption, with
   parameters stored in the file so they can be raised later.
2. A flexible schema that can grow (templates, timestamps, history) through versioned migrations.
3. Decrypted data never written to disk: no plaintext journal, temporary or WAL files.
4. Safe saving: a crash or a full disk never leaves a broken database.
5. Lossless import of V1/V2 files, and no change to the source file.
6. A small, documented format that can be read again in 20 years, specified independently of the
   code.

### 5.3 Encryption: options considered

| Option | How it works | Verdict |
|---|---|---|
| **SQLCipher** | A fork of SQLite that encrypts each page (AES-256-CBC, HMAC-SHA512 per page) and derives the key with PBKDF2-HMAC-SHA512. | Mature and widely used, and saves only the changed pages. But PBKDF2 isn't memory-hard, it replaces the system SQLite (symbol clashes if anything else in the process uses SQLite), it needs OpenSSL, LibTomCrypt or NSS as its crypto provider, and packaging varies between distros and vcpkg. Incremental page writes don't matter for a database of a few hundred accounts. |
| **SQLite3 Multiple Ciphers** | An SQLite amalgamation with page encryption (ChaCha20-Poly1305, AES, Ascon) built in. | Self-contained and permissively licensed, but also page-level with PBKDF2-style KDFs, and less widely reviewed. |
| **SQLite SEE** | SQLite's official encryption extension. | Commercial licence. Ruled out. |
| **Field-level encryption** | A plain SQLite file with secret columns encrypted by the application. | The file shows titles, groups, URLs, counts and structure unless every column is encrypted, which loses the benefit of SQL. Ruled out. |
| **Whole-file envelope** (recommended) | The SQLite database lives only in memory. On save it is serialized (`sqlite3_serialize()`), encrypted with an AEAD under a random data key, and written in one piece; on open it is decrypted and loaded with `sqlite3_deserialize()`. | Plaintext never reaches the disk, because an in-memory database has no journal or temporary files. Any KDF and AEAD can be used, the whole file is authenticated (header included), it works with an unmodified SQLite, and the format is simple to specify and review. The cost: the whole file is rewritten on each save and held in memory, which is what ncpwsafe already does with V2. |

**Recommendation: the whole-file envelope**, using **libsodium** for the cryptography:

- **KDF: Argon2id** (`crypto_pwhash`), memory-hard. Defaults: 256 MiB and 3 passes (libsodium's
  "moderate" limits), which takes about half a second to a second to open. The parameters are stored
  in the file. `ncpw_db_create()` accepts lower limits for small machines, with a floor (for example
  64 MiB, 2 passes). When the stored parameters are below the current defaults, the next save
  upgrades them.
- **AEAD: XChaCha20-Poly1305** (`crypto_aead_xchacha20poly1305_ietf_*`). Its 24-byte nonce can be
  random on every save with no risk of reuse.
- **Memory:** `sodium_malloc()` for keys (guarded and locked pages), `sodium_memzero()` to wipe,
  `randombytes_buf()` for salts, nonces and keys.

libsodium is packaged by every Linux distribution, OpenBSD and vcpkg. Neither nettle (no Argon2) nor
OpenSSL before 3.2 (no Argon2) provides everything needed. ncpwsafe itself doesn't use nettle; only
libpwsafe does.

### 5.4 File format (version 1)

All integers are little-endian. The header is authenticated as associated data (AAD) of the payload,
so changing any header byte makes the file fail to open.

**Header**

| Offset | Size | Field |
|---|---|---|
| 0 | 8 | Magic `NCPWSAFE` (no terminating NUL) |
| 8 | 2 | Format version, `1` |
| 10 | 2 | Header length in bytes, including the key slots |
| 12 | 4 | Flags, `0` (reserved) |
| 16 | 1 | Number of key slots, `n` (1 for now) |
| 17 | 98 × n | Key slots |
| … | 24 | Payload nonce |

**Key slot** (98 bytes). The database is encrypted with a random 256-bit **data key**. Each key slot
holds the data key encrypted ("wrapped") with a key derived from one secret:

| Size | Field |
|---|---|
| 1 | Slot type: `1` = password |
| 1 | KDF: `1` = Argon2id v1.3 |
| 4 | KDF passes (`opslimit`) |
| 4 | KDF memory in KiB (`memlimit`) |
| 16 | KDF salt |
| 24 | Nonce for the wrapped key |
| 48 | Wrapped data key: 32 bytes + 16-byte tag, XChaCha20-Poly1305, with the slot's preceding bytes and the magic and version as AAD |

Slots make room for other ways to unlock the same database later, such as a key file or the YubiKey
item in the backlog, without changing the format.

**Payload**: XChaCha20-Poly1305 ciphertext of the plaintext below, followed by its 16-byte tag, with
the whole header as AAD.

| Size | Field (plaintext) |
|---|---|
| 8 | Length of the SQLite image |
| … | The SQLite image (`sqlite3_serialize()`) |
| … | Zero padding to a multiple of 64 KiB, so the file size shows only roughly how much it holds |

**Opening**: read and check the header (magic, version, lengths), derive the slot key with Argon2id,
unwrap the data key (a failure here means an incorrect password), decrypt and authenticate the
payload (a failure here means a corrupt or tampered file), then `sqlite3_deserialize()` the image
into an in-memory database and run `PRAGMA quick_check`.

**Saving**: `PRAGMA secure_delete = ON` is always set, so deleted values are zeroed in the image;
`VACUUM` before serializing drops free pages. Encrypt with a new random nonce, write to a temporary
file in the same directory with mode `0600`, `fsync()` it, `rename()` it over the database, and
`fsync()` the directory. On Windows, use `ReplaceFileW()`/`MoveFileExW()` with
`MOVEFILE_WRITE_THROUGH`. If anything fails, the old file is untouched.

**Changing the password** generates a new data key and new slot, not just a re-wrapped key, so a copy
of the old file plus the old password can't decrypt the new file.

**Passwords** are normalized to Unicode NFC before the KDF, so the same password typed on different
systems or keyboards gives the same key. The library takes UTF-8 bytes; ncpwsafe normalizes with ICU,
which it already uses. The format spec states this so other implementations do the same.

**Concurrent use**: the library takes an advisory lock (`flock()`, or `LockFileEx()` on Windows) on a
`<file>.lock` next to the database while it is open for writing. A second ncpwsafe opening the same
file is told it is in use, and can open it read-only.

**Not protected against**: an attacker replacing the file with an older copy of itself (rollback).
Detecting that needs state kept outside the file, which is out of scope.

### 5.5 Schema (version 1)

The schema version is `PRAGMA user_version`, separate from the file format version. The library
migrates older schemas when it opens a file and refuses files with a newer schema or format version
than it knows, rather than risk losing data.

```sql
CREATE TABLE meta (              -- Database-wide settings
    key   TEXT PRIMARY KEY,      -- e.g. 'created_by', 'imported_from'
    value TEXT
);

CREATE TABLE entries (           -- Accounts
    id          INTEGER PRIMARY KEY,
    uuid        BLOB NOT NULL UNIQUE CHECK (length(uuid) = 16),
    group_path  TEXT NOT NULL DEFAULT '',   -- Password Safe convention: 'a.b.c'
    title       TEXT NOT NULL,
    username    TEXT,
    password    TEXT,
    url         TEXT,
    email       TEXT,
    notes       TEXT,
    template    TEXT,            -- Password template name (§1.4); NULL for the default
    created_at  INTEGER,         -- Unix time in seconds (UTC); NULL when unknown
    modified_at INTEGER,
    password_modified_at INTEGER
);
CREATE INDEX entries_group_title ON entries (group_path, title);

CREATE TABLE entry_fields (      -- Fields without a column, kept so import is lossless
    entry_id INTEGER NOT NULL REFERENCES entries(id) ON DELETE CASCADE,
    type     INTEGER NOT NULL,   -- The Password Safe field type, e.g. FT_FILEPATH (0x65)
    value    BLOB NOT NULL
);

CREATE TABLE changes (           -- Recovery files only (§3.7); empty in a database
    uuid   BLOB NOT NULL CHECK (length(uuid) = 16),   -- The account's UUID
    change TEXT NOT NULL CHECK (change IN ('added', 'changed', 'deleted')),
    field  INTEGER,              -- For 'changed': the field type; NULL otherwise
    UNIQUE (uuid, field)
);
```

- Text is UTF-8. Notes keep their line breaks exactly as stored (§2.1).
- `entry_fields` holds V2 fields the schema has no column for (`FT_NAME`, `FT_FILEPATH`, unknown
  types), so nothing is lost on import, and an export back to V2 (D5) can write them again.
- `changes` records the change state of every field for recovery files (§3.7): a recovery file has
  `meta.kind = 'recovery'`, the changed accounts in `entries`, and one row in `changes` for each added
  or deleted account and each changed field. A deleted account has a row in `changes` but none in
  `entries`. The library opens a recovery file only when asked for one, so it can't be mistaken for a
  database.
- `PRAGMA foreign_keys = ON` on every open.
- Added later through migrations, not in version 1: password history, last-used times (left out on
  purpose: they record behaviour), tags, attachments.

### 5.6 Library: `libncpwsafe`

A separate repository, set up like libpwsafe: C17, CMake presets and toolchains, GoogleTest, a C API
with the same conventions (`PRC_*`-style result codes through an optional `int *rc`, caller-freed
results), and `docs/format.md` holding the specification in §5.4 and §5.5. ncpwsafe pulls it with
`FetchContent` in `cmake/ResolveDependencies.cmake`, like libpwsafe.

**Dependencies:** libsodium, and SQLite. Compile the SQLite amalgamation into the library, with its
symbols hidden, rather than linking the system SQLite. That lets the library set compile options
(`SQLITE_OMIT_LOAD_EXTENSION`, `SQLITE_SECURE_DELETE`, `SQLITE_TEMP_STORE=3` so temporary tables
stay in memory, `SQLITE_DEFAULT_FOREIGN_KEYS=1`) and install a memory allocator that wipes freed
memory (`SQLITE_CONFIG_MALLOC` over `sodium_memzero()` + `free()`), without affecting anything else
in the process. It also makes the format independent of the SQLite version a distribution ships.

**API sketch** (names to be settled with the library):

```c
typedef struct NcpwDb NcpwDb;
typedef struct { uint32_t opslimit; uint32_t memlimit_kib; } NcpwKdfParams;

/* Files */
int  ncpw_db_create(const char *path, const char *password, const NcpwKdfParams *kdf, NcpwDb **db, int *rc);
int  ncpw_db_open(const char *path, const char *password, int read_only, NcpwDb **db, int *rc);
int  ncpw_db_check_password(const char *path, const char *password, int *rc);
int  ncpw_db_save(NcpwDb *db, int *rc);
int  ncpw_db_save_as(NcpwDb *db, const char *path, int *rc);
int  ncpw_db_change_password(NcpwDb *db, const char *new_password, int *rc);  /* takes effect on save */
int  ncpw_db_is_dirty(const NcpwDb *db);
void ncpw_db_close(NcpwDb *db);   /* wipes keys and the in-memory database */
int  ncpw_file_is_ncpwsafe(const char *path);   /* checks the magic only */

/* Entries */
typedef struct NcpwEntry NcpwEntry;   /* uuid, group_path, title, ..., template, extra fields */
int  ncpw_entries_list(NcpwDb *db, NcpwEntry **entries, size_t *count, int *rc);
int  ncpw_entry_put(NcpwDb *db, const NcpwEntry *entry, int *rc);   /* insert or update by uuid */
int  ncpw_entry_delete(NcpwDb *db, const uint8_t uuid[16], int *rc);
void ncpw_entries_free(NcpwEntry *entries, size_t count);   /* wipes */

/* Database settings */
int  ncpw_meta_get(NcpwDb *db, const char *key, char **value, int *rc);
int  ncpw_meta_set(NcpwDb *db, const char *key, const char *value, int *rc);
```

The library knows nothing about templates: an entry's `template` is just a name, which ncpwsafe looks
up in the configuration files (§1.4).

**Testing in the library:** known-answer tests for the file layout (a committed test file with a
known password), round trips, wrong password vs corrupt file, every header field changed by one byte,
truncated files, a newer format or schema version, migrations, the lock, save failures (disk full,
read-only directory) leaving the old file intact, and that no file other than the database and the
lock is created next to it. Fuzz the header and payload parsing (libFuzzer or AFL) in CI. Run under
ASan and UBSan.

### 5.7 Changes in ncpwsafe

- **Data model:** `AccountRecord` and `AccountRecords` keep their dirty tracking, but use
  ncpwsafe's own `FieldType` enum instead of libpwsafe's `PwsFieldType`, so the model no longer
  depends on libpwsafe. libpwsafe is then used only to read V1/V2 files.
- **`AccountDb`:** opens new-format files with `ncpw_db_open()` and reads every entry into
  `AccountRecords`. On save it writes the changes back in one transaction (`ncpw_entry_put()` for new
  and changed records, `ncpw_entry_delete()` for deleted ones), then `ncpw_db_save()`. Moving the UI
  onto the library's entries directly can come later; this keeps the change small. The file type is
  found by `ncpw_file_is_ncpwsafe()`, then by trying libpwsafe.
- **V1/V2 files** open read-only, with a message offering to import them (§5.8). ncpwsafe no longer
  writes V1/V2 files.
- **Create a new database** (an item in §7.2): the TUI offers it when the database in the prefs or
  on the command line doesn't exist, asking for the password twice; the CLI gets `--create-db`.
- **`--change-password`** works on new-format files only; for a V1/V2 file it says to import it
  first. **`--export-db`** reads both. **`--test-db`** and the `e2e-mkdb` test helper write the new
  format (`e2e-mkdb` keeps a flag to write V2, for the import tests).
- **Default database:** `db-pathname` defaults to `$XDG_DATA_HOME/ncpwsafe/passwords.ncpwsafe`
  (`~/.local/share/...`, or `%APPDATA%\ncpwsafe\` on Windows) instead of `~/.pwsafe.dat`.
- **Result codes:** map the library's codes into `ResultCode`, with distinct messages for an incorrect
  password, a corrupt file, a file from a newer version, and a file in use.
- **Backups** (`backup-before-save`, `backup-count`) work unchanged: they copy the encrypted file.
- **Dependencies:** libsodium (pkg-config, and vcpkg for Windows), through the library. Update
  `AGENTS.md`, the README, CI and the devcontainers.
- **Docs:** the man page (§4) gets the import and create commands and a FILES entry for the format,
  pointing to the library's `docs/format.md`.

### 5.8 Import from Password Safe V1/V2

**CLI**

```
ncpwsafe --import [--password-file=PATH] [--new-password-file=PATH | --keep-password]
         [-o,--out=PATHNAME] [--force] SOURCE_FILE
```

- Reads `SOURCE_FILE` (V1 or V2) with libpwsafe, using the password from `--password-file` (or the
  deprecated `-P`).
- The new database's password comes from `--new-password-file`, or `--keep-password` reuses the
  source's password. One of the two is required, so a weak old password isn't carried over without
  a choice.
- The output defaults to `SOURCE_FILE` with its extension replaced by `.ncpwsafe`. An existing output file
  is refused without `--force`, as for `--export-db`.
- The source file is never changed, renamed or deleted.
- After writing, the new file is opened again and compared with the source, field by field. Then it
  prints `Imported 42 accounts from pwsafe.dat (Password Safe V2) to pwsafe.ncpwsafe` to stderr and
  exits 0. Any difference is an error, and the new file is removed.

**Mapping**

| V1/V2 | New format |
|---|---|
| `FT_UUID` | `uuid` (16 bytes from the 32 hex digits). V1 records, and V2 records without a UUID, get a new random one. |
| `FT_GROUP`, `FT_TITLE`, `FT_USER`, `FT_PASSWORD`, `FT_URL`, `FT_EMAIL`, `FT_NOTES` | The matching columns, as UTF-8. Text that isn't valid UTF-8 (old files written in a legacy code page) is converted from Windows-1252, and the import reports how many fields were converted. |
| V1 name field (`title  user` joined by the separator) | Split into `title` and `username`, as libpwsafe already does when reading. |
| `FT_NAME`, `FT_FILEPATH`, unknown types | `entry_fields`, unchanged. |
| — | `created_at`, `modified_at` and `password_modified_at` are `NULL` (unknown). `meta.imported_from` records the source format and the import time, not the source path. |

**TUI**

- Opening a V1/V2 file (from the command line, the prefs, or the unlock prompt) shows, after the
  password is accepted: "pwsafe.dat is a Password Safe V2 database. ncpwsafe can only edit it after
  importing it into a new database." with **Import**, **Open read-only** and **Cancel**.
- **Import** asks for the new file's pathname (the `.ncpwsafe` default filled in) and the new password,
  with "Keep the current password" as a choice, then imports and verifies as the CLI does and opens
  the new database. The `db-pathname` pref is set to the new file, so the old file isn't opened next
  time. The message says that the old file was left as it was, and where it is.
- **Open read-only** shows the accounts as today's read-only mode does.
- Later, optional: an "Import into this database" command that merges a V1/V2 file into the open
  database, matching accounts by UUID and asking what to do with conflicts.

### 5.9 Testing in ncpwsafe

- Unit tests: the V1/V2 → new format mapping (UUIDs, the V1 name split, legacy text, extra fields),
  and `AccountDb` saving only changed records.
- CLI tests (`cli/test_import.py`): V1 and V2 fixtures, with and without `--keep-password`, the
  default output name, an existing output without and with `--force`, a wrong source password, a
  source that isn't a Password Safe file, that the source is unchanged (compare bytes), and that the
  imported database exports the same text as the source (`assertDbContains`). Commit a small V1
  fixture, since libpwsafe only writes V2.
- CLI tests for `--create-db` and for `--change-password` refusing a V2 file.
- TUI tests: the import prompt and each of its choices, the read-only open, creating a database, and
  that the prefs point at the new file afterwards.
- The existing CLI and TUI tests switch to new-format fixtures.

### 5.10 Milestones

| # | Milestone | Depends on |
|---|---|---|
| D1 | `libncpwsafe`: format spec, create/open/save/close, password change, entries and meta API, locking, tests and fuzzing. Rename ncpwsafe's own `libncpwsafe` OBJECT target first (Q8). | — |
| D2 | ncpwsafe on the library: `FieldType`, `AccountDb` for both formats, V1/V2 read-only, create database (CLI and TUI), `--change-password`, `--export-db`, `--test-db`, `e2e-mkdb`, new default path. | D1 |
| D3 | `--import` in the CLI. | D2 |
| D4 | Import from the TUI. | D3 |
| D5 | Optional: `--export-db --format=pwsafe-v2`, for use with other Password Safe programs. | D2 |

---

## 6. Product releases

Publish ncpwsafe as ready-to-run executables, so users don't have to build it and its dependencies
from source. Pushing a version tag (`v*`) builds, tests and packages ncpwsafe for Linux and Windows
and attaches the packages, with the man page, to a GitHub release.

### 6.1 Current state

- `.github/workflows/build.yml` builds and tests `x64-windows-msvc-release` on Windows (#4) and
  `x64-linux-gcc-release` and `x64-linux-clang-release` on Ubuntu. It already runs on `v*` tags,
  but publishes nothing except the vcpkg logs when the Windows job fails.
- The version is `VERSION 0.1` in the top-level `project()`, passed to `config.h`
  (`NCPWSAFE_VERSION`) and the man page. There are no tags, and nothing checks that a tag matches
  the version. There is no `--version` option.
- The Linux executable links the system's shared libraries (ncursesw, formw, menuw, panelw, ICU,
  nettle), so it runs only on a system with the same library versions as the build machine.
- The Windows build links its vcpkg dependencies statically, but with the dynamic CRT
  (`x64-windows-static-md`), so it needs the Visual C++ Redistributable installed.
- `libpwsafe` and `libconfini` are fetched at `GIT_TAG HEAD` (`cmake/ResolveDependencies.cmake`),
  so building the same tag twice can give different results.
- `cmake --install` installs the man page (not on Windows) but not the executable (§4.1).

### 6.2 Goals

- A tag is a release: one push of `vX.Y.Z` produces the GitHub release, with no manual steps except
  writing the release notes.
- A release can be rebuilt: the tag pins ncpwsafe's own dependencies.
- The Linux executable runs on any reasonably recent x86-64 distro, without installing libraries.
- The Windows executable runs on a stock Windows 10 or later, without installing anything.
- Users can check what they downloaded: checksums for every file.

### 6.3 Versions

- [Semantic versioning](https://semver.org): `MAJOR.MINOR.PATCH`, starting from `0.1.0`. Before 1.0,
  a minor version may change behaviour or the database format; say so in the release notes.
- The version in `project()` stays the source of truth. The release job fails when the tag isn't
  `v` followed by that version, so a release can't go out with the wrong version in `--version`
  and the man page.
- Add `-V`/`--version`: print `ncpwsafe X.Y.Z` and exit 0. Add it to `--help` and the man page
  (`cli.man_page` checks this).
- Pin `libpwsafe` and `libconfini` to tags or commit hashes in `cmake/ResolveDependencies.cmake`
  (the same change is planned for libconfini in §1.4). A release of either fork is then a deliberate
  change to ncpwsafe.

### 6.4 Packages

| File | Contents |
|---|---|
| `ncpwsafe-X.Y.Z-linux-x86_64.tar.gz` | `ncpwsafe/bin/ncpwsafe`, `ncpwsafe/share/man/man1/ncpwsafe.1`, `README.md`, `LICENSE` |
| `ncpwsafe-X.Y.Z-windows-x64.zip` | `ncpwsafe\ncpwsafe.exe`, `README.md`, `LICENSE`, and the man page rendered to text or HTML (`mandoc -Thtml`), since Windows has no `man` |
| `SHA256SUMS` | The SHA-256 of each package |

- Build the archives with `cmake --install` into a staging directory, then archive it. This needs
  the missing `install(TARGETS ncpwsafe)` rule (§4.1), and keeps the packages and a source install
  the same. CPack can make the archives, but a few lines of shell are simpler to read; pick one in
  R2.
- ncpwsafe is GPLv3, and the statically linked libraries are GPL-compatible (ncurses is MIT-style,
  ICU is under the Unicode licence, nettle is LGPLv3/GPLv2+). Include their licence notices in the
  packages, in a `THIRD-PARTY-LICENSES` file, and link to the source of the tag in the release.
- Name the Linux runtime need in the release notes and the README: `xclip` (and an X or XWayland
  display) to copy to the clipboard. Windows uses the native clipboard.
- Mention auto sign-off's default of 5 minutes in the first release's notes (§3.4).

### 6.5 Linux build

- Link every dependency statically, so the executable runs on other distros than the CI runner's
  Ubuntu: ncursesw with formw/menuw/panelw, ICU, nettle, libpwsafe, libconfini.
- glibc is hard to link fully statically (`getpwnam` and locales load modules at run time), so build
  against musl in an Alpine container and link fully statically. The alternative, everything except
  glibc statically, ties the executable to the build machine's glibc version or later.
  - Check that `SetUtf8CtypeLocale()` works with musl, whose locale support is minimal but always
    UTF-8 capable.
  - Check terminal support: a static ncurses needs a terminfo database at run time. Build it to look
    in the usual places (`/usr/share/terminfo`, `/lib/terminfo`, `/etc/terminfo`) and fall back to
    built-in entries for the common terminals (`--with-fallbacks=xterm-256color,screen,tmux,linux`
    or similar), so it works when the paths differ.
- ICU's data library is large (about 30 MB). Check the executable's size, and whether the features
  ncpwsafe uses (UTF-8 conversion and case folding) work with a reduced data set built with ICU's
  data filter.
- Add a `x64-linux-gcc-static-release` preset (and toolchain settings) to `CMakePresets.json`. It
  may need the dependencies built from source in the container rather than found with pkg-config.
- Run the CLI and TUI end-to-end tests against the static executable in the container, then run its
  `--version` and `--generate-password` on a different distro image (for example Debian and Fedora)
  as a smoke test.

### 6.6 Windows build

- Switch the release build to the static CRT (`x64-windows-static`, release only, as an overlay
  triplet like `triplets/x64-windows-static-md-release.cmake`), so the Visual C++ Redistributable
  isn't needed. Set `CMAKE_MSVC_RUNTIME_LIBRARY` to match in `cmake/toolchain-windows-msvc.cmake`.
- Code signing: an unsigned executable gets SmartScreen warnings. Leave it unsigned at first and say
  so in the release notes; consider signing later (Q10).

### 6.7 Workflow

- Keep `build.yml` for pull requests and `main`. Add `release.yml`, run on `v*` tags only, or add
  release jobs to `build.yml` behind `if: startsWith(github.ref, 'refs/tags/v')`, sharing the vcpkg
  cache steps. Prefer a separate workflow if the jobs differ much (the Linux release job runs in an
  Alpine container, unlike the Ubuntu CI jobs).
- Jobs: check the version against the tag; build, test and package for Linux and for Windows; then a
  job that needs both, downloads their artifacts, writes `SHA256SUMS` and creates a **draft** GitHub
  release with `gh release create --draft`. Publishing the draft after writing the notes is the one
  manual step.
- Give only the last job `contents: write`; the build jobs keep read-only permissions.
- Consider [artifact attestations](https://docs.github.com/actions/security-for-github-actions/using-artifact-attestations)
  (`actions/attest-build-provenance`), so users can check that a package was built by the workflow
  from the tag. It's cheap to add and suits a password manager.

### 6.8 README screenshots

Add two screenshots to `README.md`, near the top, so visitors to the repository and the release
page can see what ncpwsafe looks like before downloading it:

1. The accounts list: a few groups, one collapsed, with an account selected, the title bar and the
   command bar.
2. Editing an account: the account details dialog over the accounts list, with the password masked
   and a notes field of more than one line.

- Use a made-up database only, never a real one. Add a fixture for it, for example
  `test/e2e/fixtures/screenshot.tsv`, with believable groups and accounts (Banking, Email, Shopping,
  Work) and made-up user names, so the screenshots can be taken again after a UI change.
- Take them in a common terminal at about 100×30, with the default theme in colour, and crop to the
  terminal window. Save them as PNG in `docs/images/` (`accounts-list.png`, `edit-account.png`) and
  keep each file small (optimise with `oxipng` or similar).
- Give each image alt text that describes what it shows.
- Retake the screenshots when a UI change makes them out of date. Add a line to `AGENTS.md` next to
  the man page note.
- Optional: script the screenshots with `tmuxdriver` (open the fixture, collapse a group, open an
  account) and a terminal-to-image tool, so they can be retaken with one command. Only worth it if
  the UI keeps changing.

### 6.9 Testing

- The release workflow runs the full CTest suite on each platform before packaging, as CI does.
- CLI test for `--version`: prints the version from `config.h` and exits 0.
- Test the workflow without publishing: run it on a `v0.0.0-test` style tag in a fork, or with
  `workflow_dispatch` and a dry-run input that skips creating the release.
- Manual check for each release: download both packages, check `SHA256SUMS`, open a database, copy
  a password, and read the man page.

### 6.10 Milestones

| # | Milestone | Depends on | Status |
|---|---|---|---|
| R1 | `--version`; `install(TARGETS ncpwsafe)`; pin `libpwsafe` and `libconfini`; the version-against-tag check. | M1 | |
| R2 | Release workflow: Windows package (static CRT), and the current dynamically linked Linux build packaged as is, labelled for the runner's Ubuntu version; `SHA256SUMS`; draft release. | R1 | |
| R3 | Statically linked Linux build (musl), tested on other distros, replacing R2's Linux package. | R2 | |
| R4 | README screenshots of the accounts list and of editing an account (§6.8). | — | |
| R5 | Optional: attestations, Windows code signing, distro packages (AUR, a Debian package). | R2 | |

---

## 7. Other work

Items from the old `TODO.md` that the features above don't cover. Items they do cover are listed
in §7.4.

### 7.1 Bugs

- Crash if the terminal doesn't support UTF-8. Probably fixed by #11, which links the wide-character
  ncurses libraries and sets a UTF-8 `LC_CTYPE` (falling back to `C.UTF-8`) when the user's locale
  isn't UTF-8. Still to check: a system with no UTF-8 locale installed, where
  `SetUtf8CtypeLocale()` fails, and a terminal that can't show UTF-8 such as the `TERM=linux`
  console. The ASCII fallback in §2.5 covers the rest.
- See [`ISSUES.md`](ISSUES.md) for other bugs. It has none open: the bugs found while writing the
  end-to-end tests were fixed in #11.

### 7.2 Features

- ~~Create a new database.~~ Part of the new format (§5.7, D2).
- ~~Product releases.~~ Now a feature of its own (§6).

### 7.3 Backlog

- Move strings to a string resource file for i18n.
  - Check the library used by pwsafe-cli.
- YubiKey support. The new format's key slots (§5.4) leave room for it.

### 7.4 Covered by this plan

| Old `TODO.md` item | Where |
|---|---|
| Diceware passwords | §1, passphrase kind (P3) |
| Fix password policies | §1 (P2) |
| Save password policy to account entry | §1, account links (P6) |
| Close db after timeout (test by putting the computer to sleep) | §3 auto sign-off (S1) |
| Update the CI/CD pipeline for product releases | §6 product releases |

"Colors", "Improve command bar", "F1 Help" and "Display user name in account list" were done in #12
(§7.5).

### 7.5 Done

- Auto sign-off after a period without input (S1, #13)
- The `ncpwsafe(1)` man page, its consistency and lint test, and `-h`/`--help` fixes (M1, #14)
- UI brush-up milestones U-1, U-2/U-4, U-5 and U-6, and parts of U-9 and U-10 (#12, see §2.8),
  which also cover the old items "Colors", "Improve command bar", "F1 Help" and "Display user name
  in account list"
- Multiline notes fields (`TextArea`, `DialogField::m_height`) (#12)
- Windows build with MSVC and vcpkg, in CI (#4)
- Export database, and export as CSV
- Write last opened database to prefs
- Database copy-on-backup, and backup on save
- Change master password
- Fix deleting a record
- Compatibility with Nicolas Dade's pwsafe databases
- Generate passwords from the CLI, including N passwords
- Search
- Remove the `MenuInfoData` class
- Secure random password generation (P1, #8)
- Passwords from a file (`--password-file`, `--new-password-file`); `-P` and `--new-password`
  deprecated and hidden from `ps` (#8)
- Open the database with the password given on the command line, and report why it couldn't be used
  (#8)
- Clipboard copy: run the program in `NCPWSAFE_XCLIP`, and report `xclip` errors (#8)
- Resize windows and dialogs when the terminal is resized (#9)
- End-to-end tests of the CLI and the TUI (#10)
- Fixes from the end-to-end tests (#11): non-ASCII account text, message boxes that wrap, search
  with capital letters, editing an account's group or title deleting another account, crashes with
  no account selected, `--password-policy` by name, the `--change-password` exit code, `--export-db` without `-o`,
  core dumps and ptrace blocked in Release builds, and in-source builds rejected

## 8. Open questions

- ~~**Q1. Several databases.**~~ Settled: templates belong to the user, defined by hand in the
  configuration files, and each account names its template in the new format's `template` column (§1.4, §5.5).
  Links are per database by construction, and every database shares the user's templates.
- ~~**Q2. Template links in the database.**~~ Settled by the new format (§5): the database stores
  the template name of each account; the templates themselves stay in the configuration files.
- ~~**Q3. Replace `menu` in `AccountsWin`?**~~ Settled in #12: `AccountsWin` draws the list itself
  from `AccountList`, without the `menu` library.
- ~~**Q4. Config path mismatch.**~~ Settled: the default preferences file is `ncpwsafe.ini`
  (`~/.config/ncpwsafe/ncpwsafe.ini`, or `%APPDATA%\ncpwsafe\ncpwsafe.ini` on Windows), set by
  `DEFAULT_CONFIG_FILE` in `src/ProgArgs.cpp`. The unused `NCPWSAFE_CONFIG_FILE` CMake variable was
  removed. A file at the old path, `ncpwsafe-config.ini`, isn't read; rename it.
- ~~**Q5. Enter on the unlock prompt.**~~ Settled: `Enter` on the password field submitting the
  unlock prompt is intended (commit 01e1621), so the old `TODO.md` bug is dropped.
- ~~**Q6. Remember collapsed groups?**~~ Settled: no. Saving them in the prefs file would put group
  names there in plaintext, and group names say what kinds of accounts the database holds. Collapsed
  groups stay in memory only and are forgotten when the app exits (U-2).
- ~~**Q7. Plain letter keys in the accounts list.**~~ Settled: no new plain letter keys for now.
  Type-to-jump, Vim keys (`j`/`k`, `g`/`G`) and quick-copy keys (`u`/`p`/`l`) are all left out (U-8).
- ~~**Q8. Names.**~~ Settled: the library is `libncpwsafe` with the C prefix `ncpw_`, and files
  use the extension `.ncpwsafe` and the magic `NCPWSAFE`. ncpwsafe's own OBJECT library target is
  also called `libncpwsafe` today (`src/CMakeLists.txt`); rename it (for example to `ncpwsafe_core`)
  before pulling in the new library, so the targets don't clash.

- **Q9. Recovery file name.** Replace the database's extension (`accounts.ncpwsafe-tmp`), as first
  proposed, or append to the whole name (`accounts.dat.ncpwsafe-tmp`)? Replacing it makes two
  databases that differ only in extension share one recovery file, so one sign-off can overwrite the
  other's changes. Appending avoids that, and also makes it obvious which database a recovery file
  belongs to (§3.7).
- **Q10. Windows code signing.** Sign `ncpwsafe.exe`, so SmartScreen doesn't warn on first run?
  It needs a certificate (a yearly cost, or a free signing service for open-source projects such as
  SignPath) and a secret in the workflow. Unsigned is fine for the first releases (§6.6).

## 9. Suggested order

1. ~~**P1** (RNG fix)~~: done.
2. ~~**U-1, U-5, U-6, U-2/U-4**~~ (theme, command bar, help, grouped list): done in #12.
3. ~~**S1** (auto sign-off)~~: done in #13.
4. ~~**M1** (man page)~~: done in #14.
5. **R1, R2, R4** (versions, release workflow, README screenshots): a first release of the
   application as it is, now that it has a man page. R3 (static Linux build) can follow at any point.
6. **D1 → D4** (new format, library, import): the data store everything else builds on.
7. **C1**, **P2 → P4** (configuration files, templates and CLI): the CLI half of templates without UI
   work.
8. **U-11** (message boxes) and the rest of **U-6** (help scrolling, navigation keys).
9. **P5, P6**: the template picker and account links, built on the theme.
10. **U-7/U-8** (search and navigation), then the remaining U items, S2, S3, S4 and D5.
