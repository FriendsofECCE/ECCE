#!/usr/bin/env python3
"""The Density Profile's plane integral (#749) against an analytic oracle.

  planeintegral_test.py LINEPROFILE FIXTURE-ROOT

FIXTURE-ROOT holds NAME/ (a calculation) and NAME.molden (the same ECCE-QM
wavefunction), from make_planeintegral_fixtures.py.  The oracle reads only
the molden file: for a line along a Cartesian axis the integral of a product
of two Cartesian Gaussians over the perpendicular plane is the product of the
two 1-D overlaps across it times the 1-D product along the line, so the
whole-plane value is exact.  Within a cylinder it is a brute-force polar sum
(2000 x 256 points) of the same numpy orbitals.  Checked:
  - lineprofile's plane integral of the total density and of one orbital
    agrees with the oracle at every sample;
  - its integral along the line is N (total) and 1 (one orbital);
  - the ethene pi orbital is zero on the C=C axis but not in the plane
    integral; a cylinder holds less than the whole plane, as the oracle says.
"""
import math
import os
import subprocess
import sys
import tempfile

import numpy as np

exe, root = sys.argv[1:3]
BOHR = 0.52917721092
fail = 0


def check(ok, what):
    global fail
    print(("ok   " if ok else "FAIL ") + what)
    if not ok:
        fail += 1


# ---------------------------------------------------------------- molden
SPH = {  # molden order; polynomials in unnormalised Cartesian monomials
    2: [{(0, 0, 2): 2, (2, 0, 0): -1, (0, 2, 0): -1}, {(1, 0, 1): 1}, {(0, 1, 1): 1},
        {(2, 0, 0): 1, (0, 2, 0): -1}, {(1, 1, 0): 1}],
    3: [{(0, 0, 3): 2, (2, 0, 1): -3, (0, 2, 1): -3},
        {(1, 0, 2): 4, (3, 0, 0): -1, (1, 2, 0): -1},
        {(0, 1, 2): 4, (2, 1, 0): -1, (0, 3, 0): -1},
        {(2, 0, 1): 1, (0, 2, 1): -1}, {(1, 1, 1): 1},
        {(3, 0, 0): 1, (1, 2, 0): -3}, {(2, 1, 0): 3, (0, 3, 0): -1}],
}
CART = {0: [(0, 0, 0)], 1: [(1, 0, 0), (0, 1, 0), (0, 0, 1)],
        2: [(2, 0, 0), (0, 2, 0), (0, 0, 2), (1, 1, 0), (1, 0, 1), (0, 1, 1)]}
L = {"s": 0, "p": 1, "d": 2, "f": 3}


def overlap1d(a, A, i, b, B, j):
    """Int (x-A)^i (x-B)^j exp(-a(x-A)^2 - b(x-B)^2) dx, exactly."""
    p = a + b
    P = (a * A + b * B) / p
    pre = math.exp(-a * b / p * (A - B) ** 2)
    s = 0.0
    for k in range(i + 1):
        for m in range(j + 1):
            n = k + m
            if n % 2:
                continue
            mom = math.gamma((n + 1) / 2) / p ** ((n + 1) / 2)   # Int t^n exp(-p t^2)
            s += math.comb(i, k) * math.comb(j, m) * (P - A) ** (i - k) * (P - B) ** (j - m) * mom
    return pre * s


