---
type: pitfall
title: "Test Xvfb displays must leave :0 through :9 free"
area: services
section: "Pitfalls"
paths: ["tests/apps/xdisplay.py", "tests/apps/xdisplay_test.py", "tests/dialogs/native_test.py", "tests/transport/sshd/tail_test.sh", "tests/hosts/client-session.sh"]
issues: [1024, 983]
---
Bare `Xvfb -displayfd` can allocate `:0` when no desktop currently holds
it. A test then trips its own live-desktop safety guard, and a desktop
started later can collide with the test server. Keep those guards.

The shared harness scans `:90` through `:999`, checking both
`/tmp/.X11-unix/XN` and `/tmp/.XN-lock`. That scan races with parallel
ctest runs: the losing Xvfb exits, but an `xdpyinfo` readiness check still
succeeds against the winner's server, so two tests share one display and
the first to finish kills the other's apps (#983 merge,
`apps_organizer_one_process` failing in `runQuit`). `Display` therefore
starts Xvfb with `-displayfd` on the chosen number, counts it as its own
only once that report arrives, and on a lost race tries the next free
number. `tests/dialogs/native_test.py` still scans without that check. `ECCE_TEST_XDISPLAYS=a-b` may restrict the range,
but ranges and explicit numbers below `:10` are rejected. The native
dialog test applies the same lower bound to `ECCE_TEST_XDISPLAY_BASE`.
The sshd Tail test passes an explicitly selected high display to
`-displayfd`, using the descriptor only for readiness. `xvfb-run -a`
already starts its automatic search at `:99`; the host-container helper
uses `:90` inside its isolated container.

Run `srun -c 2 nice python3 tests/apps/xdisplay_test.py` for allocation
and override checks plus two real private Xvfb servers. It needs no ECCE
install; only the live-server check skips without Xvfb.
