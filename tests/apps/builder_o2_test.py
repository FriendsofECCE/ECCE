#!/usr/bin/env python3
"""Builder: two O atoms bond, and a double-click adds one atom (#401).

    tests/apps/builder_o2_test.py            own Xvfb and services

Headless, through ECCE_BUILDER_SCRIPT on a calculation (pbc_edit_test's
harness), with the commands the Build panel and the viewer run: element O
(default shape Bent) in Atom mode, a click in free space, then a click on
the first O's open site or on the first O itself; "dclick" replays the
viewer's two presses of a double-click on an atom.
"""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                       # noqa: E402
import fixture                    # noqa: E402
import pbc_edit_test as pe        # noqa: E402

START = ["wait 2000", "cmd clear", "pertab O", "mode atom", "select -1",
         "click", "info"]

CASES = (
    #  A click on an open site of the first O.
    ("nub", START + ["selectnub O", "click", "info", "bonds O O"]),
    #  A click on the first O itself (same element and shape: bond a new O
    #  to it), then the new O double-clicked.
    ("atom", START + ["selectelem O", "click", "info", "bonds O O",
                      "dclick O", "info", "bonds O O"]),
    #  Add H on a lone O: the view zooms out if the water leaves it (#409).
    ("addh", START + ["zoom 0.45", "cmd addh", "info", "inview"]),
    #  Tools > Layer Builder is a pane of its own (#401).
    ("layerpane", ["wait 2000", "panel Layer Builder",
                   "panesize Layer Builder"]),
)


def counts(log):
    return [(int(a), int(n)) for a, n in
            re.findall(r"BUILDER: atoms=(\d+) lattice=\w+ nubs=(\d+)", log)]


def bonds(log):
    return [int(n) for n in re.findall(r"BUILDER: bonds O-O=(\d+)", log)]


def problems(name, log):
    n, b = counts(log), bonds(log)
    found = []
    if name in ("nub", "atom"):
        if not n or n[0] != (3, 2):
            found.append("first click: atoms, nubs %s, expected O + 2 nubs"
                         % (n[:1],))
        elif len(n) < 2 or n[1][0] - n[1][1] != 2 or b[:1] != [1]:
            found.append("second click: atoms, nubs %s, O-O bonds %s; "
                         "expected two bonded O" % (n[1:2], b[:1]))
    #  The first press of a double-click is a click (a third O on the
    #  second); the second press must add nothing more.
    if name == "atom" and (len(n) < 3 or n[2][0] - n[2][1] != 3):
        found.append("double-click on an O: atoms, nubs %s -> %s; "
                     "expected exactly one more O" % (n[1:2], n[2:3]))
    if name == "addh":
        if not n or n[-1][0] - n[-1][1] != 3:
            found.append("Add H on O: atoms, nubs %s; expected water" % (n[-1:],))
        if not re.search(r"BUILDER: inview outside=0", log):
            found.append("after Add H an atom is outside the view (#409)")
    if name == "layerpane" and not re.search(
            r"PANESIZE Layer Builder: shown", log):
        found.append("Tools > Layer Builder did not show its pane")
    return found


def runCase(display, results, case, url, args, verbose, timeout):
    name, script = case[0], case[1]
    work = os.path.join(fixture.stateHome(), "builder-o2", name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    path = os.path.join(work, "o2.script")
    with open(path, "w") as handle:
        handle.write("\n".join(script + ["quit"]) + "\n")
    result = apps.run(display, "builder", args=args, windowTimeout=120,
                      settle=timeout,
                      env={"ECCE_BUILDER_SCRIPT": path,
                           "ECCE_REALUSER": fixture.USER})
    log = result.log or ""
    if os.environ.get("BUILDER_O2_LOG"):
        with open(os.environ["BUILDER_O2_LOG"] + "." + name, "w") as h:
            h.write(log)
    tail = "\n".join("      | " + l for l in log.splitlines()[-20:])
    if result.crashed:
        results.fail("builder O2 " + name, "builder CRASHED\n" + tail)
        return
    found = problems(name, log)
    for p in found:
        results.fail("builder O2 " + name, p + "\n" + tail)
    if not found:
        print("ok    builder O2 %s" % name)


def main():
    pe.CASES = CASES
    pe.runCase = runCase
    pe.FIXTURE = "builder-o2-water"
    return pe.main()


if __name__ == "__main__":
    sys.exit(main())
