#!/usr/bin/env python3
"""Gaussian 16 scan point selection updates MOs and its point note (#549), headless.

Uses the real parser fixture (a three-point water bond scan); --legacy
stores no point tables, as for a scan run without Pop=Always.
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
import scan_panel_test as sp  # noqa: E402
from ecceqm_point_orbitals_test import (labels, rowEnergies, listed,
                                       tableDoc, tsDoc, vectorDoc)
sys.path.insert(0, os.path.join(REPO, "tests", "parsers"))
from g16_scan_orbitals_test import records, values, printed_energies
from harness import say  # noqa: E402

check = bo.check
WATER = """title: water
type: molecule
num_atoms: 3
atom_info: symbol cart
atom_list:
%s
attr_list:
0 1 0 0 0
0 1 0 0 0
0 1 0 0 0
atom_type_list:
1 1
1 1
1 1
num_bonds: 2
bond_list:
1 2 1.0 1
1 3 1.0 1
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--legacy", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built" % exe)
    import xdisplay

    rec = records()
    pointE = [o + v for o, v in printed_energies()]
    frames = [[values(r)[i:i+3] for i in (0, 3, 6)] for r in rec["SCANGEOM"]]
    s = harness.Session(build, "g16scanmo", {}, local=True, keep=args.keep)
    png = os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
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
        calc = os.path.join(project, "calc-g16-scan-mo")
        shutil.copytree(sp.BASE, calc, symlinks=True)
        meta = os.path.join(calc, ".ecce-meta")
        with open(meta) as f:
            text = f.read().replace("GeoVib", "Geometry")
        with open(meta, "w") as f:
            f.write(text)
        # Density evaluators and the MOs panel must see the run's own basis.
        tmpl = open(os.path.join(sp.BASE, "Parameters", "BasisSet.ecce_basisset")).read()
        lib = open(os.path.join(REPO, "data/admin/basissets/STO-3G.BAS")).read()
        groups, bodies = "", ""
        for el in ("H", "O"):
            groups += re.search(r'<Group Tag="%s".*?</Group>' % el, tmpl,
                                re.S).group(0).replace("6-31G*", "STO-3G")
            bodies += "element=%s\n" % el + re.search(
                r"^atom=%s\n(.*?)(?=^atom=|\Z)" % el, lib, re.S | re.M).group(1)
        basis = ("<MolecularConfig><Map>" + groups + "</Map><Data>\n"
                 "BasisSet=STO-3G\nType=pople Category=ORBITAL Spherical=UnknownCoordSys "
                 "ContType=UnknownContType\n" + bodies +
                 "EndBasisSet\n</Data></MolecularConfig>")
        with open(os.path.join(calc, "Parameters", "BasisSet.ecce_basisset"), "w") as f:
            f.write(basis)
        props = os.path.join(calc, "Props")
        for name in os.listdir(props):
            if name != "ESCF":
                os.unlink(os.path.join(props, name))
        with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(WATER % "\n".join("%s %.6f %.6f %.6f" % ((el,) + tuple(x))
                                       for el, x in zip("OHH", frames[0])))
        docs = {
            "SCANCOORD": sp.vector("SCANCOORD", values(rec["SCANCOORD"][-1]), "Angstrom"),
            "SCANENERGY": sp.vector("SCANENERGY", values(rec["SCANENERGY"][-1]), "Hartree"),
            "SCANGEOM": sp.tsvector(frames),
            "ORBENG": vectorDoc("ORBENG", values(rec["ORBENG"][-1]), "Hartree"),
            "ORBOCC": vectorDoc("ORBOCC", values(rec["ORBOCC"][-1]), "electrons"),
            "MO": tableDoc("MO", 7, 7, values(rec["MO"][-1])),
        }
        for key, cols, collabels, units in (("PTORB", 2, "Energy Occupation", "NA"),
                                            ("PTMO", 7, labels(7, "BasFun"), "Electrons")):
            if key in rec and not args.legacy:
                docs[key] = tsDoc(key, 7, cols, collabels, [values(r) for r in rec[key]], units)
        for name, text in docs.items():
            with open(os.path.join(props, name), "w") as f:
                f.write(text)
        steps = [0, 1, 2, 0]
        lines = ["wait 6000", "list MOs", "wait 1500", "list Relaxed Scan", "wait 1500"]
        for k in steps:
            lines += ["scanclick %d" % k, "wait 1000", "list MOs", "wait 1500",
                      "mostate", "list Relaxed Scan"]
        lines.append("quit")
        text, _ = bo.runScript(s, display, env, wrappers,
            ["-context", "file://" + calc + "/"], lines, png, "g16scanmo",
            {"ECCE_PANEL_MODE": "detail"})
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        check("unknown command" not in text and "no Properties" not in text and "not in the Properties list" not in text,
              "script opened the property panels")
        notes = re.findall(r"BUILDER: monote (.*)", text)
        check(len(notes) == len(steps), "every MOs state was reported")
        if len(notes) != len(steps):
            say(text[-3000:])
            return 1
        for i, k in enumerate(steps):
            try:
                rows = rowEnergies(text, i)
            except ValueError:
                rows = []
            if args.legacy:
                check("last computed point" in notes[i], "legacy: final-point note")
                check(listed(rows, pointE[-1]), "legacy: final point energies")
            else:
                check("scan point %d" % (k+1) in notes[i], "note names scan point %d" % (k+1))
                check(listed(rows, pointE[k]), "MOs list point %d energies" % (k+1))
        say("notes: " + " / ".join(notes))
        if bo.failures:
            say(text[-10000:])
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAIL: " + "; ".join(bo.failures) if bo.failures else "PASS")
    return 1 if bo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
