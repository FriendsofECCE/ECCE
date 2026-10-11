#!/usr/bin/env python3
"""qm_opt: ecce-qm geometry optimisations against ORCA 6.1.1 (opt_oracle.json).

    opt_test.py ECCE_QM_EXE [--quick] [--table] [--filter REGEX]

Both programs start from the same pushed-off geometry (gradient_test.pushed).
ecce-qm runs with opt_conv tight against ORCA's TightOpt; with normal
convergence it is run again only to count steps.  Compared: final energy
(HF 1e-6; DFT 1e-5, as in run_tests.py: the grids differ), every interatomic
distance (1e-3 angstrom) and every angle (0.1 degree).
"""
import itertools, json, math, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from gradient_test import pushed, write_input

TOL_E_HF, TOL_E_DFT, TOL_R, TOL_A = 1e-6, 1e-5, 1e-3, 0.1
QUICK = ("h2o_hf_r", "co_hf_r", "o2_hf_ro", "nh3_b3lyp_r")

def optimise(exe, work, ent, conv):
    charge, mult, atoms = pushed(ent["molecule"])
    prefix = {"r": "", "u": "u", "ro": "ro"}[ent["reference"]]
    inp = os.path.join(work, "opt.qm")
    write_input(inp, charge, mult, atoms, prefix + ent["method"], ent["basis"], "optimize", None)
    with open(inp, "a") as f:
        f.write("opt_conv %s\n" % conv)
    p = subprocess.run([exe, inp], capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("ecce-qm failed: " + p.stderr[-300:])
    out = p.stdout
    block = re.search(r"begin atoms.*?\n(.*?)end atoms", out, re.S).group(1)
    geo = [[l.split()[1]] + [float(v) for v in l.split()[3:6]] for l in block.splitlines()]
    return dict(energy=float(re.search(r"^energy_total\s+(\S+)", out, re.M).group(1)), atoms=geo,
                steps=int(re.search(r"^opt_evaluations\s+(\d+)", out, re.M).group(1)),
                converged="opt_converged yes" in out)

def internals(atoms):
    xyz = [a[1:] for a in atoms]
    dist = {}
    for i, j in itertools.combinations(range(len(xyz)), 2):
        dist[(i, j)] = math.dist(xyz[i], xyz[j])
    ang = {}
    for j in range(len(xyz)):
        for i, k in itertools.combinations([a for a in range(len(xyz)) if a != j], 2):
            u = [xyz[i][m] - xyz[j][m] for m in range(3)]
            v = [xyz[k][m] - xyz[j][m] for m in range(3)]
            c = sum(a * b for a, b in zip(u, v)) / math.sqrt(sum(a * a for a in u) * sum(b * b for b in v))
            ang[(i, j, k)] = math.degrees(math.acos(max(-1.0, min(1.0, c))))
    return dist, ang

def compare(a, b):
    da, aa = internals(a)
    db, ab = internals(b)
    return (max(abs(da[k] - db[k]) for k in da),
            max([abs(aa[k] - ab[k]) for k in aa] or [0.0]))

def main():
    args = sys.argv[1:]
    exe = args.pop(0)
    quick = "--quick" in args
    table = "--table" in args
    pat = re.compile(args[args.index("--filter") + 1]) if "--filter" in args else None
    oracle = json.load(open(os.path.join(HERE, "opt_oracle.json")))
    bad = 0
    if table:
        print("| case | dE (Eh) | max dR (A) | max dAngle (deg) | steps ecce tight | steps ORCA TightOpt | steps ecce normal | steps ORCA Opt |")
        print("|---|---|---|---|---|---|---|---|")
    with tempfile.TemporaryDirectory() as work:
        for name, ent in sorted(oracle.items()):
            if (quick and name not in QUICK) or (pat and not pat.search(name)):
                continue
            tight = optimise(exe, work, ent, "tight")
            dE = tight["energy"] - ent["tightopt"]["energy"]
            dr, da = compare(tight["atoms"], ent["tightopt"]["atoms"])
            tol = TOL_E_HF if ent["method"] == "hf" else TOL_E_DFT
            ok = tight["converged"] and abs(dE) <= tol and dr <= TOL_R and da <= TOL_A
            bad += not ok
            if table:
                normal = optimise(exe, work, ent, "normal")
                print("| %s | %+.2e | %.1e | %.3f | %d | %d | %d | %d |" % (
                    name, dE, dr, da, tight["steps"], ent["tightopt"]["cycles"], normal["steps"], ent["opt"]["cycles"]))
            else:
                print("%-12s dE = %+.2e  dR = %.1e A  dAngle = %.3f deg  steps %d (ORCA %d)  %s" % (
                    name, dE, dr, da, tight["steps"], ent["tightopt"]["cycles"], "ok" if ok else "FAIL"), flush=True)
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
