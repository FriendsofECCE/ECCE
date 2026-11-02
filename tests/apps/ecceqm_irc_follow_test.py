#!/usr/bin/env python3
"""IRC playback with "Show MO while playing" (#757), headless.

    tests/apps/ecceqm_irc_follow_test.py [--build build-cmake] [--keep]

Stores the real ecce-qm HCN IRC output (tests/parsers/fixtures/ecceqm/
hcn_irc_orb.qmout, through ecceqm.parse) as a calculation, opens the MOs
and IRC Path panels, selects the HOMO, switches "Show MO while playing" on
and steps through three points.  Checked: every step logs the orbital
matched by overlap and the MOs panel then shows that orbital; with the
option off a step changes neither.  Exit status 77 (skip) without Xvfb,
import or the binaries.
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
OUT = os.path.join(REPO, "tests", "parsers", "fixtures", "ecceqm", "hcn_irc_orb.qmout")
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


HCN = """title: hcn
type: molecule
num_atoms: 3
atom_info: symbol cart
atom_list:
C 0.088786 0.154661 0.000000
N 1.259291 -0.141034 0.000000
H 0.374388 1.252939 0.000000
attr_list:
 0.000000   1   0   0   0
 0.000000   1   0   0   0
 0.000000   1   0   0   0
atom_type_list:
        6   1
        7   1
        1   1
