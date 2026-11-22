# ncpwsafe
An ncurses terminal program that manages encrypted account password database files
that are compatible with the V1 and V2 [Password Safe](https://www.pwsafe.org/)
database formats.

**This is alpha-quality code at best, use at your own risk.**
Be sure to make a backup of your account password database 
before using this application.

On Linux, `xclip` is required to copy data to the clipboard from the CLI.
On Windows, the Windows clipboard is used directly.

## Get Started

To build:
```
$ cmake -DCMAKE_BUILD_TYPE:STRING=Debug -B build
$ cmake --build build
$ ./build/bin/ncpwsafe
```

### Windows

ncpwsafe builds natively on Windows with MSVC and the Windows SDK.
Dependencies are installed by [vcpkg](https://vcpkg.io) from `vcpkg.json`.
ncurses is built from the overlay port in `ports/ncurses`, which adds MSVC
support to the upstream vcpkg port using the ncurses Windows console driver.

Requirements:
- Visual Studio 2022 or later, or the Build Tools for Visual Studio, with the
  "Desktop development with C++" workload (MSVC, the Windows SDK, CMake, and Ninja)
- vcpkg; the copy bundled with Visual Studio works, or set `VCPKG_ROOT`
  to a clone of https://github.com/microsoft/vcpkg

From a Developer PowerShell or Developer Command Prompt for Visual Studio (x64):
```
> cmake --preset x64-windows-msvc-debug
> cmake --build --preset x64-windows-msvc-debug
> ctest --preset x64-windows-msvc-debug
> .\build\bin\ncpwsafe.exe
```
Use the `x64-windows-msvc-release` presets for a release build. It builds
only the release libraries of the dependencies (the overlay triplet in
`triplets/`). The first configure builds all dependencies from source and
takes a while. glog is only installed when configuring with `-DUSE_GLOG=ON`.

The executable is statically linked against its dependencies,
so `ncpwsafe.exe` can be copied and run on its own.
On Windows the configuration file is `%APPDATA%\ncpwsafe\ncpwsafe-config.ini`,
and `${HOME}` in preference values falls back to `%USERPROFILE%`.

## Colours

ncpwsafe uses colour when the terminal supports it. Set the `NO_COLOR`
environment variable (see https://no-color.org), or `theme = mono` in the
configuration file (`~/.config/ncpwsafe/ncpwsafe-config.ini`, or
`%APPDATA%\ncpwsafe\ncpwsafe-config.ini` on Windows), to draw with
bold, underline, dim and reverse video only.

## Auto sign-off

So that an account database isn't left open on an unattended computer,
ncpwsafe quits after 5 minutes without a key press, **discarding any unsaved
changes**. It clears the screen and exits with status 124. Time while the
computer is asleep counts, so a laptop opened again later has signed off.

Change the period with `auto-sign-off` in the configuration file: a number
with an optional unit `s`, `m` or `h` (a number without a unit is minutes),
or `0` to turn auto sign-off off:
```
auto-sign-off = 15m
```

## Testing

Unit tests (GoogleTest) and end-to-end tests of the CLI commands and the
ncurses UI run through CTest:
```
$ cmake --preset=x64-linux-gcc-debug
$ cmake --build --preset=x64-linux-gcc-debug
$ ctest --preset=x64-linux-gcc-debug
```
The end-to-end tests need Python 3, and the UI tests also need `tmux`
(they are skipped without it). See [test/README.md](test/README.md) for
requirements, running a subset of the tests, and how the tests work.

On Windows the unit tests and the CLI end-to-end tests run, but the UI
end-to-end tests are skipped: they drive the UI through `tmux`, which isn't
available there. This is a gap in testing: nothing automatically tests the
ncurses UI on Windows (the Windows console driver, keyboard input, the
clipboard, resizing), so check UI changes on Windows by hand.

## Known issues
Known bugs are tracked in [docs/ISSUES.md](docs/ISSUES.md).

# Attribution
This project contains code from StackOverflow users @Ammo Goettsch and @eerpini,
see https://stackoverflow.com/questions/9405985/linux-executing-child-process-with-piped-stdin-stdout

This project uses the ncurses libraries maintained by Thomas E. Dickey
at https://invisible-island.net/

# Copyright
Copyright (C) 2023 Ian Boisvert

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
