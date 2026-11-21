"""CLI: --test-db (Debug builds only)."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

if not e2elib.ncpwsafe_has_test_db():
    print("--test-db is not available in this build (Debug builds only), skipping")
    sys.exit(e2elib.SKIP_EXIT_CODE)


class TestDbTest(e2elib.E2ETestCase):

    def generate(self, db, *args):
        pw = self.password_file("test-db password")
        proc = self.run_ncpwsafe("--test-db", "--password-file", pw, *args, db)
        return proc, pw

    def test_generate(self):
        db = self.tmp / "test.dat"
        proc, pw = self.generate(db, "--test-db-groups=4", "--test-db-items=25")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn(f"Generating test database at {db}", proc.stdout)
        rows = self.export(db, pw)
        self.assertEqual(len(rows), 25)
        groups = {row[0] for row in rows}
        self.assertLessEqual(len(groups), 4)
        for group, title, user, password, _ in rows:
            self.assertTrue(title and user and password, (group, title, user, password))

    def test_generate_defaults(self):
        db = self.tmp / "test.dat"
        proc, pw = self.generate(db)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertEqual(len(self.export(db, pw)), 50)

    def test_generate_russian(self):
        db = self.tmp / "test.dat"
        proc, pw = self.generate(db, "--test-db-lang=ru", "--test-db-items=10")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        rows = self.export(db, pw)
        self.assertEqual(len(rows), 10)
        # Cyrillic titles
        self.assertTrue(any(any("Ѐ" <= ch <= "ӿ" for ch in row[1]) for row in rows), rows)

    def test_exists(self):
        db = self.write_file("test.dat", "keep me")
        proc, _ = self.generate(db)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(f"File {db} already exists", proc.stderr)
        self.assertEqual(db.read_text(), "keep me")

    def test_exists_force(self):
        db = self.write_file("test.dat", "overwrite me")
        proc, pw = self.generate(db, "--force", "--test-db-items=3")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertEqual(len(self.export(db, pw)), 3)

    def test_invalid_language(self):
        proc, _ = self.generate(self.tmp / "test.dat", "--test-db-lang=fr")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("Valid languages for test database are: en-CA, ru", proc.stderr)

    def test_invalid_counts(self):
        for arg, msg in (("--test-db-groups=0", "group count must be > 0"),
                         ("--test-db-items=0", "item count must be > 0")):
            with self.subTest(arg=arg):
                proc, _ = self.generate(self.tmp / "test.dat", arg)
                self.assertNotEqual(proc.returncode, 0)
                self.assertIn(msg, proc.stderr)


if __name__ == "__main__":
    e2elib.main()