def read_molden(path):
    lines = open(path).read().splitlines()
    atoms, shells, mos, spherical = [], {}, [], set()
    sec, i = None, 0
    while i < len(lines):
        t = lines[i].strip()
        if t.startswith("["):
            sec = t.split("]")[0][1:].upper()
            if sec in ("5D", "5D7F", "5D10F"): spherical.add(2)
            if sec in ("7F", "5D7F"): spherical.add(3)
            unit = BOHR if "AU" in t.upper() else 1.0
            i += 1
            continue
        if sec == "ATOMS" and t:
            w = t.split()
            atoms.append(np.array([float(v) for v in w[3:6]]) * unit)
        elif sec == "GTO" and t:
            a = int(t.split()[0]) - 1
            shells[a] = []
            i += 1
            while i < len(lines) and lines[i].strip():
                w = lines[i].split()
                l, n = L[w[0].lower()], int(w[1])
                prims = [tuple(float(v.replace("D", "E")) for v in lines[i + 1 + k].split()[:2])
                         for k in range(n)]
                shells[a].append((l, prims))
                i += n + 1
        elif sec == "MO" and t.startswith("Sym"):
            mo = {"occ": 0.0, "c": []}
            while i + 1 < len(lines) and "=" in lines[i + 1]:
                i += 1
                k, v = lines[i].split("=", 1)
                if k.strip() == "Occup": mo["occ"] = float(v)
                if k.strip() == "Ene": mo["e"] = float(v)
            while i + 1 < len(lines) and lines[i + 1].strip() and "=" not in lines[i + 1] \
                    and not lines[i + 1].strip().startswith("["):
                i += 1
                mo["c"].append(float(lines[i].split()[1]))
            mos.append(mo)
        i += 1
    # One term per (AO, primitive, monomial): centre (bohr), exponent, powers, coefficient.
    terms, ao = [], 0
    for a in range(len(atoms)):
        A = atoms[a] / BOHR
        for l, prims in shells[a]:
            comps = SPH[l] if l in spherical else [{c: 1.0} for c in CART[l]]
            for comp in comps:
                tl = []
                for alpha, cc in prims:
                    norm = 1 / math.sqrt(overlap1d(alpha, 0, 2 * l, alpha, 0, 0)
                                         * overlap1d(alpha, 0, 0, alpha, 0, 0) ** 2)
                    for pw, f in comp.items():
                        tl.append([ao, A, alpha, pw, cc * norm * f])
                s = sum(t[4] * u[4] * np.prod([overlap1d(t[2], t[1][d], t[3][d], u[2], u[1][d], u[3][d])
                                               for d in range(3)]) for t in tl for u in tl)
                for t in tl:
                    t[4] /= math.sqrt(s)
                terms += tl
                ao += 1
    return atoms, terms, ao, mos


class Oracle:
    def __init__(self, path, axis):
        self.atoms, self.terms, self.nao, self.mos = read_molden(path)
        self.axis = axis
        T = self.terms
        n = len(T)
        perp = [d for d in range(3) if d != axis]
        self.S = np.empty((n, n))          # overlap across the plane
        for p in range(n):
            for q in range(p, n):
                v = 1.0
                for d in perp:
                    v *= overlap1d(T[p][2], T[p][1][d], T[p][3][d], T[q][2], T[q][1][d], T[q][3][d])
                self.S[p, q] = self.S[q, p] = v
        self.ao = np.array([t[0] for t in T])
        self.coef = np.array([t[4] for t in T])

    def termcoef(self, mos, weights):
        """Coefficient of each term in each orbital, weighted: D = sum w c c."""
        C = np.array([[self.mos[m]["c"][a] for a in self.ao] for m in mos]) * self.coef
        return C, np.array(weights)

    def plane(self, s_ang, mos, weights):
        """Whole-plane integral at axis coordinate s (A), electrons per A."""
        C, w = self.termcoef(mos, weights)
        x = s_ang / BOHR
        g = np.array([(x - t[1][self.axis]) ** t[3][self.axis] *
                      math.exp(-t[2] * (x - t[1][self.axis]) ** 2) for t in self.terms])
        Cg = C * g
        val = np.einsum("mp,pq,mq,m->", Cg, self.S, Cg, w)   # bohr^-1
        return val / BOHR

    def psi(self, pts_ang, mos):
        X = pts_ang / BOHR
        vals = np.zeros((len(mos), len(X)))
        C, _ = self.termcoef(mos, [1] * len(mos))
        for p, t in enumerate(self.terms):
            r = X - t[1]
            f = np.exp(-t[2] * (r ** 2).sum(1)) * np.prod(r ** np.array(t[3]), axis=1)
            vals += np.outer(C[:, p], f)
        return vals * BOHR ** -1.5                          # A^-3/2

    def cylinder(self, s_ang, radius, origin, mos, weights):
        """Brute force: Gauss-Legendre in r on [0, R] (2000), 256 angles."""
        x, w = np.polynomial.legendre.leggauss(2000)
        r = 0.5 * radius * (x + 1)
        wr = 0.5 * radius * w * r
        phi = (np.arange(256) + 0.5) * 2 * math.pi / 256
        perp = [d for d in range(3) if d != self.axis]
        R, P = np.meshgrid(r, phi, indexing="ij")
        pts = np.zeros((R.size, 3))
        pts[:, self.axis] = s_ang
        pts[:, perp[0]] = origin[perp[0]] + (R * np.cos(P)).ravel()
        pts[:, perp[1]] = origin[perp[1]] + (R * np.sin(P)).ravel()
        total = 0.0
        for chunk in range(0, len(pts), 64000):
            v = self.psi(pts[chunk:chunk + 64000], mos)
            dens = (np.array(weights)[:, None] * v ** 2).sum(0)
            ww = np.repeat(wr, len(phi))[chunk:chunk + 64000] * (2 * math.pi / len(phi))
            total += (dens * ww).sum()
        return total


