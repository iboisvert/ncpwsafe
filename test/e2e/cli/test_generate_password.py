"""CLI: --generate-password."""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import e2elib  # noqa: E402

ALPHA = "A-Za-z"
SYMBOL = re.escape("!#$%&*@^")
# Policy number -> (name, regex for one password character), see src/Policy.cpp
POLICIES = {
    1: ("alpha/digit/symbol", f"[{ALPHA}0-9{SYMBOL}]"),
    2: ("alpha/digit", f"[{ALPHA}0-9]"),
    3: ("easy-to-read alpha/digit/symbol", f"[ACDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz2345679{SYMBOL}]"),
    4: ("digits only", "[0-9]"),
    5: ("hex digits only", "[0-9a-fA-F]"),
}


class GeneratePasswordTest(e2elib.E2ETestCase):

    def generate(self, *args):
        proc = self.run_ncpwsafe("--generate-password", *args)
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        lines = proc.stdout.splitlines()
        self.assertTrue(lines and lines[0].startswith("Generating "), proc.stdout)
        return lines[0], lines[1:]

    def test_defaults(self):
        header, passwords = self.generate()
        self.assertEqual(header, "Generating 1 password of length 20 using policy alpha/digit")
        self.assertEqual(len(passwords), 1)
        self.assertRegex(passwords[0], rf"^{POLICIES[2][1]}{{20}}$")

    def test_count_and_length(self):
        header, passwords = self.generate("--password-count=5", "--password-length=12")
        self.assertIn("Generating 5 passwords of length 12", header)
        self.assertEqual(len(passwords), 5)
        for pw in passwords:
            self.assertEqual(len(pw), 12)
        self.assertEqual(len(set(passwords)), 5, "generated passwords should differ")

    def test_policy_by_number(self):
        for number, (name, charset) in POLICIES.items():
            with self.subTest(policy=number):
                header, passwords = self.generate(f"--password-policy={number}", "--password-count=20",
                                                  "--password-length=40")
                self.assertTrue(header.endswith(f"using policy {name}"), header)
                for pw in passwords:
                    self.assertRegex(pw, rf"^{charset}{{40}}$")

    def test_policy_by_name(self):
        # ParseArgs used to store the 0-based index of a policy given by name,
        # which main() took as 1-based, so a name selected the policy before it
        for number, (name, charset) in POLICIES.items():
            with self.subTest(policy=name):
                header, passwords = self.generate(f"--password-policy={name}", "--password-count=5")
                self.assertTrue(header.endswith(f"using policy {name}"), header)
                for pw in passwords:
                    self.assertRegex(pw, rf"^{charset}+$")

    def test_invalid_count(self):
        proc = self.run_ncpwsafe("--generate-password", "--password-count=0")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("Generated password count must be > 0", proc.stderr)

    def test_invalid_length(self):
        proc = self.run_ncpwsafe("--generate-password", "--password-length=0")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("Generated password length must be > 0", proc.stderr)

    def test_invalid_policy_number(self):
        proc = self.run_ncpwsafe("--generate-password", "--password-policy=99")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("Invalid policy for generated passwords", proc.stderr)

    def test_invalid_policy_name(self):
        proc = self.run_ncpwsafe("--generate-password", "--password-policy=no-such-policy")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("Invalid policy for generated passwords", proc.stderr)

    def test_policy_zero_rejected(self):
        # 0 used to be accepted, and main() indexed the policy table at -1
        proc = self.run_ncpwsafe("--generate-password", "--password-policy=0")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("Invalid policy for generated passwords", proc.stderr)

    def test_policy_with_trailing_text_rejected(self):
        # Names used to be matched as a prefix of the argument
        for policy in ("2x", "alpha/digit-and-more"):
            with self.subTest(policy=policy):
                proc = self.run_ncpwsafe("--generate-password", f"--password-policy={policy}")
                self.assertNotEqual(proc.returncode, 0)
                self.assertIn("Invalid policy for generated passwords", proc.stderr)


if __name__ == "__main__":
    e2elib.main()
