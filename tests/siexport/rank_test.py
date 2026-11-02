#!/usr/bin/env python3
"""ecce-si-export --rank on a folder of related calculations (#703).

Five copies of one stored calculation with set energies: three isomers (the
lowest is the reference), one with another basis (flagged, left out of the
delta), one excluded.  Also --ref, --per-fu, a cell mismatch and an
ecce-contacts descriptor.  Every delta is compared with a hand computation.
"""
import csv
import io
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
SCRIPT = os.path.join(ROOT, "scripts", "ecce_si_export.py")
SRC = os.path.join(ROOT, "tests", "ecceqm", "fixtures", "h2o-geovib")
NS = "http://www.emsl.pnl.gov/ecce:"
KJ = 2625.499638
EV = 27.211386246
fails = []


def check(name, cond, info=""):
    print(("ok   " if cond else "FAIL ") + name + ("" if cond else " " + str(info)))
    if not cond:
        fails.append(name)


def setprop(calc, name, v):
    open(os.path.join(calc, "Props", name), "w").write(
        '<?xml version="1.0"?>\n<value name="%s" units="Hartree">%s</value>\n' % (name, v))


def make(folder, name, e, g, basis=None, excluded=None):
    d = os.path.join(folder, name)
    shutil.copytree(SRC, d)
    setprop(d, "TE", e)
    setprop(d, "EGIBBS", g)
    if basis:
        open(os.path.join(d, "Parameters", ".ecce-meta"), "w").write(
            "BasisSet.ecce_basisset\t%sname\t\t%s\n" % (NS, basis))
    if excluded:
        open(os.path.join(d, ".ecce-meta"), "a").write(".\t%sexcluded\t\t%s\n" % (NS, excluded))
    return d


def rank(args, *calcs):
    p = subprocess.run([sys.executable, SCRIPT, "--rank", "csv"] + list(args) + list(calcs),
                       capture_output=True, text=True)
    return p, {r["name"]: r for r in csv.DictReader(io.StringIO(p.stdout))}, \
        [r["name"] for r in csv.DictReader(io.StringIO(p.stdout))]


tmp = tempfile.mkdtemp(prefix="rank-")
try:
    f = os.path.join(tmp, "isomers")
    os.makedirs(f)
    make(f, "iso_b", -76.30, -76.28)
    make(f, "iso_a", -76.40, -76.35)
    make(f, "iso_c", -76.25, -76.30)       # higher E but lowest G than iso_b
    make(f, "iso_big", -80.00, -80.00, basis="cc-pVQZ")
    make(f, "iso_x", -90.00, -90.00, excluded="did not converge")

    p, rows, order = rank([], f)
    check("exit 0", p.returncode == 0, p.stderr)
    check("order by dE, then the left-out rows", order[:3] == ["iso_a", "iso_b", "iso_c"], order)
    a, b, c = rows["iso_a"], rows["iso_b"], rows["iso_c"]
    check("reference is 0", a["dE_kJ"] == "0.0000" and a["dG_kJ"] == "0.0000", a)
    check("dE kJ and eV", abs(float(b["dE_kJ"]) - 0.10 * KJ) < 1e-3 and
          abs(float(b["dE_eV"]) - 0.10 * EV) < 1e-3, b)
    check("dG against the lowest G", abs(float(c["dG_kJ"]) - 0.05 * KJ) < 1e-3 and
          abs(float(a["dG_kJ"])) < 1e-3 and abs(float(b["dG_kJ"]) - 0.07 * KJ) < 1e-3, (a, b, c))
    big = rows["iso_big"]
    check("other basis: no delta, named in flags", big["dE_kJ"] == "-" and
          "different method/basis" in big["flags"] and "cc-pVQZ" in big["flags"], big)
    check("other basis warned on stderr", "iso_big" in p.stderr, p.stderr)
    x = rows["iso_x"]
    check("excluded: no delta, reason given", x["dE_kJ"] == "-" and
          "did not converge" in x["flags"], x)
    check("composition shown", a["composition"] == "H2O1" or a["composition"] == "H2O", a)

    p, rows, _ = rank(["--ref", "iso_b"], f)
    check("--ref: delta to iso_b, negative for iso_a",
          abs(float(rows["iso_a"]["dE_kJ"]) + 0.10 * KJ) < 1e-3 and
          rows["iso_b"]["dE_kJ"] == "0.0000", rows["iso_a"])
    p, rows, _ = rank(["--ref", "iso_big"], f)
    check("--ref of a different method re-bases on that method", p.returncode == 0 and
          rows["iso_a"]["dE_kJ"] == "-" and rows["iso_big"]["dE_kJ"] == "0.0000", p.stdout)

    p, rows, _ = rank(["--per-fu", "2", "--per-site", "4"], f)
    check("per formula unit and per site",
          abs(float(rows["iso_b"]["dE_per_fu_kJ"]) - 0.05 * KJ) < 1e-3 and
          abs(float(rows["iso_b"]["dE_per_site_eV"]) - 0.025 * EV) < 1e-3, rows["iso_b"])
    p, rows, _ = rank(["--per-fu", "auto"], f)
    check("--per-fu auto detects the formula units (gcd of the composition = 1)",
          abs(float(rows["iso_b"]["dE_per_fu_kJ"]) - 0.10 * KJ) < 1e-3, rows["iso_b"])

    # Different cell, different composition.
    cell = make(f, "iso_cell", -76.31, -76.31)
    m = os.path.join(cell, "Parameters", "chemsys.mvm")
    t = open(m).read().replace("num_atoms: 3", "lattice_vectors: \n 5.0 0 0\n 0 5.0 0\n 0 0 5.0\nnum_atoms: 3", 1)
    open(m, "w").write(t)
    p, rows, _ = rank([], f)
    check("a different cell is flagged, delta still given",
          "cell differs" in rows["iso_cell"]["flags"] and rows["iso_cell"]["dE_kJ"] != "-", rows["iso_cell"])
    comp = make(f, "iso_comp", -76.32, -76.32)
    m = os.path.join(comp, "Parameters", "chemsys.mvm")
    t = open(m).read().replace("O -0.00000 0.00000 0.124640", "S -0.00000 0.00000 0.124640")
    open(m, "w").write(t)
    p, rows, _ = rank([], f)
    check("a different composition is flagged", "composition" in rows["iso_comp"]["flags"], rows["iso_comp"])
    p, rows, _ = rank(["--substitution", "O>S"], f)
    check("a stated substitution is not flagged", rows["iso_comp"]["flags"] == "", rows["iso_comp"])

    # Descriptor from ecce-contacts: shortest Na-O distance.
    d = make(f, "iso_na", -76.5, -76.5)
    m = os.path.join(d, "Parameters", "chemsys.mvm")
    os.remove(os.path.join(d, "Props", "GEOMTRACE"))
    t = open(m).read()
    t = t.replace("O -0.00000 0.00000 0.124640", "Na 0 0 0").replace(
        "H 0.00000 0.762824 -0.472870", "O 2.3 0 0").replace("H -0.00000 -0.762824 -0.472870", "O -2.4 0 0")
    open(m, "w").write(t)
    p, rows, _ = rank(["--descriptor", "d_NaO=dist:Na-O", "--descriptor", "cn=cn:Na"], d)
    check("descriptor: shortest Na-O and the number of donors",
          rows["iso_na"]["d_NaO"] == "2.300" and rows["iso_na"]["cn"] == "2", (p.stdout, p.stderr))
finally:
    shutil.rmtree(tmp, ignore_errors=True)

print("FAILED: " + ", ".join(fails) if fails else "all ok")
sys.exit(1 if fails else 0)
