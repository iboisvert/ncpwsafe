"""TUI: adding, editing and deleting accounts, saving and discarding changes."""
import hashlib
import re
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
        app.type(title)
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
        self.assertRegex(screen, r"x ▾ Travel  q+  1 x\nx    Airline +carol +x")
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
        # The fields are in sections
        self.assertRegex(screen, r"x Account q+ x[^\n]*\n[^\n]*x Group:")
        self.assertRegex(screen, r"x Credentials q+ x[^\n]*\n[^\n]*x User:")
        self.assertRegex(screen, r"x Notes q+ x[^\n]*\n[^\n]*x Notes:")
        # The password is hidden until ^V, as 8 bullets whatever its length
        self.assertNotIn("pw-work-4", screen)
        self.assertRegex(screen, r"Password: +•{8} ")
        app.send("C-v")
        app.wait_for("pw-work-4")
        self.assertNotIn("••", app.text())
        app.send("C-v")
        app.wait_gone("pw-work-4")
        app.wait_for("••••••••")

    def test_empty_password_not_masked(self):
        app, _, _ = self.open_db(spec="sparse")
        app.wait_highlighted("No user")
        app.send("Enter")
        screen = app.wait_for("View Account")
        self.assertRegex(screen, r"Password: +x")
        self.assertNotIn("•", screen)

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
        self.assertRegex(screen, r"x ▾ Banking  q+  3 x\nx    Bookstore +bob +x\nx    Chequing +alice +x\nx    Savings +alice +x")
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
        self.assertRegex(screen, r"x ▾ Banking  q+  2 x\nx    Savings +alice +x\nx    Term deposit +alice +x")
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

    def test_title_marks_unsaved_changes(self):
        app, _, _ = self.open_db()
        self.assertNotIn("*", app.screen()[0])
        self.select(app, "Chequing")
        app.send("C-d")
        app.wait_for("Delete account Chequing")
        app.send("y")
        app.wait_gone("Delete account")
        app.wait_until(lambda: "small.dat * " in app.screen()[0], "the title to mark unsaved changes")

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
        app.send("Down", "Down")
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

    def test_close_unedited_account(self):
        # Line breaks, tabs and trailing spaces don't survive a one-row form field,
        # but a field that wasn't edited isn't a change: closing doesn't ask
        app, db, pw = self.open_db(spec="roundtrip")
        for _, title, *_ in e2elib.read_spec("roundtrip"):
            with self.subTest(title=title):
                self.select(app, title)
                app.send("Enter")
                app.wait_for("View Account")
                app.send("C-x")
                app.wait_until(lambda: "has changed" in app.text() or "View Account" not in app.text(),
                               "the dialog to close")
                asked = "has changed" in app.text()
                if asked:
                    # Discard, so the next accounts can still be checked
                    app.send("y")
                    app.wait_gone("View Account")
                self.assertFalse(asked, "asked to discard changes")
        # Nothing changed, ^X exits without asking
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)
        self.assertDbContains(db, pw, e2elib.read_spec("roundtrip"))

    def test_edit_keeps_unedited_fields(self):
        # Saving an edit to the title keeps the other fields as they were stored
        app, db, pw = self.open_db(spec="roundtrip")
        expected = []
        for group, title, user, password, notes in e2elib.read_spec("roundtrip"):
            self.select(app, title)
            app.send("Enter")
            app.wait_for("View Account")
            self.fill_details(app, title=title + "2")
            app.send("C-s")
            app.wait_gone("View Account")
            expected.append((group, title + "2", user, password, notes))
        self.save_and_exit(app)
        self.assertDbContains(db, pw, expected)

    def open_notes(self, app, title):
        """Open the account `title` and move to the first row of its notes."""
        self.select(app, title)
        app.send("Enter")
        app.wait_for("View Account")
        app.send(*["Down"] * FIELDS.index("Notes"))

    def notes_rows(self, app):
        """The text in the rows of the notes field, up to a gap of two spaces."""
        screen = app.screen()
        first = next(i for i, line in enumerate(screen) if "Notes:  " in line)
        start = screen[first].index("Notes:") + len("Notes:    ")
        return [re.split(r" {2,}", line[start:])[0].rstrip() for line in screen[first:first + 3]]

    def test_notes_show_three_rows(self):
        app, _, _ = self.open_db(spec="roundtrip")
        self.select(app, "Long")
        app.send("Enter")
        app.wait_for("View Account")
        self.assertEqual(self.notes_rows(app), ["one", "two", "three"])

    def test_notes_wrap_long_lines(self):
        # 24 wide characters take 48 columns: 20 on the first row of 40 columns
        app, _, _ = self.open_db(spec="roundtrip")
        self.select(app, "Wide")
        app.send("Enter")
        app.wait_for("View Account")
        notes = "日本語" * 8
        self.assertEqual(self.notes_rows(app), [notes[:20], notes[20:], ""])

    def test_notes_scroll(self):
        # Down moves through the rows of the notes, which scroll to show the cursor
        app, _, _ = self.open_db(spec="roundtrip")
        self.open_notes(app, "Long")
        app.send("Down", "Down", "Down")
        app.wait_until(lambda: self.notes_rows(app) == ["two", "three", "four"], "the notes to scroll")
        app.send("Down")
        app.wait_until(lambda: self.notes_rows(app) == ["three", "four", "five"], "the notes to scroll")
        # Up moves back through them, then on to the field above
        app.send(*["Up"] * 4)
        app.wait_until(lambda: self.notes_rows(app) == ["one", "two", "three"], "the notes to scroll back")
        app.send("Up", "End")
        app.type("x")
        app.wait_for("Email:    x")

    def test_edit_notes(self):
        # Edits change only what is edited, line breaks keep the notes' own style
        app, db, pw = self.open_db(spec="roundtrip")
        # Add a line in the middle
        self.open_notes(app, "MultiLine")
        app.send("End", "Enter")
        app.type("middle")
        app.send("C-s")
        app.wait_gone("View Account")
        # Add to the second line of notes with Windows line breaks
        self.open_notes(app, "CrLf")
        app.send("Down", "End")
        app.type("!")
        app.send("C-s")
        app.wait_gone("View Account")
        # A first line break in one-line notes is the one Password Safe uses
        self.open_notes(app, "Plain")
        app.send("End", "Enter")
        app.type("Two")
        app.send("C-s")
        app.wait_gone("View Account")
        # Backspace at the start of a line joins it to the line before
        self.open_notes(app, "BlankFirst")
        app.send("Down", "BSpace")
        app.send("C-s")
        app.wait_gone("View Account")
        self.save_and_exit(app)
        notes = {"MultiLine": "line one\nmiddle\nline two", "CrLf": "line one\r\nline two!",
                 "Plain": "One line\r\nTwo", "BlankFirst": "after a blank line"}
        expected = [row[:4] + (notes.get(row[1], row[4]),) for row in e2elib.read_spec("roundtrip")]
        self.assertDbContains(db, pw, expected)

if __name__ == "__main__":
    e2elib.main()
