#!/usr/bin/env python3
"""Regenerates irc_oracle.json: ORCA 6.1.1 OptTS then IRC (both directions) of
HCN <-> HNC for irc_test.py.

    gen_irc_oracle.py WORKDIR [CASE ...]

Needs ORCA (not part of the tests).  The IRC job starts from the OptTS
geometry (j.xyz); the path length of each ORCA frame is the mass-weighted
chord sum from the TS, so it can be set against ecce-qm's s.
"""
import glob, json, math, os, re, subprocess, sys
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from ts_cases import CASES, GUESS, internals

ORCA = glob.glob("/opt/orca/orca_6*/orca")[0]
ORACLE = os.path.join(HERE, "irc_oracle.json")
MASS = {"H": 1.00782503, "C": 12.0, "N": 14.0030740}
BOHR = 0.529177210903


def orca_input(c):
    s = "! HF %s OptTS TightOpt VeryTightSCF Freq NoRI NoAutoStart NoFrozenCore PAL4\n" % c["basis"]
    s += "%geom Calc_Hess true end\n%scf MaxIter 300 end\n* xyz 0 1\n"
    s += "".join("%-2s %.8f %.8f %.8f\n" % a for a in GUESS) + "*\n"
    s += "$new_job\n! HF %s IRC VeryTightSCF NoRI NoAutoStart NoFrozenCore PAL4\n" % c["basis"]
    s += "%irc MaxIter 40 InitHess calc_anfreq Direction both end\n%scf MaxIter 300 end\n* xyzfile 0 1 j.xyz\n"
    return s


def frames(path):
    lines, out = open(path).read().splitlines(), []
    while lines:
        n = int(lines.pop(0))
        e = float(re.search(r"E\s+(-?\d+\.\d+)", lines.pop(0)).group(1))
        out.append((e, [(l.split()[0], [float(v) for v in l.split()[1:4]]) for l in lines[:n]]))
        del lines[:n]
    return out


def chord(a, b):
    """Mass-weighted distance after removing translation and rotation (Kabsch)."""
    m = np.array([MASS[s] for s, _ in a])
    p, q = np.array([x for _, x in a]) / BOHR, np.array([x for _, x in b]) / BOHR
    p, q = p - m @ p / m.sum(), q - m @ q / m.sum()
    u, _, vt = np.linalg.svd((q * m[:, None]).T @ p)
    d = np.diag([1, 1, np.sign(np.linalg.det(u @ vt))])
    q = q @ (u @ d @ vt)
    return math.sqrt(float(m @ ((q - p) ** 2).sum(axis=1)))


def direction(path, ts):
    pts, prev, s = [], ts, 0.0
    for e, xyz in frames(path):
        s += chord(prev, xyz)
        pts.append((s, e))
        prev = xyz
    return dict(s=[p[0] for p in pts], energy=[p[1] for p in pts],
                end_internals=internals([x for _, x in prev]), end_energy=pts[-1][1])


def main():
    work, names = sys.argv[1], sys.argv[2:] or list(CASES)
    out = json.load(open(ORACLE)) if os.path.exists(ORACLE) else {}
    for name in names:
        d = os.path.join(work, name)
        os.makedirs(d, exist_ok=True)
        if not os.path.exists(os.path.join(d, "j_job2_IRC_F_trj.xyz")):
            open(os.path.join(d, "j.inp"), "w").write(orca_input(CASES[name]))
            with open(os.path.join(d, "j.out"), "w") as f:
                subprocess.run([ORCA, "j.inp"], cwd=d, stdout=f, stderr=subprocess.STDOUT)
        txt = open(os.path.join(d, "j.out")).read()
        if "ORCA TERMINATED NORMALLY" not in txt or "IRC PATH SUMMARY" not in txt:
            raise RuntimeError("ORCA did not finish: " + d)
        lines = open(os.path.join(d, "j.xyz")).read().splitlines()[2:]
        ts = [(l.split()[0], [float(v) for v in l.split()[1:4]]) for l in lines if l.strip()]
        e_ts = float(re.search(r"E\s+(-?\d+\.\d+)", open(os.path.join(d, "j.xyz")).read().splitlines()[1]).group(1))
        dirs = [direction(os.path.join(d, "j_job2_IRC_%s_trj.xyz" % t), ts) for t in "FB"]
        out[name] = dict(program="ORCA 6.1.1", ts_xyz=[[s] + x for s, x in ts], ts_energy=e_ts, forward=dirs[0], backward=dirs[1])
        json.dump(out, open(ORACLE, "w"), indent=1, sort_keys=True)
        print(name, "ok", [(round(x["end_energy"], 6), len(x["s"])) for x in dirs])


main()
