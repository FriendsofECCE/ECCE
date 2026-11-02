#!/usr/bin/env python3
"""ecce-density-field on the committed fixtures against Multiwfn 2026.9.20.

  densityfields_cli_test.py ECCE_DENSITY_FIELD FIXTURE-ROOT

The reference numbers were produced by Multiwfn (menu 1, properties at a
point, RDG_maxrho = 0 and ELF_addminimal = 0 in settings.ini) on the molden
file of the same ECCE-QM wavefunction (tests/tdat/make_densityfields_fixtures.py).
Multiwfn is not needed here.  Tolerances are the issues' (ELF, LOL 1e-4 abs,
rho 1e-6 rel, s 1e-3 where rho > 1e-4); o2 is the UHF triplet.
"""
import os
import subprocess
import sys
import tempfile

exe, root = sys.argv[1:3]

# name, point (A), rho, ELF, LOL, s, sign(l2)rho
REF = [
 ("water", (0, 0, 0.5), 0.7812906458, 0.7963511523, 0.583474158, 0.353420668, -0.7812906458),
 ("water", (0.3, 0.4, -0.9), 0.04157627753, 0.7040181527, 0.2703124463, 1.109331225, -0.04157627753),
 ("water", (0, 1.5, 0.2), 0.00539780925, 0.2013878064, 0.0761732185, 2.466163769, -0.00539780925),
 ("water", (0.2, 0.9, -0.3), 0.1427284213, 0.9813640066, 0.5023892127, 0.7152697923, -0.1427284213),
 ("o2", (0.1, 0.2, 0.3), 0.3247561193, 0.6596883787, 0.507462812, 0.3850487945, 0.3247561193),
 ("o2", (0.0, 0.9, 0.0), 0.03637912637, 0.2543434942, 0.2118794932, 1.102598868, 0.03637912637),
 ("o2", (0.5, 0.0, 1.2), 0.01255379761, 0.3999442834, 0.1436539584, 1.67234548, -0.01255379761),
 ("dimer", (-0.9, 0.5, 0.0), 0.1053295539, 0.7315957513, 0.3813152624, 0.7810760895, 0.1053295539),
 ("dimer", (0.4, 0.2, 0.0), 0.05660355363, 0.7106344095, 0.3062614964, 0.9880485408, -0.05660355363),
 ("dimer", (0.0, 2.0, 0.5), 2.495296827e-05, 3.398455431e-05, 0.00133966089, 18.55677109, 2.495296827e-05),
]

fail = 0
for name in sorted(set(r[0] for r in REF)):
    rows = [r for r in REF if r[0] == name]
    cmd = [exe, os.path.join(root, name)]
    for r in rows:
        cmd += ["--at", "%g,%g,%g" % r[1]]
    out = [l.split() for l in subprocess.check_output(cmd, universal_newlines=True).splitlines()
           if not l.startswith("#")]
    for r, o in zip(rows, out):
        got = [float(v) for v in o]
        rho, elf, lol, s, sl = got[3], got[14], got[15], got[16], got[17]
        bad = []
        if abs(rho - r[2]) > 1e-6*r[2]: bad.append("rho %g vs %g" % (rho, r[2]))
        if abs(elf - r[3]) > 1e-4: bad.append("ELF %g vs %g" % (elf, r[3]))
        if abs(lol - r[4]) > 1e-4: bad.append("LOL %g vs %g" % (lol, r[4]))
        if r[2] > 1e-4 and abs(s - r[5]) > 1e-3: bad.append("s %g vs %g" % (s, r[5]))
        if abs(sl - r[6]) > 1e-6*abs(r[6]): bad.append("sign(l2)rho %g vs %g" % (sl, r[6]))
        if bad:
            fail += 1
            print("FAIL %s %s: %s" % (name, r[1], "; ".join(bad)))

# A cube: header, size and one value agree with the point evaluation.
with tempfile.TemporaryDirectory() as tmp:
    cube = os.path.join(tmp, "elf.cube")
    subprocess.check_call([exe, os.path.join(root, "water"), "--field", "elf", "--spacing", "0.5",
                           "--margin", "1.5", "-o", cube], stderr=subprocess.DEVNULL)
    lines = open(cube).read().splitlines()
    natoms = int(lines[2].split()[0])
    nx, ny, nz = [int(lines[3 + k].split()[0]) for k in range(3)]
    ox, oy, oz = [float(v) for v in lines[2].split()[1:4]]
    h = float(lines[3].split()[1])
    vals = [float(v) for l in lines[6 + natoms:] for v in l.split()]
    if natoms != 3 or len(vals) != nx*ny*nz:
        fail += 1
        print("FAIL cube: %d atoms, %d values for %dx%dx%d" % (natoms, len(vals), nx, ny, nz))
    else:
        i, j, k = nx//2, ny//2, nz//2
        a = 0.52917721092
        p = ((ox + i*h)*a, (oy + j*h)*a, (oz + k*h)*a)
        o = subprocess.check_output([exe, os.path.join(root, "water"), "--at", "%.9f,%.9f,%.9f" % p],
                                    universal_newlines=True).splitlines()[1].split()
        if abs(float(o[14]) - vals[(i*ny + j)*nz + k]) > 1e-4:
            fail += 1
            print("FAIL cube value at the middle: %s vs %s" % (o[14], vals[(i*ny + j)*nz + k]))

print("failures: %d" % fail)
sys.exit(1 if fail else 0)
