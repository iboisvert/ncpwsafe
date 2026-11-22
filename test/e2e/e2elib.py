"""Shared helpers for the ncpwsafe end-to-end tests.

The tests are run by CTest (see test/e2e/CMakeLists.txt), which sets these
environment variables:

    NCPWSAFE      path of the ncpwsafe executable under test
    E2E_MKDB      path of the e2e-mkdb fixture builder
    E2E_FIXTURES  directory holding the fixture specs (*.tsv) and password files
    TMUX_BIN      path of tmux, empty if it isn't installed (TUI tests only)

Every test runs in its own temporary directory, which is also $HOME (and,
on Windows, %USERPROFILE% and %APPDATA%), so the preferences file and
database backups never touch the real home directory.

The CLI tests run on Linux and Windows. The TUI tests need tmux, so they
only run on Linux, and are skipped on Windows.
"""

import csv
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

E2E_DIR = Path(__file__).resolve().parent
IS_WINDOWS = sys.platform == "win32"
SKIP_EXIT_CODE = 77  # Matches SKIP_RETURN_CODE in CMakeLists.txt
PRC_ERR_FAIL = 1  # ncpwsafe exit code for failure, or for exiting the UI without saving


def _env_path(name, default=None):
    value = os.environ.get(name)
    if value:
        return Path(value)
    if default is not None:
        return default
    sys.stderr.write(f"Environment variable {name} is not set, run the tests with ctest\n")
    sys.exit(2)


NCPWSAFE = _env_path("NCPWSAFE")
E2E_MKDB = _env_path("E2E_MKDB")
FIXTURES = _env_path("E2E_FIXTURES", E2E_DIR / "fixtures")

# Fixture: password file and account spec for the "small" database
SMALL_PASSWORD = (FIXTURES / "small.pw").read_text(encoding="utf-8").splitlines()[0]


def read_spec(name="small"):
    """Read a fixture spec as a list of (group, title, user, password, notes) tuples."""
    rows = []
    for line in (FIXTURES / f"{name}.tsv").read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        fields += [""] * (5 - len(fields))
        rows.append(tuple(fields[:5]))
    return rows


def sorted_rows(rows):
    return sorted(rows, key=lambda r: (r[0], r[1], r[2]))


def ncpwsafe_has_test_db():
    """`--test-db` is only compiled into Debug builds."""
    out = subprocess.run([str(NCPWSAFE), "--help"], capture_output=True, text=True,
                         encoding="utf-8", errors="replace",
                         env=os.environ.copy(), check=False).stdout
    return "--test-db " in out


IS_DEBUG_BUILD = ncpwsafe_has_test_db()


def expected_failure_if(condition):
    """`unittest.expectedFailure` when `condition` is true, e.g. a bug only
    detected by Debug build assertions."""
    return unittest.expectedFailure if condition else (lambda func: func)


class E2ETestCase(unittest.TestCase):
    """Base class: a private temp dir and $HOME per test, plus helpers."""

    maxDiff = None

    def setUp(self):
        # On Linux under /tmp rather than $TMPDIR, so paths shown on screen stay short.
        # resolve() expands Windows 8.3 short names (C:\Users\RUNNER~1\...)
        tmp_root = "/tmp" if not IS_WINDOWS and Path("/tmp").is_dir() else None
        self.tmp = Path(tempfile.mkdtemp(prefix="ncpwsafe-e2e-", dir=tmp_root)).resolve()
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.home = self.tmp / "home"
        self.home.mkdir()
        self.env = os.environ.copy()
        self.env["HOME"] = str(self.home)
        # Never reach a real clipboard
        self.env.pop("NCPWSAFE_XCLIP", None)
        if IS_WINDOWS:
            # ncpwsafe keeps its preferences under %APPDATA% on Windows
            appdata = self.home / "AppData" / "Roaming"
            appdata.mkdir(parents=True)
            self.env["USERPROFILE"] = str(self.home)
            self.env["APPDATA"] = str(appdata)
            self.env["LOCALAPPDATA"] = str(self.home / "AppData" / "Local")
            self.config_file = appdata / "ncpwsafe" / "ncpwsafe-config.ini"
        else:
            self.config_file = self.home / ".config" / "ncpwsafe" / "ncpwsafe-config.ini"

    # Files --------------------------------------------------------------

    def write_file(self, name, content):
        """Write a text file in the temp dir, with the platform's line endings."""
        path = self.tmp / name
        path.write_text(content, encoding="utf-8")
        return path

    def password_file(self, password, name=None):
        return self.write_file(name or f"pw-{abs(hash(password))}.txt", password + "\n")

    def make_db(self, spec="small", password=SMALL_PASSWORD, name=None):
        """Build a database from fixtures/<spec>.tsv, returns (db_path, password_file)."""
        db = self.tmp / (name or f"{spec}.dat")
        pw = self.password_file(password, name=f"{db.stem}.pw")
        subprocess.run([str(E2E_MKDB), str(db), str(pw), str(FIXTURES / f"{spec}.tsv")],
                       check=True, env=self.env)
        return db, pw

    def backups_of(self, db):
        """Backup files ncpwsafe made of `db`: <stem>-YYYYMMDD_HHMMSS<suffix>."""
        return sorted(db.parent.glob(f"{db.stem}-????????_??????{db.suffix}"))

    # Running ncpwsafe ---------------------------------------------------

    def run_ncpwsafe(self, *args, timeout=30):
        return subprocess.run([str(NCPWSAFE), *map(str, args)], capture_output=True, text=True,
                              encoding="utf-8", errors="replace",
                              env=self.env, cwd=self.tmp, timeout=timeout,
                              stdin=subprocess.DEVNULL, check=False)

    def assertNotCrashed(self, proc):
        """ncpwsafe exited by itself rather than crashing: killed by a signal
        on Linux, or an NTSTATUS exception code or abort() (3) on Windows."""
        if IS_WINDOWS:
            self.assertFalse(proc.returncode >= 0xC0000000 or proc.returncode == 3,
                             f"ncpwsafe crashed with exit code {proc.returncode:#x}")
        else:
            self.assertGreaterEqual(proc.returncode, 0, "ncpwsafe was killed by a signal")

    def export(self, db, pwfile):
        """Export `db` and return its rows without UUIDs, as (group, title, user, password, notes)."""
        out = self.tmp / "export.csv"
        proc = self.run_ncpwsafe("--export-db", "--force", "--password-file", pwfile, "-o", out, db)
        self.assertEqual(proc.returncode, 0, f"export failed:\n{proc.stdout}{proc.stderr}")
        rows = parse_export(out.read_text(encoding="utf-8"))
        out.unlink()
        return rows

    def assertDbContains(self, db, pwfile, expected_rows):
        self.assertEqual(sorted_rows(self.export(db, pwfile)), sorted_rows(expected_rows))


