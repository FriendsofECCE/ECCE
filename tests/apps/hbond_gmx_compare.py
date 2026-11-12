#!/usr/bin/env python3
"""Hydrogen bonds of the NaCl-dissolving example's last frame (#978): an
independent count by the Builder's criterion (periodic minimum image) next
to `gmx hbond` on the same frame.  Prints both; gmx counts N/O acceptors
only, so the count here is given without and with Cl- acceptors.

    hbond_gmx_compare.py [dist_A angle_deg]
"""
import math
import os
import shutil
import subprocess
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RUN = os.path.join(REPO, "data", "client", "examples", "genchem",
                   "gases-liquids-solids", "gromacs-nacl-dissolving", "1-dynamics")
dist = float(sys.argv[1]) if len(sys.argv) > 1 else 3.5
angle = float(sys.argv[2]) if len(sys.argv) > 2 else 30.0


def readGro(path):
    lines = open(path).read().splitlines()
    n = int(lines[1])
    atoms = []
    for l in lines[2:2 + n]:
        name = l[10:15].strip()
        xyz = [float(l[20 + 8 * k:28 + 8 * k]) * 10 for k in range(3)]
        atoms.append((l[5:10].strip(), name, l[:5], xyz))
    box = [float(v) * 10 for v in lines[2 + n].split()]
    return atoms, box


def mic(d, box):
    if len(box) != 3:
        sys.exit("triclinic box: not handled here")
    return [x - b * round(x / b) for x, b in zip(d, box)]


def count(atoms, box, cl):
    donors = []
    acc = []
    for i, (res, name, rnum, p) in enumerate(atoms):
        if name.startswith("OW"):
            acc.append(i)
        if name.startswith("HW"):
            j = i - 1
            while not atoms[j][1].startswith("OW"):
                j -= 1
            donors.append((i, j))
        if cl and name.startswith("CL"):
            acc.append(i)
    n = 0
    for h, d in donors:
        pd, ph = atoms[d][3], atoms[h][3]
        dh = mic([a - b for a, b in zip(ph, pd)], box)
        for a in acc:
            if a == d:
                continue
            da = mic([x - y for x, y in zip(atoms[a][3], pd)], box)
            r = math.hypot(*da)
            if r > dist or r < 1.7:
                continue
            c = sum(x * y for x, y in zip(dh, da)) / (math.hypot(*dh) * r)
            if c >= math.cos(math.radians(angle)):
                n += 1
    return n


atoms, box = readGro(os.path.join(RUN, "confout.gro"))
print("atoms %d, box %s A" % (len(atoms), " ".join("%.2f" % b for b in box)))
print("independent count, O acceptors only: %d" % count(atoms, box, False))
print("independent count, O and Cl- acceptors: %d" % count(atoms, box, True))

work = tempfile.mkdtemp(prefix="hbgmx-")
for f in os.listdir(RUN):
    if f != "prod.log":
        shutil.copy(os.path.join(RUN, f), work)
gmx = shutil.which("gmx") or shutil.which("gmx_mpi")
env = dict(os.environ, GMX_MAXBACKUP="-1")
r = subprocess.run([gmx, "grompp", "-f", "prod.mdp", "-c", "confout.gro", "-p",
                    "topol.top", "-o", "t.tpr", "-maxwarn", "5"], cwd=work,
                   capture_output=True, text=True, env=env)
if r.returncode:
    print(r.stdout[-800:], r.stderr[-800:])
    sys.exit("grompp failed")
cmd = [gmx, "hbond", "-s", "t.tpr", "-f", "confout.gro", "-num", "n.xvg",
       "-r", str(dist / 10), "-a", str(angle)]
r = subprocess.run(cmd, cwd=work, input="SOL\nSOL\n", capture_output=True,
                   text=True, env=env)
if r.returncode:
    cmd = [gmx, "hbond", "-s", "t.tpr", "-f", "confout.gro", "-num", "n.xvg",
           "-r", "group SOL", "-t", "group SOL", "-hbr", str(dist / 10), "-hba", str(angle)]
    r = subprocess.run(cmd, cwd=work, capture_output=True, text=True, env=env)
print(r.stdout[-600:], r.stderr[-1200:])
try:
    rows = [l.split() for l in open(os.path.join(work, "n.xvg")) if l[0] not in "#@"]
    print("gmx hbond SOL-SOL (first column set): %s" % rows[0])
except Exception as e:
    print("no n.xvg:", e)
shutil.rmtree(work, ignore_errors=True)
