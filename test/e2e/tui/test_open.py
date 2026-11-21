"""TUI: opening an account database."""
import os
import sys
import unittest
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

    def test_long_message_wrapped(self):
        # A message wider than the terminal is wrapped to fit, it used to
        # not be shown at all because its window couldn't be created
        missing = self.tmp / "a directory with a long name" / "another long directory name" / "missing.dat"
        message = f"File {missing} does not exist"
        cols = 50
        self.assertGreater(len(message), cols)
        app = self.start(missing, cols=cols)
        app.wait_for("Unlock Database")
        app.type("whatever")
        app.send("Enter")
        screen = app.wait_for("Enter Close")
        for word in message.split():
            self.assertIn(word, screen)
        app.send("Enter")
        app.wait_gone("Enter Close")
        app.wait_for("Unlock Database")

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

    @unittest.skipIf(os.getuid() == 0, "/proc/<pid> files are owned by root anyway")
    @unittest.skipUnless(Path("/proc/self/task").is_dir(), "needs Linux /proc")
    def test_core_dumps_disabled_in_release(self):
        # Release builds call prctl(PR_SET_DUMPABLE, 0) so the decrypted
        # database can't be read with ptrace or from a core dump. The kernel
        # then makes the process's /proc/<pid> files owned by root. Debug
        # builds stay dumpable so a debugger can attach
        app, _, _ = self.open_db()
        pid = app.pid()
        self.assertIsNotNone(pid)
        owner = os.stat(f"/proc/{pid}/status").st_uid
        if e2elib.IS_DEBUG_BUILD:
            self.assertEqual(owner, os.getuid())
        else:
            self.assertEqual(owner, 0)

    def test_config_option(self):
        config = self.tmp / "custom.ini"
        app, db, _ = self.open_db("-c", config)
        app.send("C-x")
        app.wait_exit()
        self.assertIn(f"db-pathname = {db}", config.read_text())
        self.assertFalse(self.config_file.exists())


if __name__ == "__main__":
    e2elib.main()
