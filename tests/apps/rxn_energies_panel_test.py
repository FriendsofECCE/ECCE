#!/usr/bin/env python3
"""Reaction Energies panel (#419 stage B2), headless, with the tree's binaries.

    tests/apps/rxn_energies_panel_test.py [--build build-cmake] [--png DIR]

Three calculations (reactant, product, a QST job that refers to them) are
built from the energies printed in the real Gaussian 16 output
tests/parsers/fixtures/gaussian-16/hcn_irc.log (the TS energy and the two
ends of its path, read by regex here) plus constructed ZPE/H/G values.  The
Builder opens the TS job's Reaction Energies panel and the script command
"rxnenergies" exports the CSV, which must hold the differences worked out
here.  Also: a product with another basis set (no energies, the difference named), a
reactant with no optimised geometry (no energies, says why), and an ordinary
calculation (no panel).  Exit status 77 (skip) without Xvfb, import or
binaries.
"""

import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import irc_panel_test as irc  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
BASE = irc.BASE
KJ = 2625.4996
KCAL = 627.5095
TS_RUNTYPE = "Transition State (QST) + Frequencies"


def value(name, number, units="Hartree"):
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<value name="%s" units="%s">%.10f</value>\n' % (name, units, number))


def geomtrace():
    """One step: the three atoms of irc.HCN."""
    xyz = "   ".join("%.6f" % v for v in (-0.645876, 0.087638, 0.0,
                                          0.522582, -0.147102, 0.0,
                                          0.429503, 1.000360, 0.0))
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<tsvectable columnLabel="Atom" columnLabels="X Y Z" columns="3" '
            'name="GEOMTRACE" rowLabel="Geometry Step" rowLabels="1 2 3" rows="3" '
            'units="Angstrom" vectorLabel="Coordinate" vectors="1">\n'
            '  <step number="1">%s</step>\n</tsvectable>\n' % xyz)


def makeCalc(project, name, energy, zpe, trace=True, basis=None, runtype=None,
             picks=None):
    calc = os.path.join(project, name)
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(BASE, calc, symlinks=True)
    props = os.path.join(calc, "Props")
    with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
        f.write(irc.HCN)
    h = energy + zpe + 0.004 + 0.1 * zpe
    for key, val, units in (("TE", energy, "Hartree"), ("EZEROPT", zpe, "Hartree"),
                            ("ENTHALPY", h, "Hartree"), ("EGIBBS", h - 0.025, "Hartree"),
                            ("THERMOTEMP", 298.15, "Kelvin")):
        with open(os.path.join(props, key), "w") as f:
            f.write(value(key, val, units))
    if trace:
        with open(os.path.join(props, "GEOMTRACE"), "w") as f:
            f.write(geomtrace())
    if basis:
        for path in (os.path.join(calc, "Parameters", ".ecce-meta"),
                     os.path.join(calc, "Parameters", "BasisSet.ecce_basisset")):
            text = open(path).read().replace("6-31G*", basis)
            open(path, "w").write(text)
    if runtype:
        path = os.path.join(calc, ".ecce-meta")
        text = re.sub(r"(ecce:runtype\t\t)[^\n]*", lambda m: m.group(1) + runtype,
                      open(path).read())
        open(path, "w").write(text)
    if picks:
        with open(os.path.join(calc, "Parameters", "SetupParams"), "w") as f:
            for key, url in picks:
                f.write("%s|file://%s||1|1|menu_input\n" % (key, url))
    return calc


def rows(path):
    lines = open(path).read().splitlines()
    data = [l.split(",") for l in lines if l and not l.startswith("#")]
    return lines, data[0], dict((r[0], r) for r in data[1:])