num_bonds: 2
bond_list:
1 2 1.00000 1
1 3 1.00000 1
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "ecceqmircfollow", {}, local=True, keep=args.keep)
    png = os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)

    out = open(OUT).read()
    feed = lambda t: parsed(subprocess.run(["perl", PARSE], input=t, capture_output=True,
                                           text=True).stdout)
    table = feed(blocks(out, "irc_table")[-1])
    coord = [float(v) for v in table["IRCCOORD"][0]["values"][0].split()]
    energy = [float(v) for v in table["IRCENERGY"][0]["values"][0].split()]
    frames = []
    for t in blocks(out, "irc_geometry"):
        rec = feed(t)["IRCGEOM"][0]
        frames.append([[float(v) for v in row.split()] for row in rec["values"]])
    ptorb = [numbers(feed(t)["PTORB"][0]) for t in blocks(out, "point_orbitals")]
    ptmo = [numbers(feed(t)["PTMO"][0]) for t in blocks(out, "point_mo_coefficients")]
    fin = feed(blocks(out, "orbitals")[-1])
    finmo = feed(blocks(out, "mo_coefficients")[-1])["MO"][0]
    nbf = len(ptorb[0]) // 2
    say("%d path points, %d IRC frames, %d point orbital sets, %d orbitals"
        % (len(coord), len(frames), len(ptmo), nbf))

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
        calc = os.path.join(project, "calc-hcn-ircfollow")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(sp.BASE, calc, symlinks=True)
        props = os.path.join(calc, "Props")
        for name in os.listdir(props):
            if name != "ESCF":
                os.unlink(os.path.join(props, name))
        with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(HCN)
        docs = {
            "IRCCOORD": sp.vector("IRCCOORD", coord, "sqrt(amu)*bohr"),
            "IRCENERGY": sp.vector("IRCENERGY", energy, "Hartree"),
            "IRCGEOM": sp.tsvector(frames),
            "ORBENG": vectorDoc("ORBENG", numbers(fin["ORBENG"][0]), "Hartree"),
            "ORBOCC": vectorDoc("ORBOCC", numbers(fin["ORBOCC"][0]), "electrons"),
            "MO": tableDoc("MO", nbf, nbf, numbers(finmo)),
            "PTORB": tsDoc("PTORB", nbf, 2, "Energy Occupation", ptorb, "NA"),
            "PTMO": tsDoc("PTMO", nbf, nbf, labels(nbf, "BasFun"), ptmo, "Electrons"),
        }
        # The overlap matrix needs the calculation's own basis: sto-3g on C, N, H.
        # The format of a real ECCE-QM run: one group per element.
        tmpl = open(os.path.join(REPO, "tests", "modiagram", "fixtures", "ecceqm-methanol",
                                 "Parameters", "BasisSet.ecce_basisset")).read()
        group = re.search(r"<Group Tag=\"C\".*?</Group>\n", tmpl, re.S).group(0)
        basis = ("<MolecularConfig>\n<Map>\n" + "".join(
            group.replace('"C"', '"%s"' % e).replace("6-31G*", "STO-3G") for e in "CNH"))
        lib = open(os.path.join(REPO, "data", "admin", "basissets", "STO-3G.BAS")).read()
        data = "BasisSet=STO-3G\nType=pople Category=ORBITAL Spherical=UnknownCoordSys ContType=UnknownContType\n"
        for e in "CNH":
            body = re.search(r"^atom=%s\n(.*?)(?=^atom=|\Z)" % e, lib, re.S | re.M).group(1)
            data += "element=%s\n%s" % (e, body)
        basis += "</Map>\n<Data>\n" + data + "EndBasisSet\n</Data>\n</MolecularConfig>"
        with open(os.path.join(calc, "Parameters", "BasisSet.ecce_basisset"), "w") as f:
            f.write(basis)
        meta = os.path.join(calc, ".ecce-meta")
        with open(meta) as f:
            text = f.read().replace("Gaussian-16", "ECCE-QM")
        with open(meta, "w") as f:
            f.write(text)
        for name, text in docs.items():
            with open(os.path.join(props, name), "w") as f:
                f.write(text)

        step = ["ircstep next", "wait 800", "ircplayopt flush", "wait 1500", "mostate"]
        lines = ["wait 6000", "panel MOs", "wait 1500",
                 "panel IRC Path", "wait 1500", "ircstep first", "wait 1000",
                 "ircplayopt flush", "wait 1500", "mostate",
                 "ircplayopt mo off", "ircstep next", "wait 800", "ircplayopt flush",
                 "wait 1500", "mostate",
                 "ircplayopt mo on", "ircstep next", "wait 800", "ircplayopt flush",
                 "wait 1500", "mostate"] + step + step + ["quit"]
        dump = os.path.join(s.state, "overlap.dump")
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            lines, png, "ecceqmircfollow",
            {"ECCE_PANEL_MODE": "detail", "ECCE_IRC_FOLLOW_DUMP": dump})
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        follow = re.findall(r"BUILDER: ircfollow point=(\d+) mo=(\d+) overlap=([\d.]+)", text)
        steps = re.findall(r"BUILDER: ircstep next point=(\d+) .* mo=(-?\d+)", text)
        states = re.findall(r"^selectedMo (\d+)", text, re.M)
        say("ircfollow: %s" % follow)
        say("ircstep: %s; MOs panel selectedMo after each state: %s" % (steps, states))
        check(len(follow) == 3, "three ticks with the option on were matched (got %d)" % len(follow))
        check(len(steps) == 4 and steps[0][1] == "0",
              "the step with the option off matched nothing")
        if len(follow) == 3 and len(states) >= 3:
            shown = [int(v) for v in states[-3:]]
            check(shown == [int(m) for _, m, _ in follow],
                  "the MOs panel shows the matched orbital after each tick: %s" % shown)
            check(all(float(o) > 0.8 for _, _, o in follow),
                  "each match has |overlap| above 0.8 with the previous point's orbital")
            # Independent check: |c_prev S c_new| from the stored coefficients and
            # the overlap matrix the Builder dumped, argmax by numpy.
            import numpy as np
            mats = {}
            lines_ = open(dump).read().split("\n")
            k = 0
            while k < len(lines_) and lines_[k].startswith("point"):
                _, pt, n = lines_[k].split()
                n = int(n)
                mats[int(pt)] = np.array([float(v) for v in lines_[k + 1:k + 1 + n * n]]).reshape(n, n)
                k += 1 + n * n
            prev = int(states[-4]) - 1 if len(states) >= 4 else None
            for pt, mo, _ in follow:
                pt, mo = int(pt), int(mo) - 1
                C = np.array(ptmo[pt - 1]).reshape(nbf, nbf)
                P = np.array(ptmo[pt - 2]).reshape(nbf, nbf)
                ov = np.abs(C @ mats[pt] @ P[prev])
                check(int(np.argmax(ov)) == mo,
                      "point %d: numpy argmax of |overlap| is MO %d, code chose %d"
                      % (pt, int(np.argmax(ov)) + 1, mo + 1))
                prev = mo
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
