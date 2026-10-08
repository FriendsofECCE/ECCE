---
type: pitfall
title: "macOS: wrappers must not bake ECCE_HOME, and there is no timeout(1) (#133)"
area: services
paths: [CMakeLists.txt, packaging/ecce-home.sh.in, packaging/ecce.in, packaging/gateway/ecce-gateway-start, tests/macos/remote-reach.sh, tests/packaging/no-build-paths.sh]
issues: [133]
---
The wrappers (`ecce`, `ecce-<app>`, `ecce-gateway-*`, ...) used to default
`ECCE_HOME` to `ECCE_HOME_DIR`, a configure-time path. The macOS CI build
sets that to the runner's staging tree, so ECCE.app shipped
`/Users/runner/work/ECCE/ECCE/stage/ecce` in 51 scripts and the systemd unit; only the app's
launcher (which exports `ECCE_HOME`) hid it. With `ECCE_RELOCATABLE` (on
for APPLE) the wrappers take `ECCE_HOME` relative to their own resolved
location (`packaging/ecce-home.sh.in`); Linux packages keep `/opt/ecce`.
CI fails any package whose installed text files name the build directory
(`tests/packaging/no-build-paths.sh`).

macOS has no `timeout(1)`. A call to it fails with 127, which in a
reachability test reads as "not answering": alpha.7's `ecce -remote`
reported a reachable central server as down on both ports for exactly
this reason. Reachability now goes through Perl's IO::Socket::IP and
reports the reason per port (refused, no route, lookup failure, timeout).
On macOS, "No route to host" to a LAN address can also be Local Network
privacy denying the program (System Settings > Privacy & Security >
Local Network), not the network.
