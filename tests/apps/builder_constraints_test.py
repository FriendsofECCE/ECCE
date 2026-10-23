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
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
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
          "not used by an Energy run" in m.group(2),
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
    m = re.search(r"CALCED: constraints count=(\d+) note=(.*)", log)
    if expectZcoord:
        check(m is not None and m.group(1) == "1" and
              m.group(2).strip() == "1 constraint, used by this Geometry run",
              "%s: the editor holds the constraint and says it is used: %s" %
              (label, m.group(2) if m else "?"), run_tests._tail(log, 20))
        check(not re.search(r"CALCED: verify (UNSURE|other)", log),
              "%s: Verify has nothing to add: %s" % (label, re.findall(r"CALCED: verify.*", log)))
    else:
        check(m is not None and m.group(1) == "2" and
              m.group(2).strip() == "2 constraints, not used by an Energy run",
              "%s: the editor warns the run type ignores the constraints: %s" %
              (label, m.group(2) if m else "?"), run_tests._tail(log, 20))
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


def qmGeoVibCase(display, project):
    """ECCE-QM GeoVib: the dialog says the same as the editor's line."""
    calc = os.path.join(project, "qm-geovib")
    shutil.copytree(os.path.join(REPO, "tests", "ecceqm", "fixtures", "h2o-geovib"),
                    calc, symlinks=True)
    meta = os.path.join(calc, ".ecce-meta")
    text = open(meta, errors="surrogateescape").read()
    open(meta, "w", errors="surrogateescape").write(
        re.sub(r"(ecce:state\t\t)[^\n]*", r"\g<1>Ready", text))
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT",
                       ["wait 4000", "constraints show", "constraints select 0 1",
                        "constraints press constrain no", "constraints list",
                        "wait 500", "quit"], "file://" + calc + "/")
    m = re.search(r"constraints rows=(\d+) note: (.*)", log)
    check(m is not None and m.group(1) == "1" and
          "not used by a GeoVib run (frequencies need an unconstrained minimum)"
          in m.group(2),
          "ECCE-QM GeoVib: the dialog says the constraint is not used: %s"
          % (m.group(2) if m else "?"), run_tests._tail(log, 25))


def clickCase(display, url):
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000",
        "atomclick plain 2", "atomclick shift 3", "atomclick plain 2",
        "atomclick ctrl 3", "atomclick ctrl 4", "atomclick ctrl 3",
        "wait 500", "quit"], url)
    tail = run_tests._tail(log, 25)
    sel = re.findall(r"BUILDER: selected after (\w+) click on (\d+):([ \d]*)", log)
    got = [(m[0], m[1], m[2].split()) for m in sel]
    check(len(got) == 6, "six clicks reported (%d)" % len(got), tail)
    if len(got) != 6:
        return
    check(got[1][2] == ["2", "3"], "Shift+click adds atom 3: %s" % got[1][2], tail)
    check(got[2][2] == ["2"], "a plain click selects only the atom clicked: %s" % got[2][2], tail)
    check(got[3][2] == ["2", "3"], "Ctrl+click adds atom 3, as Shift+click does: %s" % got[3][2], tail)
    check(got[4][2] == ["2", "3", "4"], "Ctrl+click adds atom 4 in pick order: %s" % got[4][2], tail)
    check(got[5][2] == ["2", "4"], "Ctrl+click on a selected atom takes it out again: %s" % got[5][2], tail)


