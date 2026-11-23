#!/usr/bin/env python3
"""Take the README screenshots (docs/images/*.png) again, e.g. after a UI change.

    python3 docs/images/make-screenshots.py [--build build]

Opens the made-up database of test/e2e/fixtures/screenshot.tsv in ncpwsafe inside a
private tmux server (as the TUI tests do, see test/e2e/tmuxdriver.py), sized 100x30,
with the default theme in colour, and saves:

  accounts-list.png  The accounts list: groups, one collapsed, an account selected
  edit-account.png   The account details dialog over the list, the password masked
                     and notes of more than one line

Each screen is captured with its colours (tmux capture-pane -e), written as HTML
and rendered to PNG by headless Chrome or Chromium, then cropped and compressed
with ImageMagick. Needs a build (e2e-mkdb and ncpwsafe), tmux, Chrome or Chromium,
ImageMagick and the DejaVu Sans Mono font (another monospace font is used without it).
Never point it at a real database.
"""

import argparse
import html
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
# No __pycache__ in the source tree, as for the tests (PYTHONDONTWRITEBYTECODE)
sys.dont_write_bytecode = True
sys.path.insert(0, str(REPO / "test" / "e2e"))
from tmuxdriver import TmuxApp  # noqa: E402

FIXTURE = REPO / "test" / "e2e" / "fixtures" / "screenshot.tsv"
PASSWORD = (REPO / "test" / "e2e" / "fixtures" / "small.pw").read_text(encoding="utf-8").splitlines()[0]
COLS, ROWS = 100, 30

# The terminal's colours (Tango, on a dark background)
DEFAULT_FG, DEFAULT_BG = "#d3d7cf", "#1e1e1e"
PALETTE = ["#2e3436", "#cc0000", "#4e9a06", "#c4a000", "#3465a4", "#75507b", "#06989a", "#d3d7cf"]
# Around the terminal, cropped away afterwards; a colour the screen never uses
PAGE_BG = "#ff00ff"
FONT = '"DejaVu Sans Mono", "Liberation Mono", monospace'

SGR = re.compile(r"\x1b\[([0-9;:]*)m")
OTHER_ESC = re.compile(r"\x1b[^\[]|\x1b\[[0-9;?]*[A-Za-ln-z]|[\x0e\x0f]")


def mix(a, b, t):
    """The colour t of the way from a to b, as #rrggbb."""
    ca = [int(a[i:i + 2], 16) for i in (1, 3, 5)]
    cb = [int(b[i:i + 2], 16) for i in (1, 3, 5)]
    return "#" + "".join(f"{round(x + (y - x) * t):02x}" for x, y in zip(ca, cb))


class Style:
    def __init__(self):
        self.reset()

    def reset(self):
        self.fg = self.bg = None
        self.bold = self.dim = self.reverse = self.underline = False

    def apply(self, params):
        codes = [int(c) for c in re.split("[;:]", params) if c] or [0]
        i = 0
        while i < len(codes):
            c = codes[i]
            if c == 0:
                self.reset()
            elif c == 1:
                self.bold = True
            elif c == 2:
                self.dim = True
            elif c == 4:
                self.underline = True
            elif c == 7:
                self.reverse = True
            elif c == 22:
                self.bold = self.dim = False
            elif c == 24:
                self.underline = False
            elif c == 27:
                self.reverse = False
            elif 30 <= c <= 37:
                self.fg = PALETTE[c - 30]
            elif 90 <= c <= 97:
                self.fg = PALETTE[c - 90]
            elif c == 39:
                self.fg = None
            elif 40 <= c <= 47:
                self.bg = PALETTE[c - 40]
            elif 100 <= c <= 107:
                self.bg = PALETTE[c - 100]
            elif c == 49:
                self.bg = None
            elif c in (38, 48) and i + 2 < len(codes) and codes[i + 1] == 5:
                # 256 colours: only the basic 8 are used (see Theme), map the rest to them
                colour = PALETTE[codes[i + 2] % 8]
                if c == 38:
                    self.fg = colour
                else:
                    self.bg = colour
                i += 2
            i += 1

    def css(self):
        fg, bg = self.fg or DEFAULT_FG, self.bg or DEFAULT_BG
        if self.reverse:
            fg, bg = bg, fg
        if self.dim:
            fg = mix(fg, bg, 0.45)
        css = []
        if fg != DEFAULT_FG:
            css.append(f"color:{fg}")
        if bg != DEFAULT_BG:
            css.append(f"background:{bg}")
        if self.bold:
            css.append("font-weight:bold")
        if self.underline:
            css.append("text-decoration:underline")
        return ";".join(css)


