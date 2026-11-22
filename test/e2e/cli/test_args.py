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

    def test_help(self):
        proc = self.run_ncpwsafe("--help")
        self.assertUsage(proc)
        for cmd in ("--generate-password", "--export-db", "--change-password"):
            self.assertIn(cmd, proc.stdout)

    def test_help_short(self):
        self.assertUsage(self.run_ncpwsafe("-h"))

    def test_help_shows_config_file_under_home(self):
        proc = self.run_ncpwsafe("--help")
        # On Windows the default is %APPDATA% + "/ncpwsafe/...", with mixed separators
        self.assertIn(str(self.config_file), proc.stdout.replace("/", os.sep))

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

    def test_password_and_password_file(self):
        db, pw = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "--password=x", "--password-file", pw, "-o", "out.csv", db)
        self.assertUsage(proc)
        self.assertIn("Only one of --password and --password-file may be specified", proc.stderr)

    def test_new_password_and_new_password_file(self):
        db, pw = self.make_db()
        proc = self.run_ncpwsafe("--change-password", "--password-file", pw, "--new-password=x",
                                 "--new-password-file", pw, db)
        self.assertUsage(proc)
        self.assertIn("Only one of --new-password and --new-password-file may be specified", proc.stderr)

    def test_missing_password_file(self):
        db, _ = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "--password-file", self.tmp / "missing.pw", "-o", "out.csv", db)
        self.assertUsage(proc)
        self.assertIn("Unable to read password file", proc.stderr)
        self.assertIn("missing.pw", proc.stderr)

    def test_password_deprecation_warning(self):
        db, _ = self.make_db()
        proc = self.run_ncpwsafe("--export-db", f"--password={e2elib.SMALL_PASSWORD}", "-o", "out.csv", db)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("Warning: --password is deprecated", proc.stderr)

    def test_new_password_deprecation_warning(self):
        db, pw = self.make_db()
        proc = self.run_ncpwsafe("--change-password", "--password-file", pw, "--new-password=new-secret", db)
        self.assertIn("Warning: --new-password is deprecated", proc.stderr)

    def test_password_file_does_not_warn(self):
        db, pw = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", "out.csv", db)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertNotIn("deprecated", proc.stderr)


if __name__ == "__main__":
    e2elib.main()
