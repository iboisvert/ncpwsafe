"""TUI: moving the selection in the accounts list, and search."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()

# The accounts list for fixtures/small.tsv, a heading for each group:
#   ▾ (no group)  ────  2
#      Router         admin
#      Wiki           wikiuser
#   ▾ Banking  ────  2
#      Chequing       alice
#      Savings        alice
#   ▾ Email  ────  2
#      Personal mail  alice@example.com
#      Work mail      asmith
#   ▾ Shopping  ────  1
#      Bookstore      bob


class NavigationTest(e2elib.TuiTestCase):

    def assertPath(self, app, steps):
        """Send each key and check what is selected after it."""
        for key, expected in steps:
            app.send(key)
            app.wait_highlighted(expected)
            self.assertEqual(app.highlighted(), [expected], f"after {key}")

    def test_first_account_selected(self):
        app, _, _ = self.open_db()
        self.assertEqual(app.highlighted(), ["Router"])

    def test_down_and_up(self):
        app, _, _ = self.open_db()
        # Down and Up step through headings and accounts in order
        self.assertPath(app, [
            ("Down", "Wiki"), ("Down", "Banking"), ("Down", "Chequing"), ("Down", "Savings"),
            ("Down", "Email"), ("Down", "Personal mail"), ("Down", "Work mail"), ("Down", "Shopping"),
            ("Down", "Bookstore"),
            ("Up", "Shopping"), ("Up", "Work mail"), ("Up", "Personal mail"), ("Up", "Email"),
            ("Up", "Savings"), ("Up", "Chequing"), ("Up", "Banking"), ("Up", "Wiki"), ("Up", "Router"),
            ("Up", "(no group)"),
        ])

    def test_collapse_and_expand(self):
        app, _, _ = self.open_db()
        # Left on an account goes to its heading, Left on a heading collapses the group
        self.assertPath(app, [("Down", "Wiki"), ("Down", "Banking"), ("Down", "Chequing"), ("Left", "Banking")])
        app.send("Left")
        app.wait_for("▸ Banking")
        self.assertNotIn("Chequing", app.text())
        self.assertEqual(app.highlighted(), ["Banking"])
        # Down skips the accounts of a collapsed group
        self.assertPath(app, [("Down", "Email"), ("Up", "Banking")])
        # Right on a collapsed heading expands the group, then goes to its first account
        app.send("Right")
        app.wait_for("▾ Banking")
        self.assertIn("Chequing", app.text())
        self.assertEqual(app.highlighted(), ["Banking"])
        self.assertPath(app, [("Right", "Chequing")])

    def test_enter_and_space_toggle_heading(self):
        app, _, _ = self.open_db()
        self.assertPath(app, [("Down", "Wiki"), ("Down", "Banking")])
        app.send("Enter")
        app.wait_for("▸ Banking")
        app.send("Space")
        app.wait_for("▾ Banking")
        self.assertNotIn("View Account", app.text())

    def test_stays_at_ends(self):
        app, _, _ = self.open_db()
        self.assertPath(app, [("Up", "(no group)"), ("Up", "(no group)"), ("End", "Bookstore"),
                              ("Down", "Bookstore")])

    def test_home_and_end(self):
        app, _, _ = self.open_db()
        self.assertPath(app, [("End", "Bookstore"), ("Home", "(no group)")])

    def test_page_down_and_up(self):
        app, _, _ = self.open_db()
        # Everything fits on one page, so a page goes to the ends
        self.assertPath(app, [("NPage", "Bookstore"), ("PPage", "(no group)")])

    def test_scrolls_to_selection(self):
        # Only the first rows fit in a short terminal
        app, _, _ = self.open_db(rows=8)
        self.assertNotIn("Bookstore", app.text())
        self.assertPath(app, [("End", "Bookstore")])
        self.assertNotIn("Router", app.text())
        self.assertPath(app, [("Home", "(no group)")])
        self.assertIn("Router", app.text())

    def test_sparse_fields(self):
        # An account without a user name has nothing in the user column
        app, _, _ = self.open_db(spec="sparse")
        self.assertRegex(app.text(), r"x ▾ Banking  q+  2 x\nx    No user +x\nx    Savings +alice +x")
        self.assertPath(app, [("Down", "Savings"), ("Down", "Savings")])

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
        app.send("Down", "Down", "Down")
        app.wait_highlighted("Chequing")
        app.send("/")
        app.wait_for("Search:")
        app.type("book")
        app.wait_highlighted("Bookstore")
        app.send("Escape")
        app.wait_gone("Search:")
        # Back to where the search started
        app.wait_highlighted("Chequing")

    def test_search_cancel_from_group_heading(self):
        # The saved position used to be an account record, which a heading
        # doesn't have, so Esc went to the first account instead
        app, _, _ = self.open_db()
        app.send("Down", "Down")
        app.wait_highlighted("Banking")
        app.send("/")
        app.wait_for("Search:")
        app.type("book")
        app.wait_highlighted("Bookstore")
        app.send("Escape")
        app.wait_gone("Search:")
        app.wait_highlighted("Banking")
        self.assertEqual(app.highlighted(), ["Banking"])

    def test_search_cleared_from_group_heading(self):
        # Deleting the whole query goes back to where the search started
        app, _, _ = self.open_db()
        app.send("Down", "Down")
        app.wait_highlighted("Banking")
        app.send("/")
        app.wait_for("Search:")
        app.type("book")
        app.wait_highlighted("Bookstore")
        app.send(*["BSpace"] * len("book"))
        app.wait_highlighted("Banking")

    def test_search_from_group_heading_finds_first_account(self):
        # A search started on a heading used to start after the first account,
        # and never wrapped around to it
        app, _, _ = self.open_db()
        app.send("End", "Up")
        app.wait_highlighted("Shopping")
        app.send("/")
        app.wait_for("Search:")
        app.type("router")
        app.wait_highlighted("Router")

    def test_search_expands_collapsed_group(self):
        app, _, _ = self.open_db()
        app.send("End", "Left", "Left")
        app.wait_for("▸ Shopping")
        app.send("/")
        app.wait_for("Search:")
        app.type("book")
        app.wait_highlighted("Bookstore")
        self.assertIn("▾ Shopping", app.text())

    def test_search_no_match(self):
        app, _, _ = self.open_db()
        app.send("/")
        app.wait_for("Search:")
        app.type("zzzz")
        app.send("Enter")
        app.wait_gone("Search:")
        self.assertEqual(app.highlighted(), ["Router"])

    def test_search_mixed_case(self):
        # The query used to be compared without lower-casing it, so a query
        # with a capital never matched
        app, _, _ = self.open_db()
        app.send("/")
        app.wait_for("Search:")
        app.type("Book")
        app.wait_highlighted("Bookstore", timeout=2)


if __name__ == "__main__":
    e2elib.main()
