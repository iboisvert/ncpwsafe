# ncpwsafe development plan

This plan covers two features:

1. [Password templates](#1-password-templates): named, reusable password generation settings.
2. [UI brush-up](#2-ui-brush-up): visual grouping, highlighting, and navigation improvements.

Each section describes the current state, the proposed design, and the work broken into
milestones that can be merged on their own. [Open questions](#3-open-questions) at the end
need a decision before or during implementation.

---

## 1. Password templates

### 1.1 Current state

- `PasswordPolicy` (`src/Policy.{h,cpp}`) has five fixed `Composition` values (alpha/digit/symbol,
  alpha/digit, easy-to-read, digits, hex) and a length. You cannot change the character sets.
- `GeneratePasswordDlg` steps through the compositions (Space) and lengths 4–56 in steps of 4 (`<`/`>`).
- The CLI command `--generate-password` takes `--password-count`, `--password-length` and
  `--password-policy=1..5`.
- No settings are saved. Each account's policy is lost when the dialog closes (see `TODO.md`:
  "Save password policy to account entry", "Diceware passwords", "Fix password policies").
- **Security issue:** `RandomState` (`src/RandUtils.h`) uses `std::mt19937` with a 32-bit seed, and
  `MakePassword()` picks characters with `n % N`. The Mersenne Twister is not cryptographically secure:
  its output can be predicted after 624 observed values, and a 32-bit seed limits it to 2³² possible
  sequences no matter the password length. The modulo also adds a small bias. This must be fixed as part
  of this feature (milestone P1).

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
| `max_age_days` | int | Marks accounts linked to this template as stale after N days. Shown in the account list (see U-4) when the database stores a last-modified time. Optional: the V1/V2 formats may have no such field. |
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

You can copy a built-in to make a user template, but you can't edit or delete the built-ins.

### 1.4 Configuration file

**Location:** `~/.config/ncpwsafe/password-gen.json` on Linux, next to the existing INI prefs. Resolve it
as `$XDG_CONFIG_HOME/ncpwsafe/password-gen.json`, falling back to `~/.config/...` when
`XDG_CONFIG_HOME` is unset. Use `~/.config` rather than `~/.cache`, because cleanup tools and backups
treat `~/.cache` as disposable and templates and account links must not be lost. Add an
`--password-gen-config=PATH` CLI option to override it, mirroring `config_file_`.

**Format** (versioned, so the format can change later):

```json
{
  "version": 1,
  "default_template": "strong",
  "templates": {
    "work-sso": {
      "kind": "random",
      "description": "Corporate SSO: 16-64 chars, no quotes",
      "length": 24,
      "lowercase": true, "uppercase": true, "digits": true, "symbols": true,
      "custom_symbols": "!@#$%^&*-_=+",
      "exclude_chars": "'\"`",
      "require_each_class": true
    },
    "memorable": {
      "kind": "passphrase",
      "word_count": 5,
      "wordlist": "eff-large",
      "separator": "-",
      "capitalization": "first",
      "append_digit": true,
      "max_length": 40
    }
  },
  "accounts": {
    "0123456789ABCDEF0123456789ABCDEF": "work-sso"
  }
}
```

**Linking templates to accounts.** libpwsafe's V1/V2 record fields (`FT_NAME`, `FT_UUID`, `FT_GROUP`,
`FT_TITLE`, `FT_USER`, `FT_NOTES`, `FT_PASSWORD`, `FT_URL`, `FT_EMAIL`) have no password-policy field.
Instead, the config file's `accounts` map links an account's `FT_UUID` to a template name. Trade-offs:

- (+) The database file stays compatible with other Password Safe clients.
- (−) The links don't travel with the database to another machine.
- (−) The file contains account UUIDs in plaintext. UUIDs are random and reveal nothing about the
  account, but the file does show how many accounts have templates. This is acceptable.
- If several databases are used, key by database as well: `"accounts": { "<db-path>": { "<uuid>": "name" } }`.
  **See open question Q1.**
- Delete a link when its account is deleted. Ignore links to UUIDs that no longer exist, and remove
  them when the file is next saved.

Do not store generated passwords in this file.

**JSON library:** the project has no JSON dependency yet. Add `nlohmann/json` (header-only) through
`FetchContent` in `cmake/ResolveDependencies.cmake`, or from the system package (`nlohmann-json3-dev` /
`json-devel`) found with `find_package`. Update the devcontainer Dockerfiles and CI to match.

**Write safety:** write to a temporary file in the same directory, then `rename()`. Create the
directory with mode `0700` and the file with `0600`. If the file is invalid, report the error with its
line and column, fall back to the built-ins, and never overwrite the file.

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
--show-template=NAME             Print a template's JSON
--password-gen-config=PATH       Override the template file location
```

