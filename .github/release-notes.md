<!--
Starting point for the notes of a release: .github/workflows/release.yml fills in
VERSION and creates the draft release with them. Write the changes, delete this
comment and what doesn't apply, then publish the draft.
-->

## Changes

- ...

<!-- Before 1.0, say here if this version changes behaviour or the database format. -->

## Downloads

| File | For |
|---|---|
| `ncpwsafe-VERSION-linux-x86_64.tar.gz` | Linux on x86-64 with glibc GLIBC_VERSION or newer (for example Ubuntu 22.04, Debian 12, Fedora 36 and later). The other libraries are linked in. |
| `ncpwsafe-VERSION-windows-x64.zip` | Windows 10 or later, 64-bit. Nothing else needs to be installed. |
| `SHA256SUMS` | The SHA-256 checksum of each file: `sha256sum -c SHA256SUMS --ignore-missing` |

On Linux, extract the archive and run `ncpwsafe/bin/ncpwsafe`, or copy `bin/ncpwsafe` and
`share/man/man1/ncpwsafe.1` under a directory such as `~/.local` (`man ncpwsafe`).
Copying to the clipboard needs `xclip` and an X or XWayland display.

On Windows, extract the zip and run `ncpwsafe.exe` in Windows Terminal or a console.
`ncpwsafe.1.html` is the manual. The executable isn't signed, so SmartScreen may warn the
first time it runs.

## Notes

- ncpwsafe quits after 5 minutes without a key press, discarding unsaved changes
  (auto sign-off). Change the period with the `auto-sign-off` preference, or set it to `0` to
  turn it off; see ncpwsafe(1).
- The source of this release is the tag [vVERSION](https://github.com/iboisvert/ncpwsafe/tree/vVERSION);
  `THIRD-PARTY-LICENSES` in each package lists the libraries linked into it.
