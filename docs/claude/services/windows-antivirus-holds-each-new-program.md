---
type: pitfall
title: "Windows: some antivirus holds every program it has not seen for 5-16 s at its first start"
area: services
paths: ["packaging/windows/ecce.cmd", "packaging/windows/ecce-prewarm", "packaging/windows/ecce-splash", "packaging/gateway/ecce-first-start"]
issues: [522, 444]
---
Measured on caesium (managed Windows 11, WithSecure Elements with Defender
passive), 2026-10-25: the first run of an ECCE program with a hash the
antivirus has not seen takes 5-16 s longer, every later run of it ~0.05 s.
It is per file hash and machine-wide, so every release pays it once for each
of the ~37 programs in `bin\`. Copying the install to a new path costs
nothing; appending 16 bytes to the `.exe`s (a new hash) brings it all back,
which is how to reproduce a fresh install without an MSI. Defender alone
(NerdEgg, Windows 10): 0.2 s. Mapping an exe as an image costs nothing; the
hold is at process creation, and a process created suspended and terminated
at once takes the whole check: the next real start is fast.

What it cost before #522: `ecce-first-start` ran `ecce-localdata.exe` twice
before its window (screen empty ~14 s), `organizer.exe` 10-16 s before its
own splash could show, `builder.exe` ~8 s at the first Viewer, each
`eccejobmaster`/`eccejobstore` of the example import likewise. Python
(signed), MSYS2 and Strawberry programs were not held; the python tree is
not compiled at start (stdlib in `python313.zip`, wx writes 3 `.pyc` once).

- `ecce-first-start` reads the LocalData preference itself; keep any ECCE
  program out of the path before a first window.
- `ecce-prewarm` (started first by `ecce.cmd`) creates each `bin\*.exe`
  suspended and ends it, Organizer first, once per install
  (`~/.ECCE/prewarmed`). One at a time: the antivirus checks one program
  at a time, and two workers made the real Organizer start wait behind them.
- `ecce-splash` (Python) shows the start-up card from before the broker
  until an `organizer.exe` window exists; it cannot be tested over ssh
  (no visible windows in that session).