- `--template` and `--password-policy` cannot be used together. Map `--password-policy=N` to the
  matching built-in.
- `--password-length` overrides the template's `length`, or its `word_count` for a passphrase. Reject it
  if the result breaks `min_length` or `max_length`.
- Without `--template`, use `default_template`.
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
  - `^E` opens the template editor.
- **Template editor dialog** (new, built on `Dialog`): one form per template kind, using checkboxes for
  the booleans. It shows a live sample and an entropy estimate, and runs validation (for example, "no
  character classes enabled" or "a length of 8 can't fit required classes 3+3+3"). Saves to the JSON
  file.
- **Template manager** (new): list, create, copy, rename, and delete templates. Reached from the
  accounts window (`^P`) or from the editor.
- **Account details dialog**: add a read-only "Template:" row showing the linked template, or
  "(default)" when none is linked.

### 1.8 Code structure

| New or changed | Responsibility |
|---|---|
| `src/SecureRandom.{h,cpp}` (replaces `RandUtils.h`) | `getrandom()` when `HAVE_SYS_RANDOM_H` is set, otherwise `/dev/urandom` or nettle's `yarrow256`. `uniform(n)` uses rejection sampling, which has no modulo bias. |
| `src/PasswordTemplate.{h,cpp}` | Plain data struct with `Validate()`, `EstimateEntropyBits()` and `Generate(SecureRandom&)`. Replaces `PasswordPolicy`. |
| `src/WordList.{h,cpp}` | Built-in and file-based word lists. |
| `src/PasswordTemplateStore.{h,cpp}` | Loads and saves JSON through `Filesystem` so tests can mock it. Merges built-in and user templates and holds the account links. Owned by `PWSafeApp`. |
| `GeneratePasswordCommand`, `GeneratePasswordDlg` | Use the store and templates. |
| `TemplateEditorDlg`, `TemplateManagerWin` | New dialogs. Follow the `ScopedResizeHandler` pattern. |

Zero generated-password buffers after use (the volatile memset in `Utils.cpp`), the same as other
password-handling code.

### 1.9 Testing

- `SecureRandom::uniform`: run a chi-square check of the distribution over many samples. Make the
  random source injectable so tests can use a fixed sequence.
- `PasswordTemplate`: every constraint (class requirements, exclusions, length bounds, repeats,
  passphrase capitalization and separators), plus `Validate()` rejecting templates that can't be
  satisfied.
- Entropy estimates checked against known values (for example, 6 EFF words is about 77.5 bits).
- `PasswordTemplateStore`: round-trip load and save, invalid JSON, a future `version`, missing file,
  write errors (mock `Filesystem`), and file permissions.
- CLI: `--template` together with `--password-policy`, unknown template names, and that each existing
  `--password-policy=N` matches its built-in.

### 1.10 Milestones

| # | Milestone | Depends on |
|---|---|---|
| P1 | `SecureRandom` replaces `RandomState`, which fixes the RNG weakness. Ship this on its own, first. | — |
| P2 | `PasswordTemplate` (random kind) and built-ins; `GeneratePasswordCommand` uses them; `--password-policy` still works. | P1 |
| P3 | Passphrase kind and embedded EFF word lists. | P2 |
| P4 | JSON store, `--template`, `--list-templates`, `--password-gen-config`. Add the JSON dependency. | P2 |
| P5 | Generate Password dialog rebuilt with the template picker. | P4, U-1 for styling |
| P6 | Account links (store and `^T`); the details dialog shows the template. | P4, P5 |
| P7 | Template editor and manager dialogs. | P5 |

---

## 2. UI brush-up

### 2.1 Current state

- Colour is not enabled: `start_color()` is commented out in `PWSafeApp::InitTUI`.
- Styling is limited to `A_BOLD` (dialog labels and the generated password) and `A_REVERSE`/`A_DIM`
  (command bar).
- The accounts window is a flat, two-column `menu` of titles. Groups (`FT_GROUP`) are not shown as
  groups, which makes large databases hard to scan.
- The command bar shows only as many commands as fit. `TODO.md` lists "Print page #", a "More"
  command, and "F1 Help".
- A dialog's labels and fields have the same visual weight. Read-only and editable fields look alike.

### 2.2 Goals

1. Make structure easy to see: group headings, section headings, and focus should stand out at a glance.
2. Make navigation faster, especially in databases with hundreds of accounts.
3. Keep it usable on monochrome terminals, `TERM=linux`, and under `NO_COLOR`.

### 2.3 Theme system (core of this feature)

Add a `Theme` with **semantic roles**, so code never uses raw colour pairs:

| Role | Default (colour terminal) | Monochrome fallback |
|---|---|---|
| `title` (window/app title) | bold, bright white on blue | bold + reverse |
| `heading` (group headers, dialog section headings) | bold, cyan | bold + underline |
| `label` (field labels) | default fg, dim | dim |
| `field` (editable input) | default fg on dark grey bg, underline | underline |
| `field_readonly` | default fg | normal |
| `field_focus` | black on yellow | reverse |
| `selection` (menu cursor) | black on cyan | reverse |
| `selection_inactive` | default fg on dark grey bg | dim + reverse |
| `secret` (passwords) | bold, green | bold |
| `muted` (hints, user name in list, counts) | dim | dim |
| `key` (command bar key) | bold, black on white | bold + reverse |
| `keydesc` (command bar description) | default on dark grey bg | reverse |
| `error` / `warning` / `success` | bold red / yellow / green | bold / bold / normal |
| `search_match` | bold, underline, yellow | bold + underline |

Implementation:
- `src/Theme.{h,cpp}`: `enum class Role`, `attr_t Theme::Attr(Role)`, and an RAII helper
  `ScopedAttr a(win, Role::heading);` that pairs `wattron` and `wattroff` automatically.
- Call `start_color()` and `use_default_colors()` in `InitTUI` when `has_colors()`. Use the monochrome
  column when the terminal has no colour, when `NO_COLOR` is set, or with `--no-color`.
- Define the colour pairs once, at startup and after `KEY_RESIZE`; some terminals reset them.
- Users can override styles in the `[theme]` section of the INI prefs, for example
  `heading = bold cyan` or `selection = black on cyan`. Ship built-in themes `default`, `light`
  (for light terminal backgrounds), and `mono`, selected with the pref `theme = default`.
- Use colour for groupings and highlights in the existing windows:
  - **Accounts window:** a heading row for each group (see U-2), `selection` for the cursor, `muted`
    user names, `title` for the box title.
  - **Dialogs:** `title` for the dialog title bar; `heading` for section dividers, for example
    Account / Credentials / Notes in `AccountDetailsDlg`; `label`, `field` and `field_focus` for form
    rows; `secret` for password fields.
  - **Command bar:** `key` for the key and `keydesc` for the description, so each key/description pair
    reads as one unit (similar to nano and htop).
  - **Message boxes:** an `error`/`warning`/`success` icon or prefix and a coloured border.

### 2.4 Suggested usability improvements

Ordered from most to least useful for the effort.

**U-2. Group the accounts list by `FT_GROUP`.** Replace the flat two-column list with one column that
shows group heading rows (styled with `heading`, followed by the item count) and the accounts indented
beneath. Headings can be collapsed with `←`/`→` or `Space`, and the collapsed state is saved in prefs.
Accounts with no group go under "(no group)". If a menu item can't carry the heading role, use a custom
list widget; ncurses `menu` doesn't style individual items well. Nested groups (`a.b.c`, the
Password Safe convention) show as a tree.

