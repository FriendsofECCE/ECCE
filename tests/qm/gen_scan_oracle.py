#!/usr/bin/env python3
"""Regenerates scan_oracle.json: ORCA 6.1.1 relaxed scans for scan_test.py.

    gen_scan_oracle.py WORKDIR [CASE ...]

Needs ORCA (not part of the tests).  TightOpt and VeryTightSCF, so that the
ORCA scan is converged better than ecce-qm's thresholds.  The UHF case asks
for a broken-symmetry start.
"""
import glob, json, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from scan_cases import CASES

ORCA = glob.glob("/opt/orca/orca_6*/orca")[0]
ORACLE = os.path.join(HERE, "scan_oracle.json")


def orca_input(c):
    kind, a, b, n = c["scan"]
    head = "! %s %s TightOpt VeryTightSCF NoRI NoAutoStart NoFrozenCore PAL4" % (c["orca"], c["basis"])
    if c["method"] == "b3lyp":
        head += " NoCosX DefGrid3"
    s = head + "\n%scf MaxIter 300" + (" BrokenSym 1,1 end\n" if c.get("bs") else " end\n")
    s += "%%geom Scan %s = %g, %g, %d end end\n" % (c["orca_scan"], a, b, n)
    s += "* xyz 0 %d\n" % c["mult"]
    s += "".join("%-2s %.8f %.8f %.8f\n" % at for at in c["atoms"]()) + "*\n"
    return s


def parse(path):
    txt = open(path).read()
    if "ORCA TERMINATED NORMALLY" not in txt:
        raise RuntimeError("ORCA did not finish: " + path)
    sec = txt.split("RELAXED SURFACE SCAN RESULTS")[-1].split("Actual Energy")[1].split("The Calculated Surface")[0]
    return [(float(a), float(b)) for a, b in re.findall(r"^\s*(-?\d+\.\d+)\s+(-?\d+\.\d+)\s*$", sec, re.M)]


def main():
    work = sys.argv[1]
    names = sys.argv[2:] or list(CASES)
    out = json.load(open(ORACLE)) if os.path.exists(ORACLE) else {}
    for name in names:
        d = os.path.join(work, name)
        os.makedirs(d, exist_ok=True)
        open(os.path.join(d, "j.inp"), "w").write(orca_input(CASES[name]))
        with open(os.path.join(d, "j.out"), "w") as f:
            subprocess.run([ORCA, "j.inp"], cwd=d, stdout=f, stderr=subprocess.STDOUT)
        pts = parse(os.path.join(d, "j.out"))
        out[name] = dict(program="ORCA 6.1.1", value=[p[0] for p in pts], energy=[p[1] for p in pts])
        print(name, len(pts), "points")
        json.dump(out, open(ORACLE, "w"), indent=1, sort_keys=True)


main()
