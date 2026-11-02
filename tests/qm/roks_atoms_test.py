#!/usr/bin/env python3
"""Restricted open-shell SCF on the ground-state atoms H-Ar, Ca, Cr and Cu,
and on NO, O2 and CH2 (#790).

    roks_atoms_test.py ECCE-QM-EXE BASIS-DIR [WORKDIR]

Every ROHF and ROKS (B3LYP) run, def2-SVP, must converge with the default
settings, and its energy must not lie below the unrestricted energy of the
same atom (E_RO >= E_U - 1e-6): the restricted wave function is the
unrestricted one with a constraint.  Atoms with an unevenly filled degenerate
shell (O and S 2p4, F and Cl 2p5) are the cases that once oscillated.
Sc-Ni are left out: their 4s/3d near-degeneracy (3d(n)4s2 against
3d(n+1)4s1) defeats ROHF, UHF and UKS alike with the default settings, a
separate problem from the one tested here.

The molecules converge by aufbau and must keep the energies the engine gave
before the maximum overlap method existed (MOM once locked NO 50 mEh high).
Runs are single-threaded so that the XC sums, and the cycle path, repeat.
"""
import os, subprocess, sys, tempfile

ATOMS = [("H", 2), ("He", 1), ("Li", 2), ("Be", 1), ("B", 2), ("C", 3), ("N", 4),
         ("O", 3), ("F", 2), ("Ne", 1), ("Na", 2), ("Mg", 1), ("Al", 2), ("Si", 3),
         ("P", 4), ("S", 3), ("Cl", 2), ("Ar", 1), ("Ca", 1), ("Cr", 7), ("Cu", 2)]
# name, multiplicity, atoms (angstrom), {method: energy with aufbau-only ROHF}
MOLECULES = [
    ("NO", 2, [("N", 0, 0, 0), ("O", 0, 0, 1.5)],
     {"rohf": -128.949223533819, "rob3lyp": -129.611642504800}),
    ("O2", 3, [("O", 0, 0, 0), ("O", 0, 0, 1.21)],
     {"rohf": -149.469877589309, "rob3lyp": -150.140772956098}),
    ("CH2", 3, [("C", 0, 0, 0), ("H", 0, 0.9978, 0.5765), ("H", 0, -0.9978, 0.5765)],
     {"rohf": -38.881911520555, "rob3lyp": -39.085972557852}),
]
# The issue's own case: O at B3LYP/aug-cc-pVTZ sits on the pruned-grid
# gradient floor and needs the energy-plateau convergence rule.
LARGE = [("O", 3, "rob3lyp", "ub3lyp", "aug-cc-pVTZ")]
METHODS = [("rohf", "uhf"), ("rob3lyp", "ub3lyp")]
BASIS = "def2-SVP"
TOL = 1e-6


def run(exe, bdir, d, name, mult, method, atoms=None, basis=BASIS):
    atoms = atoms or [(name, 0, 0, 0)]
    run_dir = tempfile.mkdtemp(prefix="%s_%s_" % (name, method), dir=d)
    inp = os.path.join(run_dir, "in.qmin")
    out = os.path.join(run_dir, "out.qmout")
    with open(inp, "w") as f:
        f.write("title %s\ncharge 0\nmultiplicity %d\nmethod %s\nbasis %s\nruntype energy\n"
                "geometry\n" % (name, mult, method, basis))
        for a in atoms:
            f.write(" %s %.6f %.6f %.6f\n" % a)
        f.write("end\n")
    env = dict(os.environ, OMP_NUM_THREADS="1")
    p = subprocess.run([exe, "--basis-dir", bdir, "-o", out, inp], env=env, capture_output=True)
    r = {}
    # 241 is ecce-qm's "SCF did not converge"; any other non-zero exit is a
    # crash or a bad input and counts as a failed run.
    if p.returncode not in (0, 241):
        sys.stderr.write("%s %s: ecce-qm exit %d\n%s" % (name, method, p.returncode, p.stderr.decode()[-2000:]))
    elif os.path.exists(out):
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
    for sym, mult, ro, u, basis in LARGE:
        ok, it, e = run(exe, bdir, d, sym, mult, ro, basis=basis)
        uok, _, eu = run(exe, bdir, d, sym, mult, u, basis=basis)
        msg = "%-2s %-8s %s converged=%s cycles=%3d E=%.8f  E(%s)=%.8f" % (sym, ro, basis, ok, it, e, u, eu)
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
    for name, mult, atoms, ref in MOLECULES:
        for method, eref in ref.items():
            ok, it, e = run(exe, bdir, d, name, mult, method, atoms)
            msg = "%-3s %-8s converged=%s cycles=%3d E=%.9f  expected %.9f" % (name, method, ok, it, e, eref)
            if not ok:
                msg += "  FAIL: not converged"
            elif abs(e - eref) > TOL:
                msg += "  FAIL: off by %.2e" % (e - eref)
            else:
                msg = "ok   " + msg
            if "FAIL" in msg:
                fails += 1
            print(msg, flush=True)
    print("%d failure(s)" % fails)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
