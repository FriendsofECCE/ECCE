---
type: pitfall
title: "Windows broker: a retry in the same folder fails while the first mosquitto is still starting"
area: services
paths: [packaging/windows/ecce-broker-win]
issues: [958]
---
On a laptop where Windows Defender scans a new mosquitto.exe, the first broker
start can take longer than ecce-broker-win waited (20 s). The retry then ran in
the same folder while the first mosquitto.exe was still starting and held
mosquitto.log and passwd open, so attempts 2 and 3 failed with "Unable to open
log file ... for writing" / "Unable to open pwfile": it looked like a path
problem (the profile was C:\Users\Käyttäjä) but was timing. `kill` of the
background pid does not always stop the native process.

Now attempt 1 waits up to 60 s (25 s for later ones) and stops waiting if the
process has exited; each attempt has its own folder (a1, a2, a3) with its own
passwd/acl/log; a failed attempt is stopped with kill -9 and taskkill on its
Windows pid.
