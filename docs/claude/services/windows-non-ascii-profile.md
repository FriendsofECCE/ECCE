---
type: pitfall
title: "Windows: a profile folder with å/ä/ö broke the start twice, silently (#922)"
area: services
paths: ["packaging/windows/ecce-broker-win", "packaging/windows/ecce-splash", "packaging/windows/ecce.cmd", "src/dsm/edsiimpl/EDSIServerCentral.C", "src/comm/rcommand/DirectTransportWin.C"]
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
- Still open: `DirectTransportWin.C` widens with CP_UTF8 although ECCE's
  strings are ANSI; any other XML built from paths has the same trap. A
  UTF-8 `activeCodePage` manifest on every exe is the general fix.

Test without a new account: unpack the zip into a fake profile and run
`ecce.cmd` with USERPROFILE, HOMEPATH, LOCALAPPDATA, APPDATA, TEMP, TMP
pointing there (scheduled task, interactive principal). ecce-splash now
reports a crashed or windowless Organizer and logs each step with its time
in `%USERPROFILE%\.ECCE\start.log`.
