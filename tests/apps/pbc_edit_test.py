#!/usr/bin/env python3
"""Editing a periodic structure in the Builder, headless.

    tests/apps/pbc_edit_test.py            own Xvfb and services
    tests/apps/pbc_edit_test.py --gdb      backtrace if the builder dies

Run by tests/apps/run_tests.py as well.  Each case opens a Builder on a
water calculation and runs a scene script (ECCE_VIEWER_SCENE, the Builder's
"pbc...", "cmd" and "fragdump" commands) that presses the Periodic Builder's
buttons and the Builder's editing commands in a given order, then writes the
structure out.  The first case is the one reported: a carbon with four
open valences (nubs), Create lattice, Generate, Add Hydrogens.

Passing means the Builder ran the script and exited by itself, no nub was
left without its parent atom, and the result has the expected atoms with
every hydrogen bonded at a C-H/O-H distance next to its parent.
"""
import argparse
import math
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                                                   # noqa: E402
import fixture                                                # noqa: E402

FIXTURE = "pbc-edit-water"
#  The water calculation without its properties: a property panel (normal
#  modes) would step the edited structure with water's three atoms.
BASE = os.path.join(HERE, "fixtures", "calc-water-vib")
MARKERS = ("ended by SIG", "Segmentation fault", "double free or corruption",
           "terminate called", "Unhandled standard exception",
           "Unhandled unknown exception")

CARBON = ["cmd clear", "cmd add", "pbcopen", "pbcpress create"]

#  (name, script, {element: count} expected at the end, heavy element)
CASES = (
    ("generate-addh", CARBON + ["pbcpress generate", "fragdump generated",
                                "cmd addh"], {"C": 1, "H": 4}, "C"),
    ("addh-generate", CARBON + ["cmd addh", "pbcpress generate"],
     {"C": 1, "H": 4}, "C"),
    ("supercell-addh", CARBON + ["pbcpress generate", "pbcpress replicate 2",
                                 "pbcpress super", "fragdump super",
                                 "cmd addh"], {"C": 8, "H": 32}, "C"),
    ("fold-addh", CARBON + ["pbcpress generate", "pbcpress fold",
                            "fragdump folded", "cmd addh"],
     {"C": 1, "H": 4}, "C"),
    #  Generate rebuilds the atoms and drops the selection: no stale indices.
    ("selection-generate", CARBON + ["select 2", "pbcpress generate",
                                     "cmd addh"], {"C": 1, "H": 4}, "C"),
    #  Methane in a 5 A cell, Generate, then a bigger cell with "keep atom
    #  positions" ticked: the molecule must still be whole afterwards.
    ("resize-methane", ["cmd clear", "cmd add", "cmd addh", "pbcopen",
                        "pbcpress create", "pbcpress generate",
                        "fragdump generated", "pbcset a 12", "pbcset b 10",
                        "pbcset c 10"], {"C": 1, "H": 4}, "C"),
    ("fold-methane", ["cmd clear", "cmd add", "cmd addh", "pbcopen",
                      "pbcpress create", "pbcpress generate", "pbcset a 12",
                      "pbcset b 10", "pbcset c 10", "pbcpress fold"],
     {"C": 1, "H": 4}, "C"),
    ("molecule", ["cmd removeh", "pbcopen", "pbcpress create",
                  "pbcpress generate", "fragdump generated", "cmd addh"],
     {"O": 1, "H": 2}, "O"),
)


def readDump(path):
    """{'counts': {...}, 'atoms': [(sym, xyz, nbonds, frac)]}"""
    counts, atoms, vectors = {}, [], []
    with open(path) as handle:
        for line in handle:
            w = line.split()
            if not w:
                continue
            if w[0] == "atom":
                frac = [float(v) for v in w[8:11]] if "frac" in w else None
                atoms.append((w[1], [float(v) for v in w[2:5]], int(w[6]),
                              frac))
            elif w[0] == "vector":
                vectors.append([float(v) for v in w[1:4]])
            elif len(w) == 2:
                counts[w[0]] = int(w[1])
    return {"counts": counts, "atoms": atoms, "vectors": vectors}


def imageDistance(p, q, vectors):
    """Shortest distance from p to any lattice image of q."""
    best = math.dist(p, q)
    shifts = (-1, 0, 1)
    for i in shifts:
        for j in shifts:
            for k in shifts:
                t = [q[m] + i * vectors[0][m] + j * vectors[1][m]
                     + k * vectors[2][m] for m in range(3)]
                best = min(best, math.dist(p, t))
    return best


