#!/usr/bin/env python3
"""The MOs panel follows the scan point selected in the Relaxed Scan panel (#549), headless.

    tests/apps/ecceqm_point_orbitals_test.py [--build build-cmake] [--keep]

Runs the bundled ecce-qm on an H2O O-H bond scan (HF/STO-3G, 5 points,
tests/parsers/fixtures/ecceqm/h2o_scan_orb.qmin), stores what the real
ecceqm.parse returns as the calculation's scan properties, final orbitals
and per-point orbitals (PTORB, PTMO), opens the MOs and Relaxed Scan panels
and selects the first and the last point.  Checked: the MOs panel lists the
engine's orbital energies of the selected point, they differ between the
two points, and its note names the point.  Exit status 77 (skip) without
Xvfb, import, ecce-qm or the binaries.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import scan_panel_test as sp  # noqa: E402
from ecceqm_scan_panel_test import engine, parsed  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
DECK = os.path.join(REPO, "tests", "parsers", "fixtures", "ecceqm", "h2o_scan_orb.qmin")
PARSE = os.path.join(REPO, "scripts", "parsers", "ecceqm.parse")
EV = 27.211386
H2O = """title: h2o
type: molecule
num_atoms: 3
atom_info: symbol cart
atom_list:
O 0.000000 0.000000 0.117000
H 0.000000 0.757000 -0.467000
H 0.000000 -0.757000 -0.467000
attr_list:
 0.000000   1   0   0   0
 0.000000   1   0   0   0
 0.000000   1   0   0   0
atom_type_list:
        8   1
        1   1
        1   1
num_bonds: 2
bond_list:
1 2 1.00000 1
1 3 1.00000 1
"""


def blocks(out, name):
    return re.findall(r"^begin %s\b.*?^end %s\n" % (name, name), out, re.S | re.M)


def numbers(rec):
    return [float(v) for row in rec["values"] for v in row.split()]


def labels(n, prefix):
    return " ".join("%s-%d" % (prefix, i + 1) for i in range(n))


def vectorDoc(name, values, units):
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="%s" rowLabel="Basis Function" rowLabels="%s" rows="%d" '
            'units="%s">%s</vector>\n'
            % (name, labels(len(values), "MO"), len(values), units,
               "   ".join("%.12f" % v for v in values)))


def tableDoc(name, rows, cols, values):
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<table columnLabel="Basis Function" columnLabels="%s" columns="%d" '
            'name="%s" rowLabel="Basis Function" rowLabels="%s" rows="%d" '
            'units="Electrons">%s</table>\n'
            % (labels(cols, "BasFun"), cols, name, labels(rows, "MO"), rows,
               "   ".join("%.10f" % v for v in values)))


def tsDoc(name, rows, cols, collabels, tables, units):
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
           '<tsvectable columnLabel="Column" columnLabels="%s" columns="%d" '
           'name="%s" rowLabel="Point" rowLabels="%s" rows="%d" '
           'units="%s" vectorLabel="MO" vectors="1">\n'
           % (collabels, cols, name, labels(rows, "MO"), rows, units)]
    for i, t in enumerate(tables):
        out.append('  <step number="%d">%s</step>\n'
                   % (i + 1, "   ".join("%.10f" % v for v in t)))
    out.append("</tsvectable>\n")
    return "".join(out)


def rowEnergies(text, which):
    """(MO number, numbers) of the MOs table's rows after the which-th mostate."""
    part = text.split("BUILDER: mostate\n")[which + 1]
    vals = []
    for line in part.splitlines():
        if line.startswith("BUILDER:"):
            break
        if line.startswith("row "):
            cells = [c.strip() for c in line[4:].split("|")]
            vals.append((int(cells[0]), [float(v) for v in re.findall(r"-?\d+\.\d+", line)]))
    return vals


