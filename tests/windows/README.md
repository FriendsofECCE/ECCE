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
- `central_win.py <tree> <state> <host:port> <user> <password>` - a Windows client against a central server (#247): the first-start answer `server:<host:port>`, the session as `ecce.cmd` starts it, the login through an `-pipe` auth file (the server account must be the Windows user name, `ECCE_REALUSER`), a MOPAC calculation made on the server and its input saved and read back over WebDAV. On a failure it lists and photographs the Organizer's windows (`titles.ps1`, `winshot.ps1`). Needs the desktop (scheduled task).
- `e2e_win.py <tree> <state> [codes]` - local jobs end to end through the apps' test hooks; give the tree and state paths with a space to cover a user name with one (the temp folder is below the state folder).
- `launch_local.py complete|cancel [--state DIR]` - local job end to end, no GUI (by default with the per-session broker of `packaging/windows/ecce-broker-win`, checking that state messages arrive and the ACL holds; `cancel` lists the job's process group before and after; `WINTEST=NO_MESSAGING` skips the broker; `WINTEST_HOME` runs it against an install tree made by `packaging/windows/bundle-shell.sh`, `WINTEST_BASE_PATH` sets the PATH).

## Strawberry Perl
Portable 5.40.5.1 unpacked to `%USERPROFILE%\strawberry` (from
https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_54051_64bit/strawberry-perl-5.40.5.1-64bit-portable.zip,
sha256 6619fe7eeef921ccddb4aac3972fb602a0c690a3074205b863ade998d7bc79a6;
the `portable` entry of strawberryperl.com/releases.json lists the msi URL
with this zip's hash and size).

## The self-contained package
`ECCE-windows-<version>.zip` runs on a Windows 10/11 with nothing installed.
Pieces, all from `packaging/windows/`:
- `bundle-shell.sh` - sh and coreutils (MSYS2 `usr\bin`).
- `bundle-runtime.sh` - the UCRT64 DLLs (`ldd`, only from `/ucrt64/bin`), `mosquitto` and `mosquitto_passwd` into `bin`.
- `bundle-tools.ps1` - Strawberry Perl portable (pinned above; only `perl\` and the `c\bin` DLLs kept) and Python 3.13.5 embeddable
  (https://www.python.org/ftp/python/3.13.5/python-3.13.5-embed-amd64.zip, sha256
  7d2650fd9d1b9d002d4a315d5f354247fd6a44f30517c7ef577b08f57a0fb6d9) with the wxPython 4.3.1 cp313 win_amd64 wheel
  (sha256 0ae85e266dbb99fe46bc26920aa87b22fa3f83dc51b321bcd9afa51fa68340e7) unpacked into its `Lib\site-packages`.
  `python3.exe` is a copy of `python.exe`, because the scripts call `python3`.
- `ecce.cmd` starts the Organizer with the package's own PATH; `make-shortcuts.ps1` adds the Start menu entry.

## CI
`build.yml`, job "Windows (MSYS2 UCRT64)" (experimental, `continue-on-error`):
same packages, ccache, build, ctest (transport_process, fragreaders,
fragreaders_de, parsers), install to `stage/ecce`, `bundle-shell.sh`,
the three bundle steps, the zip, then, from the unpacked zip with only
System32 on PATH, `launch_local.py complete|cancel` and `ci-run.ps1` (starts
each app on the runner desktop, screenshots; without `-Msys` it is that
clean-PATH mode), and the `windows-run` artifact (shots, logs).
Run it with `gh workflow run build.yml --ref <branch> -f only=windows`.
