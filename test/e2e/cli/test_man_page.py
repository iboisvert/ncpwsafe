"""The ncpwsafe(1) man page: it documents every option, preference and key, and is valid mdoc."""
import os
import re
import shutil
import subprocess
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

REPO = e2elib.E2E_DIR.parent.parent
MAN_SOURCE = REPO / "man" / "ncpwsafe.1.in"

# Options compiled into Debug builds only, which `--help` doesn't show in Release builds
DEBUG_ONLY_OPTIONS = {"test-db", "test-db-lang", "test-db-groups", "test-db-items"}


def section(text, name):
    """The lines of the mdoc section `.Sh name`, up to the next `.Sh`."""
    match = re.search(rf"^\.Sh {re.escape(name)}\n(.*?)(?=^\.Sh |\Z)", text, re.M | re.S)
    assert match, f"No section {name}"
    return match.group(1)


def described_options(man):
    """Options with an entry (`.It Fl ...`) in DESCRIPTION (the commands) or OPTIONS,
    as (long options, short options)."""
    items = "\n".join(re.findall(r"^\.It (.*Fl .*)$", section(man, "DESCRIPTION") + section(man, "OPTIONS"), re.M))
    return (set(re.findall(r"\bFl -([a-z][a-z-]*)", items)),
            set(re.findall(r"\bFl ([a-zA-Z])\b", items)))


def action_keys():
    """The keys of every `Action` (key, name, description) registered with the command bar."""
    keys = set()
    for source in (REPO / "src").glob("*.cpp"):
        text = source.read_text(encoding="utf-8")
        if "Action" not in text and "CommandBarWin" not in text:
            continue
        for key in re.findall(r'\{\s*(?:~?CBOPTS_READONLY,\s*)?"([^"]+)",\s*"[A-Z]', text):
            keys.update(k for k in key.split(",") if k)
    return keys


def documented_keys(man):
    """The keys listed with `.It Ic` in the KEYS section, single letters in lower case."""
    keys = set()
    for line in re.findall(r"^\.It Ic (.*)$", section(man, "KEYS"), re.M):
        for key in line.split(" , "):
            keys.add(key.strip().replace("\\&", ""))
    return keys


class ManPageTest(e2elib.E2ETestCase):

    def setUp(self):
        super().setUp()
        self.man = MAN_SOURCE.read_text(encoding="utf-8")

    def help_text(self):
        proc = self.run_ncpwsafe("--help")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        return proc.stdout

    def test_long_options_match_help(self):
        in_help = set(re.findall(r"(?<![\w-])--([a-z][a-z-]*)", self.help_text()))
        in_man = described_options(self.man)[0]
        self.assertTrue(in_help, "no options found in --help")
        self.assertEqual(in_help - in_man, set(), "options in --help but not in the man page")
        extra = in_man - in_help
        if e2elib.IS_DEBUG_BUILD:
            self.assertEqual(extra, set(), "options in the man page but not in --help")
        else:
            self.assertLessEqual(extra, DEBUG_ONLY_OPTIONS, "options in the man page but not in --help")

    def test_short_options_match_help(self):
        in_help = set(re.findall(r"(?<![\w-])-([a-zA-Z])\b", self.help_text()))
        in_man = described_options(self.man)[1]
        self.assertEqual(in_help, in_man)

    def test_preferences_documented(self):
        prefs_h = (REPO / "src" / "Prefs.h").read_text(encoding="utf-8")
        keys = set(re.findall(r'static constexpr const char \*\w+ = "([^"]+)";', prefs_h))
        self.assertTrue(keys, "no preference keys found in Prefs.h")
        documented = set(re.findall(r"^\.It Cm ([\w.-]+)", section(self.man, "PREFERENCES"), re.M))
        self.assertEqual(keys - documented, set(), "preferences missing from PREFERENCES")

    def test_command_bar_keys_documented(self):
        keys = action_keys()
        self.assertIn("^S", keys)
        documented = {k.lower() if len(k) == 1 else k for k in documented_keys(self.man)}
        missing = {k for k in keys if (k.lower() if len(k) == 1 else k) not in documented}
        self.assertEqual(missing, set(), "command bar keys missing from KEYS")

    def test_version_filled_in(self):
        page = os.environ.get("NCPWSAFE_MAN_PAGE")
        if not page:
            self.skipTest("NCPWSAFE_MAN_PAGE is not set, run the tests with ctest")
        text = Path(page).read_text(encoding="utf-8")
        self.assertNotIn("@NCPWSAFE", text)
        self.assertRegex(text, r"(?m)^\.Os ncpwsafe \d+\.\d+")

    @unittest.skipUnless(shutil.which("groff") or shutil.which("mandoc"), "needs groff or mandoc")
    def test_lint(self):
        page = Path(os.environ.get("NCPWSAFE_MAN_PAGE") or MAN_SOURCE)
        if shutil.which("mandoc"):
            cmd = ["mandoc", "-Tlint", "-Wwarning", str(page)]
        else:
            # -ww: all warnings, -z: no output
            cmd = ["groff", "-mdoc", "-ww", "-z", "-K", "utf8", str(page)]
        proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
        self.assertEqual((proc.returncode, proc.stdout + proc.stderr), (0, ""))


if __name__ == "__main__":
    e2elib.main()