def run(calc, *args):
    with tempfile.TemporaryDirectory() as tmp:
        csv = os.path.join(tmp, "out.csv")
        out = subprocess.check_output([exe, calc] + list(args) + ["--csv", csv],
                                      universal_newlines=True)
        rows = [l.split(",") for l in open(csv).read().splitlines() if not l.startswith("#")]
    head = rows[0]
    data = np.array([[float(v) for v in r] for r in rows[1:]])
    integrals = [float(l.split("integral")[1]) for l in out.splitlines() if "integral" in l]
    return head, data, integrals


def axis_of(atoms, i, j):
    d = np.abs(atoms[j] - atoms[i])
    a = int(np.argmax(d))
    assert d.sum() - d[a] < 1e-6, "the test needs a line along a Cartesian axis"
    return a, 1.0 if atoms[j][a] > atoms[i][a] else -1.0


for name, (i, j), nelec in (("hf", (1, 2), 10), ("c2h4", (1, 2), 16)):
    calc = os.path.join(root, name)
    from_molden = read_molden(os.path.join(root, name + ".molden"))[0]
    axis, sign = axis_of(from_molden, i - 1, j - 1)
    orc = Oracle(os.path.join(root, name + ".molden"), axis)
    occ = [m for m, mo in enumerate(orc.mos) if mo["occ"] > 0]
    weights = [orc.mos[m]["occ"] for m in occ]
    origin = orc.atoms[i - 1]
    absN = sum(weights)
    check(abs(absN - nelec) < 1e-9, "%s: %d electrons in the molden file" % (name, nelec))

    # Total density, whole plane, a long line so the tails are inside.
    head, data, ints = run(calc, "--atoms", str(i), str(j), "--quantity", "plane",
                           "--show", "total", "--margin", "6")
    s, got = data[:, 0], data[:, 1]
    want = np.array([orc.plane(origin[axis] + sign * v, occ, weights) for v in s])
    err = np.max(np.abs(got - want)) / np.max(want)
    check(err < 2e-3, "%s total: plane integral vs analytic, max dev %.2e of the peak %.3f e/A"
          % (name, err, np.max(want)))
    check(abs(ints[-1] - nelec) < 1e-3 * nelec,
          "%s total: integral along the line %.5f (N = %d)" % (name, ints[-1], nelec))

    # One orbital, unweighted: area 1.  The HOMO for HF; for ethene the pi.
    homo = occ[-1]
    head, data, ints = run(calc, "--atoms", str(i), str(j), "--quantity", "plane",
                           "--show", "selected", "--mo", str(homo + 1), "--margin", "6")
    want = np.array([orc.plane(origin[axis] + sign * v, [homo], [1.0]) for v in data[:, 0]])
    err = np.max(np.abs(data[:, 1] - want)) / np.max(want)
    check(err < 2e-3, "%s MO %d: plane integral vs analytic, max dev %.2e" % (name, homo + 1, err))
    check(abs(ints[-1] - 1) < 1e-3, "%s MO %d: integral along the line %.5f (1)"
          % (name, homo + 1, ints[-1]))

    if name == "c2h4":
        # The pi orbital: nothing on the axis, ~0.5 e/A per plane at the bond midpoint.
        head, line, _ = run(calc, "--atoms", str(i), str(j), "--quantity", "psi2",
                            "--show", "selected", "--mo", str(homo + 1))
        k = int(np.argmin(np.abs(data[:, 0] - data[-1, 0] / 2 - data[0, 0] / 2)))
        check(np.max(np.abs(line[:, 1])) < 1e-10 and data[k, 1] > 0.1,
              "c2h4 pi: on the axis max psi^2 %.1e, plane integral at mid-bond %.4f e/A"
              % (np.max(np.abs(line[:, 1])), data[k, 1]))
        # Cylinders of 0.5 and 1.5 A against the brute-force sum, at three planes.
        for R in (0.5, 1.5):
            head, cyl, ints = run(calc, "--atoms", str(i), str(j), "--quantity", "plane",
                                  "--show", "selected", "--mo", str(homo + 1), "--radius", str(R),
                                  "--points", "41")
            for v in (cyl[5, 0], cyl[20, 0], cyl[30, 0]):
                k = int(np.argmin(np.abs(cyl[:, 0] - v)))
                want = orc.cylinder(origin[axis] + sign * v, R, origin, [homo], [1.0])
                whole = orc.plane(origin[axis] + sign * v, [homo], [1.0])
                check(abs(cyl[k, 1] - want) < 2e-3 * whole + 1e-6 and want < whole,
                      "c2h4 pi within %.1f A at s = %.3f: %.5f vs brute force %.5f (whole plane %.5f)"
                      % (R, v, cyl[k, 1], want, whole))

print("FAILED %d" % fail if fail else "all passed")
sys.exit(1 if fail else 0)
