#!/usr/bin/env python3
"""Independent check of `ecce-fragorb dos` (#171): per-MO Mulliken numbers.

    dos_oracle.py <ecce-fragorb> <ecce-results>

Water in an STO-3G basis (s and p shells only).  The overlap matrix S is
computed here from the Gaussian-overlap recursion (Obara-Saika), MOs are the
eigenvectors of an extended-Hueckel-type H in the Loewdin basis (any
S-orthonormal set tests the populations, no SCF is needed), written to a
Molden file and converted with `ecce-results convert`.  For every MO i the
textbook quantities
    gross(X,i) = sum_{mu in X} c_mu,i (S c_i)_mu
    OP(A,B,i)  = 2 sum_{mu in A, nu in B} c_mu,i c_nu,i S_mu,nu
are compared with the area under each narrow peak of TDOS, PDOS(A), PDOS(B)
and OPDOS(A,B) of `dos --json` (closed-shell MOs count twice), and the
occupied overlap population sum n_i OP_i with the CLI's and with the
integrated OPDOS at the Fermi level.  A = O, B = both H.
Tolerance 2e-5 per MO (the CLI prints 8 significant digits).  Exit 77
without numpy.
"""
import json
import os
import subprocess
import sys
import tempfile

try:
    import numpy as np
except ImportError:
    print("SKIP: no numpy")
    sys.exit(77)

TOL = 2e-5
HARTREE_EV = 27.211386245988
BOHR = [("O", 8, (0.0, 0.0, 0.2216648744)),
        ("H", 1, (0.0, 1.4309006216, -0.8866594977)),
        ("H", 1, (0.0, -1.4309006216, -0.8866594977))]
C1S = [0.15432897, 0.53532814, 0.44463454]
SHELLS = {  # atom type: [(l, exponents, coefficients)]
    "O": [(0, [130.70932, 23.808861, 6.4436083], C1S),
          (0, [5.0331513, 1.1695961, 0.38038896], [-0.09996723, 0.39951283, 0.70011547]),
          (1, [5.0331513, 1.1695961, 0.38038896], [0.15591627, 0.60768372, 0.39195739])],
    "H": [(0, [3.42525091, 0.62391373, 0.1688554], C1S)]}
H_ON_SITE = {("O", 0, 0): -20.5, ("O", 0, 1): -1.0, ("O", 1, 2): -0.5, ("H", 0, 0): -0.5}


def os1d(a, b, A, B, imax, jmax):
    p = a + b
    P = (a * A + b * B) / p
    S = np.zeros((imax + 2, jmax + 2))
    S[0, 0] = np.sqrt(np.pi / p) * np.exp(-a * b / p * (A - B) ** 2)
    for i in range(imax + 1):
        for j in range(jmax + 1):
            if i == 0 and j == 0:
                continue
            if j == 0:
                S[i, 0] = (P - A) * S[i - 1, 0] + (i - 1) / (2 * p) * (S[i - 2, 0] if i > 1 else 0)
            else:
                S[i, j] = (P - B) * S[i, j - 1] + (i * S[i - 1, j - 1] + (j - 1) * (S[i, j - 2] if j > 1 else 0)) / (2 * p)
    return S


def prim_overlap(a, la, A, b, lb, B):
    t = 1.0
    for k in range(3):
        t *= os1d(a, b, A[k], B[k], la[k], lb[k])[la[k], lb[k]]
    return t


def pnorm(a, l):
    return (2 * a / np.pi) ** 0.75 * (2 * np.sqrt(a) if l else 1.0)


def build():
    """AO list (atom, shell index, l, cartesian power, exps, coeffs), then S."""
    aos = []
    for ia, (el, z, xyz) in enumerate(BOHR):
        for ish, (l, ex, co) in enumerate(SHELLS[el]):
            for pw in ([(0, 0, 0)] if l == 0 else [(1, 0, 0), (0, 1, 0), (0, 0, 1)]):
                aos.append((ia, ish, l, pw, ex, [c * pnorm(e, l) for e, c in zip(ex, co)], el))
    n = len(aos)
    S = np.zeros((n, n))
    for i in range(n):
        for j in range(n):
            Ai, Bj = BOHR[aos[i][0]][2], BOHR[aos[j][0]][2]
            S[i, j] = sum(ci * cj * prim_overlap(ei, aos[i][3], Ai, ej, aos[j][3], Bj)
                          for ei, ci in zip(aos[i][4], aos[i][5])
                          for ej, cj in zip(aos[j][4], aos[j][5]))
    d = np.sqrt(np.diag(S))
    return aos, S / np.outer(d, d), d


