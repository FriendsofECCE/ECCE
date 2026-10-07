---
type: pitfall
title: "Xvfb's stderr must not go to a pipe nobody reads"
area: services
section: "Pitfalls"
paths: ["tests/apps/xdisplay.py", "tests/apps/run_tests.py"]
issues: [127]
---
**Xvfb's stderr must not go to a pipe nobody reads.** On the Ubuntu CI
runner Xvfb logs xkbcomp "Could not resolve keysym" warnings for every
client keymap: about 150 KB over one smoke-suite run, against 264 bytes
on Debian trixie. With `stderr=PIPE` and no reader, the 64 KiB pipe filled
about a dozen apps in. Xvfb then blocked in `write()`, and the whole
server stopped answering: no window for the next app, load average 0,
Xvfb "running". The suite reported it as "X server stopped answering
after <app>", and the named app moved (msgdialog, then metadyn) as the
output per app grew. It never reproduced locally. `xdisplay.Display` now
logs to an unlinked temporary file. Under `ECCE_APPS_TRACE` it prints the
file's size and tail at teardown, and it adds them to the display failure.
