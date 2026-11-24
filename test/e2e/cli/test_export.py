"""CLI: --export-db."""
import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402


class ExportTest(e2elib.E2ETestCase):

    def test_export(self):
        db, pw = self.make_db()
        out = self.tmp / "out.csv"
        proc = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", out, db)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn(f"Exporting account database at {out}", proc.stdout)
        self.assertIn("Warning! The exported file will contain unencrypted passwords.", proc.stdout)
        rows = e2elib.parse_export(out.read_text())
        self.assertEqual(e2elib.sorted_rows(rows), e2elib.sorted_rows(e2elib.read_spec()))

    def test_export_has_uuids(self):
        db, pw = self.make_db()
        out = self.tmp / "out.csv"
        self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", out, db)
        uuids = [line.split(",", 1)[0] for line in out.read_text().splitlines()[1:]]
        self.assertEqual(len(uuids), len(e2elib.read_spec()))
        self.assertEqual(len(set(uuids)), len(uuids))
        for uuid in uuids:
            self.assertRegex(uuid, r"^[0-9a-fA-F-]{32,36}$")

    def test_export_empty_db(self):
        db, pw = self.make_db("empty")
        self.assertEqual(self.export(db, pw), [])

    def test_password_file_uses_first_line_only(self):
        db, _ = self.make_db()
        pw = self.write_file("multi.pw", e2elib.SMALL_PASSWORD + "\nsecond line\nthird\n")
        self.assertDbContains(db, pw, e2elib.read_spec())

    def test_password_file_without_newline(self):
        db, _ = self.make_db()
        pw = self.write_file("nonl.pw", e2elib.SMALL_PASSWORD)
        self.assertDbContains(db, pw, e2elib.read_spec())

    def test_output_exists(self):
        db, pw = self.make_db()
        out = self.write_file("out.csv", "keep me\n")
        proc = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", out, db)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(f"Output file {out} already exists", proc.stderr)
        self.assertEqual(out.read_text(), "keep me\n")

    def test_output_exists_force(self):
        db, pw = self.make_db()
        out = self.write_file("out.csv", "overwrite me\n")
        proc = self.run_ncpwsafe("--export-db", "--force", "--password-file", pw, "-o", out, db)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertTrue(out.read_text().startswith("UUID,Group,Title,User,Password,Notes\n"))

    def test_database_missing(self):
        _, pw = self.make_db()
        missing = self.tmp / "missing.dat"
        proc = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", "out.csv", missing)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(f"Account database file {missing} does not exist", proc.stderr)

    def test_database_required(self):
        _, pw = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", "out.csv")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("Account database file is required", proc.stderr)

    def test_password_required(self):
        # Without --password-file, see test_password_prompt.py
        db, _ = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "--password-file", self.write_file("empty.pw", "\n"),
                                 "-o", "out.csv", db)
        self.assertEqual(proc.returncode, e2elib.PRC_ERR_FAIL)
        self.assertIn("Account database password is required", proc.stderr)
        self.assertFalse((self.tmp / "out.csv").exists())

    def test_output_required(self):
        # ValidateArgs used to check fs::Exists(*args.output_file_) on the
        # empty optional, which crashed
        db, pw = self.make_db()
        proc = self.run_ncpwsafe("--export-db", "--password-file", pw, db)
        self.assertNotEqual(proc.returncode, 0)
        self.assertNotCrashed(proc)
        self.assertIn("Output file is required", proc.stderr)

    def test_wrong_password(self):
        db, _ = self.make_db()
        pw = self.password_file("not the password")
        out = self.tmp / "out.csv"
        proc = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", out, db)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(f"An error occurred reading database file {db}", proc.stderr)
        self.assertFalse(out.exists())

    @unittest.skipIf(e2elib.IS_WINDOWS, "directory permission bits don't apply on Windows")
    @unittest.skipIf(hasattr(os, "geteuid") and os.geteuid() == 0, "root can write to read-only directories")
    def test_output_not_writable(self):
        db, pw = self.make_db()
        ro = self.tmp / "ro"
        ro.mkdir(mode=0o500)
        self.addCleanup(ro.chmod, 0o700)
        proc = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", ro / "out.csv", db)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("creating export file", proc.stderr)

    def test_output_directory_missing(self):
        db, pw = self.make_db()
        out = self.tmp / "no-such-dir" / "out.csv"
        proc = self.run_ncpwsafe("--export-db", "--password-file", pw, "-o", out, db)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("creating export file", proc.stderr)
        self.assertFalse(out.parent.exists())


if __name__ == "__main__":
    e2elib.main()
