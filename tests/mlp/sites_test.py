#!/usr/bin/env python3
"""ecce-sites (#728 site screening) against crystallographic references.

    tests/mlp/sites_test.py --sites <ecce-sites>

References (International Tables for Crystallography, Vol. A, Fm-3m, No. 225):
- the conventional cell of a structure in Fm-3m has 48 x 4 = 192 operations;
- rock salt (Na 4a, Cl 4b) has one kind of empty site, 8c (1/4,1/4,1/4), the
  tetrahedral hole of both sublattices: 8 per cell, 4 Na + 4 Cl at a*sqrt(3)/4;
- fluorite CaF2 (Ca 4a, F 8c) has one empty site, 4b (1/2,1/2,1/2), the
  centre of the empty F8 cube: 4 per cell, 8 F at a*sqrt(3)/4;
- in a 2x2x2 rock-salt supercell the 32 Cl (an fcc lattice) form C(32,2) = 496
  pairs in 5 minimum-image shells, (a/2)(110), (200), (211), (220), (222):
  192, 48, 192, 48 and 16 pairs.
- rutile TiO2 (P4_2/mnm, No. 136, 16 operations; Ti 2a, O 4f, u = 0.305): the
  largest void is 4d (0,1/2,1/4) in the c channels, 4 per cell, 4 O at
  sqrt((0.305^2 + 0.195^2) a^2 + (c/4)^2) = 1.82 A.
SKIPs (77) without the program.
"""
import argparse
import math
import os
import shutil
import subprocess
import sys
import tempfile

failures = []


def check(ok, what):
    print("  %s %s" % ("ok  " if ok else "FAIL", what), flush=True)
    if not ok:
        failures.append(what)
    return ok


def cubic(a, basis, rep=1):
    atoms = []
    for i in range(rep):
        for j in range(rep):
            for k in range(rep):
                for s, x, y, z in basis:
                    atoms.append((s, (x + i) * a, (y + j) * a, (z + k) * a))
    L = a * rep
    return "%d\nLattice=\"%g 0 0 0 %g 0 0 0 %g\" Properties=species:S:1:pos:R:3 pbc=\"T T T\"\n%s" % (
        len(atoms), L, L, L, "".join("%s %.6f %.6f %.6f\n" % t for t in atoms))


FCC = [(0, 0, 0), (.5, .5, 0), (.5, 0, .5), (0, .5, .5)]
NACL = [("Na",) + p for p in FCC] + [("Cl", x + .5, y, z) for x, y, z in FCC]
CAF2 = [("Ca",) + p for p in FCC] + [("F", x + dx, y + .25, z + .25) for x, y, z in FCC
                                     for dx in (.25, .75)]


