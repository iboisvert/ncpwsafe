"""CLI: --change-password.

The new password is always typed at a prompt on the terminal, twice. The current
password is read from --password-file, or prompted for too.
"""
import hashlib
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

NEW_PASSWORD = "a brand new passphrase"
PROMPT = "Account database password: "
NEW_PROMPT = "New account database password: "
CONFIRM_PROMPT = "Confirm new password: "


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ChangePasswordTest(e2elib.E2ETestCase):
    """The tests that fail before the new password is prompted for, which run without a terminal"""

    def test_wrong_password(self):
        db, _ = self.make_db()
        before = digest(db)
        proc = self.run_ncpwsafe("--change-password", "--password-file", self.password_file("not the password"), db)
        self.assertEqual(proc.returncode, e2elib.PRC_ERR_FAIL)
        # Found before prompting for the new password, which would fail without a terminal
        self.assertIn("Incorrect account database password", proc.stderr)
        self.assertNotIn("No terminal", proc.stderr)
        self.assertNotIn("password changed", proc.stdout)
        self.assertEqual(digest(db), before)
        self.assertEqual(self.backups_of(db), [])

    def test_new_password_needs_terminal(self):
        db, pw = self.make_db()
        before = digest(db)
        proc = self.run_ncpwsafe("--change-password", "--password-file", pw, db)
        self.assertEqual(proc.returncode, e2elib.PRC_ERR_FAIL)
        # --password-file is for the current password only
        self.assertIn("No terminal to prompt for the password on\n", proc.stderr)
        self.assertEqual(digest(db), before)

    def test_database_missing(self):
        _, pw = self.make_db()
        missing = self.tmp / "missing.dat"
        proc = self.run_ncpwsafe("--change-password", "--password-file", pw, missing)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(f"Account database file {missing} does not exist", proc.stderr)


@unittest.skipUnless(e2elib.HAS_PTY, "needs a pseudo-terminal")
class ChangePasswordPromptTest(e2elib.E2ETestCase):

    def change(self, db, pw, new_password=NEW_PASSWORD, confirm=None):
        """Change the password of `db`, the current one read from the file `pw`. Returns the TerminalApp."""
        app = self.start_on_terminal("--change-password", "--password-file", pw, db)
        app.answer(NEW_PROMPT, new_password)
        app.answer(CONFIRM_PROMPT, new_password if confirm is None else confirm)
        app.wait()
        return app

    def test_change_password(self):
        db, pw = self.make_db()
        app = self.change(db, pw)
        self.assertEqual(app.returncode, 0, app.output)
        self.assertIn("Account database password changed", app.output)
        # The current password comes from the file
        self.assertNotIn(PROMPT, app.output)
        # The new password isn't echoed
        self.assertNotIn(NEW_PASSWORD, app.output)
        # Contents are intact under the new password
        self.assertDbContains(db, self.password_file(NEW_PASSWORD), e2elib.read_spec())
        # and the old password no longer works
        old = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", "old.csv", db)
        self.assertNotEqual(old.returncode, 0)

    def test_prompt_for_current_password(self):
        db, _ = self.make_db()
        app = self.start_on_terminal("--change-password", db)
        app.answer(PROMPT, e2elib.SMALL_PASSWORD)
        app.answer(NEW_PROMPT, NEW_PASSWORD)
        app.answer(CONFIRM_PROMPT, NEW_PASSWORD)
        self.assertEqual(app.wait(), 0, app.output)
        self.assertNotIn(e2elib.SMALL_PASSWORD, app.output)
        self.assertTrue(app.echo_enabled())
        self.assertDbContains(db, self.password_file(NEW_PASSWORD), e2elib.read_spec())

    def test_wrong_password_at_prompt(self):
        db, _ = self.make_db()
        before = digest(db)
        app = self.start_on_terminal("--change-password", db)
        app.answer(PROMPT, "not the password")
        self.assertEqual(app.wait(), e2elib.PRC_ERR_FAIL)
        self.assertIn("Incorrect account database password", app.output)
        # Not asked for a new password that couldn't be used
        self.assertNotIn(NEW_PROMPT, app.output)
        self.assertEqual(digest(db), before)

    def test_change_password_twice(self):
        db, pw = self.make_db()
        self.assertEqual(self.change(db, pw).returncode, 0)
        new_pw = self.password_file(NEW_PASSWORD)
        self.assertEqual(self.change(db, new_pw, e2elib.SMALL_PASSWORD).returncode, 0)
        self.assertDbContains(db, pw, e2elib.read_spec())

    def test_non_ascii_password(self):
        db, pw = self.make_db()
        new_password = "пароль façade 密码"
        self.assertEqual(self.change(db, pw, new_password).returncode, 0)
        # The file holds the same UTF-8 bytes as the terminal sent
        self.assertDbContains(db, self.password_file(new_password), e2elib.read_spec())

    def test_backup_created(self):
        db, pw = self.make_db()
        before = db.read_bytes()
        self.change(db, pw)
        backups = self.backups_of(db)
        self.assertEqual(len(backups), 1, backups)
        # The backup is the database as it was before the change
        self.assertEqual(backups[0].read_bytes(), before)
        self.assertDbContains(backups[0], pw, e2elib.read_spec())

    def test_passwords_do_not_match(self):
        db, pw = self.make_db()
        before = digest(db)
        app = self.change(db, pw, confirm=NEW_PASSWORD + "x")
        self.assertEqual(app.returncode, e2elib.PRC_ERR_FAIL)
        self.assertIn("The passwords do not match", app.output)
        self.assertNotIn("password changed", app.output)
        self.assertEqual(digest(db), before)
        self.assertEqual(self.backups_of(db), [])

    def test_new_password_required(self):
        db, pw = self.make_db()
        before = digest(db)
        app = self.change(db, pw, "")
        self.assertEqual(app.returncode, e2elib.PRC_ERR_FAIL)
        self.assertIn("New account database password is required", app.output)
        self.assertEqual(digest(db), before)

    def test_end_of_input_at_new_password(self):
        db, pw = self.make_db()
        before = digest(db)
        app = self.start_on_terminal("--change-password", "--password-file", pw, db)
        app.expect(NEW_PROMPT)
        app.send("\x04")  # Ctrl-D
        self.assertEqual(app.wait(), e2elib.PRC_ERR_FAIL)
        self.assertIn("No password was entered", app.output)
        self.assertNotIn(CONFIRM_PROMPT, app.output)
        self.assertEqual(digest(db), before)


if __name__ == "__main__":
    e2elib.main()
