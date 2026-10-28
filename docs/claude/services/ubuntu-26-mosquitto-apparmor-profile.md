---
type: pitfall
title: Ubuntu 26.04's mosquitto AppArmor profile blocks the per-user broker; ecce-client adds a local block (#646)
area: services
paths: ["packaging/gateway/ecce-apparmor-mosquitto", "packaging/postinst.in", "packaging/prerm.in", "tests/apparmor/"]
issues: [646]
---
Ubuntu 26.04's `mosquitto` ships `/etc/apparmor.d/mosquitto`, attached to
`/usr/sbin/mosquitto`, allowing only the system service's files. The
per-user broker of `ecce-gateway-start` (config, log, pid, Unix socket in
`~/.ECCE`, or `$XDG_RUNTIME_DIR|/tmp/ecce-<uid>-<cksum>/` for long paths)
is denied at the first `open` of `mosquitto.conf`, and the start ends with
"broker did not come up within 10s". Debian has no such profile; nothing
shows in the broker's own log (the denial is in `dmesg`/audit).

Fix: the profile ends with `include if exists <local/mosquitto>`.
`packaging/gateway/ecce-apparmor-mosquitto install` (called from the
client postinst, not run if `/etc/apparmor.d/mosquitto` is absent) puts a
block between `# BEGIN ECCE (ecce-client)` / `# END ECCE` into
`/etc/apparmor.d/local/mosquitto`, replacing an older copy, and runs
`apparmor_parser -r` when AppArmor is enabled. `remove` (prerm, on
remove only) deletes the block and leaves the site's lines. Nothing here
may fail the install. The state-dir rules are anchored at `@{HOME}/.ECCE`,
so an `ECCE_REALUSERHOME` elsewhere is not covered. RPM distros use
SELinux; no profile there, nothing done.

Also covered, untested under AppArmor: the central server's
`ecce_users_auth.so` plugin and ACL under `$ECCE_HOME/server`, and the
shared broker's `/var/lib/ecce-broker` (a DynamicUser state dir resolves
to `/var/lib/private/ecce-broker`).

Test: `tests/apparmor/run_tests.sh` (block script) and
`tests/apparmor/live_broker.sh` (CI job `apparmor` on `ubuntu-26.04`:
broker fails without the block, then relays a message with it).