def dummyCase(display, url, calcdir):
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "add N Tetrahedral 0 0 0", "wait 500", "mode atom",
        "element H", "selectnub N", "click", "selectnub N", "click",
        "selectnub N", "click",
        "element X", "selectnub N", "click",
        "coords", "bonds N X", "bonds N H", "bonds N Nub",
        "cmd clean", "coords", "symmetry", "save", "wait 3000", "quit"], url)
    tail = run_tests._tail(log, 30)
    if os.environ.get("CONSTRAINTS_TEST_LOG"):
        open(os.environ["CONSTRAINTS_TEST_LOG"], "w").write(log)
    atoms = re.findall(r"BUILDER: atom (\d+) (\w+) (\S+) (\S+) (\S+)", log)
    first = [a for a in atoms if int(a[0]) <= 5]
    syms = [a[1] for a in atoms[:5]]
    check(syms == ["N", "H", "H", "H", "X"], "N, three H and a dummy X at the free site: %s" % syms, tail)
    if syms != ["N", "H", "H", "H", "X"]:
        return
    def xyz(a):
        return [float(v) for v in a[2:5]]
    import math
    n, x = xyz(atoms[0]), xyz(atoms[4])
    d = math.dist(n, x)
    check(0.8 <= d <= 1.6, "X-N distance is a bond length: %.3f" % d, tail)
    hs = [xyz(a) for a in atoms[1:4]]
    cen = [sum(h[i] for h in hs) / 3 for i in range(3)]
    axis = [x[i] - n[i] for i in range(3)]
    toH = [cen[i] - n[i] for i in range(3)]
    cosang = sum(a * b for a, b in zip(axis, toH)) / (math.hypot(*axis) * math.hypot(*toH))
    check(cosang < -0.9, "X sits opposite the three H (C3 axis): cos %.3f" % cosang, tail)
    m = re.findall(r"BUILDER: bonds N-(\w+)=(\d+)", log)
    check(("X", "1") in m, "the X is bonded to N in the Builder: %s" % m, tail)
    forms = re.findall(r"BUILDER: formula=(\S*)", log)
    check(forms and all(f == "H3N" for f in forms), "the formula leaves X out: %s" % forms, tail)
    check(re.search(r"BUILDER: pointgroup=\S+", log) is not None, "symmetry ran with X present", tail)
    after = atoms[5:10]
    check(len(after) == 5 and after[4][1] == "X", "Clean keeps the X", tail)
    # the code input: X is a centre with no basis and no bond lines
    log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                       ["wait 3000", "ready", "basis 6-31G", "wait 1500", "info",
                        "button save", "wait 6000", "quit"], url)
    d = deck(calcdir)
    check(d is not None, "dummy: the editor wrote a deck", run_tests._tail(log, 20))
    if d is None:
        return
    xl = [l for l in d.splitlines() if l.split()[:1] == ["X"]]
    check(len(xl) == 1, "dummy: X is in the NWChem geometry once: %s" % xl, d[:800])
    libs = re.findall(r"^\s*(\w+)\s+library", d, re.M)
    check(sorted(set(libs)) == ["H", "N"], "dummy: no basis set for X: %s" % libs, d[:800])
    check("connect" not in d.lower() and not re.search(r"^\s*bond\b", d, re.M),
          "dummy: no bond or connectivity lines reach the input", d[:800])


def unsupportedCase(display, url):
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "info", "mode atom", "element X", "wait 500", "select -1",
        "click", "info", "quit"], url)
    tail = run_tests._tail(log, 25)
    check(re.search(r"BUILDER: message: Quantum\S* does not support dummy atoms", log) is not None,
          "choosing X in a Quantum ESPRESSO calculation says so", tail)
    atoms = re.findall(r"BUILDER: atoms=(\d+)", log)
    check(len(atoms) == 2 and atoms[0] == atoms[1],
          "and no X is added by a click: %s" % atoms, tail)


def atomsOf(log):
    return re.findall(r"BUILDER: atom (\d+) (\w+) (\S+) (\S+) (\S+)", log)


