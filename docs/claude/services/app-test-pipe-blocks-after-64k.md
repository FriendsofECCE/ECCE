---
type: pitfall
title: "An app test whose output pipe is not read freezes the app after 64 KiB of log"
area: services
paths: ["tests/apps/apps.py", "tests/apps/viewer_first_open_test.py"]
issues: [1249]
---
`apps.run` starts the app with stdout and stderr on one pipe. The pipe
holds 64 KiB on Linux; once it is full the app's next write to stderr
blocks its main thread, so timers, test commands (`ECCE_TEST_ORGANIZER`)
and messages all stop, with no crash and no error. A round that logs a lot
(every example set re-imported, `ECCE_TEST_MESSAGES`) then looks like a
stuck GUI: the command answers are missing and the log ends mid-stream.

`apps.run` therefore reads the pipe in a thread from the start
(`_drain`). A test that starts an app with its own `subprocess.Popen` and
`stdout=PIPE` must do the same, or write to a file. A log that ends near
65536 bytes is this, not the app.
