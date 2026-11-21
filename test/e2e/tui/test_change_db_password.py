"""TUI: changing the account database password (^C in the accounts list)."""
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()

TITLE = "Change Account Database Password"
NEW_PASSWORD = "a brand new passphrase"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ChangeDbPasswordTest(e2elib.TuiTestCase):

    def fill(self, app, current, new, confirm):
        app.send("C-c")
        app.wait_for(TITLE)
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
        # Still in the dialog
        self.assertIn(TITLE, app.text())
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
        app.send("C-x")
        app.wait_gone(TITLE)
        self.assertEqual(digest(db), before)

    def test_cancel(self):
        app, db, _ = self.open_db()
        before = digest(db)
        app.send("C-c")
        app.wait_for(TITLE)
        app.send("C-x")
        app.wait_gone(TITLE)
        self.assertEqual(digest(db), before)


if __name__ == "__main__":
    e2elib.main()
