# ncpwsafe
An ncurses terminal program that manages encrypted account password database files
that are compatible with the V1 and V2 [Password Safe](https://www.pwsafe.org/)
database formats.

**This is alpha-quality code at best, use at your own risk.**
Be sure to make a backup of your account password database 
before using this application.

`xclip` is required to copy data to the clipboard from the CLI,

## Get Started

To build:
```
$ cmake -DCMAKE_BUILD_TYPE:STRING=Debug -B build
$ cmake --build build
$ ./build/src/ncpwsafe
```

### Windows

ncpwsafe builds on Windows with [MinGW-w64](https://www.mingw-w64.org/) gcc,
with dependencies provided by [vcpkg](https://vcpkg.io/).
MSVC is not supported, because the ncurses `form` and `menu` libraries
do not build with MSVC.

Requirements:
- MinGW-w64 gcc on the `PATH`, for example from [MSYS2](https://www.msys2.org/) (UCRT64) or [WinLibs](https://winlibs.com/)
- [Ninja](https://ninja-build.org/) and CMake 3.21 or later
- A clone of vcpkg, with the `VCPKG_ROOT` environment variable set to its location

To build:
```
> cmake --preset x64-windows-mingw-release
> cmake --build --preset x64-windows-mingw-release
> build\bin\ncpwsafe.exe
```

The first configure builds the dependencies with vcpkg (`x64-mingw-static` triplet),
which takes a while. To enable logging, add `-DUSE_GLOG=ON` to the configure command.

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