def screen_to_html(capture):
    """The output of tmux capture-pane -e as an HTML page showing the terminal."""
    style = Style()
    rows = []
    for line in (capture.split("\n") + [""] * ROWS)[:ROWS]:
        line = OTHER_ESC.sub("", line)
        cells = []  # (text, css) runs
        pos = 0
        for m in list(SGR.finditer(line)) + [None]:
            text = line[pos:m.start()] if m else line[pos:]
            if text:
                cells.append((text, style.css()))
            if m:
                style.apply(m.group(1))
                pos = m.end()
        # Pad to the terminal's width, in the style the line ended with
        width = sum(len(t) for t, _ in cells)
        if width < COLS:
            cells.append((" " * (COLS - width), Style().css()))
        rows.append("".join(f'<span style="{css}">{html.escape(t)}</span>' if css else html.escape(t)
                            for t, css in cells))
        style.reset()
    body = "\n".join(rows)
    return f"""<!DOCTYPE html>
<html><head><meta charset="utf-8"><style>
body {{ margin: 0; background: {PAGE_BG}; }}
pre {{ display: inline-block; margin: 0; padding: 10px 12px; background: {DEFAULT_BG}; color: {DEFAULT_FG};
       font-family: {FONT}; font-size: 15px; line-height: normal; }}
</style></head><body><pre>{body}</pre></body></html>
"""


def find_program(*names):
    for name in names:
        path = shutil.which(name)
        if path:
            return path
    sys.exit(f"Needs one of: {', '.join(names)}")


def render(page, png, tmp):
    """Render an HTML page to a cropped, compressed PNG."""
    chrome = find_program("google-chrome", "google-chrome-stable", "chromium", "chromium-browser")
    magick = shutil.which("magick") or find_program("convert")
    page_file = tmp / (png.stem + ".html")
    page_file.write_text(page, encoding="utf-8")
    raw = tmp / (png.stem + "-raw.png")
    # A profile of its own, so the user's browser profile isn't touched
    subprocess.run([chrome, "--headless=new", "--disable-gpu", "--hide-scrollbars",
                    f"--user-data-dir={tmp / 'chrome-profile'}", "--force-device-scale-factor=1",
                    "--window-size=1400,1000", f"--screenshot={raw}", page_file.as_uri()],
                   check=True, capture_output=True)
    # Crop the page background around the terminal, drop metadata, compress
    subprocess.run([magick, str(raw), "-fuzz", "1%", "-trim", "+repage", "-strip",
                    "-define", "png:compression-level=9", "-define", "png:compression-filter=5",
                    str(png)], check=True)
    print(f"Wrote {png.relative_to(REPO)} ({png.stat().st_size // 1024} KiB)")


def select_heading(app, group):
    """Move the selection down to a group heading."""
    for _ in range(ROWS * 2):
        if app.highlighted() == [group]:
            return
        app.send("Down")
        app.wait_until(lambda: app.highlighted(), "a selection")
    sys.exit(f"Group heading {group} not found")


def select_account(app, title):
    app.send("/")
    app.wait_for("Search:")
    app.type(title)
    app.send("Enter")
    app.wait_gone("Search:")
    app.wait_highlighted(title)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--build", type=Path, default=REPO / "build", help="the build directory (default build)")
    parser.add_argument("--out", type=Path, default=REPO / "docs" / "images", help="where to write the PNGs")
    args = parser.parse_args()
    ncpwsafe = args.build / "bin" / "ncpwsafe"
    mkdb = args.build / "test" / "e2e" / "e2e-mkdb"
    for path in (ncpwsafe, mkdb):
        if not path.exists():
            sys.exit(f"{path} not found: build first, with the tests (BUILD_TESTING)")
    tmux = find_program("tmux")
    args.out.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="ncpwsafe-screenshots-") as tmp_name:
        tmp = Path(tmp_name)
        # A home of its own: no preferences, default theme; colour on
        env = {k: v for k, v in os.environ.items() if k not in ("NO_COLOR", "TMUX")}
        env.update(HOME=str(tmp), LANG="C.UTF-8", LC_ALL="C.UTF-8")
        db, pw = tmp / "accounts.dat", tmp / "accounts.pw"
        pw.write_text(PASSWORD + "\n", encoding="utf-8")
        subprocess.run([str(mkdb), str(db), str(pw), str(FIXTURE)], check=True, env=env)

        app = TmuxApp(tmux, env, tmp, tmp / "exit-code", cols=COLS, rows=ROWS)
        try:
            app.start([ncpwsafe, "--password-file", pw, db])
            app.wait_for("Enter View")

            # 1. Shopping collapsed, an account in Work selected
            select_heading(app, "Shopping")
            app.send("Left")
            app.wait_for("▸ Shopping")
            select_account(app, "Team wiki")
            render(screen_to_html(app.capture()), args.out / "accounts-list.png", tmp)

            # 2. The details of an account with notes of several lines
            select_account(app, "Chequing")
            app.send("Enter")
            app.wait_for("View Account")
            app.wait_for("Overdraft limit")
            render(screen_to_html(app.capture()), args.out / "edit-account.png", tmp)
        finally:
            app.close()


if __name__ == "__main__":
    main()