def run(s, display, env, wrappers, calc, png, prefix, lines, panel="Reaction Energies"):
    return bo.runScript(
        s, display, env, wrappers, ["-context", "file://" + calc + "/"], lines, png,
        prefix, {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": panel})


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "rxnenergies", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binaries: %s" % os.path.realpath(os.path.join(s.home, "bin", "builder")))

    _, energy, ts = irc.logPath()
    eR, eTS, eP = energy[0], ts, energy[-1]
    zR, zTS, zP = 0.0150, 0.0128, 0.0152
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))

        react = makeCalc(project, "hcn", eR, zR)
        prod = makeCalc(project, "hnc", eP, zP)
        prod2 = makeCalc(project, "hnc-big", eP, zP, basis="cc-pVDZ")
        noopt = makeCalc(project, "hcn-input", eR, zR, trace=False)
        R, P = "ES.Runtype.GeomOpt.QSTReactant", "ES.Runtype.GeomOpt.QSTProduct"
        jobs = {
            "ok": makeCalc(project, "ts-ok", eTS, zTS, runtype=TS_RUNTYPE,
                           picks=[(R, react), (P, prod)]),
            "warn": makeCalc(project, "ts-warn", eTS, zTS, runtype=TS_RUNTYPE,
                             picks=[(R, react), (P, prod2)]),
            "noopt": makeCalc(project, "ts-noopt", eTS, zTS, runtype=TS_RUNTYPE,
                              picks=[(R, noopt), (P, prod)]),
        }

        # Absolute hartree for each level, from the log's energies.
        def expect(state_z):
            zs = state_z
            def h(e, z): return e + z + 0.004 + 0.1 * z
            return {
                "E": (eR, eTS, eP),
                "E + ZPE": (eR + zs[0], eTS + zs[1], eP + zs[2]),
                "H": (h(eR, zs[0]), h(eTS, zs[1]), h(eP, zs[2])),
                "G": (h(eR, zs[0]) - 0.025, h(eTS, zs[1]) - 0.025, h(eP, zs[2]) - 0.025),
            }
        want = expect((zR, zTS, zP))

        for case in ("ok", "warn", "noopt"):
            out = os.path.join(s.state, "rxn-%s.csv" % case)
            if os.path.exists(out):
                os.unlink(out)
            kcal = os.path.join(s.state, "rxn-%s-kcal.csv" % case)
            if os.path.exists(kcal):
                os.unlink(kcal)
            text, shots = run(s, display, env, wrappers, jobs[case], png, "rxn" + case,
                              ["wait 6000", "shot panel", "rxnenergies %s" % out,
                               "rxnenergies %s kcal" % kcal, "quit"])
            check("segmentation" not in text.lower(), "%s: no crash" % case)
            if not os.path.exists(out):
                say(text[-1500:])
                check(False, "%s: the panel exported" % case)
                continue
            say("screenshot %s: %s" % (case, shots.get("panel")))
            lines, head, data = rows(out)
            check(any(l.startswith("# calculation: ts-") for l in lines),
                  "%s: calculation named" % case)
            warnings = [l for l in lines if l.startswith("# WARNING")]
            if case == "noopt":
                check(not data and any("NO ENERGIES" in l and "hcn-input" in l and
                                       "no optimised geometry" in l for l in lines),
                      "no optimised reactant: no energies, and the reason is named")
                continue
            if case == "warn":
                check(not data and any("NO ENERGIES" in l and "basis set differs" in l and
                                       "product cc-pVDZ" in l for l in lines),
                      "a different basis set: no energies, the reason names what differs")
                check("rxnenergies levels=0" in text, "the panel holds no levels")
                continue
            check(sorted(data) == sorted(want), "%s: four levels E, E + ZPE, H, G" % case)
            for scale, path in ((KJ, out), (KCAL, kcal)):
                _, h2, d2 = rows(path)
                for label, (a, b, c) in want.items():
                    r = d2.get(label)
                    ok = (r is not None and abs(float(r[1]) - a) < 1e-8
                          and abs(float(r[2]) - b) < 1e-8 and abs(float(r[3]) - c) < 1e-8
                          and abs(float(r[4]) - (b - a) * scale) < 1e-3
                          and abs(float(r[5]) - (c - a) * scale) < 1e-3)
                    check(ok, "%s %s: %s barrier and reaction energy" % (
                          case, h2[4].split("_", 1)[1], label))
            check(not warnings and not any("NO ENERGIES" in l for l in lines),
                  "matching calculations: no warning, no problem")
            check(float(data["E"][4]) > 0, "the barrier is positive")

        # An ordinary calculation has no Reaction Energies panel.
        text, _ = run(s, display, env, wrappers, react, png, "rxnplain",
                      ["wait 4000", "rxnenergies %s" % os.path.join(s.state, "x.csv"),
                       "quit"], panel="Energies")
        check("no Reaction Energies panel" in text, "a plain calculation has no panel")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