def run(prog, work, name, text, *args):
    path = os.path.join(work, name + ".xyz")
    open(path, "w").write(text)
    r = subprocess.run([prog, args[0], path] + list(args[1:]), stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE, universal_newlines=True)
    rows = [l.split("\t") for l in r.stdout.splitlines()[1:]]
    return r.returncode, rows, r.stderr


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sites", required=True)
    args = ap.parse_args()
    if not os.access(args.sites, os.X_OK):
        print("SKIP: no ecce-sites at", args.sites)
        sys.exit(77)
    work = tempfile.mkdtemp(prefix="ecce-sites-")
    try:
        a = 5.64
        rc, rows, err = run(args.sites, work, "nacl", cubic(a, NACL), "symmetry")
        check(rc == 0 and rows == [] and "operations\t192" in subprocess.run(
            [args.sites, "symmetry", os.path.join(work, "nacl.xyz")],
            stdout=subprocess.PIPE, universal_newlines=True).stdout, "rock salt: 192 operations")

        out = os.path.join(work, "out")
        rc, rows, err = run(args.sites, work, "nacl", cubic(a, NACL), "interstitial",
                            "--element", "Li", "--out", out)
        print("   ", err.strip())
        if check(rc == 0 and len(rows) == 1, "rock salt: one distinct interstitial site (%d)" % len(rows)):
            r = rows[0]
            f = sorted(float(v) % 0.5 for v in r[1:4])
            check(all(abs(v - .25) < 1e-6 for v in f), "at 8c (1/4,1/4,1/4): %s" % r[1:4])
            check(r[5] == "8" and r[6] == "8", "weight 8, coordination 8: %s %s" % (r[5], r[6]))
            check(abs(float(r[4]) - a * math.sqrt(3) / 4) < 2e-3, "clearance a*sqrt(3)/4 = %s" % r[4])
            check(r[7] == "Cl 2.44 x4, Na 2.44 x4", "environment %r" % r[7])
            files = sorted(os.listdir(out))
            check(files == ["host.xyz", "site-01.xyz", "sites.tsv"], "files written: %s" % files)
            site = open(os.path.join(out, "site-01.xyz")).read().splitlines()
            check(site[0] == "9" and site[-1].startswith("Li ") and "Lattice=" in site[1]
                  and "mult=" not in site[1], "site-01.xyz = host + Li, periodic")

        b = 5.463
        rc, rows, err = run(args.sites, work, "caf2", cubic(b, CAF2), "interstitial",
                            "--element", "F")
        print("   ", err.strip())
        if check(rc == 0 and len(rows) == 1, "fluorite: one distinct interstitial site (%d)" % len(rows)):
            r = rows[0]
            f = sorted(float(v) % 0.5 for v in r[1:4])
            check(all(abs(v) < 1e-6 or abs(v - .5) < 1e-6 for v in f), "at 4b (1/2,1/2,1/2): %s" % r[1:4])
            check(r[5] == "4" and r[6] == "8" and r[7].startswith("F 2.37 x8"),
                  "weight 4, 8 F at a*sqrt(3)/4: %s %s %s" % (r[5], r[6], r[7]))

        ra, rc_, u = 4.594, 2.959, 0.305
        rut = [("Ti", 0, 0, 0), ("Ti", .5, .5, .5), ("O", u, u, 0), ("O", 1 - u, 1 - u, 0),
               ("O", .5 + u, .5 - u, .5), ("O", .5 - u, .5 + u, .5)]
        text = "6\nLattice=\"%g 0 0 0 %g 0 0 0 %g\"\n" % (ra, ra, rc_) + "".join(
            "%s %.6f %.6f %.6f\n" % (s, x * ra, y * ra, z * rc_) for s, x, y, z in rut)
        rc, rows, err = run(args.sites, work, "rutile", text, "interstitial", "--element", "Li",
                            "--min-host", "1.5")
        print("   ", err.strip())
        if check(rc == 0 and rows and "16 operations" in err, "rutile: 16 operations, sites found"):
            r = rows[0]
            f = sorted(float(v) for v in r[1:4])
            want = math.sqrt(((u ** 2 + (.5 - u) ** 2) * ra * ra) + (rc_ / 4) ** 2)
            check(f == [0.0, 0.25, 0.5] and r[5] == "4" and r[7] == "O 1.82 x4"
                  and abs(float(r[4]) - want) < 2e-3,
                  "largest void 4d (0,1/2,1/4), weight 4, 4 O at %.3f: %s" % (want, r))

        sup = cubic(a, NACL, rep=2)
        rc, rows, err = run(args.sites, work, "nacl222", sup, "vacancy", "--element", "Cl")
        print("   ", err.strip())
        want = [(a / 2 * math.sqrt(2), 192), (a, 48), (a / 2 * math.sqrt(6), 192),
                (a * math.sqrt(2), 48), (a * math.sqrt(3), 16)]
        got = [(float(r[3]), int(r[4])) for r in rows]
        check(rc == 0 and len(got) == 5 and all(abs(g[0] - w[0]) < 2e-3 and g[1] == w[1]
                                                for g, w in zip(got, want)),
              "NaCl 2x2x2, Cl pairs: 5 shells %s" % got)
        check(sum(g[1] for g in got) == 496, "496 = C(32,2) pairs in all")
        rc, rows, err = run(args.sites, work, "nacl222", sup, "vacancy", "--element", "Cl",
                            "--max-dist", "8", "--out", os.path.join(work, "vac"))
        check(rc == 0 and len(rows) == 4, "--max-dist 8 keeps 4 shells (%d)" % len(rows))
        first = open(os.path.join(work, "vac", "site-01.xyz")).read().splitlines()
        check(first[0] == "62" and "distance=3.988" in first[1], "site-01.xyz: 62 atoms, pair at 3.988 A")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("%d failure(s)" % len(failures))
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
