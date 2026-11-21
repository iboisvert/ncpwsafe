"""Drive ncpwsafe's ncurses UI inside a detached tmux server.

Each `TmuxApp` owns a private tmux server (its own `-L` socket name and an
empty config), so tests can run in parallel and never touch the user's own
tmux sessions. The program runs in a shell that writes its exit code to a
file, so tests can check how the app exited.

    app = TmuxApp(env, cwd, exit_file, cols=100, rows=30)
    app.start(["/path/to/ncpwsafe", "db.dat"])
    app.wait_for("Unlock Database")
    app.type("secret")
    app.send("Enter")
    app.wait_for("Banking")
    assert app.highlighted() == ["Router"]
    app.send("C-x")
    assert app.wait_exit() == 1

Key names are tmux's (`send-keys`): Enter, Escape, Tab, BSpace, DC, Up,
Down, Left, Right, Home, End, PPage, NPage, C-a ... C-z.
"""

import itertools
import os
import re
import shlex
import subprocess
import time

DEFAULT_TIMEOUT = 10.0
POLL_INTERVAL = 0.05
# The terminal type inside tmux. "screen" is in every ncurses install
TERM = "screen"

_counter = itertools.count()
_SGR = re.compile(r"\x1b\[([0-9;:]*)m")
# Other escape sequences and the SO/SI shift characters ncurses uses for
# line drawing on the "screen" terminal
_OTHER_ESC = re.compile(r"\x1b[^\[]|\x1b\[[0-9;?]*[A-Za-ln-z]|[\x0e\x0f]")


class TimeoutError(AssertionError):
    pass


class TmuxApp:

    def __init__(self, tmux_bin, env, cwd, exit_file, cols=100, rows=30):
        self.tmux_bin = str(tmux_bin)
        self.env = dict(env)
        # Shorten ncurses' wait after Escape so Esc is handled promptly
        self.env.setdefault("ESCDELAY", "25")
        self.env.pop("TMUX", None)  # Running inside tmux must not matter
        self.cwd = str(cwd)
        self.exit_file = exit_file
        self.cols, self.rows = cols, rows
        self.socket = f"ncpwsafe-e2e-{os.getpid()}-{next(_counter)}"
        self.started = False

    # tmux ---------------------------------------------------------------

    def tmux(self, *args, check=True):
        proc = subprocess.run([self.tmux_bin, "-L", self.socket, "-f", os.devnull, *args],
                              env=self.env, cwd=self.cwd, capture_output=True, text=True,
                              timeout=DEFAULT_TIMEOUT, check=False)
        if check and proc.returncode != 0:
            raise RuntimeError(f"tmux {' '.join(args)} failed: {proc.stderr.strip()}")
        return proc

    def start(self, argv):
        """Start `argv` in a new tmux session sized cols x rows."""
        cmd = " ".join(shlex.quote(str(a)) for a in argv)
        exit_file = shlex.quote(str(self.exit_file))
        # Write the exit code, then linger briefly so the final screen can be read
        shell = f"{cmd}; echo $? > {exit_file}.tmp && mv {exit_file}.tmp {exit_file}; sleep 2"
        self.tmux("start-server", ";",
                  "set-option", "-g", "default-terminal", TERM, ";",
                  "set-option", "-g", "status", "off", ";",
                  "new-session", "-d", "-s", "main", "-x", str(self.cols), "-y", str(self.rows),
                  "sh", "-c", shell)
        self.started = True

    def close(self):
        if self.started:
            self.tmux("kill-server", check=False)
            self.started = False

    # Input --------------------------------------------------------------

    def send(self, *keys):
        """Send tmux key names, e.g. send("C-a"), send("Down", "Enter")."""
        self.tmux("send-keys", "-t", "main", *keys)

    def type(self, text):
        """Type literal text."""
        self.tmux("send-keys", "-t", "main", "-l", "--", text)

    def resize(self, cols, rows):
        self.cols, self.rows = cols, rows
        self.tmux("resize-window", "-t", "main", "-x", str(cols), "-y", str(rows))

    # Output -------------------------------------------------------------

    def _capture(self, escapes):
        args = ["capture-pane", "-p", "-t", "main"]
        if escapes:
            args.append("-e")
        return self.tmux(*args).stdout

    def screen(self):
        """The screen as a list of lines (line drawing appears as ACS letters: lqk x mqj)."""
        lines = [_OTHER_ESC.sub("", line) for line in self._capture(False).split("\n")]
        return (lines + [""] * self.rows)[:self.rows]

    def text(self):
        return "\n".join(self.screen())

    def highlighted(self, include_command_bar=False):
        """Text drawn in reverse video (e.g. the selected menu item), one entry per run.

        The command bar on the bottom row is also reverse video, it is left
        out unless `include_command_bar`.
        """
        runs = []
        lines = self._capture(True).split("\n")[:self.rows]
        if not include_command_bar:
            lines = lines[:self.rows - 1]
        for line in lines:
            line = _OTHER_ESC.sub("", line)
            reverse, current, pos = False, [], 0
            for m in _SGR.finditer(line + "\x1b[0m"):
                if reverse:
                    current.append(line[pos:m.start()])
                pos = m.end()
                codes = [c for c in re.split("[;:]", m.group(1)) if c] or ["0"]
                for code in codes:
                    if code == "7":
                        reverse = True
                    elif code in ("0", "27"):
                        if reverse and "".join(current).strip():
                            runs.append("".join(current).strip())
                        reverse, current = False, []
        return runs

    def wait_for(self, pattern, timeout=DEFAULT_TIMEOUT, regex=False):
        """Wait until `pattern` (text, or a regex if `regex`) is on screen. Returns the screen text."""
        deadline = time.monotonic() + timeout
        while True:
            text = self.text()
            if (re.search(pattern, text, re.M) if regex else pattern in text):
                return text
            if time.monotonic() > deadline:
                raise TimeoutError(f"Timed out waiting for {pattern!r}, screen is:\n{text}")
            time.sleep(POLL_INTERVAL)

    def wait_gone(self, pattern, timeout=DEFAULT_TIMEOUT):
        """Wait until `pattern` is no longer on screen."""
        deadline = time.monotonic() + timeout
        while True:
            text = self.text()
            if pattern not in text:
                return text
            if time.monotonic() > deadline:
                raise TimeoutError(f"Timed out waiting for {pattern!r} to go away, screen is:\n{text}")
            time.sleep(POLL_INTERVAL)

    def wait_until(self, predicate, what, timeout=DEFAULT_TIMEOUT):
        """Wait until predicate() is true."""
        deadline = time.monotonic() + timeout
        while not predicate():
            if time.monotonic() > deadline:
                raise TimeoutError(f"Timed out waiting for {what}, screen is:\n{self.text()}")
            time.sleep(POLL_INTERVAL)

    def wait_highlighted(self, text, timeout=DEFAULT_TIMEOUT):
        self.wait_until(lambda: text in self.highlighted(), f"{text!r} to be highlighted", timeout)

    def exit_code(self):
        """The program's exit code, or None while it is running."""
        try:
            return int(self.exit_file.read_text())
        except (FileNotFoundError, ValueError):
            return None

    def wait_exit(self, timeout=DEFAULT_TIMEOUT):
        self.wait_until(lambda: self.exit_code() is not None, "the program to exit", timeout)
        return self.exit_code()
