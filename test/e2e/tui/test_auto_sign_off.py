"""TUI: auto sign-off quits after a period without input, discarding unsaved changes."""
import sys
import time
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()

EXIT_AUTO_SIGN_OFF = 124


class AutoSignOffTest(e2elib.TuiTestCase):

    def set_auto_sign_off(self, value):
        self.config_file.parent.mkdir(parents=True, exist_ok=True)
        self.config_file.write_text(f"auto-sign-off = {value}\n")

    def assertSignedOff(self, app, period="2 seconds", discarded=False):
        screen = app.wait_for(f"ncpwsafe: signed off after {period} without input")
        if discarded:
            self.assertIn("Unsaved changes were discarded.", screen)
        else:
            self.assertNotIn("Unsaved changes", screen)
        self.assertEqual(app.wait_exit(), EXIT_AUTO_SIGN_OFF)
        # The screen was cleared: no account is left on it
        self.assertNotIn("Chequing", app.text())

    def test_signs_off_in_accounts_list(self):
        self.set_auto_sign_off("2s")
        app, _, _ = self.open_db()
        self.assertSignedOff(app)

    def test_unsaved_changes_discarded(self):
        self.set_auto_sign_off("2s")
        app, db, pw = self.open_db()
        # Delete the selected account, which isn't saved
        app.send("C-d")
        app.wait_for("Delete account Router")
        app.send("y")
        app.wait_gone("Delete account Router")
        app.wait_for(r"small\.dat \*", regex=True)
        self.assertSignedOff(app, discarded=True)
        # The file still has every account
        self.assertDbContains(db, pw, e2elib.read_spec())

    def test_signs_off_at_unlock_prompt(self):
        self.set_auto_sign_off("2s")
        db, _ = self.make_db()
        app = self.start(db)
        app.wait_for("Unlock Database")
        app.type("half a passw")
        self.assertSignedOff(app)

    def test_signs_off_in_dialog(self):
        self.set_auto_sign_off("2s")
        app, _, _ = self.open_db()
        app.send("Enter")
        app.wait_for("View Account")
        self.assertSignedOff(app)

    def test_signs_off_in_message_box(self):
        self.set_auto_sign_off("2s")
        app, _, _ = self.open_db()
        app.send("C-d")
        app.wait_for("Delete account Router")
        self.assertSignedOff(app)

    def test_signs_off_in_search_bar(self):
        self.set_auto_sign_off("2s")
        app, _, _ = self.open_db()
        app.send("/")
        app.wait_for("Search:")
        self.assertSignedOff(app)

    def test_signs_off_in_help(self):
        self.set_auto_sign_off("2s")
        app, _, _ = self.open_db()
        app.send("C-g")
        app.wait_for(" Keys ")
        self.assertSignedOff(app)

    def test_keys_keep_it_open(self):
        self.set_auto_sign_off("3s")
        app, _, _ = self.open_db()
        # The time without input is what's being tested, so this test waits on the clock.
        # A key every second for twice the period keeps ncpwsafe open
        for key in ["Down", "Up"] * 3:
            app.send(key)
            time.sleep(1)
            self.assertIsNone(app.exit_code(), "signed off although keys were pressed")
        self.assertSignedOff(app, period="3 seconds")

    def test_zero_turns_it_off(self):
        self.set_auto_sign_off("0")
        app, _, _ = self.open_db()
        # Waits on the clock: still open well after a period that would have signed off
        time.sleep(3)
        self.assertIsNone(app.exit_code())
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)

    def test_number_without_unit_is_minutes(self):
        # 1 minute: still open after a few seconds
        self.set_auto_sign_off("1")
        app, _, _ = self.open_db()
        time.sleep(3)
        self.assertIsNone(app.exit_code())
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)


if __name__ == "__main__":
    e2elib.main()
