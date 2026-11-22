"""TUI: account databases with non-ASCII text.

ncpwsafe used to run ncurses in the C locale and link the narrow character
libraries, so new_item() rejected every group or title with non-ASCII text,
and the menu's item array ended at the first one: those accounts and every
account after them were missing, and an empty menu crashed. Account text is
UTF-8, so ncpwsafe now uses a UTF-8 character set (LC_CTYPE) even when the
user's locale isn't UTF-8, e.g. LANG=C.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()

# The accounts list for fixtures/unicode.tsv. The first account is
# non-ASCII, and ASCII accounts come after non-ASCII ones:
#   ▾ (no group)  ────  2
#      Café                 josé
#      Router               admin
#   ▾ Банк  ────  2
#      Wiki                 wikiuser
#      Сберегательный счёт  алиса
ACCOUNTS = ["Café", "Router", "Банк", "Wiki", "Сберегательный счёт"]


class UnicodeTest(e2elib.TuiTestCase):

    def set_locale(self, locale):
        self.env["LANG"] = locale
        self.env["LC_ALL"] = locale

    def assertAccountsShown(self, app):
        screen = app.text()
        self.assertRegex(screen, r"x ▾ \(no group\)  q+  2 x\nx    Café +josé +x\nx    Router +admin +x")
        self.assertRegex(screen, r"x ▾ Банк  q+  2 x\nx    Wiki +wikiuser +x\nx    Сберегательный счёт +алиса +x")
        self.assertEqual(app.highlighted(), ["Café"])

    def test_accounts_shown_utf8_locale(self):
        app, _, _ = self.open_db(spec="unicode")
        self.assertAccountsShown(app)

    def test_accounts_shown_c_locale(self):
        self.set_locale("C")
        app, _, _ = self.open_db(spec="unicode")
        self.assertAccountsShown(app)

    def test_view_account_c_locale(self):
        # With an empty menu, Enter used to fail an assertion in Debug builds,
        # and Release builds opened an account that wasn't in the list
        self.set_locale("C")
        app, _, _ = self.open_db(spec="unicode")
        app.send("End")
        app.wait_highlighted("Сберегательный счёт")
        app.send("Enter")
        screen = app.wait_for("View Account")
        self.assertRegex(screen, r"Group: +Банк")
        self.assertRegex(screen, r"Title: +Сберегательный счёт")
        self.assertRegex(screen, r"Notes: +Заметка")
        # Nothing was changed, so there's nothing to discard
        app.send("C-x")
        app.wait_gone("View Account")
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)

    def test_edit_keeps_text_c_locale(self):
        # Without a UTF-8 character set the form fields can't hold UTF-8 text,
        # and saving the account would change it
        self.set_locale("C")
        app, db, pw = self.open_db(spec="unicode")
        app.send("End")
        app.wait_highlighted("Сберегательный счёт")
        app.send("Enter")
        app.wait_for("View Account")
        app.send("Down", "Down", "End")  # User
        app.type("2")
        app.wait_for("алиса2")
        app.send("C-s")
        app.wait_gone("View Account")
        app.send("C-s")
        self.assertEqual(app.wait_exit(), 0)
        expected = [r for r in e2elib.read_spec("unicode") if r[2] != "алиса"]
        expected.append(("Банк", "Сберегательный счёт", "алиса2", "пароль-1", "Заметка"))
        self.assertDbContains(db, pw, expected)

    def test_search_and_delete_prompt(self):
        # Search is case insensitive for non-ASCII text too
        app, _, _ = self.open_db(spec="unicode")
        app.send("/")
        app.wait_for("Search:")
        app.type("СЧЁТ")
        app.wait_highlighted("Сберегательный счёт")
        app.send("Enter")
        app.wait_gone("Search:")
        app.send("C-d")
        app.wait_for("Delete account Сберегательный счёт?")
        app.send("n")
        app.wait_gone("Delete account")

    def test_title_with_control_character(self):
        # ncurses used to refuse to create a menu item it can't print. The account
        # is listed with the character replaced. No account has a group, so there are no headings
        app, _, _ = self.open_db(spec="control-chars")
        self.assertRegex(app.text(), r"x Bell\? +user +x\nx Router +admin +x")
        self.assertEqual(app.highlighted(), ["Bell?"])
        app.send("Down")
        app.wait_highlighted("Router")

    def test_type_non_ascii_c_locale(self):
        # Keys used to be read and given to the form a byte at a time, so a
        # multibyte character was dropped or entered as escaped bytes
        self.set_locale("C")
        app, db, pw = self.open_db()
        app.send("Enter")
        app.wait_for("View Account")
        app.send("Down", "Down", "End")  # User
        app.type("é日本")
        app.wait_for("adminé日本")
        app.send("BSpace")
        app.wait_for(r"adminé日 +x", regex=True)
        app.send("C-s")
        app.wait_gone("View Account")
        app.send("End")
        app.wait_highlighted("Bookstore")
        app.send("/")
        app.wait_for("Search:")
        app.type("É日")
        app.wait_highlighted("Router")
        app.send("Enter")
        app.wait_gone("Search:")
        app.send("C-s")
        self.assertEqual(app.wait_exit(), 0)
        expected = [r for r in e2elib.read_spec() if r[1] != "Router"]
        expected.append(("", "Router", "adminé日", "pw-router-6", ""))
        self.assertDbContains(db, pw, expected)


if __name__ == "__main__":
    e2elib.main()
