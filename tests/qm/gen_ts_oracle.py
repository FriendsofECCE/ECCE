#!/usr/bin/env python3
"""Regenerates ts_oracle.json: ORCA 6.1.1 OptTS of HCN <-> HNC for ts_test.py.

    gen_ts_oracle.py WORKDIR [CASE ...]

Needs ORCA (not part of the tests).  TightOpt and VeryTightSCF, so ORCA is
converged better than ecce-qm's thresholds; the Hessian is computed at the
start and the frequencies at the end.
"""
import glob, json, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from ts_cases import CASES, GUESS, internals

ORCA = glob.glob("/opt/orca/orca_6*/orca")[0]
ORACLE = os.path.join(HERE, "ts_oracle.json")


def orca_input(c):
    s = "! HF %s OptTS TightOpt VeryTightSCF Freq NoRI NoAutoStart NoFrozenCore PAL4\n" % c["basis"]
    s += "%geom Calc_Hess true end\n%scf MaxIter 300 end\n* xyz 0 1\n"
    s += "".join("%-2s %.8f %.8f %.8f\n" % a for a in GUESS) + "*\n"
    return s


def parse(path):
    txt = open(path).read()
    if "ORCA TERMINATED NORMALLY" not in txt or txt.count("THE OPTIMIZATION HAS CONVERGED") != 1:
        raise RuntimeError("ORCA did not converge: " + path)
    e = float(re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", txt)[-1])
    geo = txt.split("CARTESIAN COORDINATES (ANGSTROEM)")[-1].split("\n\n")[0]
    xyz = [[float(v) for v in l.split()[1:4]] for l in geo.splitlines() if re.match(r"\s*[A-Z][a-z]?\s+-?\d", l)]
    freq = txt.split("VIBRATIONAL FREQUENCIES")[-1].split("NORMAL MODES")[0]
    freq = [float(f) for f in re.findall(r"^\s*\d+:\s+(-?\d+\.\d+) cm", freq, re.M)][6:]
    return dict(energy=e, internals=internals(xyz), freq=freq)


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
        out[name] = dict(program="ORCA 6.1.1", **parse(os.path.join(d, "j.out")))
        print(name, out[name])
        json.dump(out, open(ORACLE, "w"), indent=1, sort_keys=True)


main()