**U-3. Detail pane.** On wide terminals (≥ 120 columns), show the selected account's details (group,
title, user, URL, email, notes preview; password masked) in a read-only pane on the right, so you don't
have to open a dialog just to look. It updates as the cursor moves.

**U-4. Richer list rows.** Show `title` plus the `muted` user name on each row, which covers the backlog
item "Display user name in account list if account title not unique". Show the URL host when there is
room. Optionally mark accounts linked to a template that has `max_age_days` set.

**U-5. Command bar overhaul.** This resolves the backlog item "Improve command bar".
- Key/description pairs use the `key` and `keydesc` styles.
- When commands don't fit, show `F1 More`, which opens the help overlay (U-6).
- Show a status section on the right: database name, `[RO]` when read-only, `*` when modified,
  the position in the list (`42/310`), and the active search filter.

**U-6. F1 help overlay.** A scrollable panel listing every key for the current context, built from the
`Action` lists that each window already registers with `CommandBarWin`, so it needs no extra upkeep.
This resolves the backlog item "F1 Help".

**U-7. Better search.** Filter as you type (fuzzy match on title, user, group, and URL), highlight the
matches with `search_match`, `Esc` to clear, and a match count in the status section. `n`/`N` or
`^L` move to the next or previous match (keeping the existing `^L` convention).