def listed(rows, energies):
    """Every energy appears in its row, in hartree or eV (as displayed)."""
    if len(rows) != len(energies):
        return False
    for mo, vals in rows:
        e = energies[mo - 1]
        if not any(abs(v - e) < 2e-4 or abs(v - e * EV) < 5e-3 for v in vals):
            return False
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    qm = engine(build)
    if not qm:
        harness.skip("ecce-qm is not built in %s" % build)
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "ecceqmptorb", {}, local=True, keep=args.keep)
    png = os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)

    run = subprocess.run([qm, DECK], capture_output=True, text=True,
                         env=dict(os.environ, ECCE_QM_SERIAL="1"))
    check(run.returncode == 0, "ecce-qm ran the scan deck")
    out = run.stdout
    feed = lambda t: parsed(subprocess.run(["perl", PARSE], input=t, capture_output=True,
                                           text=True).stdout)
    table = feed(blocks(out, "scan_table")[-1])
    coord = [float(v) for v in table["SCANCOORD"][0]["values"][0].split()]
    energy = [float(v) for v in table["SCANENERGY"][0]["values"][0].split()]
    frames = []
    for t in blocks(out, "scan_geometry"):
        rec = feed(t)["SCANGEOM"][0]
        frames.append([[float(v) for v in row.split()] for row in rec["values"]])
    ptorb = [numbers(feed(t)["PTORB"][0]) for t in blocks(out, "point_orbitals")]
    ptmo = [numbers(feed(t)["PTMO"][0]) for t in blocks(out, "point_mo_coefficients")]
    fin = feed(blocks(out, "orbitals")[-1])
    finmo = feed(blocks(out, "mo_coefficients")[-1])["MO"][0]
    nbf = len(ptorb[0]) // 2
    check(len(frames) == len(ptorb) == len(ptmo) == 5 and nbf == 7,
          "5 points with 7 orbitals each from the parser")
    pointE = [t[0::2] for t in ptorb]
    say("HOMO point 1 %.6f, point 5 %.6f hartree" % (pointE[0][4], pointE[-1][4]))

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
        calc = os.path.join(project, "calc-h2o-ptorb")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(sp.BASE, calc, symlinks=True)
        props = os.path.join(calc, "Props")
        for name in os.listdir(props):
            if name != "ESCF":
                os.unlink(os.path.join(props, name))
        with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(H2O)
        docs = {
            "SCANCOORD": sp.vector("SCANCOORD", coord, "Angstrom"),
            "SCANENERGY": sp.vector("SCANENERGY", energy, "Hartree"),
            "SCANGEOM": sp.tsvector(frames),
            "ORBENG": vectorDoc("ORBENG", numbers(fin["ORBENG"][0]), "Hartree"),
            "ORBOCC": vectorDoc("ORBOCC", numbers(fin["ORBOCC"][0]), "electrons"),
            "MO": tableDoc("MO", nbf, nbf, numbers(finmo)),
            "PTORB": tsDoc("PTORB", nbf, 2, "Energy Occupation", ptorb, "NA"),
            "PTMO": tsDoc("PTMO", nbf, nbf, labels(nbf, "BasFun"), ptmo, "Electrons"),
        }
        for name, text in docs.items():
            with open(os.path.join(props, name), "w") as f:
                f.write(text)

        lines = ["wait 6000", "panel MOs", "wait 1500", "mostate",
                 "panel Relaxed Scan", "wait 1500", "scanclick 0", "wait 1000", "mostate",
                 "scanclick 4", "wait 1000", "mostate", "quit"]
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            lines, png, "ecceqmptorb", {"ECCE_PANEL_MODE": "detail"})
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        notes = re.findall(r"BUILDER: monote (.*)", text)
        if len(notes) != 3 or text.count("BUILDER: mostate\n") != 3:
            say(text[-3000:])
            check(False, "three MOs panel states were reported")
            return 1
        say("notes: %s" % " / ".join(notes))
        before, first, last = (rowEnergies(text, i) for i in range(3))
        # The Relaxed Scan panel may already be open and have selected its
        # first row; either way note and energies must agree.
        if "last computed point" in notes[0]:
            check(listed(before, pointE[-1]), "unselected: the last point's energies")
        else:
            check("scan point 1" in notes[0] and listed(before, pointE[0]),
                  "the first row selected on opening: point 1's note and energies")
        check("scan point 1" in notes[1], "after selecting point 1 the note names it")
        check(listed(first, pointE[0]), "after selecting point 1 the panel lists point 1's energies")
        check("scan point 5" in notes[2], "after selecting point 5 the note names it")
        check(listed(last, pointE[-1]), "after selecting point 5 the panel lists point 5's energies")
        check([v for _, v in first] != [v for _, v in last], "the listed energies differ between point 1 and point 5")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
