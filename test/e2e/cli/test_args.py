"""CLI: help, argument parsing and validation."""
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

PRC_ERR_FAIL = 1  # Exit code when arguments are invalid


class ArgsTest(e2elib.E2ETestCase):

    def assertUsage(self, proc):
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("Usage: ncpwsafe [COMMAND] [OPTIONS] [DATABASE_FILE]", proc.stdout)

    def assertHelp(self, proc):
        # Asking for help isn't an error
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertEqual(proc.stderr, "")
        self.assertIn("Usage: ncpwsafe [COMMAND] [OPTIONS] [DATABASE_FILE]", proc.stdout)

    def test_help(self):
        proc = self.run_ncpwsafe("--help")
        self.assertHelp(proc)
        for cmd in ("--generate-password", "--export-db", "--change-password"):
            self.assertIn(cmd, proc.stdout)

    def test_help_short(self):
        # -h used to be rejected as an invalid option, which also shows the usage
        self.assertHelp(self.run_ncpwsafe("-h"))

    def test_help_with_other_options(self):
        self.assertHelp(self.run_ncpwsafe("--read-only", "--help", "some.dat"))

    def test_help_shows_config_file_under_home(self):
        proc = self.run_ncpwsafe("--help")
        # On Windows the default is %APPDATA% + "/ncpwsafe/...", with mixed separators
        self.assertIn(str(self.config_file), proc.stdout.replace("/", os.sep))

    def test_version(self):
        for option in ("--version", "-V"):
            proc = self.run_ncpwsafe(option)
            self.assertEqual(proc.returncode, 0, proc.stderr)
            self.assertEqual(proc.stderr, "")
            self.assertEqual(proc.stdout, f"ncpwsafe {e2elib.VERSION}\n")
        self.assertRegex(e2elib.VERSION, r"^\d+\.\d+\.\d+$")

    def test_version_in_help(self):
        self.assertIn("-V,--version", self.run_ncpwsafe("--help").stdout)

    def test_unknown_option(self):
        proc = self.run_ncpwsafe("--no-such-option")
        self.assertUsage(proc)
        self.assertIn("no-such-option", proc.stderr)

    def test_two_commands(self):
        db, pw = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "--change-password", "--password-file", pw, "-o", "out.csv", db)
        self.assertUsage(proc)
        self.assertIn("Only one command may be specified", proc.stderr)

    def test_two_database_files(self):
        db, pw = self.make_db()
        self.assertUsage(self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", "out.csv", db, db))

    def test_option_not_valid_for_command(self):
        db, pw = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "--read-only", "--password-file", pw, "-o", "out.csv", db)
        self.assertUsage(proc)
        self.assertFalse((self.tmp / "out.csv").exists())

    def test_generate_option_not_valid_for_export(self):
        db, pw = self.make_db()
        self.assertUsage(self.run_ncpwsafe("--export-db", "--password-count=3", "--password-file", pw,
                                           "-o", "out.csv", db))

    def test_password_file_not_valid_for_generate(self):
        _, pw = self.make_db()
        self.assertUsage(self.run_ncpwsafe("--generate-password", "--password-file", pw))

    def test_password_options_removed(self):
        # Passwords on the command line can be seen by other users: they are read from
        # --password-file or prompted for
        db, pw = self.make_db()
        for args in (["-P", "x"], ["--password=x"], ["--new-password=x"], ["--new-password-file", pw]):
            with self.subTest(args=args):
                proc = self.run_ncpwsafe("--change-password", "--password-file", pw, *args, db)
                self.assertUsage(proc)
                self.assertIn("option", proc.stderr)
        self.assertDbContains(db, pw, e2elib.read_spec())

    def test_password_options_not_in_help(self):
        out = self.run_ncpwsafe("--help").stdout
        self.assertNotIn("-P,", out)
        self.assertNotIn("--password=", out)
        self.assertNotIn("--new-password", out)

    def test_missing_password_file(self):
        db, _ = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "--password-file", self.tmp / "missing.pw", "-o", "out.csv", db)
        self.assertEqual(proc.returncode, PRC_ERR_FAIL)
        # Not a usage error
        self.assertNotIn("Usage:", proc.stdout)
        self.assertIn("Unable to read password file", proc.stderr)
        self.assertIn("missing.pw", proc.stderr)
        self.assertFalse((self.tmp / "out.csv").exists())

    def test_invalid_args_before_password_file(self):
        # The arguments are checked before the password file is read
        proc = self.run_ncpwsafe("--export-db", "--password-file", self.tmp / "missing.pw", "-o", "out.csv",
                                 self.tmp / "missing.dat")
        self.assertUsage(proc)
        self.assertNotIn("Unable to read password file", proc.stderr)


if __name__ == "__main__":
    e2elib.main()