**U-8. Keyboard navigation polish.**
- Vim keys in lists (`j`/`k`, `g`/`G`) in addition to the arrow keys and Home/End/PgUp/PgDn.
- Type-to-jump: typing a letter in the list moves to the next title starting with it.
- `Tab` and `Shift-Tab` move between dialog fields; make sure `Enter` on the password field doesn't
  submit (an existing bug in `TODO.md`).
- Show quick-copy keys in the list: `u` copies the user name, `p` the password, `l` the URL.

**U-9. Password reveal and clipboard feedback.** Mask the password in dialogs by default (`••••`) and
toggle it with `^R`. After copying, show a temporary message ("Password copied, clears in 30s") with a
countdown, which works with the clipboard-timeout idea in §1.3.

**U-10. Unsaved-changes and read-only indicators.** Show `*` in the title bar when there are unsaved
changes, and `[read-only]` in a warning colour. When the database is read-only, dim the editing commands
in the command bar instead of hiding them, so you can see that they exist.

**U-11. Confirmations and messages.** Use one consistent `MessageBox` style with a default button
(shown in `field_focus`), `y`/`n` shortcuts, and `Esc` to cancel. Show errors from the libpwsafe
`ResultCode` as readable text.

**U-12. Mouse support (optional).** `mousemask()` for clicking list rows, scroll-wheel scrolling,
and clicking command bar entries. Off by default; enabled with the `mouse = true` pref.

**U-13. Minimum-size handling.** When the terminal is smaller than the minimum layout (for example,
60×15), show a centered message ("Terminal too small: 52×12, need 60×15") instead of a broken layout.
This works with the existing `ScopedResizeHandler` stack.

**U-14. Help on first run.** When there is no prefs file, show a short message pointing to `F1`.

### 2.5 Accessibility and compatibility

- Never use colour as the only signal. Every role keeps a bold, underline, reverse, or symbol cue (see
  the monochrome column).