def problems(work, case):
    """What is wrong with the dumps of one finished case."""
    name, script, expected, heavy = case
    found = []
    for line in script + ["fragdump final"]:
        if line.startswith("fragdump "):
            dump = readDump(os.path.join(work, line.split()[1] + ".txt"))
            if dump["counts"].get("orphanNubs", -1) != 0:
                found.append("%s: %s nub(s) without a parent atom"
                             % (line.split()[1],
                                dump["counts"].get("orphanNubs")))
    final = readDump(os.path.join(work, "final.txt"))
    have = {}
    for sym, _xyz, _nb, _f in final["atoms"]:
        have[sym] = have.get(sym, 0) + 1
    if final["counts"].get("lattice") != 1:
        found.append("final: no lattice")
    for sym, n in sorted(expected.items()):
        if have.get(sym, 0) != n:
            found.append("final: %d %s, expected %d" % (have.get(sym, 0),
                                                        sym, n))
    if have.get("Nub", 0) != expected.get("Nub", 0):
        found.append("final: %d nub(s) left after Add Hydrogens"
                     % have.get("Nub", 0))
    if name.endswith("-methane"):
        lengths = sorted(round(math.dist([0, 0, 0], v), 2)
                         for v in final["vectors"])
        if lengths != [10.0, 10.0, 12.0]:
            found.append("final: cell edges %s, expected 10 10 12" % lengths)
    parents = [xyz for sym, xyz, _nb, _f in final["atoms"] if sym == heavy]
    #  Generate puts every atom into the cell, so a hydrogen can sit at the
    #  far face from its parent: a periodic image, drawn without its bond.
    for sym, xyz, nb, _f in final["atoms"]:
        if sym != "H":
            continue
        direct = min(math.dist(xyz, p) for p in parents) if parents else 99
        d = min(imageDistance(xyz, p, final["vectors"]) for p in parents) \
            if parents else 99
        if name.endswith("-methane") and not 1.0 <= direct <= 1.2:
            found.append("final: H at %s is %.2f A from C: molecule split"
                         % (xyz, direct))
        if not 0.9 <= d <= 1.2 or (direct <= 1.2 and nb != 1):
            found.append("final: H at %s has %d bond(s), %.2f A from the "
                         "nearest %s image" % (xyz, nb, d, heavy))
    return found


def runCase(display, results, case, url, args, verbose, timeout):
    name, script, _expected, _heavy = case
    what = "pbc edit " + name
    work = os.path.join(fixture.stateHome(), "pbc-edit", name)
    if os.path.exists(work):
        shutil.rmtree(work)
    os.makedirs(work)
    path = os.path.join(work, "scene")
    with open(path, "w") as handle:
        handle.write("\n".join(script + ["fragdump final", ""]))
    result = apps.run(display, "builder", args=args, windowTimeout=120,
                      settle=timeout,
                      env={"ECCE_VIEWER_SCENE": path,
                           "ECCE_VIEWER_SCENE_OUT": work,
                           "ECCE_REALUSER": fixture.USER})
    log = result.log or ""
    tail = "\n".join("      | " + l for l in log.splitlines()[-25:])
    found = [m for m in MARKERS if m in log]
    failed = os.path.join(work, "FAILED")
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
    elif not os.path.exists(os.path.join(work, "final.txt")):
        results.fail(what, "no final structure was written\n" + tail)
    else:
        for p in problems(work, case):
            results.fail(what, p)
        if verbose:
            final = readDump(os.path.join(work, "final.txt"))
            results.notes.append("%-16s %s" % (name, " ".join(
                "%s=%s" % kv for kv in sorted(final["counts"].items()))))
    if os.environ.get("PBCEDIT_LOG"):
        with open(os.environ["PBCEDIT_LOG"] + "." + name, "w") as h:
            h.write(log)
        for f in os.listdir(work):
            if f.endswith(".txt"):
                shutil.copy(os.path.join(work, f), "%s.%s.%s"
                            % (os.environ["PBCEDIT_LOG"], name, f))


def check(display, results, verbose=False, timeout=120, only=None):
    """Run every case; record failures in results."""
    calc = os.path.join(fixture.stateHome(), "pbc-edit", FIXTURE)
    if os.path.exists(calc):
        shutil.rmtree(calc)
    shutil.copytree(BASE, calc)
    shutil.rmtree(os.path.join(calc, "Props"))
    os.makedirs(os.path.join(calc, "Props"))
    url, error = fixture.install(FIXTURE, source_dir=calc)
    if error:
        results.checks += 1
        results.fail("pbc edit", "could not install %s: %s" % (FIXTURE, error))
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
            runCase(display, results, case, url, args, verbose, timeout)
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
    parser.add_argument("-v", "--verbose", action="store_true")
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
                     "gdb -q -batch -ex run -ex bt -ex quit --args")

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
                check(display, results, verbose=args.verbose,
                      timeout=args.timeout, only=args.only)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for note in results.notes:
        print("  " + note)
    for failure in results.failures:
        print("FAIL  " + failure)
    if not results.failures:
        print("PASS  pbc edit (%d cases)" % results.checks)
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
