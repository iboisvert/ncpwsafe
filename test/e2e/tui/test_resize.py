"""TUI: resizing the terminal relays out every window, including stacked dialogs."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()

SIZES = [(80, 24), (120, 40), (60, 20), (100, 30)]
# The account details dialog is about 70 columns wide
DIALOG_SIZES = [(80, 24), (120, 40), (76, 22), (100, 30)]


class ResizeTest(e2elib.TuiTestCase):

    def assertFrame(self, app):
        """The main window's box fills the terminal, with the command bar under it."""
        cols, rows = app.cols, app.rows
        screen = app.screen()
        self.assertEqual(len(screen[0].rstrip()), cols, screen[0])
        self.assertTrue(screen[0].startswith("lqq") and screen[0].rstrip().endswith("qqk"), screen[0])
        self.assertIn(f"ncpwsafe {e2elib.VERSION}", screen[0])
        bottom = screen[rows - 2]
        self.assertEqual(bottom.rstrip(), "m" + "q" * (cols - 2) + "j", "\n".join(screen))
        for line in screen[1:rows - 2]:
            self.assertTrue(line.startswith("x") and line.rstrip().endswith("x") and len(line.rstrip()) == cols,
                            "\n".join(screen))

    def resize(self, app, cols, rows):
        app.resize(cols, rows)
        app.wait_until(lambda: app.screen()[rows - 2].startswith("mqq")
                       and app.screen()[rows - 2].rstrip().endswith("qj")
                       and len(app.screen()[0].rstrip()) == cols,
                       f"layout at {cols}x{rows}")

    def test_resize_accounts_list(self):
        app, _, _ = self.open_db()
        app.send("Down", "Down", "Down")
        app.wait_highlighted("Chequing")
        for cols, rows in SIZES:
            with self.subTest(size=f"{cols}x{rows}"):
                self.resize(app, cols, rows)
                self.assertFrame(app)
                self.assertIn("Enter View", app.screen()[-1])
                self.assertRegex(app.text(), r"x    Chequing +alice +x\nx    Savings +alice +x")
                # The selection is kept
                self.assertEqual(app.highlighted(), ["Chequing"])
        # And the app still responds
        app.send("Down")
        app.wait_highlighted("Savings")

    def test_resize_with_dialog(self):
        app, _, _ = self.open_db()
        app.send("Down", "Down", "Down", "Enter")
        app.wait_for("View Account")
        for cols, rows in DIALOG_SIZES:
            with self.subTest(size=f"{cols}x{rows}"):
                self.resize(app, cols, rows)
                self.assertFrame(app)
                screen = app.wait_for("View Account")
                self.assertRegex(screen, r"User: +alice")
                # The dialog stays centred
                line = next(line for line in app.screen() if "View Account" in line)
                left = line.index("l", 1)
                right = len(line.rstrip()) - 1 - line.rstrip()[::-1].index("k", 1)
                self.assertLessEqual(abs(left - (cols - 1 - right)), 2, line)
                self.assertIn("^X Cancel", app.screen()[-1])
        # The dialog still takes input, and the list under it is intact
        app.send("C-x")
        app.wait_gone("View Account")
        self.assertRegex(app.text(), r"x    Chequing +alice +x\nx    Savings +alice +x")
        self.assertEqual(app.highlighted(), ["Chequing"])

    def test_resize_with_message_over_dialog(self):
        app, _, _ = self.open_db()
        app.send("C-a")
        app.wait_for("View Account")
        app.send("C-s")  # No title
        app.wait_for("Account title is required")
        for cols, rows in DIALOG_SIZES:
            with self.subTest(size=f"{cols}x{rows}"):
                self.resize(app, cols, rows)
                self.assertFrame(app)
                screen = app.text()
                self.assertIn("Account title is required", screen)
                self.assertIn("View Account", screen)
                self.assertIn("Enter Close", app.screen()[-1])
        app.send("Enter")
        app.wait_gone("Account title is required")
        self.assertIn("View Account", app.text())

    def test_dialog_wider_than_terminal(self):
        app, _, _ = self.open_db()
        app.send("Down", "Down", "Down", "Enter")
        app.wait_for("View Account")
        # Narrower than the dialog: it is clipped, but the app keeps going
        app.resize(50, 20)
        app.wait_until(lambda: len(app.screen()[0].rstrip()) == 50, "the 50 column layout")
        self.resize(app, 100, 30)
        self.assertFrame(app)
        self.assertRegex(app.wait_for("View Account"), r"User: +alice")
        app.send("C-x")
        app.wait_gone("View Account")
        self.assertEqual(app.highlighted(), ["Chequing"])

    def test_resize_with_search_bar(self):
        app, _, _ = self.open_db()
        app.send("/")
        app.wait_for("Search:")
        app.type("book")
        app.wait_highlighted("Bookstore")
        self.resize(app, 80, 24)
        self.assertIn("Search: book", app.screen()[-1])
        app.send("Enter")
        app.wait_gone("Search:")
        self.assertEqual(app.highlighted(), ["Bookstore"])

    def test_resize_unlock_prompt(self):
        db, _ = self.make_db()
        app = self.start(db)
        app.wait_for("Unlock Database")
        self.resize(app, 80, 24)
        self.assertFrame(app)
        self.assertIn("Unlock Database", app.text())
        app.type(e2elib.SMALL_PASSWORD)
        app.send("Enter")
        app.wait_for("Enter View")
        self.assertFrame(app)

    def test_too_small_then_restored(self):
        app, _, _ = self.open_db()
        app.resize(15, 4)  # Below the minimum size, windows are left truncated
        app.resize(90, 26)
        self.resize(app, 90, 26)
        self.assertFrame(app)
        self.assertRegex(app.text(), r"x    Chequing +alice +x\nx    Savings +alice +x")
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)


if __name__ == "__main__":
    e2elib.main()
