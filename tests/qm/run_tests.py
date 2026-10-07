#!/usr/bin/env python3
"""qm_scf: ecce-qm against ORCA (and NWChem) numbers recorded in oracle.json.

    run_tests.py ECCE_QM_EXE [--filter REGEX] [--table] [--quick]

Tolerances (hartree):
  total energy vs ORCA, HF              1e-6
  total energy vs ORCA, DFT             1e-5  (grids differ: ours is 99 radial x
                                        590 Lebedev, ORCA DefGrid3, NWChem xfine)
  total energy vs ORCA, PBE and PBE0    1e-4  (ORCA's PBE differs from NWChem's
                                        and Gaussian's by 1-2e-5; see README.md)
  total energy vs NWChem, all           1e-6 (grid error grows with size: 6e-7 for benzene)
  orbital energies                      1e-5
  orbital energies, restricted open     1e-4 occupied, 5e-4 for the lowest virtual,
                                        higher virtuals not compared (ORCA's
                                        canonicalisation of the ROHF Fock
                                        matrix differs from ours; README.md)
Compared orbitals: all occupied plus the lowest 3 virtual per spin.
"""
import json, os, re, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
TOL_E_HF, TOL_E_DFT, TOL_E_PBE, TOL_NW, TOL_ORB = 1e-6, 1e-5, 1e-4, 1e-6, 1e-5
SLOW = ("benzene",)

def write_input(ent, path):
    ref = {"r": "r", "u": "u", "ro": "ro"}[ent["reference"]]
    with open(path, "w") as f:
        f.write("title %s\ncharge %d\nmultiplicity %d\nmethod %s%s\nbasis %s\n" % (
            ent["molecule"], ent["charge"], ent["multiplicity"], ref, ent["method"], ent["basis"]))
        f.write("geometry\n")
        for a in ent["atoms"]:
            f.write("%s %.10f %.10f %.10f\n" % tuple(a))
        f.write("end\n")

def parse_output(path):
    res = {"orbitals": {}}
    cur = None
    for line in open(path):
        t = line.split()
        if not t:
            continue
        if t[0] == "energy_total":
            res["energy"] = float(t[1])
        elif t[0] == "converged":
            res["converged"] = t[1] == "yes"
        elif t[0] == "wall_seconds":
            res["seconds"] = float(t[1])
        elif t[0] == "begin" and t[1] == "orbitals":
            cur = t[2]
            res["orbitals"][cur] = []
        elif t[0] == "end" and t[1] == "orbitals":
            cur = None
        elif cur:
            res["orbitals"][cur].append((float(t[2]), float(t[1])))  # (occupation, energy) as ORCA rows
    return res

def main():
    args = sys.argv[1:]
    exe = args.pop(0)
    filt = None
    if "--filter" in args:
        i = args.index("--filter"); filt = re.compile(args[i + 1]); del args[i:i + 2]
    quick = "--quick" in args
    data = json.load(open(os.path.join(HERE, "oracle.json")))
    env = dict(os.environ)
    env.setdefault("ECCE_BASIS_DIR", os.path.join(HERE, "..", "..", "data", "admin", "basissets"))
    fails = 0
    print("%-34s %12s %12s %10s %8s" % ("case", "dE(ORCA)", "dE(NWChem)", "max d eps", "sec"))
    with tempfile.TemporaryDirectory() as tmp:
        for name in sorted(data):
            ent = data[name]
            if filt and not filt.search(name):
                continue
            if quick and ent["molecule"] in SLOW:
                continue
            inp, out = os.path.join(tmp, name + ".qm"), os.path.join(tmp, name + ".out")
            write_input(ent, inp)
            t0 = time.time()
            p = subprocess.run([exe, "-o", out, inp], env=env, capture_output=True, text=True)
            sec = time.time() - t0
            if p.returncode != 0 or not os.path.exists(out):
                print("%-34s FAILED to run: %s" % (name, p.stderr.strip()[-200:]))
                fails += 1
                continue
            r = parse_output(out)
            dft = ent["method"] != "hf"
            tol_e = (TOL_E_PBE if ent["method"] in ("pbe", "pbe0") else TOL_E_DFT) if dft else TOL_E_HF
            ro = ent["reference"] == "ro"
            o = ent["orca"]
            de = r["energy"] - o["energy"]
            dnw = (r["energy"] - ent["nwchem"]["energy"]) if "nwchem" in ent else None
            # orbital energies
            maxd = 0.0
            orb_ok = True
            for spin, rows in o["orbitals"].items():
                mine = r["orbitals"].get(spin if spin in r["orbitals"] else "restricted")
                if spin != "restricted" and "restricted" in r["orbitals"]:
                    mine = r["orbitals"]["restricted"]
                nocc = sum(1 for occ, _ in rows if occ > 0)
                n = min(len(rows), nocc + (1 if ro else 3))
                for k in range(n):
                    d = abs(mine[k][1] - rows[k][1])
                    maxd = max(maxd, d)
                    tol = (1e-4 if k < nocc else 5e-4) if ro else TOL_ORB
                    if d > tol:
                        orb_ok = False
            ok = r.get("converged") and abs(de) < tol_e and orb_ok and (dnw is None or abs(dnw) < TOL_NW)
            if not ok:
                fails += 1
            print("%-34s %12.2e %12s %10.2e %8.1f %s" % (
                name, de, ("%.2e" % dnw) if dnw is not None else "-", maxd, sec, "" if ok else "  <-- FAIL"), flush=True)
    print("failures:", fails)
    return 1 if fails else 0

sys.exit(main())
