#!/usr/bin/env python3
"""Ghost centres in ecce-qm (#1091).

    ghost_test.py ECCE_QM_EXE BASIS_DIR

1. a ghost-only geometry is refused (no electrons, nothing to compute);
2. water A at the dimer geometry with B's atoms as ghosts is lower than A
   alone (basis-set superposition), HF and B3LYP, by < 10 kJ/mol;
3. the full dimer with no ghosts reproduces its recorded energy;
4. ghosts with a gradient/optimisation run type are refused.
"""
import os, re, subprocess, sys, tempfile

EH_KJ = 2625.4996
A = [("O", 0.0, 0.0, 0.0), ("H", 0.965, 0.0, 0.0), ("H", -0.24, 0.93, 0.0)]
B = [("O", 2.91, 0.0, 0.0), ("H", 3.229, 0.491, 0.757), ("H", 3.229, 0.491, -0.757)]
# Dimer energies (hartree, def2-SVP) of this input from a run without ghosts.
BASELINE = {"hf": -151.9289883942, "b3lyp": -152.6532969969}
TOL = 1e-8


def run(exe, bdir, d, tag, method, atoms, runtype="energy"):
    inp, out = os.path.join(d, tag + ".qmin"), os.path.join(d, tag + ".qmout")
    with open(inp, "w") as f:
        f.write("charge 0\nmultiplicity 1\nmethod %s\nbasis def2-SVP\nruntype %s\n"
                "conv_energy 1e-10\ngeometry\n" % (method, runtype))
        for a in atoms:
            f.write(" %s %.6f %.6f %.6f%s\n" % (a[:4] + (" ghost" if len(a) > 4 else "",)))
        f.write("end\n")
    p = subprocess.run([exe, "--basis-dir", bdir, "-o", out, inp],
                       env=dict(os.environ, OMP_NUM_THREADS="1"), capture_output=True, text=True)
    return p, out


def energy(out):
    m = re.search(r"^energy_total\s+(\S+)", open(out).read(), re.M)
    return float(m.group(1))


def main():
    exe, bdir = sys.argv[1], sys.argv[2]
    d = tempfile.mkdtemp(prefix="ghost_test_")
    bad = []

    p, _ = run(exe, bdir, d, "only", "hf", [("O", 0, 0, 0, "g")])
    if p.returncode == 0 or "no real atom" not in p.stderr:
        bad.append("ghost-only input not refused: rc=%d %s" % (p.returncode, p.stderr[-200:]))

    p, _ = run(exe, bdir, d, "gopt", "hf", A + [(s, x, y, z, "g") for s, x, y, z in B], "gradient")
    if p.returncode == 0 or "ghost atoms" not in p.stderr:
        bad.append("gradient with ghosts not refused")

    for method in ("hf", "b3lyp"):
        p, o = run(exe, bdir, d, "mono_" + method, method, A)
        e_mono = energy(o)
        p, o = run(exe, bdir, d, "gh_" + method, method, A + [(s, x, y, z, "g") for s, x, y, z in B])
        if p.returncode != 0:
            bad.append("%s ghost run failed: %s" % (method, p.stderr[-300:]))
            continue
        e_gh = energy(o)
        text = open(o).read()
        if not re.search(r"\bghost\b", text):
            bad.append("%s: output does not mark the ghost centres" % method)
        de = (e_gh - e_mono) * EH_KJ
        print("%s: E(A)=%.10f E(A in AB basis)=%.10f  dE=%.3f kJ/mol" % (method, e_mono, e_gh, de))
        # The lowering is the monomer's share of the dimer BSSE; at SVP the
        # whole counterpoise correction is a few kJ/mol, so 10 is generous.
        if not (-10.0 < de < -1e-4):
            bad.append("%s: ghost lowering %.4f kJ/mol outside (-10, 0)" % (method, de))

        p, o = run(exe, bdir, d, "dimer_" + method, method, A + B)
        e_dim = energy(o)
        print("%s: E(dimer)=%.10f" % (method, e_dim))
        base = BASELINE[method]
        if base is None:
            bad.append("%s: no recorded dimer energy (measured %.10f)" % (method, e_dim))
        elif abs(e_dim - base) > TOL:
            bad.append("%s: dimer energy %.10f differs from recorded %.10f" % (method, e_dim, base))

    for b in bad:
        print("FAIL:", b)
    print("PASS" if not bad else "FAILED")
    return 1 if bad else 0


sys.exit(main())
