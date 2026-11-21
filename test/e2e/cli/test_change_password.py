"""CLI: --change-password."""
import hashlib
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

NEW_PASSWORD = "a brand new passphrase"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ChangePasswordTest(e2elib.E2ETestCase):

    def change(self, db, pw, new_pw):
        return self.run_ncpwsafe("--change-password", "--password-file", pw, "--new-password-file", new_pw, db)

    def test_change_password(self):
        db, pw = self.make_db()
        new_pw = self.password_file(NEW_PASSWORD)
        proc = self.change(db, pw, new_pw)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("Account database password changed", proc.stdout)
        # Contents are intact under the new password
        self.assertDbContains(db, new_pw, e2elib.read_spec())
        # and the old password no longer works
        old = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", "old.csv", db)
        self.assertNotEqual(old.returncode, 0)

    def test_change_password_twice(self):
        db, pw = self.make_db()
        new_pw = self.password_file(NEW_PASSWORD)
        self.change(db, pw, new_pw)
        self.change(db, new_pw, pw)
        self.assertDbContains(db, pw, e2elib.read_spec())

    def test_new_password_option(self):
        db, pw = self.make_db()
        proc = self.run_ncpwsafe("--change-password", "--password-file", pw, f"--new-password={NEW_PASSWORD}", db)
        self.assertIn("Account database password changed", proc.stdout)
        self.assertDbContains(db, self.password_file(NEW_PASSWORD), e2elib.read_spec())

    def test_backup_created(self):
        db, pw = self.make_db()
        before = db.read_bytes()
        self.change(db, pw, self.password_file(NEW_PASSWORD))
        backups = self.backups_of(db)
        self.assertEqual(len(backups), 1, backups)
        # The backup is the database as it was before the change
        self.assertEqual(backups[0].read_bytes(), before)
        self.assertDbContains(backups[0], pw, e2elib.read_spec())

    def test_wrong_password(self):
        db, _ = self.make_db()
        before = digest(db)
        proc = self.change(db, self.password_file("not the password"), self.password_file(NEW_PASSWORD))
        self.assertIn("Incorrect account database password", proc.stdout)
        self.assertEqual(digest(db), before)

    @unittest.expectedFailure
    def test_wrong_password_exit_code(self):
        # BUG: main() never sets the exit code for --change-password, so it
        # exits 0 even when the password is not changed
        db, _ = self.make_db()
        proc = self.change(db, self.password_file("not the password"), self.password_file(NEW_PASSWORD))
        self.assertNotEqual(proc.returncode, 0)

    def test_new_password_required(self):
        db, pw = self.make_db()
        proc = self.run_ncpwsafe("--change-password", "--password-file", pw, db)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("New account database password is required", proc.stderr)

    def test_database_missing(self):
        _, pw = self.make_db()
        missing = self.tmp / "missing.dat"
        proc = self.change(missing, pw, self.password_file(NEW_PASSWORD))
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(f"Account database file {missing} does not exist", proc.stderr)


if __name__ == "__main__":
    e2elib.main()
