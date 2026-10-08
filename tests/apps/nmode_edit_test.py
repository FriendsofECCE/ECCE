#!/usr/bin/env python3
"""The Normal Modes panel on a structure edited after the calculation.

    tests/apps/nmode_edit_test.py            own Xvfb and services
    tests/apps/nmode_edit_test.py --gdb      backtrace if the builder dies

Run by tests/apps/run_tests.py as well.  Each case opens a Builder on the
water frequency calculation (calc-water-vib, with its normal modes), runs a
scene script (ECCE_VIEWER_SCENE: the Builder's "pbc...", "cmd" and "vib..."
commands) and reads the "VIBSTATE:" lines the script prints.

Water's modes have three atoms.  Once the structure in the viewer is no
longer water (a supercell, an atom added), the panel must neither animate
nor draw arrows, must leave the edited structure alone, and must say why;
on the unedited water it works as before.
"""
import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                                                   # noqa: E402
import fixture                                                # noqa: E402

FIXTURE = "nmode-edit-water"
BASE = os.path.join(HERE, "fixtures", "calc-water-vib")
MARKERS = ("ended by SIG", "Segmentation fault", "double free or corruption",
           "terminate called", "Unhandled standard exception",
           "Unhandled unknown exception")

#  Everything a user can do with the panel, then what it shows.
USE = ["vibfocus", "vibanim", "vibmode 1", "vibstep 20", "vibplay 300",
       "vibvector", "vibmode 2", "vibstate", "vibanim", "vibstep 5",
       "vibunfocus", "vibfocus", "vibunfocus", "vibstate"]
SUPERCELL = ["pbcopen", "pbcpress create", "pbcpress generate",
             "pbcpress replicate 2", "pbcpress super"]

#  (name, script, (atoms, arrows, enabled) at the first vibstate, and the
#  number of atoms at the end)
CASES = (
    ("unedited", USE, (3, 3, 1), 3),
    ("supercell", SUPERCELL + USE, (24, 0, 0), 24),
    ("added-atom", ["cmd add"] + USE, (8, 0, 0), 8),
)


def states(log):
    """[(atoms, arrows, enabled)] from the VIBSTATE lines."""
    return [tuple(int(v) for v in m)
            for m in re.findall(r"VIBSTATE: atoms=(\d+) arrows=(-?\d+) "
                                r"enabled=(\d)", log)]


def runCase(display, results, case, args, timeout):
    name, script, first, atomsAtEnd = case
    what = "nmode edit " + name
    work = os.path.join(fixture.stateHome(), "nmode-edit", name)
    if os.path.exists(work):
        shutil.rmtree(work)
    os.makedirs(work)
    path = os.path.join(work, "scene")
    with open(path, "w") as handle:
        handle.write("\n".join(script + [""]))
    result = apps.run(display, "builder", args=args, windowTimeout=120,
                      settle=timeout,
                      env={"ECCE_VIEWER_SCENE": path,
                           "ECCE_VIEWER_SCENE_OUT": work,
                           "ECCE_REALUSER": fixture.USER})
    log = result.log or ""
    tail = "\n".join("      | " + l for l in log.splitlines()[-25:])
    found = [m for m in MARKERS if m in log]
    failed = os.path.join(work, "FAILED")
    got = states(log)
    if result.crashed:
        results.fail(what, "builder CRASHED (%s)\n%s"
                     % (result.signalName or result.returncode, tail))
    elif os.path.exists(failed):
        with open(failed) as handle:
            results.fail(what, "the script stopped: %s\n%s"
                         % (handle.read().strip(), tail))
    elif not result.exitedAfterWindow:
        results.fail(what, "builder did not finish the script within %ds\n%s"
                     % (timeout, tail))
    elif found:
        results.fail(what, "its output contains %s\n%s"
                     % (", ".join(repr(m) for m in found), tail))
    elif len(got) != 2:
        results.fail(what, "%d VIBSTATE lines, expected 2\n%s"
                     % (len(got), tail))
    else:
        if got[0] != first:
            results.fail(what, "atoms/arrows/enabled %s, expected %s"
                         % (got[0], first))
        if got[1][0] != atomsAtEnd:
            results.fail(what, "%d atoms at the end, expected %d: the panel "
                         "changed the structure" % (got[1][0], atomsAtEnd))
    if os.environ.get("NMODEEDIT_LOG"):
        with open(os.environ["NMODEEDIT_LOG"] + "." + name, "w") as h:
            h.write(log)


def check(display, results, verbose=False, timeout=120, only=None):
    """Run every case; record failures in results."""
    url, error = fixture.install(FIXTURE, source_dir=BASE)
    if error:
        results.checks += 1
        results.fail("nmode edit", "could not install %s: %s"
                     % (FIXTURE, error))
        return
    try:
        args = ("-context", url)
        state = fixture.stateHome()
        if (os.path.realpath(state)
                != os.path.realpath(os.path.expanduser("~"))):
            auth = fixture.authFile(os.path.join(state, ".ECCE", "auth.pipe"),
                                    user=fixture.USER)
            args = ("-pipe", auth) + args
        for case in CASES:
            if only and not re.search(only, case[0]):
                continue
            results.checks += 1
            runCase(display, results, case, args, timeout)
    finally:
        fixture.remove(FIXTURE)


class _Results(object):
    def __init__(self):
        self.failures, self.notes, self.checks = [], [], 0

    def fail(self, where, message):
        self.failures.append("%s: %s" % (where, message))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gdb", action="store_true",
                        help="print a backtrace if the builder crashes")
    parser.add_argument("--only", help="run the cases matching this regex")
    parser.add_argument("--timeout", type=int, default=120)
    args = parser.parse_args()

    import isolate
    import xdisplay
    from geomtrace_stress import _wrapBuilder

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"
    if args.gdb:
        _wrapBuilder(settings["ECCE_HOME"],
                     os.environ.get("NMODEEDIT_GDB", "gdb -q -batch -ex run -ex bt -ex quit --args"))

    results = _Results()
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % log)
                return 1
            fixture.ensureRealUserAccount()
            restore = fixture.settleUpgradeNotices()
            try:
                check(display, results, timeout=args.timeout, only=args.only)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for failure in results.failures:
        print("FAIL  " + failure)
    if not results.failures:
        print("PASS  nmode edit (%d cases)" % results.checks)
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
