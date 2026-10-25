#!/usr/bin/env python3
"""D from the mdanalysis CLI against `gmx msd` on the same real trajectories.

GROMACS is the independent oracle: its xtc reader, its PBC handling and its
Einstein fit share nothing with src/util/mdanalysis.  ECCE_MD_RESULTS names
the folder of finished runs (ar-gas-vs-liquid, ar-he-gas, water-vs-decane);
without it, gmx or the files the test SKIPs (77).
"""
import os, re, subprocess, sys, tempfile

cli = sys.argv[1]
root = os.environ.get("ECCE_MD_RESULTS", "")
if not root or subprocess.call(["which", "gmx"], stdout=subprocess.DEVNULL):
    sys.exit(77)

# folder, topology, mdp, trajectory, tpr, gmx selection, fit window ps, tolerance
CASES = [
    ("liquid Ar xtc", "ar-gas-vs-liquid/ar.top", "ar-gas-vs-liquid/prod.mdp",
     "ar-gas-vs-liquid/prod.xtc", "ar-gas-vs-liquid/prod.tpr", "System", 250, 1000, 0.03),
    ("Ar gas trr", "ar-he-gas/run-ar/ar.top", "ar-he-gas/run-ar/prod.mdp",
     "ar-he-gas/run-ar/prod.trr", "ar-he-gas/run-ar/prod.tpr", "System", 250, 1000, 0.03),
    ("SPC/E water xtc, molecule centres", "water-vs-decane/water/system.top",
     "water-vs-decane/water/npt-prod.mdp", "water-vs-decane/water/prod.xtc",
     "water-vs-decane/water/prod.tpr", "res_com of resname SOL", 1250, 2500, 0.03),
]

failed = 0
ran = 0
for name, top, mdp, traj, tpr, sel, f0, f1, tol in CASES:
    paths = [os.path.join(root, p) for p in (top, mdp, traj, tpr)]
    if not all(os.path.exists(p) for p in paths):
        print("skip", name, "(files missing)")
        continue
    ran += 1
    with tempfile.TemporaryDirectory() as tmp:
        t = tmp
        out = subprocess.run([cli, "--top", paths[0], "--mdp", paths[1], "--traj", paths[2],
                              "--fit-from", str(f0), "--fit-to", str(f1)],
                             capture_output=True, text=True)
        m = re.search(r"D = ([0-9.eE+-]+) x 1e-5", out.stdout)
        g = subprocess.run(["gmx", "msd", "-f", paths[2], "-s", paths[3], "-o", t + "/m.xvg",
                            "-beginfit", str(f0), "-endfit", str(f1), "-sel", sel],
                           capture_output=True, text=True)
        xvg = open(t + "/m.xvg").read()
        n = re.search(r"= ([0-9.eE+-]+) \(\+/-", xvg)
        if not m or not n:
            print("FAIL", name, out.stderr[:200], g.stderr[-200:])
            failed += 1
            continue
        mine, gmx = float(m.group(1)), float(n.group(1))
        ok = abs(mine / gmx - 1) < tol
        print("ok  " if ok else "FAIL", "%s: D = %.4g (ecce) vs %.4g (gmx msd) x 1e-5 cm2/s, %.2f%%"
              % (name, mine, gmx, 100 * (mine / gmx - 1)))
        failed += not ok
sys.exit(77 if not ran else (1 if failed else 0))