def molden(path, aos, C, E, occ):
    with open(path, "w") as f:
        f.write("[Molden Format]\n[Title]\n[Atoms] AU\n")
        for i, (el, z, xyz) in enumerate(BOHR):
            f.write("%s %d %d %.12f %.12f %.12f\n" % ((el, i + 1, z) + xyz))
        f.write("[GTO]\n")
        for i, (el, z, xyz) in enumerate(BOHR):
            f.write("%d 0\n" % (i + 1))
            for l, ex, co in SHELLS[el]:
                f.write(" %s %d 1.0\n" % ("sp"[l], len(ex)))
                for e, c in zip(ex, co):
                    f.write("  %.12e %.12e\n" % (e, c))
            f.write("\n")
        f.write("[MO]\n")
        for m in range(C.shape[1]):
            f.write("Sym= %da\nEne= %.14f\nSpin= Alpha\nOccup= %.1f\n" % (m + 1, E[m], occ[m]))
            for k in range(C.shape[0]):
                f.write("%d %.14e\n" % (k + 1, C[k, m]))


def main():
    frag, resexe = [os.path.abspath(a) for a in sys.argv[1:3]]
    aos, S, norm = build()
    n = len(aos)
    h = np.diag([H_ON_SITE[(a[6], a[2], a[1] if a[2] == 0 else 2)] for a in aos])
    H = 1.75 * S * (np.add.outer(np.diag(h), np.diag(h)) / 2)
    np.fill_diagonal(H, np.diag(h))
    w, U = np.linalg.eigh(S)
    X = U @ np.diag(w ** -0.5) @ U.T
    E, V = np.linalg.eigh(X @ H @ X)
    C = X @ V
    occ = np.array([2.0] * 5 + [0.0] * (n - 5))
    assert np.allclose(C.T @ S @ C, np.eye(n), atol=1e-10)

    atom = np.array([a[0] for a in aos])
    A, B = atom == 0, atom >= 1
    SC = S @ C
    grossA = np.array([C[A, i] @ SC[A, i] for i in range(n)])
    grossB = np.array([C[B, i] @ SC[B, i] for i in range(n)])
    OP = np.array([2 * C[A, i] @ S[np.ix_(A, B)] @ C[B, i] for i in range(n)])
    occOP = float(occ @ OP)

    work = tempfile.mkdtemp(prefix="dos-oracle-")
    md, h5 = os.path.join(work, "water.molden"), os.path.join(work, "results.h5")
    molden(md, aos, C, E, occ)
    env = dict(os.environ)
    env.setdefault("ECCE_HOME", os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
    env.setdefault("ECCE_REALUSER", "dosoracle")
    env.setdefault("ECCE_REALUSERHOME", work)
    env.setdefault("HOME", work)
    r = subprocess.run([resexe, "convert", md, h5], capture_output=True, text=True, env=env)
    if r.returncode:
        print("FAIL convert:", r.stdout, r.stderr)
        return 1
    fwhm, step = 0.05, 0.005
    eev = E * HARTREE_EV
    r = subprocess.run([frag, "dos", h5, "--set", "A=1", "--set", "B=2,3", "--pair", "A:B", "--fwhm", str(fwhm),
                        "--step", str(step), "--emin", "%.3f" % (eev[0] - 3), "--emax", "%.3f" % (eev[-1] + 3), "--json"],
                       capture_output=True, text=True, env=env)
    if r.returncode:
        print("FAIL dos:", r.stderr)
        return 1
    d = json.loads(r.stdout)
    g = d["grid"]
    e = np.array([x["e"] for x in g])
    t = np.array([x["tdos"] for x in g])
    pa = np.array([x["pdos"][0] for x in g])
    pb = np.array([x["pdos"][1] for x in g])
    op = np.array([x["opdos"][0] for x in g])
    iop = np.array([x["int_opdos"][0] for x in g])
    # each peak is integrated between the midpoints to its neighbours
    edges = np.concatenate([[e[0] - 1], (eev[1:] + eev[:-1]) / 2, [e[-1] + 1]])
    worst = 0.0
    print("MO   E/eV   gross(A) gross(B)  OP(A,B)   | deviation: TDOS/2 PDOS(A)/2 PDOS(B)/2 OPDOS/2")
    for i in range(n):
        s = (e >= edges[i]) & (e < edges[i + 1])
        got = [x[s].sum() * step / 2 for x in (t, pa, pb, op)]
        want = [1.0, grossA[i], grossB[i], OP[i]]
        dev = [abs(a - b) for a, b in zip(got, want)]
        worst = max(worst, max(dev))
        print("%2d %9.3f %8.5f %8.5f %9.5f | %.1e %.1e %.1e %.1e" % ((i + 1, eev[i], grossA[i], grossB[i], OP[i]) + tuple(dev)))
    sums = max(abs(grossA + grossB - 1))
    fermi = d["fermi_ev"]
    iopF = iop[np.argmin(abs(e - fermi))]
    print("max |gross(A)+gross(B)-1| (oracle) %.1e; occupied A-B overlap population: oracle %.6f  CLI %.6f  integrated OPDOS at Fermi %.6f"
          % (sums, occOP, d["occupied_overlap"][0], iopF))
    print("CLI norm_error %.2e; worst per-MO deviation %.2e (tolerance %.0e)" % (d["norm_error"], worst, TOL))
    ok = worst < TOL and abs(occOP - d["occupied_overlap"][0]) < TOL and abs(occOP - iopF) < 1e-4 and d["norm_error"] < 1e-5
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
