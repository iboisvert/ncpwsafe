"""CLI: the password prompt, for the commands that need the database password when
--password-file isn't given. --export-db is the example here; see test_change_password.py
and test_test_db.py for the prompts of the other commands.
"""
import os
import signal
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

PROMPT = "Account database password: "


class NoTerminalTest(e2elib.E2ETestCase):
    """run_ncpwsafe() runs ncpwsafe without a terminal (on Windows, without a console)"""

    def test_export_without_terminal(self):
        db, _ = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "-o", "out.csv", db)
        self.assertEqual(proc.returncode, e2elib.PRC_ERR_FAIL)
        self.assertNotCrashed(proc)
        self.assertIn("No terminal to prompt for the password on, use --password-file", proc.stderr)
        self.assertFalse((self.tmp / "out.csv").exists())

    def test_change_password_without_terminal(self):
        db, _ = self.make_db()
        proc = self.run_ncpwsafe("--change-password", db)
        self.assertEqual(proc.returncode, e2elib.PRC_ERR_FAIL)
        self.assertIn("No terminal to prompt for the password on, use --password-file", proc.stderr)

    def test_password_not_read_from_stdin(self):
        # Standard input isn't the terminal
        db, _ = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "-o", "out.csv", db, input=e2elib.SMALL_PASSWORD + "\n")
        self.assertEqual(proc.returncode, e2elib.PRC_ERR_FAIL)
        self.assertIn("No terminal to prompt for the password on", proc.stderr)
        self.assertFalse((self.tmp / "out.csv").exists())


@unittest.skipUnless(e2elib.HAS_PTY, "needs a pseudo-terminal")
class PasswordPromptTest(e2elib.E2ETestCase):

    def start_export(self, *args):
        db, _ = self.make_db()
        out = self.tmp / "out.csv"
        app = self.start_on_terminal("--export-db", *args, "-o", out, db)
        return app, out

    def test_export_prompts_for_password(self):
        app, out = self.start_export()
        app.expect(PROMPT)
        # Not echoed while it is typed
        self.assertFalse(app.echo_enabled())
        app.send(e2elib.SMALL_PASSWORD + "\r")
        self.assertEqual(app.wait(), 0, app.output)
        self.assertNotIn(e2elib.SMALL_PASSWORD, app.output)
        # Echo is back on
        self.assertTrue(app.echo_enabled())
        with open(out, encoding="utf-8", newline="") as f:
            rows = e2elib.parse_export(f.read())
        self.assertEqual(e2elib.sorted_rows(rows), e2elib.sorted_rows(e2elib.read_spec()))

    def test_password_file_does_not_prompt(self):
        db, pw = self.make_db()
        app = self.start_on_terminal("--export-db", "--password-file", pw, "-o", "out.csv", db)
        self.assertEqual(app.wait(), 0, app.output)
        self.assertNotIn(PROMPT, app.output)

    def test_wrong_password(self):
        app, out = self.start_export()
        app.answer(PROMPT, "not the password")
        self.assertEqual(app.wait(), e2elib.PRC_ERR_FAIL)
        self.assertIn("An error occurred reading database file", app.output)
        self.assertFalse(out.exists())

    def test_empty_password(self):
        app, out = self.start_export()
        app.answer(PROMPT, "")
        self.assertEqual(app.wait(), e2elib.PRC_ERR_FAIL)
        self.assertIn("Account database password is required", app.output)
        self.assertFalse(out.exists())

    def test_line_editing(self):
        # The terminal's erase character (DEL here) works while echo is off
        app, out = self.start_export()
        app.expect(PROMPT)
        app.send(e2elib.SMALL_PASSWORD + "xy\x7f\x7f\r")
        self.assertEqual(app.wait(), 0, app.output)
        self.assertTrue(out.exists())

    def test_enter_without_icrnl(self):
        # On a terminal set to -icrnl, Enter (CR) doesn't end a line in canonical mode
        db, _ = self.make_db()
        app = self.start_on_terminal("--export-db", "-o", "out.csv", db, icrnl=False)
        app.answer(PROMPT, e2elib.SMALL_PASSWORD)
        self.assertEqual(app.wait(), 0, app.output)
        self.assertTrue((self.tmp / "out.csv").exists())
        # The terminal's own setting is restored
        import termios
        self.assertFalse(app.attrs()[0] & termios.ICRNL)

    def test_too_long(self):
        # Linux keeps 4095 bytes of a line and drops the rest: refuse it rather than
        # take a password cut short
        app, out = self.start_export()
        app.answer(PROMPT, "x" * 5000)
        self.assertEqual(app.wait(), e2elib.PRC_ERR_FAIL)
        self.assertIn("The password is too long to type at the prompt, use --password-file", app.output)
        self.assertTrue(app.echo_enabled())
        self.assertFalse(out.exists())

    def test_end_of_input(self):
        app, out = self.start_export()
        app.expect(PROMPT)
        app.send("\x04")  # Ctrl-D
        self.assertEqual(app.wait(), e2elib.PRC_ERR_FAIL)
        self.assertIn("No password was entered", app.output)
        self.assertTrue(app.echo_enabled())
        self.assertFalse(out.exists())

    def test_interrupt_restores_echo(self):
        app, out = self.start_export()
        app.expect(PROMPT)
        self.assertFalse(app.echo_enabled())
        app.send("half a pass\x03")  # Ctrl-C
        # Ended by SIGINT, as without the prompt, after turning echo back on
        self.assertEqual(app.wait(), -signal.SIGINT)
        self.assertTrue(app.echo_enabled())
        self.assertFalse(out.exists())

    def test_terminate_restores_echo(self):
        app, out = self.start_export()
        app.expect(PROMPT)
        os.kill(app.pid, signal.SIGTERM)
        self.assertEqual(app.wait(), -signal.SIGTERM)
        self.assertTrue(app.echo_enabled())

    def test_suspend_prompts_again(self):
        # After Ctrl-Z, the prompt starts again when ncpwsafe is continued, without what
        # was typed before. ncpwsafe runs in a session of its own here, an orphaned process
        # group that the kernel doesn't stop for Ctrl-Z, so it prompts again at once
        app, out = self.start_export()
        app.expect(PROMPT)
        app.send("abc\x1a")  # Ctrl-Z
        app.expect(PROMPT)
        self.assertFalse(app.echo_enabled())
        app.send(e2elib.SMALL_PASSWORD + "\r")
        self.assertEqual(app.wait(), 0, app.output)
        self.assertTrue(out.exists())


if __name__ == "__main__":
    e2elib.main()
