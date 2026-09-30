#!/usr/bin/env python3
"""Submit real jobs through Launch and assert they reach "completed".

Nothing else in the tree runs Launch, gensub, eccejobmaster, eccejobstore
and eccejobmonitor together.  This starts an isolated data server and
broker (tests/apps/isolate.py), creates a calculation on it, launches it on
the `localhost` machine under the Shell queue manager with the tree's own
`launchjob` driver, waits for the run state to become "completed", and
checks that the properties reached the calculation's Props/ collection.
Everything under test comes from the build and source trees, never from an
installed /opt/ecce.

    tests/launch/run_tests.py [--build build] [--nwchem-restart] [--keep]

Default: one MOPAC job.  --nwchem-restart: run an NWChem optimisation, Reset
for Restart, run it again (#202).  Exit status 77 (CTest SKIP) when a
prerequisite is missing.
"""

import argparse
import os
import shutil
import sys
import time

import harness
from harness import REPO, Session, say

DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")
WAIT_SECONDS = 120


def mopacJob(s):
    say("--- MOPAC energy")
    name = "mopac-ch4-%d" % int(time.time())
    rundir = os.path.join(s.state, "jobs")
    os.makedirs(rundir, exist_ok=True)
    url, out = s.create(name, "mopac_es", DECK, "mopac.mop", rundir)
    if not s.check(url is not None, "calculation created"):
        say(out)
        return
    rc, out = s.launch(url)
    say("\n".join("  | " + l for l in out.strip().splitlines()[-6:]))
    if not s.check(rc == 0, "Launch ran to the end"):
        return
    state = s.waitState(url, WAIT_SECONDS)
    s.check(state == "completed", "run state reached completed (last: %s)" % state)
    time.sleep(2)
    props = s.props(url)
    say("  properties: " + " ".join(props))
    for prop in ("TE", "GEOMTRACE"):
        s.check(prop in props, "%s present in Props/" % prop)


def nwchemRestart(s):
    """#202: the restarted job must store its properties without
    eccejobstore aborting, and must not lose the ones the first run left."""
    say("--- NWChem optimisation, then Reset for Restart")
    name = "nwchem-co-restart-%d" % int(time.time())
    rundir = os.path.join(s.state, "jobs")
    os.makedirs(rundir, exist_ok=True)
    deck = os.path.join(REPO, "tests", "e2e", "fixtures", "nwchem", "co-opt.nw")
    url, out = s.create(name, "nwchem_es", deck, "nwch.nw", rundir)
    if not s.check(url is not None, "calculation created"):
        say(out)
        return
    rc, out = s.launch(url)
    if not s.check(rc == 0, "first Launch ran to the end"):
        return
    state = s.waitState(url, 180)
    s.check(state == "completed", "first run reached completed (last: %s)" % state)
    time.sleep(3)
    first = s.props(url)
    s.check("GEOMTRACE" in first and "TE" in first, "first run stored GEOMTRACE and TE")

    restartDeck = os.path.join(s.state, "restart.nw")
    with open(deck) as src, open(restartDeck, "w") as dst:
        for line in src:
            dst.write("restart CO\n" if line.strip().lower().startswith("start")
                      else line)
    rc, out = s.driver("restart", url, restartDeck, "nwch.nw")
    if not s.check(rc == 0, "reset for restart and edited deck stored"):
        say(out)
        return
    rc, out = s.launch(url)
    if not s.check(rc == 0, "restart Launch ran to the end"):
        return
    state = s.waitState(url, 180)
    s.check(state == "completed", "restarted run reached completed (last: %s)" % state)
    time.sleep(3)
    second = s.props(url)
    s.check(set(first) <= set(second), "the restart kept the first run's "
            "properties (%s)" % " ".join(sorted(set(first) - set(second))))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build"))
    ap.add_argument("--nwchem-restart", action="store_true")
    ap.add_argument("--transport", default="unset",
                    choices=("unset", "direct", "ssh"),
                    help="ECCE_TRANSPORT for the ECCE processes started "
                    "(main has only the default path; passed through as is)")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()

    harness.prerequisites(os.path.abspath(args.build),
                          ("nwchem",) if args.nwchem_restart else ("mopac",))
    s = Session(args.build, "launch",
                {"MOPAC": shutil.which("mopac") or "",
                 "NWChem": shutil.which("nwchem") or ""}, (8396, 8388),
                keep=args.keep, transport=args.transport)
    try:
        if not s.services(True):
            s.check(False, "services started")
        elif args.nwchem_restart:
            nwchemRestart(s)
        else:
            mopacJob(s)
        for name, path in sorted(s.seen.items()):
            s.check(os.path.dirname(path) == s.build,
                    "ran while the job was alive: %s" % path)
    finally:
        left = s.stop()
        if left:
            s.failures.append("processes left running: %r" % left)
    say("")
    if s.failures:
        say("FAILED (%d): %s" % (len(s.failures), "; ".join(s.failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
