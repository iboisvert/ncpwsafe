"""TUI: adding, editing and deleting accounts, saving and discarding changes."""
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()

# Fields of the account details dialog that Up/Down move between. The
# password field is skipped (it isn't O_ACTIVE), it is set with ^C
FIELDS = ["Group", "Title", "User", "URL", "Email", "Notes"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def set_account_password(app, password):
    """In the account details dialog, set the password with ^C."""
    app.send("C-c")
    app.wait_for("Change Password")
    app.type(password)
    app.send("Down")
    app.type(password)
    app.send("C-s")
    app.wait_gone("Change Password")


class EditTest(e2elib.TuiTestCase):

    def fill_details(self, app, password=None, start="Group", **values):
        """In an open details dialog, replace field values.

        `start` is the field the cursor is on, Group when the dialog opens.
        """
        if password is not None:
            set_account_password(app, password)
        current = FIELDS.index(start)
        for name in FIELDS:
            key = name.lower()
            if key not in values:
                continue
            index = FIELDS.index(name)
            app.send(*["Down" if index > current else "Up"] * abs(index - current))
            current = index
            app.send("End", *["BSpace"] * 60)
            app.type(values[key])

    def select(self, app, title):
        """Move the selection to the account `title` (search for it)."""
        app.send("/")
        app.wait_for("Search:")
        # Lower case: search doesn't match queries with capitals, see
        # test_navigation.test_search_mixed_case
        app.type(title.lower())
        app.send("Enter")
        app.wait_gone("Search:")
        app.wait_highlighted(title)

    def save_and_exit(self, app):
        app.send("C-s")
        self.assertEqual(app.wait_exit(), 0)

    def test_add_account(self):
        app, db, pw = self.open_db()
        app.send("C-a")
        app.wait_for("View Account")
        self.fill_details(app, group="Travel", title="Airline", user="carol", password="pw-air-8",
                          notes="Frequent flyer")
        app.send("C-s")
        app.wait_gone("View Account")
        # The new account is listed, in its own group, and selected
        screen = app.wait_for("Travel")
        self.assertRegex(screen, r"x Travel +x\nx Airline +x")
        self.assertEqual(app.highlighted(), ["Airline"])
        self.save_and_exit(app)
        expected = e2elib.read_spec() + [("Travel", "Airline", "carol", "pw-air-8", "Frequent flyer")]
        self.assertDbContains(db, pw, expected)

    def test_add_account_requires_title(self):
        app, _, _ = self.open_db()
        app.send("C-a")
        app.wait_for("View Account")
        self.fill_details(app, user="nobody")
        app.send("C-s")
        app.wait_for("Account title is required")
        app.send("Enter")
        app.wait_gone("Account title is required")
        # Still in the dialog, the cursor is where it was
        self.fill_details(app, start="User", title="Named now")
        app.send("C-s")
        app.wait_gone("View Account")
        app.wait_for("Named now")

    def test_add_account_cancel(self):
        app, db, pw = self.open_db()
        app.send("C-a")
        app.wait_for("View Account")
        app.send("C-x")
        app.wait_gone("View Account")
        app.send("C-x")  # No changes, so no confirmation
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)
        self.assertDbContains(db, pw, e2elib.read_spec())

    def test_change_account_password(self):
        app, db, pw = self.open_db()
        self.select(app, "Savings")
        app.send("Enter")
        app.wait_for("View Account")
        set_account_password(app, "a-new-account-pw")
        app.send("C-v")
        app.wait_for("a-new-account-pw")
        app.send("C-s")
        app.wait_gone("View Account")
        self.save_and_exit(app)
        expected = [r for r in e2elib.read_spec() if r[1] != "Savings"]
        expected.append(("Banking", "Savings", "alice", "a-new-account-pw", ""))
        self.assertDbContains(db, pw, expected)

    def test_view_account(self):
        app, _, _ = self.open_db()
        self.select(app, "Work mail")
        app.send("Enter")
        screen = app.wait_for("View Account")
        self.assertRegex(screen, r"Group: +Email")
        self.assertRegex(screen, r"Title: +Work mail")
        self.assertRegex(screen, r"User: +asmith")
        self.assertRegex(screen, r"Notes: +VPN required")
        # The password is hidden until ^V
        self.assertNotIn("pw-work-4", screen)
        app.send("C-v")
        app.wait_for("pw-work-4")
        app.send("C-v")
        app.wait_gone("pw-work-4")

    def test_edit_account(self):
        app, db, pw = self.open_db()
        self.select(app, "Savings")
        app.send("Enter")
        app.wait_for("View Account")
        self.fill_details(app, user="alice2", notes="High interest")
        app.send("C-s")
        app.wait_gone("View Account")
        self.save_and_exit(app)
        expected = [r for r in e2elib.read_spec() if r[1] != "Savings"]
        expected.append(("Banking", "Savings", "alice2", "pw-sav-2", "High interest"))
        self.assertDbContains(db, pw, expected)

    def test_edit_account_moves_group(self):
        # AccountsWin::UpdateMenu() used to delete another account (here
        # "Work mail") when the edit moved the account in the sorted records
        app, db, pw = self.open_db()
        self.select(app, "Bookstore")
        app.send("Enter")
        app.wait_for("View Account")
        self.fill_details(app, group="Banking")
        app.send("C-s")
        app.wait_gone("View Account")
        # Shopping is now empty and goes away, Bookstore is listed under Banking
        screen = app.wait_gone("Shopping")
        self.assertRegex(screen, r"x Banking +x\nx Bookstore +Chequing +x\nx Savings +x")
        self.assertEqual(app.highlighted(), ["Bookstore"])
        self.save_and_exit(app)
        expected = [r for r in e2elib.read_spec() if r[1] != "Bookstore"]
        expected.append(("Banking", "Bookstore", "bob", "pw-book-5", ""))
        self.assertDbContains(db, pw, expected)

    def test_edit_account_title_moves(self):
        # Renamed, Chequing sorts after Savings. UpdateMenu() used to delete
        # Savings, which took its place in the sorted records
        app, db, pw = self.open_db()
        self.select(app, "Chequing")
        app.send("Enter")
        app.wait_for("View Account")
        self.fill_details(app, title="Term deposit")
        app.send("C-s")
        app.wait_gone("View Account")
        screen = app.wait_gone("Chequing")
        self.assertRegex(screen, r"x Banking +x\nx Savings +Term deposit +x")
        self.assertEqual(app.highlighted(), ["Term deposit"])
        self.save_and_exit(app)
        expected = [r for r in e2elib.read_spec() if r[1] != "Chequing"]
        expected.append(("Banking", "Term deposit", "alice", "pw-cheq-1", "Main account"))
        self.assertDbContains(db, pw, expected)

    def test_edit_account_discard(self):
        app, _, _ = self.open_db()
        self.select(app, "Savings")
        app.send("Enter")
        app.wait_for("View Account")
        self.fill_details(app, user="changed")
        app.send("C-x")
        app.wait_for("The account entry has changed. Discard changes?")
        app.send("n")
        app.wait_gone("Discard changes?")
        self.assertIn("changed", app.text())  # Still editing
        app.send("C-x")
        app.wait_for("Discard changes?")
        app.send("y")
        app.wait_gone("View Account")
        # Nothing changed, so exit doesn't ask
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)

    def test_delete_account(self):
        app, db, pw = self.open_db()
        self.select(app, "Chequing")
        app.send("C-d")
        app.wait_for("Delete account Chequing")
        app.send("y")
        app.wait_gone("Chequing")
        self.save_and_exit(app)
        self.assertDbContains(db, pw, [r for r in e2elib.read_spec() if r[1] != "Chequing"])

    def test_delete_account_no(self):
        app, db, pw = self.open_db()
        self.select(app, "Chequing")
        app.send("C-d")
        app.wait_for("Delete account Chequing")
        app.send("n")
        app.wait_gone("Delete account")
        self.assertIn("Chequing", app.text())
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)
        self.assertDbContains(db, pw, e2elib.read_spec())

    def test_delete_unique_title_omits_user(self):
        # The uniqueness check in AccountsWin::DeleteEntry() used to be
        # inverted, so the user was named unless every account had the same title
        app, _, _ = self.open_db()
        self.select(app, "Chequing")
        app.send("C-d")
        screen = app.wait_for("Delete account Chequing")
        self.assertIn("Delete account Chequing?", screen)

    def test_delete_on_group_heading_does_nothing(self):
        # A group heading has no account. DeleteEntry() used to fail an
        # assertion in Debug builds
        app, _, _ = self.open_db()
        app.send("Down")
        app.wait_highlighted("Banking")
        app.send("C-d", "Right")
        app.wait_highlighted("Chequing")
        self.assertNotIn("Delete account", app.text())

    def test_delete_same_title_names_user(self):
        app, _, _ = self.open_db()
        # Add a second "Savings" account for another user
        app.send("C-a")
        app.wait_for("View Account")
        self.fill_details(app, group="Banking", title="Savings", user="bob")
        app.send("C-s")
        app.wait_gone("View Account")
        app.send("C-d")
        app.wait_for("Delete account Savings with user bob?")

    def test_exit_discard_changes(self):
        app, db, _ = self.open_db()
        before = digest(db)
        self.select(app, "Chequing")
        app.send("C-d")
        app.wait_for("Delete account Chequing")
        app.send("y")
        app.wait_gone("Chequing")
        app.send("C-x")
        app.wait_for("The database has changed. Discard changes?")
        app.send("n")
        app.wait_gone("Discard changes?")
        self.assertIsNone(app.exit_code())
        app.send("C-x")
        app.wait_for("Discard changes?")
        app.send("y")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)
        self.assertEqual(digest(db), before)
        self.assertEqual(self.backups_of(db), [])

    def test_save_makes_backup(self):
        app, db, pw = self.open_db()
        before = db.read_bytes()
        self.select(app, "Chequing")
        app.send("C-d")
        app.wait_for("Delete account Chequing")
        app.send("y")
        app.wait_gone("Chequing")
        self.save_and_exit(app)
        backups = self.backups_of(db)
        self.assertEqual(len(backups), 1, backups)
        self.assertEqual(backups[0].read_bytes(), before)
        self.assertIn(f"backup.0 = {backups[0]}", self.config_file.read_text())

    def test_save_without_changes(self):
        app, db, _ = self.open_db()
        before = digest(db)
        self.save_and_exit(app)
        self.assertEqual(digest(db), before)
        self.assertEqual(self.backups_of(db), [])


if __name__ == "__main__":
    e2elib.main()
