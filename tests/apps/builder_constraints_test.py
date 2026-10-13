#!/usr/bin/env python3
"""
Builder: Geometry Constraints (#306).  Constraints belong to the structure and
are defined in the Builder at any run type; picked in the wrong order they are
reordered by the bonding; the Calculation Editor's input uses them only for
run types that can.

Local data mode, NH3 with a dummy atom X bonded to N in NWChem calculations
(run types Energy and Geometry) made by tests/filedsi/resourceTest "nwchem".
ECCE_BUILDER_SCRIPT "constraints ..." drives the dialog's own code (no
synthetic mouse input).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402

failures = []
DASH = "–"


def check(ok, what, detail=""):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
        if detail:
            print(detail)


def lines(log, tag):
    return re.findall(r"BUILDER: %s(.*)" % tag, log)


def deck(calcdir):
    inputs = os.path.join(calcdir, "Inputs")
    if not os.path.isdir(inputs):
        return None
    names = [f for f in os.listdir(inputs) if f.endswith((".nw", ".nwin"))]
    return open(os.path.join(inputs, names[0]), errors="replace").read() \
        if names else None


def energyCase(display, url):
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "constraints show",
        "constraints select 0 4 1",           # N X H: centre picked first
        "constraints press constrain no",
        "constraints select 1 2 3",           # H H H: no centre
        "constraints press constrain no",
        "constraints select 1 2",             # H H: not bonded
        "constraints press constrain no",
        "constraints select 1 2 3",
        "constraints press constrain yes",    # confirmed: added as picked
        "constraints select 0",
        "constraints press fix yes",          # NWChem: no fixed atoms with these
        "constraints list", "savestate", "save", "wait 2500", "savestate",
        "constraints stored", "wait 500", "quit"], url)
    tail = run_tests._tail(log, 25)
    pick = re.findall(r"BUILDER: constraints pick: (.*?)ECCE_BUILDER_SCRIPT", log, re.S)
    check(len(pick) >= 4, "four selections reported (%d)" % len(pick), tail)
    if len(pick) < 4:
        return
    check(("centre N1)" in pick[0]) and "N1" in pick[0] and "X5" in pick[0],
          "N-X-H picked centre-first reads as an angle about N: %s" % pick[0], tail)
    check("not form an angle" in pick[1] or "centre atom" in pick[1],
          "H-H-H says it is not an angle: %s" % pick[1], tail)
    check("not bonded" in pick[2], "H-H says it is not a bond: %s" % pick[2], tail)
    res = lines(log, r"constraints constrain:")
    check(len(res) == 4 and res[0].startswith(" Added Angle") and "centre N1" in res[0],
          "the reordered angle was added: %s" % (res[0] if res else "?"), tail)
    check(res[1].startswith(" Not added") and res[2].startswith(" Not added"),
          "unconfirmed odd picks are not added", tail)
    check(res[3].startswith(" Added Angle H2" + DASH + "H3"),
          "a confirmed odd pick is added in the picked order: %s" % res[3], tail)
    fix = lines(log, r"constraints fix:")
    check(len(fix) == 1 and "Not fixed" in fix[0],
          "NWChem refuses fixed atoms beside angle constraints: %s" % (fix[0] if fix else "?"), tail)
    m = re.search(r"constraints rows=(\d+) note: (.*)", log)
    check(m is not None and m.group(1) == "2" and
          "used only by geometry optimisations" in m.group(2) and "Energy" in m.group(2),
          "the note says the constraints are not used by an Energy run: %s"
          % (m.group(2) if m else "?"), tail)
    st = states(log)
    check(len(st) == 2 and st[0][0] == "1" and st[1][0] == "0",
          "defining constraints marks the structure unsaved; Save clears it %s" % (st,), tail)
    m = re.search(r"constraints stored=(-?\d+)", log)
    check(m is not None and m.group(1) == "2",
          "Save stored the constraints with the calculation (%s)" % (m.group(1) if m else "?"), tail)


def states(log):
    return re.findall(r"BUILDER: dirty=(\d) save_enabled=(\d) toolbar_save_enabled=(\d)", log)


def reopenCase(display, url, calcdir, runtype):
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT",
                       ["wait 4000", "constraints show", "constraints list", "wait 500", "quit"], url)
    m = re.search(r"constraints rows=(\d+) note: (.*)", log)
    tail = run_tests._tail(log, 20)
    return m, tail


def deckCase(display, url, calcdir, expectZcoord, label):
    log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                       ["wait 3000", "ready", "basis 6-31G", "wait 1500", "info",
                        "button save", "wait 6000", "constraints", "quit"], url)
    d = deck(calcdir)
    check(d is not None, "%s: the Calculation Editor wrote an input deck" % label,
          " ".join(sorted(os.listdir(os.path.join(calcdir, "Inputs")))) + "\n" +
          "\n".join(l for l in log.splitlines() if re.search(
              r"ECCE_CALCED_SCRIPT|FEEDBACK|notready|CALCED: theory", l)))
    if d is None:
        return
    m = re.search(r"CALCED: constraints button=(\d) count=(\d+) note=(.*)", log)
    want = ("" if expectZcoord is None else "")
    if expectZcoord:
        check(m is not None and m.group(1) == "1" and m.group(2) == "1" and
              m.group(3).strip() == "1 constraint defined",
              "%s: the editor notes the constraint, no restriction: %s" %
              (label, m.group(3) if m else "?"), run_tests._tail(log, 20))
        check(not re.search(r"CALCED: verify (UNSURE|other)", log),
              "%s: Verify has nothing to add: %s" % (label, re.findall(r"CALCED: verify.*", log)))
    else:
        check(m is not None and m.group(1) == "1" and m.group(2) == "2" and
              m.group(3).strip() == "2 constraints defined: used only by geometry optimisations",
              "%s: the editor says the constraints are used only by geometry "
              "optimisations: %s" % (label, m.group(3) if m else "?"), run_tests._tail(log, 20))
        v = re.search(r"CALCED: verify UNSURE: (.*)", log)
        check(v is not None and "left out of this input" in v.group(1),
              "%s: Verify notes it: %s" % (label, v.group(1) if v else "?"))
    has = "zcoord" in d and "constant" in d
    check(has == expectZcoord, "%s: deck %s the constraints" %
          (label, "holds" if expectZcoord else "leaves out"), d[:1500])


def geomCase(display, url):
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "constraints show",
        "constraints select 4 0 1", "constraints press constrain no",
        "constraints list", "save", "wait 2500", "constraints stored", "wait 500", "quit"], url)
    tail = run_tests._tail(log, 25)
    m = re.search(r"constraints rows=(\d+) note: (.*)", log)
    check(m is not None and m.group(1) == "1" and "used by this calculation" in m.group(2),
          "Geometry run type: the note says the constraints are used: %s"
          % (m.group(2) if m else "?"), tail)


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "builderconstraints-localdata")
        problem = sym.makeCalculations(folder, ("nwchem",))
        if problem:
            results.fail("builder-constraints", "calculations not made:\n" + problem)
            return
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            base = os.path.join(folder, "users", "local", "proj-nw")
            energy = os.path.join(base, "nh3-energy")
            geom = os.path.join(base, "nh3-geom")
            energyCase(display, "file://" + energy + "/")
            m, tail = reopenCase(display, "file://" + energy + "/", energy, "Energy")
            check(m is not None and m.group(1) == "2",
                  "reopened: the constraints are still there (%s)" % (m.group(1) if m else "?"), tail)
            deckCase(display, "file://" + energy + "/", energy, False,
                     "Energy run")
            geomCase(display, "file://" + geom + "/")
            deckCase(display, "file://" + geom + "/", geom, True, "Geometry run")
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if failures:
            results.fail("builder-constraints", "%d wrong: %s"
                         % (len(failures), "; ".join(failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