def parse_export(text):
    """Parse --export-db output, check the header and drop the UUID column."""
    reader = csv.reader(text.splitlines())
    header = next(reader)
    assert header == ["UUID", "Group", "Title", "User", "Password", "Notes"], header
    return [tuple(row[1:6]) for row in reader]


def main():
    """Run the tests in __main__; exit 77 (skipped) if every test was skipped."""
    program = unittest.main(exit=False, verbosity=2)
    result = program.result
    if not result.wasSuccessful():
        sys.exit(1)
    if result.testsRun > 0 and len(result.skipped) == result.testsRun:
        sys.exit(SKIP_EXIT_CODE)
    sys.exit(0)


class TuiTestCase(E2ETestCase):
    """Base class for TUI tests: runs ncpwsafe in a private tmux server.

    Importing a module that uses this class exits 77 (skipped) when tmux
    isn't available, see require_tmux().
    """

    COLS, ROWS = 100, 30

    def setUp(self):
        super().setUp()
        self.env["DISPLAY"] = ":e2e"  # Copy to clipboard gives up without DISPLAY
        self.clip_file = self.tmp / "clipboard.txt"
        self.env["NCPWSAFE_XCLIP"] = str(self.clipboard_stub())
        self.app = None

    def tearDown(self):
        if self.app is not None:
            self.app.close()
        super().tearDown()

    def clipboard_stub(self, exit_code=0):
        """An xclip stand-in that saves what it is given in self.clip_file.

        ncpwsafe runs xclip with only DISPLAY and XAUTHORITY set, so the
        output path is written into the script rather than passed in.
        """
        stub = self.tmp / f"xclip-stub-{exit_code}"
        if exit_code == 0:
            body = f"exec /bin/cat > '{self.clip_file}'\n"
        else:
            body = f"/bin/cat > /dev/null\nexit {exit_code}\n"
        stub.write_text("#!/bin/sh\n" + body)
        stub.chmod(0o755)
        return stub

    def start(self, *args, cols=None, rows=None):
        from tmuxdriver import TmuxApp
        self.app = TmuxApp(os.environ["TMUX_BIN"], self.env, self.tmp, self.tmp / "exit-code",
                           cols=cols or self.COLS, rows=rows or self.ROWS)
        self.app.start([NCPWSAFE, *args])
        return self.app

    def open_db(self, *args, spec="small", cols=None, rows=None):
        """Build a database and open it with --password-file, wait for the accounts list."""
        db, pw = self.make_db(spec)
        app = self.start("--password-file", pw, *args, db, cols=cols, rows=rows)
        app.wait_for("Enter View")
        return app, db, pw


def require_tmux():
    """Exit 77 (skipped) if tmux isn't available."""
    if IS_WINDOWS:
        # tmux can't host a native Windows console program
        print("The TUI tests need tmux and don't run on Windows yet, skipping")
        sys.exit(SKIP_EXIT_CODE)
    if not os.environ.get("TMUX_BIN") or not Path(os.environ["TMUX_BIN"]).exists():
        print("tmux is not installed, skipping TUI tests")
        sys.exit(SKIP_EXIT_CODE)
