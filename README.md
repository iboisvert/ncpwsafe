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
$ ./build/src/ncpwsafe
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
> .\build\x64-windows-msvc-debug\bin\ncpwsafe.exe
```
Use the `x64-windows-msvc-release` presets for a release build.
The first configure builds all dependencies from source and takes a while.

The executable is statically linked against its dependencies,
so `ncpwsafe.exe` can be copied and run on its own.
On Windows the configuration file is `%APPDATA%\ncpwsafe\ncpwsafe-config.ini`,
and `${HOME}` in preference values falls back to `%USERPROFILE%`.

## Known issues
- Resize of terminal window not supported
- Crash if when opening database with Unicode characters if the locale does not support Unicode

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
