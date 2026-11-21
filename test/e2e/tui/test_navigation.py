"""TUI: moving the selection in the accounts list, and search."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()

# The accounts list for fixtures/small.tsv, two columns:
#   Router         Wiki
#
#   Banking
#   Chequing       Savings
#
#   Email
#   Personal mail  Work mail
#
#   Shopping
#   Bookstore


class NavigationTest(e2elib.TuiTestCase):

    def assertPath(self, app, steps):
        """Send each key and check what is selected after it."""
        for key, expected in steps:
            app.send(key)
            app.wait_highlighted(expected)
            self.assertEqual(app.highlighted(), [expected], f"after {key}")

    def test_first_item_selected(self):
        app, _, _ = self.open_db()
        self.assertEqual(app.highlighted(), ["Router"])

    def test_down_and_up(self):
        app, _, _ = self.open_db()
        self.assertPath(app, [
            ("Down", "Banking"), ("Down", "Chequing"), ("Down", "Email"), ("Down", "Personal mail"),
            ("Down", "Shopping"), ("Down", "Bookstore"),
            ("Up", "Shopping"), ("Up", "Personal mail"), ("Up", "Email"), ("Up", "Chequing"),
            ("Up", "Banking"), ("Up", "Router"),
        ])

    def test_left_and_right(self):
        app, _, _ = self.open_db()
        # Right and Left step through every item in order, skipping blanks
        self.assertPath(app, [
            ("Right", "Wiki"), ("Right", "Banking"), ("Right", "Chequing"), ("Right", "Savings"),
            ("Right", "Email"), ("Left", "Savings"), ("Left", "Chequing"), ("Left", "Banking"),
            ("Left", "Wiki"), ("Left", "Router"),
        ])

    def test_second_column(self):
        app, _, _ = self.open_db()
        self.assertPath(app, [("Right", "Wiki"), ("Down", "Banking"), ("Down", "Chequing"),
                              ("Right", "Savings"), ("Up", "Banking")])

    def test_stays_at_ends(self):
        app, _, _ = self.open_db()
        self.assertPath(app, [("Up", "Router"), ("Left", "Router"), ("End", "Bookstore"),
                              ("Down", "Bookstore"), ("Right", "Bookstore")])

    def test_home_and_end(self):
        app, _, _ = self.open_db()
        self.assertPath(app, [("End", "Bookstore"), ("Home", "Router")])

    def test_page_down_and_up(self):
        app, _, _ = self.open_db()
        # Everything fits on one page
        self.assertPath(app, [("NPage", "Router"), ("End", "Bookstore"), ("PPage", "Bookstore")])

    def test_search(self):
        app, _, _ = self.open_db()
        app.send("/")
        bar = app.wait_for("Search:").split("\n")[-1]
        for action in ("^L Next", "Enter Exit", "Esc Cancel"):
            self.assertIn(action, bar)
        app.type("book")
        app.wait_highlighted("Bookstore")
        app.send("Enter")
        app.wait_gone("Search:")
        self.assertEqual(app.highlighted(), ["Bookstore"])
        self.assertIn("Enter View", app.screen()[-1])

    def test_search_matches_user_and_notes(self):
        app, _, _ = self.open_db()
        for query, expected in (("asmith", "Work mail"), ("team wiki", "Wiki")):
            with self.subTest(query=query):
                app.send("Home", "/")
                app.wait_for("Search:")
                app.type(query)
                app.wait_highlighted(expected)
                app.send("Enter")
                app.wait_gone("Search:")

    def test_search_next(self):
        app, _, _ = self.open_db()
        app.send("/")
        app.wait_for("Search:")
        app.type("mail")
        app.wait_highlighted("Personal mail")
        app.send("C-l")
        app.wait_highlighted("Work mail")
        app.send("C-l")  # Wraps around
        app.wait_highlighted("Personal mail")

    def test_search_cancel(self):
        app, _, _ = self.open_db()
        app.send("Down", "Down")
        app.wait_highlighted("Chequing")
        app.send("/")
        app.wait_for("Search:")
        app.type("book")
        app.wait_highlighted("Bookstore")
        app.send("Escape")
        app.wait_gone("Search:")
        # Back to where the search started
        app.wait_highlighted("Chequing")

    def test_search_no_match(self):
        app, _, _ = self.open_db()
        app.send("/")
        app.wait_for("Search:")
        app.type("zzzz")
        app.send("Enter")
        app.wait_gone("Search:")
        self.assertEqual(app.highlighted(), ["Router"])

    @unittest.expectedFailure
    def test_search_mixed_case(self):
        # BUG: AccountRecord::FieldContainsCaseInsensitive() lower-cases the
        # field but not the query, so a query with a capital never matches
        app, _, _ = self.open_db()
        app.send("/")
        app.wait_for("Search:")
        app.type("Book")
        app.wait_highlighted("Bookstore", timeout=2)


if __name__ == "__main__":
    e2elib.main()