- Test with 8 and 256 colours, `TERM=linux`, `TERM=screen`/tmux, `NO_COLOR=1`, and light and dark
  backgrounds.
- Box-drawing and symbols (`▸ ▾ • …`) need UTF-8. Use ASCII (`> v * ...`) when the locale isn't UTF-8,
  which also helps with the known "Crash if terminal doesn't support UTF" bug.

### 2.6 Code structure

| New or changed | Responsibility |
|---|---|
| `src/Theme.{h,cpp}` | Roles, colour pairs, prefs parsing, `ScopedAttr`. |
| `PWSafeApp` | Owns `Theme`; enables colour; status data for the command bar. |
| `AccountsWin` | Group headings, collapsing, richer rows, type-to-jump; probably a custom list widget instead of `menu`. |
| `DetailPaneWin` (new) | Read-only detail view (U-3). |
| `HelpWin` (new) | F1 overlay built from `Action` lists. |
| `CommandBarWin` | Styled pairs, More/F1, status section. |
| `Dialog`, `MessageBox` | Use roles; sections; masked secrets; consistent buttons. |
| `Prefs` | `theme`, `[theme]` overrides, `mouse`, collapsed groups. |

### 2.7 Testing

- `Theme`: parsing styles from prefs (valid and invalid), the monochrome fallback, and `NO_COLOR`.
- Grouping logic (sorting, nested groups, collapsed state) as a pure function separate from ncurses,
  so it can be unit-tested.
- Manual checklist for each milestone: the terminal matrix in §2.5, resizing in every window,
  read-only mode, and an empty database.

### 2.8 Milestones

| # | Milestone | Depends on |
|---|---|---|
| U-1 | `Theme` and roles, colour enabled, existing windows switched to roles (no layout changes). | — |
| U-5 | Command bar overhaul. | U-1 |
| U-6 | F1 help overlay. | U-5 |
| U-2 / U-4 | Grouped accounts list with richer rows. | U-1 |
| U-7 / U-8 | Search and navigation polish. | U-2 |
| U-9 / U-10 / U-11 | Dialog polish: masking, indicators, message boxes. | U-1 |
| U-3 | Detail pane. | U-2 |
| U-12 / U-13 / U-14 | Mouse, minimum-size handling, first-run help. | — |

---

## 3. Open questions

- **Q1. Several databases.** Should account links be keyed by database path, or is a flat UUID map
  enough? UUIDs are unique across databases in practice, so a flat map works unless databases are
  copied (copies share UUIDs).
- **Q2. Template links in the database.** If a later libpwsafe version supports the V3 format, its
  per-record password-policy field (`0x10`) and named policies (header `0x10`) could store links inside
  the database. Should the JSON format be designed so it can migrate to that?
- **Q3. Replace `menu` in `AccountsWin`?** Group headings and per-row styling are much easier with a
  custom list widget. This is the biggest risk in the UI work, so prototype it during U-2.
- **Q4. Config path mismatch.** `ProgArgs.cpp` defaults to `ncpwsafe-config.ini`, but CMake's
  `NCPWSAFE_CONFIG_FILE` (and `AGENTS.md`) say `ncpwsafe.ini`. Settle this before adding a second
  config file.

## 4. Suggested order

1. **P1** (RNG fix): small, and it fixes a security issue.
2. **U-1** (theme): every later UI change builds on it.
3. **P2 → P4** (templates and CLI): delivers the CLI half of the feature without UI work.
4. **U-5, U-6, U-2/U-4**: the most visible navigation gains.
5. **P5 → P7**: the template UI, which by then can use the theme.
6. The remaining U items.

`TODO.md` should be updated to point at this plan, and to remove items it covers: "Diceware passwords",
"Fix password policies", "Save password policy to account entry", "Colors", "Improve command bar",
"F1 Help", and "Display user name in account list if account title not unique".
