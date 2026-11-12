#!/usr/bin/env python3
"""Redock imatinib into 1IEP with a receptor and ligand prepared by ECCE.

This is the acceptance check for the docking preparation (#455): if the
typing, the merged hydrogens or the torsion tree are wrong, the top pose
does not come back on the crystal ligand.
Usage: redock_1iep.py <ecce-pdbqt> <1iep.pdb> <workdir> [vina]
"""
import math
import os
import shutil
import subprocess
import sys


def rmsd(a, b):
    return math.sqrt(sum((x - y) ** 2 for p, q in zip(a, b)
                         for x, y in zip(p, q)) / len(a))


def main():
    exe, pdb, work = sys.argv[1], sys.argv[2], sys.argv[3]
    vina = sys.argv[4] if len(sys.argv) > 4 else shutil.which("vina")
    if not vina or not os.path.exists(pdb):
        print("vina or 1IEP missing; skipping")
        return 77
    os.makedirs(work, exist_ok=True)

    # Chain A only: 1IEP is a dimer and the pocket of interest is in A.
    rec_pdb = os.path.join(work, "rec.pdb")
    lig_pdb = os.path.join(work, "lig.pdb")
    crystal = []
    with open(pdb) as f, open(rec_pdb, "w") as r, open(lig_pdb, "w") as l:
        for line in f:
            if len(line) < 22 or line[21] != "A":
                continue
            if line.startswith("ATOM"):
                r.write(line)
            elif line.startswith("HETATM") and line[17:20].strip() == "STI":
                l.write(line)
                crystal.append((float(line[30:38]), float(line[38:46]),
                                float(line[46:54])))

    rec = os.path.join(work, "receptor.pdbqt")
    lig = os.path.join(work, "ligand.pdbqt")
    lmap = os.path.join(work, "ligand.map")
    for cmd in ([exe, "--receptor", rec_pdb, "-o", rec],
                [exe, "--ligand", lig_pdb, "--ligand-residue", "STI",
                 "-o", lig, "--map", lmap, "--name", "imatinib"]):
        p = subprocess.run(cmd, capture_output=True, text=True)
        if p.returncode != 0:
            print("FAIL preparation:", p.stderr)
            return 1

    cx = sum(c[0] for c in crystal) / len(crystal)
    cy = sum(c[1] for c in crystal) / len(crystal)
    cz = sum(c[2] for c in crystal) / len(crystal)
    cfg = os.path.join(work, "vina.vinain")
    with open(cfg, "w") as f:
        f.write("receptor = %s\nligand = %s\n" % (rec, lig))
        f.write("center_x = %.3f\ncenter_y = %.3f\ncenter_z = %.3f\n" % (cx, cy, cz))
        f.write("size_x = 25\nsize_y = 25\nsize_z = 25\n")
        f.write("exhaustiveness = 32\nnum_modes = 9\nseed = 20261111\n")

    out = os.path.join(work, "poses.pdbqt")
    p = subprocess.run([vina, "--config", cfg, "--out", out],
                       capture_output=True, text=True)
    if p.returncode != 0:
        print("FAIL vina:", p.stdout[-2000:], p.stderr[-2000:])
        return 1

    # ligand.map gives, for each written PDBQT atom, its 1-based atom in the
    # input; that is what lets a pose be laid back over the input structure.
    idx = [int(x) - 1 for x in open(lmap)]
    models, scores, cur = [], [], []
    for line in open(out):
        if line.startswith("MODEL"):
            cur = []
        elif line.startswith("ENDMDL"):
            models.append(cur)
        elif "VINA RESULT" in line:
            scores.append(float(line.split(":")[1].split()[0]))
        elif line.startswith(("ATOM", "HETATM")):
            cur.append((float(line[30:38]), float(line[38:46]),
                        float(line[46:54])))

    print("%d poses, %d crystal heavy atoms, %d written ligand atoms"
          % (len(models), len(crystal), len(idx)))
    best = None
    for n, (m, s) in enumerate(zip(models, scores)):
        pairs = [(m[k], crystal[idx[k]]) for k in range(len(m))
                 if 0 <= idx[k] < len(crystal)]
        r = rmsd([p[0] for p in pairs], [p[1] for p in pairs])
        print("pose %d: Vina score %.2f kcal/mol, heavy-atom RMSD %.2f A"
              % (n + 1, s, r))
        if n == 0:
            best = r
        if n == 2:
            break
    if best is None or best > 2.0:
        print("FAIL top pose RMSD %.2f A is over 2 A" % (best or 99))
        return 1
    print("top pose within 2 A of the crystal ligand")
    return 0


if __name__ == "__main__":
    sys.exit(main())
