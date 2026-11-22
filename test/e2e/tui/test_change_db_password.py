"""TUI: changing the account database password (^C in the accounts list)."""
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()

TITLE = "Change Account Database Password"
NEW_PASSWORD = "a brand new passphrase"
# The command bar while the dialog is open
DIALOG_KEYS = ("^S Save and close", "^X Cancel")
UNSAVED = "Changing the password will save them"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ChangeDbPasswordTest(e2elib.TuiTestCase):

    def assertDialogCommandBar(self, app):
        app.wait_until(lambda: all(k in app.screen()[-1] for k in DIALOG_KEYS), "the dialog's command bar")
        self.assertNotIn("Enter View", app.screen()[-1])

    def fill(self, app, current, new, confirm):
        app.send("C-c")
        app.wait_for(TITLE)
        self.fill_open(app, current, new, confirm)

    def fill_open(self, app, current, new, confirm):
        """Fill in the open dialog and press ^S."""
        app.type(current)
        app.send("Down")
        app.type(new)
        app.send("Down")
        app.type(confirm)
        # Passwords are not echoed
        screen = app.text()
        for pw in (current, new, confirm):
            if pw:
                self.assertNotIn(pw, screen)
        app.send("C-s")

    def select(self, app, title):
        app.send("/")
        app.wait_for("Search:")
        app.type(title)
        app.send("Enter")
        app.wait_gone("Search:")
        app.wait_highlighted(title)

    def make_unsaved_changes(self, app):
        """Edit Savings' user, delete Chequing and add Airline, without saving.

        Returns the rows the database should then have.
        """
        self.select(app, "Savings")
        app.send("Enter")
        app.wait_for("View Account")
        app.send("Down", "Down", "End", *["BSpace"] * 20)  # User
        app.type("alice2")
        app.send("C-s")
        app.wait_gone("View Account")
        self.select(app, "Chequing")
        app.send("C-d")
        app.wait_for("Delete account Chequing")
        app.send("y")
        app.wait_gone("Delete account")
        app.send("C-a")
        app.wait_for("View Account")
        app.type("Travel")
        app.send("Down")
        app.type("Airline")
        app.send("C-s")
        app.wait_gone("View Account")
        app.wait_for("Airline")
        self.assertIn("small.dat * ", app.screen()[0])
        rows = [r for r in e2elib.read_spec() if r[1] not in ("Chequing", "Savings")]
        return rows + [("Banking", "Savings", "alice2", "pw-sav-2", ""), ("Travel", "Airline", "", "", "")]

    def test_change_password_writes_unsaved_changes(self):
        # Changing the password used to read the file again, which undid the
        # unsaved edits and brought back deleted accounts, then wrote the mix
        app, db, pw = self.open_db()
        expected = self.make_unsaved_changes(app)
        app.send("C-c")
        app.wait_for(UNSAVED)
        self.assertIn("Y Yes", app.screen()[-1])
        app.send("y")
        app.wait_gone(UNSAVED)
        app.wait_for(TITLE)
        self.fill_open(app, e2elib.SMALL_PASSWORD, NEW_PASSWORD, NEW_PASSWORD)
        app.wait_gone(TITLE)
        # Written: no longer marked as unsaved, and the list is unchanged
        app.wait_until(lambda: "small.dat * " not in app.screen()[0], "the changes to be saved")
        screen = app.text()
        self.assertNotIn("Chequing", screen)
        self.assertRegex(screen, r"Savings +alice2")
        self.assertRegex(screen, r"▾ Travel .*\n.*Airline")
        self.assertDbContains(db, self.password_file(NEW_PASSWORD), expected)
        # Nothing left to discard
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)
        self.assertNotIn("Discard changes", app.text())

    def test_unsaved_changes_no(self):
        app, db, pw = self.open_db()
        before = digest(db)
        self.make_unsaved_changes(app)
        app.send("C-c")
        app.wait_for(UNSAVED)
        app.send("n")
        app.wait_gone(UNSAVED)
        self.assertNotIn(TITLE, app.text())
        self.assertIn("Enter View", app.screen()[-1])
        self.assertEqual(digest(db), before)
        # The changes are still there, unsaved
        self.assertIn("small.dat * ", app.screen()[0])
        self.assertIn("Airline", app.text())

    def test_no_warning_without_changes(self):
        app, _, _ = self.open_db()
        app.send("C-c")
        screen = app.wait_for(TITLE)
        self.assertNotIn(UNSAVED, screen)

    def test_change_password(self):
        app, db, pw = self.open_db()
        before = digest(db)
        self.fill(app, e2elib.SMALL_PASSWORD, NEW_PASSWORD, NEW_PASSWORD)
        app.wait_gone(TITLE)
        app.wait_until(lambda: digest(db) != before, "the database to be written")
        self.assertIn("Enter View", app.screen()[-1])
        # The database is written straight away, with a backup
        self.assertDbContains(db, self.password_file(NEW_PASSWORD), e2elib.read_spec())
        self.assertEqual(len(self.backups_of(db)), 1)
        app.send("C-x")
        app.wait_exit()
        self.assertNotEqual(self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", "x.csv", db).returncode, 0)

    def test_wrong_current_password(self):
        app, db, _ = self.open_db()
        before = digest(db)
        self.fill(app, "not the password", NEW_PASSWORD, NEW_PASSWORD)
        app.wait_for("Account database password is incorrect")
        app.send("Enter")
        app.wait_gone("password is incorrect")
        # Still in the dialog, with its command bar back
        self.assertIn(TITLE, app.text())
        self.assertDialogCommandBar(app)
        app.send("C-x")
        app.wait_gone(TITLE)
        self.assertEqual(digest(db), before)

    def test_passwords_do_not_match(self):
        app, db, _ = self.open_db()
        before = digest(db)
        self.fill(app, e2elib.SMALL_PASSWORD, NEW_PASSWORD, NEW_PASSWORD + "x")
        app.wait_for("New passwords do not match")
        app.send("Enter")
        app.wait_gone("do not match")
        self.assertDialogCommandBar(app)
        app.send("C-x")
        app.wait_gone(TITLE)
        self.assertEqual(digest(db), before)

    def test_empty_new_password_confirm_no(self):
        app, db, _ = self.open_db()
        before = digest(db)
        self.fill(app, e2elib.SMALL_PASSWORD, "", "")
        app.wait_for("Password is empty. Are you sure?")
        app.send("n")
        app.wait_gone("Password is empty")
        self.assertIn(TITLE, app.text())
        self.assertDialogCommandBar(app)
        app.send("C-x")
        app.wait_gone(TITLE)
        self.assertEqual(digest(db), before)

    def test_cancel(self):
        app, db, _ = self.open_db()
        before = digest(db)
        app.send("C-c")
        app.wait_for(TITLE)
        self.assertDialogCommandBar(app)
        app.send("C-x")
        app.wait_gone(TITLE)
        self.assertEqual(digest(db), before)
        # The accounts list's command bar is back
        app.wait_until(lambda: "Enter View" in app.screen()[-1], "the accounts list's command bar")


if __name__ == "__main__":
    e2elib.main()