def placeCase(display, empty, nh3, qe):
    """Add Dummy Atom (#306): methane's C has no open site."""
    import math
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "add C Tetrahedral 0 0 0", "cmd addh", "wait 500", "coords",
        "constraints select 0 1 2 3", "dummy beyond 1.0", "coords",
        "constraints select 0 1 2 3", "dummy between 0.5", "quit"],
        "file://" + empty + "/")
    tail = run_tests._tail(log, 30)
    blocks = log.split("BUILDER: formula=")
    check(len(blocks) >= 3, "methane built and X placed (%d)" % len(blocks), tail)
    if len(blocks) >= 3:
        before = atomsOf(blocks[0])
        after = atomsOf(blocks[1])
        check(len(before) == 5 and len(after) == 6 and after[5][1] == "X",
              "one X added to CH4: %s" % [a[1] for a in after], tail)
        if len(after) == 6 and after[5][1] == "X":
            xyz = [[float(v) for v in a[2:5]] for a in after]
            c, x = xyz[0], xyz[5]
            axis = [x[i] - c[i] for i in range(3)]
            cen = [sum(xyz[h][i] for h in (1, 2, 3)) / 3 - c[i] for i in range(3)]
            cosang = sum(a * b for a, b in zip(axis, cen)) / (math.hypot(*axis) * math.hypot(*cen))
            check(abs(math.hypot(*axis) - 1.0) < 0.0005, "X is 1.000 A from C: %.4f" % math.hypot(*axis), tail)
            check(cosang < -0.9999, "X is on the C3 axis, opposite the three H: cos %.5f" % cosang, tail)
    check(re.findall(r"BUILDER: dummy (\w+): (.*)", log) == [("beyond", "added"), ("between", "added")],
          "both modes report added: %s" % re.findall(r"BUILDER: dummy .*", log), tail)
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "constraints select 1 2 3", "dummy centre", "coords", "quit"],
        "file://" + nh3 + "/")
    tail = run_tests._tail(log, 30)
    at = atomsOf(log)
    check(len(at) == 6 and at[5][1] == "X", "NH3: an X added: %s" % [a[1] for a in at], tail)
    if len(at) == 6 and at[5][1] == "X":
        xyz = [[float(v) for v in a[2:5]] for a in at]
        cen = [sum(xyz[h][i] for h in (1, 2, 3)) / 3 for i in range(3)]
        check(max(abs(xyz[5][i] - cen[i]) for i in range(3)) < 0.0005,
              "NH3: X at the centroid of the three H: %s vs %s" % (xyz[5], cen), tail)
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "constraints select 0 1", "dummy centre", "quit"],
        "file://" + qe + "/")
    check(re.search(r"BUILDER: dummy centre: refused: Quantum\S* does not support dummy atoms", log) is not None,
          "a code without dummy atoms refuses", run_tests._tail(log, 20))


def modeCase(display, empty, full):
    """An empty structure opens ready to place its highlighted element; a
    finished one in Select mode, so looking cannot add atoms."""
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "info", "quit"], "file://" + empty + "/")
    got = re.findall(r"BUILDER: mode=(\w+)", log)
    check(got == ["atom"], "an empty structure opens in add-atom mode: %s" % got,
          run_tests._tail(log, 25))
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "info", "quit"], "file://" + full + "/")
    got = re.findall(r"BUILDER: mode=(\w+)", log)
    check(got == ["select"], "a structure with atoms opens in Select mode: %s" % got,
          run_tests._tail(log, 25))


def periodicCase(display, qe, nw):
    """A periodic code's calculation opens with the Periodic Builder (#300)."""
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "info", "quit"],
        "file://" + qe + "/")
    tail = run_tests._tail(log, 25)
    got = re.findall(r"BUILDER: periodic_builder=(\w+)", log)
    check(got and got[0] == "shown", "Quantum ESPRESSO opens with the Periodic Builder: %s" % got, tail)
    lat = re.findall(r"BUILDER: atoms=\d+ lattice=(\w+)", log)
    check(lat and lat[0] == "no", "and no cell is invented: %s" % lat, tail)
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
        "wait 4000", "info", "quit"], "file://" + nw + "/")
    got = re.findall(r"BUILDER: periodic_builder=(\w+)", log)
    check(got == ["hidden"], "NWChem opens without it: %s" % got, run_tests._tail(log, 25))


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
            clickCase(display, "file://" + energy + "/")
            empty = os.path.join(base, "empty-nw")
            modeCase(display, empty, energy)
            placeCase(display, empty, energy, os.path.join(base, "nh3-qe"))
            dummyCase(display, "file://" + empty + "/", empty)
            periodicCase(display, os.path.join(base, "nh3-qe"), energy)
            unsupportedCase(display, "file://" + os.path.join(base, "nh3-qe") + "/")
            geomCase(display, "file://" + geom + "/")
            deckCase(display, "file://" + geom + "/", geom, True, "Geometry run")
            qmGeoVibCase(display, base)
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
