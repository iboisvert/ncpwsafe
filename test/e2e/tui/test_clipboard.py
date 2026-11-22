"""TUI: copying the user name and password to the clipboard.

xclip is replaced by a stub (NCPWSAFE_XCLIP) that writes what it is given
to a file, see e2elib.TuiTestCase.clipboard_stub().
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()


class ClipboardTest(e2elib.TuiTestCase):

    def clipboard(self):
        return self.clip_file.read_text() if self.clip_file.exists() else None

    def wait_clipboard(self, app, expected):
        app.wait_until(lambda: self.clipboard() == expected, f"clipboard to contain {expected!r}")

    def select_chequing(self, app):
        app.send("Down", "Down", "Down")
        app.wait_highlighted("Chequing")

    def test_copy_user(self):
        app, _, _ = self.open_db()
        self.select_chequing(app)
        app.send("C-u")
        self.wait_clipboard(app, "alice")

    def test_copy_password(self):
        app, _, _ = self.open_db()
        self.select_chequing(app)
        app.send("C-p")
        self.wait_clipboard(app, "pw-cheq-1")
        # The password is never shown
        self.assertNotIn("pw-cheq-1", app.text())

    def test_copy_on_group_does_nothing(self):
        app, _, _ = self.open_db()
        app.send("Down", "Down")
        app.wait_highlighted("Banking")
        app.send("C-u", "C-p", "Right")
        app.wait_highlighted("Chequing")
        self.assertIsNone(self.clipboard())

    def test_copy_missing_user_and_password(self):
        # An account without a user or password used to abort, constructing a
        # std::string from the missing (null) field
        app, _, _ = self.open_db(spec="sparse")
        app.wait_highlighted("No user")
        app.send("C-u")
        self.wait_clipboard(app, "")
        self.clip_file.unlink()
        app.send("C-p")
        self.wait_clipboard(app, "")

    def test_copy_in_details_dialog(self):
        app, _, _ = self.open_db()
        self.select_chequing(app)
        app.send("Enter")
        app.wait_for("View Account")
        app.send("C-p")
        self.wait_clipboard(app, "pw-cheq-1")
        app.send("C-u")
        self.wait_clipboard(app, "alice")

    def test_copy_unsaved_edit(self):
        # The dialog copies what is in the fields, not the saved account
        app, _, _ = self.open_db()
        self.select_chequing(app)
        app.send("Enter")
        app.wait_for("View Account")
        app.send("Down", "Down", "End")
        app.type("-edited")
        app.send("C-u")
        self.wait_clipboard(app, "alice-edited")

    def test_display_not_set(self):
        del self.env["DISPLAY"]
        app, _, _ = self.open_db()
        self.select_chequing(app)
        app.send("C-u")
        app.wait_for("The DISPLAY environment variable is not set.")
        app.send("Enter")
        app.wait_gone("DISPLAY environment")
        self.assertIn("Enter View", app.screen()[-1])
        self.assertIsNone(self.clipboard())

    def test_xclip_fails(self):
        self.env["NCPWSAFE_XCLIP"] = str(self.clipboard_stub(exit_code=1))
        app, _, _ = self.open_db()
        self.select_chequing(app)
        app.send("C-p")
        app.wait_for("An error occurred copying data to the clipboard")
        app.send("Enter")
        app.wait_gone("An error occurred")
        self.assertIn("Enter View", app.screen()[-1])

    def test_xclip_missing(self):
        self.env["NCPWSAFE_XCLIP"] = str(self.tmp / "no-such-xclip")
        app, _, _ = self.open_db()
        self.select_chequing(app)
        app.send("C-u")
        app.wait_for("An error occurred copying data to the clipboard")


if __name__ == "__main__":
    e2elib.main()
