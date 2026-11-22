"""TUI: read-only mode (-r)."""
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ReadOnlyTest(e2elib.TuiTestCase):

    def test_command_bar(self):
        app, _, _ = self.open_db("-r")
        bar = app.screen()[-1]
        for action in ("Enter View", "^U Copy user", "^P Copy password", "^X Exit"):
            self.assertIn(action, bar)
        for action in ("^A Add", "^D Delete", "^S Save", "^C Change password"):
            self.assertNotIn(action, bar)

    def test_title_shows_read_only(self):
        app, _, _ = self.open_db("-r")
        self.assertIn("small.dat [read-only]", app.screen()[0])

    def test_editing_keys_do_nothing(self):
        app, db, pw = self.open_db("-r")
        before = digest(db)
        app.send("Down")  # Wiki
        app.send("Down")  # Banking
        app.send("Down")  # Chequing
        app.wait_highlighted("Chequing")
        for key in ("C-a", "C-d", "C-c", "C-s"):
            app.send(key)
        # A key the app handles, to know the ones before it were processed
        app.send("Down")
        app.wait_highlighted("Savings")
        screen = app.text()
        for dialog in ("View Account", "Delete account", "Change Account Database Password"):
            self.assertNotIn(dialog, screen)
        self.assertIsNone(app.exit_code(), "^S must not save and exit")
        # No changes and read-only, ^X exits without asking
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)
        self.assertEqual(digest(db), before)
        self.assertEqual(self.backups_of(db), [])

    def test_details_are_read_only(self):
        app, db, _ = self.open_db("-r")
        before = digest(db)
        app.send("Down", "Down", "Down")
        app.wait_highlighted("Chequing")
        app.send("Enter")
        screen = app.wait_for("View Account")
        self.assertRegex(screen, r"User: +alice")
        bar = app.screen()[-1]
        self.assertIn("^X Exit", bar)
        self.assertNotIn("^S", bar)
        self.assertNotIn("^C", bar)
        app.type("zzz")
        app.send("BSpace", "C-s")
        self.assertNotIn("zzz", app.text())
        self.assertIn("View Account", app.text(), "^S must not close a read-only dialog")
        # ^V still shows the password
        app.send("C-v")
        app.wait_for("pw-cheq-1")
        app.send("C-x")
        app.wait_gone("View Account")
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)
        self.assertEqual(digest(db), before)


if __name__ == "__main__":
    e2elib.main()
