#!/usr/bin/env python3
"""Restricted open-shell SCF on the ground-state atoms H-Ar, Ca, Cr and Cu (#790).

    roks_atoms_test.py ECCE-QM-EXE BASIS-DIR [WORKDIR]

Every ROHF and ROKS (B3LYP) run, def2-SVP, must converge with the default
settings, and its energy must not lie below the unrestricted energy of the
same atom (E_RO >= E_U - 1e-6): the restricted wave function is the
unrestricted one with a constraint.  Atoms with an unevenly filled degenerate
shell (O and S 2p4, F and Cl 2p5) are the cases that once oscillated.
Sc-Ni are left out: their 4s/3d near-degeneracy (3d(n)4s2 against
3d(n+1)4s1) defeats ROHF, UHF and UKS alike with the default settings, a
separate problem from the one tested here.
"""
import os, subprocess, sys, tempfile

ATOMS = [("H", 2), ("He", 1), ("Li", 2), ("Be", 1), ("B", 2), ("C", 3), ("N", 4),
         ("O", 3), ("F", 2), ("Ne", 1), ("Na", 2), ("Mg", 1), ("Al", 2), ("Si", 3),
         ("P", 4), ("S", 3), ("Cl", 2), ("Ar", 1), ("Ca", 1), ("Cr", 7), ("Cu", 2)]
METHODS = [("rohf", "uhf"), ("rob3lyp", "ub3lyp")]
BASIS = "def2-SVP"
TOL = 1e-6


def run(exe, bdir, d, sym, mult, method):
    inp = os.path.join(d, "%s_%s.qmin" % (sym, method))
    out = inp[:-5] + ".qmout"
    with open(inp, "w") as f:
        f.write("title %s\ncharge 0\nmultiplicity %d\nmethod %s\nbasis %s\nruntype energy\n"
                "geometry\n %s 0 0 0\nend\n" % (sym, mult, method, BASIS, sym))
    env = dict(os.environ, OMP_NUM_THREADS=os.environ.get("OMP_NUM_THREADS", "2"))
    subprocess.run([exe, "--basis-dir", bdir, "-o", out, inp], env=env, capture_output=True)
    r = {}
    if os.path.exists(out):
        for line in open(out):
            t = line.split()
            if len(t) >= 2 and t[0] in ("converged", "iterations", "energy_total"):
                r[t[0]] = t[1]
    return r.get("converged") == "yes", int(r.get("iterations", 0)), float(r.get("energy_total", "nan"))


def main():
    exe, bdir = sys.argv[1], sys.argv[2]
    d = sys.argv[3] if len(sys.argv) > 3 else tempfile.mkdtemp(prefix="roks_atoms_")
    fails = 0
    for sym, mult in ATOMS:
        for ro, u in METHODS:
            ok, it, e = run(exe, bdir, d, sym, mult, ro)
            uok, _, eu = run(exe, bdir, d, sym, mult, u)
            msg = "%-2s %-8s converged=%s cycles=%3d E=%.8f  E(%s)=%.8f" % (sym, ro, ok, it, e, u, eu)
            if not ok:
                msg += "  FAIL: not converged"
            elif not uok:
                msg += "  FAIL: unrestricted reference did not converge"
            elif e < eu - TOL:
                msg += "  FAIL: below the unrestricted energy by %.2e" % (eu - e)
            else:
                msg = "ok   " + msg
            if "FAIL" in msg:
                fails += 1
            print(msg, flush=True)
    print("%d failure(s)" % fails)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
