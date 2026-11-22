"""TUI: saving the database fails."""
import os
import stat
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

e2elib.require_tmux()

RETRY = "Retry?"


@unittest.skipIf(hasattr(os, "geteuid") and os.geteuid() == 0, "root can write to a read-only directory")
class SaveErrorTest(e2elib.TuiTestCase):

    def open_locked_db(self):
        """Open a database, delete an account, and make the database's directory read-only.

        The database is written to a temporary file that is then renamed, so
        saving fails when the directory can't be written to.
        """
        self.db_dir = self.tmp / "locked"
        self.db_dir.mkdir()
        db, pw = self.make_db(name="locked/small.dat")
        app = self.start("--password-file", pw, db)
        app.wait_for("Enter View")
        app.send("Down", "Down", "Down")
        app.wait_highlighted("Chequing")
        app.send("C-d")
        app.wait_for("Delete account Chequing")
        app.send("y")
        app.wait_gone("Delete account")
        self.lock()
        self.addCleanup(self.unlock)
        return app, db, pw

    def lock(self):
        self.db_dir.chmod(stat.S_IRUSR | stat.S_IXUSR)

    def unlock(self):
        self.db_dir.chmod(stat.S_IRWXU)

    def save_fails(self, app):
        app.send("C-s")
        app.wait_for(RETRY)
        bar = app.screen()[-1]
        for action in ("Y Retry", "N Quit", "^X Cancel"):
            self.assertIn(action, bar)

    def test_cancel_goes_back_to_list(self):
        app, db, pw = self.open_locked_db()
        self.save_fails(app)
        app.send("C-x")
        app.wait_gone(RETRY)
        app.wait_for("Enter View")
        self.assertIsNone(app.exit_code(), "^X must go back to the list")
        self.assertIn("small.dat * ", app.screen()[0])
        # The changes are still there, and can be saved once the directory is writable
        self.unlock()
        app.send("C-s")
        self.assertEqual(app.wait_exit(), 0)
        self.assertDbContains(db, pw, [r for r in e2elib.read_spec() if r[1] != "Chequing"])

    def test_no_exits_with_failure(self):
        app, db, pw = self.open_locked_db()
        self.save_fails(app)
        app.send("n")
        self.assertEqual(app.wait_exit(), e2elib.PRC_ERR_FAIL)
        self.unlock()
        self.assertDbContains(db, pw, e2elib.read_spec())

    def test_retry(self):
        app, db, pw = self.open_locked_db()
        self.save_fails(app)
        # Still read-only: asks again
        app.send("y")
        app.wait_for(RETRY)
        self.assertIsNone(app.exit_code())
        self.unlock()
        app.send("y")
        self.assertEqual(app.wait_exit(), 0)
        self.assertDbContains(db, pw, [r for r in e2elib.read_spec() if r[1] != "Chequing"])


if __name__ == "__main__":
    e2elib.main()
