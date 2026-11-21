"""TUI: opening an account database."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()


class OpenTest(e2elib.TuiTestCase):

    def assertAccountsShown(self, app):
        screen = app.text()
        self.assertIn("ncpwsafe 0.1", app.screen()[0])
        # Groups, then accounts two to a row, sorted by group and title
        self.assertRegex(screen, r"x Router +Wiki +x")
        self.assertRegex(screen, r"x Banking +x\nx Chequing +Savings +x")
        self.assertRegex(screen, r"x Email +x\nx Personal mail +Work mail +x")
        self.assertRegex(screen, r"x Shopping +x\nx Bookstore +x")

    def test_open_with_password_file(self):
        app, _, _ = self.open_db()
        self.assertAccountsShown(app)
        bar = app.screen()[-1]
        for action in ("Enter View", "^A Add", "^D Delete", "^U Copy user", "^P Copy password",
                       "^S Save and exit", "^X Exit"):
            self.assertIn(action, bar)
        self.assertEqual(app.highlighted(), ["Router"])
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)

    def test_prompt_for_password(self):
        db, _ = self.make_db()
        app = self.start(db)
        screen = app.wait_for("Unlock Database")
        self.assertIn(f"Database: {db}", screen)
        self.assertIn("^S Save and continue", app.screen()[-1])
        app.type(e2elib.SMALL_PASSWORD)
        # The password is not echoed
        self.assertNotIn(e2elib.SMALL_PASSWORD, app.text())
        app.send("Enter")
        app.wait_for("Enter View")
        self.assertAccountsShown(app)

    def test_wrong_password_at_prompt(self):
        db, _ = self.make_db()
        app = self.start(db)
        app.wait_for("Unlock Database")
        app.type("not the password")
        app.send("Enter")
        app.wait_for("Incorrect password")
        app.send("Enter")
        app.wait_gone("Incorrect password")
        # Back at the prompt with the wrong password still in the field
        app.send(*["BSpace"] * len("not the password"))
        app.type(e2elib.SMALL_PASSWORD)
        app.send("Enter")
        app.wait_for("Enter View")

    def test_wrong_password_file(self):
        db, _ = self.make_db()
        pw = self.password_file("not the password")
        app = self.start("--password-file", pw, db)
        app.wait_for("The password given on the command line is incorrect")
        app.send("Enter")
        app.wait_for("Unlock Database")
        app.type(e2elib.SMALL_PASSWORD)
        app.send("Enter")
        app.wait_for("Enter View")
        self.assertAccountsShown(app)

    def test_database_does_not_exist(self):
        missing = self.tmp / "missing.dat"
        app = self.start(missing)
        app.wait_for("Unlock Database")
        app.type("whatever")
        app.send("Enter")
        app.wait_for(f"File {missing} does not exist")

    def test_cancel_prompt(self):
        db, _ = self.make_db()
        app = self.start(db)
        app.wait_for("Unlock Database")
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)

    def test_open_empty_database(self):
        app, _, _ = self.open_db(spec="empty")
        self.assertIn("^A Add", app.screen()[-1])
        app.send("C-x")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)

    def test_database_remembered(self):
        app, db, _ = self.open_db()
        app.send("C-x")
        app.wait_exit()
        app.close()
        self.assertIn(f"db-pathname = {db}", self.config_file.read_text())
        # Started without a database, the prompt offers the last one opened
        (self.tmp / "exit-code").unlink()
        app = self.start()
        self.assertIn(f"Database: {db}", app.wait_for("Unlock Database"))
        app.type(e2elib.SMALL_PASSWORD)
        app.send("Enter")
        app.wait_for("Enter View")
        self.assertAccountsShown(app)

    def test_config_option(self):
        config = self.tmp / "custom.ini"
        app, db, _ = self.open_db("-c", config)
        app.send("C-x")
        app.wait_exit()
        self.assertIn(f"db-pathname = {db}", config.read_text())
        self.assertFalse(self.config_file.exists())


if __name__ == "__main__":
    e2elib.main()
