---
type: pitfall
title: "The queued Progress box test must account for startup time (#1096)"
area: services
section: "Pitfalls"
paths: ["tests/apps/progress_contact_test.py", "src/apps/organizer/ProgressBox.C", "src/apps/organizer/CalcMgr.C"]
issues: [1096, 606]
---

The test writes `contact <url> ok <epoch>` with an epoch 20 seconds before
starting the Organizer. `CalcMgr::lastChecked()` preserves that epoch and
`ProgressBox::age()` subtracts it from the current wall clock. Startup and
earlier screenshot commands therefore count towards the displayed age.

The original assertion accepted only 20–39 seconds. A controlled 20-second
delay before the first Organizer reproduced the sole failure with the
correct display `updated 43 s ago`; the undelayed baseline passed at 25 s.
The test now bounds the age by the Organizer invocation interval, allowing
the display's rounding and its seconds/minutes/hours units. Do not replace
this with another fixed upper limit tied to machine speed.
