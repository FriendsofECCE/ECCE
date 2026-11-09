---
type: pitfall
title: "Windows: a profile folder with å/ä/ö broke the start twice, silently (#922)"
area: services
paths: ["packaging/windows/ecce-broker-win", "packaging/windows/ecce-splash", "packaging/windows/ecce.cmd", "src/dsm/edsiimpl/EDSIServerCentral.C", "src/comm/rcommand/DirectTransportWin.C", "packaging/windows/ecce.rc", "packaging/windows/ecce-utf8.rc", "CMakeLists.txt", "packaging/windows/embed-manifest.py", ".github/workflows/build.yml"]
issues: [922]
---
ECCE's own programs have no UTF-8 `activeCodePage` manifest, so on Windows
`getenv`/`fopen` work in the ANSI code page (cp1252 on Swedish Windows),
while MSYS bash and Python write UTF-8 text. Under `C:\Users\Åsa Öberg`
(a space alone was fine, tested on caesium):

- mosquitto (ANSI file API) could not open the absolute UTF-8 paths in its
  config; the broker never came up. `ecce-broker-win` now runs it with its
  folder as working directory and relative paths.
- `EDSIServerCentral` built its local-mode XML from ANSI paths, Xerces read
  it as UTF-8, failed at line 1, and the assertion aborted organizer.exe
  (0xC0000409 in ucrtbase). The paths are converted to UTF-8 first.
- Decision (2026-11-09): every exe built for Windows gets a manifest with
  `<activeCodePage>UTF-8</activeCodePage>` (Windows 10 1903+/11): the GUI
  apps through `packaging/windows/ecce.rc` (one manifest per exe, so it
  carries common controls v6, per-monitor DPI v2 and the code page, and
  sets `wxUSE_NO_MANIFEST` for wx.rc), all other exes through
  `ecce-utf8.rc` attached by the loop at the end of `CMakeLists.txt`.
  getenv, fopen, CreateProcessA then use UTF-8 like bash, Python and
  `DirectTransportWin.C`. Any new exe is covered by that loop.
- The relative paths only help for letters in the ANSI code page:
  mosquitto_passwd makes the absolute path itself (`C:\Users\Łukasz Ő`
  became `Lukasz O`, "Unable to open file"). The MSYS2 mosquitto tools are
  not built by us, so CI writes the same manifest into them
  (`packaging/windows/embed-manifest.py`, kernel32 resource API).
- `ecce-splash --gateway` must start the bundled bash by its full path:
  CreateProcess searches System32 before PATH, and WSL's
  `System32\bash.exe` (present on GitHub's runners) took over; the broker
  start failed at once and msi-test's shortcut check found no Organizer.

Test without a new account: unpack the zip into a fake profile and run
`ecce.cmd` with USERPROFILE, HOMEPATH, LOCALAPPDATA, APPDATA, TEMP, TMP
pointing there (scheduled task, interactive principal). ecce-splash now
reports a crashed or windowless Organizer and logs each step with its time
in `%USERPROFILE%\.ECCE\start.log`.

Linux has the same trap for a space: mosquitto does not honour quotes in
`log_dest file` or `listener 0 <socket>`, so `ecce-gateway-start` writes
relative names and starts the broker from the state directory when that
path has whitespace (found by `tests/launch --odd-names`, HOME "Åsa Öberg").
The central/TLS listeners still put `$ECCE_HOME/server/...` and
`$STATEDIR/dataserver/users` in the config unquoted.
