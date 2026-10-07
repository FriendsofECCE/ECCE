# Windows build and test hosts

Two hosts, same toolchain: MSYS2 UCRT64 (packages below), Strawberry Perl
portable under `%USERPROFILE%\strawberry`, repo in `%USERPROFILE%\ECCE`.

- **NerdEgg (default)**: physical Windows 10 PC, `ssh nerdegg`
  (pascal@192.168.2.15), i7-4930K 6c/12t, real GPU. Build with `JOBS=8`.
- **win10 VM (retired)**: VirtualBox on niobium, was `ssh -p 2222 andy@localhost`.
  Slow and software-GL only; keep it saved, do not start it.

## Toolchain
MSYS2 (`C:\msys64`) packages, `pacman -S --needed`:
`base git` and `mingw-w64-ucrt-x86_64-{apr-util,cmake,coin,freetype,gcc,
gcc-fortran,glib2,libjpeg-turbo,libssh,mosquitto,ninja,pkgconf,python,
wxwidgets3.2-msw,xerces-c}`.

## Files
- `b.bat` - `b.bat script.sh` runs a script in the UCRT64 login shell.
- `build.sh` - fetch the branch, build all targets (`JOBS=8` default),
  prints the FAILED count; log in `~/build.log`.
- `go.bat` - start each GUI app via `runapp.ps1` and screenshot it into
  `%USERPROFILE%\shots`. Needs an interactive session: run it from a
  scheduled task (`schtasks /create /tn ecceshot /sc once /st 00:00
  /it /tr "...\go.bat"` then `schtasks /run /tn ecceshot`) because ssh
  sessions have no desktop.
- `launch_local.py complete|cancel [--state DIR]` - local job end to end, no GUI (by default with the per-session broker of `packaging/windows/ecce-broker-win`, checking that state messages arrive and the ACL holds; `cancel` lists the job's process group before and after; `WINTEST=NO_MESSAGING` skips the broker; `WINTEST_HOME` runs it against an install tree made by `packaging/windows/bundle-shell.sh`, `WINTEST_BASE_PATH` sets the PATH).

## Strawberry Perl
Portable 5.40.5.1 unpacked to `%USERPROFILE%\strawberry` (from
https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_54051_64bit/strawberry-perl-5.40.5.1-64bit-portable.zip,
sha256 6619fe7eeef921ccddb4aac3972fb602a0c690a3074205b863ade998d7bc79a6;
the `portable` entry of strawberryperl.com/releases.json lists the msi URL
with this zip's hash and size).
