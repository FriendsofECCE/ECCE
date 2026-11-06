#!/usr/bin/env python3
"""ecce-morse (#858): Morse fit of a bond-length scan of a stored calculation.

Wx-free; reads Props/SCANCOORD and Props/SCANENERGY of a stored calculation
folder and fits

    V(r) = De (1 - exp(-a (r - re)))^2 + Einf - De

by least squares to the points within an energy window above the lowest
point (default: every point below the energy at the longest distance, the
bound part of the curve).  Reported: De, a, re, the harmonic wavenumber
omega_e (k = 2 De a^2, reduced mass of the two scanned atoms, most abundant
isotopes), the Morse anharmonicity omega_e x_e = omega_e^2 / (4 De) and
D0 = De - omega_e/2 + omega_e x_e/4.  The Builder's Relaxed Scan panel runs
`ecce-morse <calc> --json`.

Usage: ecce-morse <calc> [--window KJ_PER_MOL] [--atoms I,J] [--masses M1,M2]
                  [--json]
"""
import argparse
import importlib.machinery
import importlib.util
import json
import math
import os
import re
import sys
import xml.etree.ElementTree as ET

_here = os.path.dirname(os.path.abspath(__file__))

HARTREE_KJ = 2625.4996394799
HARTREE_EV = 27.211386245988
HARTREE_CM = 219474.6313632
AMU_KG = 1.66053906660e-27
HARTREE_J = 4.3597447222071e-18
C_CM = 2.99792458e10
CURVE_POINTS = 200


class MorseError(Exception):
    pass


# --- the fit ------------------------------------------------------------------

def morse(r, De, a, re_, Einf):
    u = 1.0 - math.exp(-a * (r - re_))
    return De * u * u + Einf - De


def _solve(A, b):
    """Gaussian elimination with partial pivoting; None if singular."""
    n = len(b)
    M = [row[:] + [b[i]] for i, row in enumerate(A)]
    for c in range(n):
        p = max(range(c, n), key=lambda i: abs(M[i][c]))
        if abs(M[p][c]) < 1e-300:
            return None
        M[c], M[p] = M[p], M[c]
        for i in range(c + 1, n):
            f = M[i][c] / M[c][c]
            for j in range(c, n + 1):
                M[i][j] -= f * M[c][j]
    x = [0.0] * n
    for i in range(n - 1, -1, -1):
        x[i] = (M[i][n] - sum(M[i][j] * x[j] for j in range(i + 1, n))) / M[i][i]
    return x


def _initial(r, e):
    """Start values from the lowest point, a parabola through it and its
    neighbours, and the energy at the longest distance."""
    i = min(range(len(e)), key=lambda k: e[k])
    re_ = r[i]
    Einf = e[max(range(len(r)), key=lambda k: r[k])]
    De = max(Einf - e[i], 1e-3 * (max(e) - min(e)) + 1e-12)
    k2 = None
    if 0 < i < len(r) - 1:
        (x0, y0), (x1, y1), (x2, y2) = (r[i - 1], e[i - 1]), (r[i], e[i]), (r[i + 1], e[i + 1])
        d = (x0 - x1) * (x0 - x2) * (x1 - x2)
        if d:
            k2 = 2 * (x2 * (y1 - y0) + x1 * (y0 - y2) + x0 * (y2 - y1)) / d
    a = math.sqrt(k2 / (2 * De)) if k2 and k2 > 0 else 2.0 / max(1e-6, max(r) - min(r))
    return [De, a, re_, Einf]


def fit(r, e, p0=None, iters=500):
    """Levenberg-Marquardt fit of (De, a, re, Einf) to points (r, e).
    Units are the caller's (r in Angstrom, e in kJ/mol here).  Returns
    (params, rms)."""
    if len(r) < 4:
        raise MorseError("a Morse fit needs at least four points; %d in the window" % len(r))
    p = list(p0 or _initial(r, e))
    lam = 1e-3

    def cost(q):
        if q[0] <= 0 or q[1] <= 0:
            return float("inf")
        try:
            return sum((morse(x, *q) - y) ** 2 for x, y in zip(r, e))
        except OverflowError:
            return float("inf")

    c = cost(p)
    for _ in range(iters):
        De, a, re_, _Einf = p
        JTJ = [[0.0] * 4 for _ in range(4)]
        JTr = [0.0] * 4
        for x, y in zip(r, e):
            ex = math.exp(-a * (x - re_))
            u = 1.0 - ex
            res = De * u * u + _Einf - De - y
            g = [u * u - 1.0, 2 * De * u * (x - re_) * ex, -2 * De * u * a * ex, 1.0]
            for i in range(4):
                JTr[i] += g[i] * res
                for j in range(4):
                    JTJ[i][j] += g[i] * g[j]
        improved = False
        while lam < 1e12:
            A = [[JTJ[i][j] + (lam * JTJ[i][i] if i == j else 0.0) for j in range(4)]
                 for i in range(4)]
            step = _solve(A, [-v for v in JTr])
            if step is None:
                lam *= 10
                continue
            q = [p[i] + step[i] for i in range(4)]
            cq = cost(q)
            if cq < c:
                rel = (c - cq) / max(c, 1e-300)
                p, c = q, cq
                lam = max(lam / 10, 1e-12)
                improved = True
                break
            lam *= 10
        if not improved or rel < 1e-15:
            break
    if not math.isfinite(c):
        raise MorseError("the Morse fit did not converge")
    return p, math.sqrt(c / len(r))


# --- the molecule -------------------------------------------------------------

def _load(module, installed):
    sys.path.insert(0, _here)
    try:
        return __import__(module)
    except ImportError:
        l = importlib.machinery.SourceFileLoader(module, os.path.join(_here, installed))
        s = importlib.util.spec_from_loader(module, l)
        m = importlib.util.module_from_spec(s)
        l.exec_module(m)
        return m
    finally:
        sys.path.pop(0)


def isotope_masses():
    """{symbol: mass of the most abundant isotope} from ECCE's Isotopes.txt."""
    cands = [os.path.join(_here, os.pardir, "data", "client", "config", "Isotopes.txt")]
    if os.environ.get("ECCE_HOME"):
        cands.insert(0, os.path.join(os.environ["ECCE_HOME"], "data", "client", "config",
                                     "Isotopes.txt"))
    for path in cands:
        if os.path.isfile(path):
            best = {}
            for ln in open(path, errors="replace"):
                f = ln.split()
                if len(f) < 5 or "-" not in f[1]:
                    continue
                sym, ab, m = f[1].split("-")[1], float(f[3]), float(f[4])
                if sym not in best or ab > best[sym][0]:
                    best[sym] = (ab, m)
            return {s: v[1] for s, v in best.items()}
    raise MorseError("Isotopes.txt not found; give the masses with --masses")


def _vector(calc, key):
    path = os.path.join(calc, "Props", key)
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError):
        return None, ""
    return [float(v) for v in (root.text or "").split()], root.get("units", "")


def scanned_atoms(calc, natoms):
    """0-based indices of the scanned bond: the stored constraint, the
    scan line of the input, or the two atoms of a diatomic."""
    gcst = os.path.join(calc, "Parameters", "GeomConstraintModel.gcst")
    if os.path.isfile(gcst):
        t = open(gcst, errors="replace").read()
        for m in re.finditer(r"<Constraint[^>]*bond[^>]*>(.*?)</Constraint>", t, re.S | re.I):
            if "<Scan" in m.group(1):
                g = re.search(r"<Group>(.*?)</Group>", m.group(1))
                if g and len(g.group(1).split()) == 2:
                    return [int(x) for x in g.group(1).split()]
    for d in ("Inputs", "Outputs"):
        folder = os.path.join(calc, d)
        if not os.path.isdir(folder):
            continue
        for name in sorted(os.listdir(folder)):
            try:
                t = open(os.path.join(folder, name), errors="replace").read(400000)
            except OSError:
                continue
            m = re.search(r"^\|?\s*\d*>?\s*Scan\s+B\s+(\d+)\s+(\d+)\s*=", t, re.M | re.I)
            if m:   # ORCA counts from 0
                return [int(m.group(1)), int(m.group(2))]
    if natoms == 2:
        return [0, 1]
    raise MorseError("which bond was scanned is not stored; name it with --atoms I,J "
                     "(counted from 1)")


def read_scan(calc):
    r, ru = _vector(calc, "SCANCOORD")
    e, _ = _vector(calc, "SCANENERGY")
    if not r or not e:
        raise MorseError("%s has no scan (Props/SCANCOORD, SCANENERGY)" % os.path.basename(calc))
    if ru == "Degree":
        raise MorseError("this scan is of an angle or torsion; a Morse fit needs a bond length")
    n = min(len(r), len(e))
    return r[:n], e[:n]


# --- the analysis -------------------------------------------------------------

def analyse(r, e_hartree, mu_amu, window_kj=None):
    """Fit and derived constants; every energy in the result has its unit in
    its key.  window_kj: points up to this far above the lowest are used;
    None: the bound points (below the energy at the longest distance)."""
    pts = sorted(zip(r, e_hartree))
    rs = [p[0] for p in pts]
    emin = min(p[1] for p in pts)
    ekj = [(p[1] - emin) * HARTREE_KJ for p in pts]
    if window_kj is None:
        top = ekj[-1]
        mode = "bound"
    else:
        top = float(window_kj)
        mode = "window"
    use = [i for i in range(len(rs)) if ekj[i] <= top + 1e-9]
    x =[rs[i] for i in use]
    y = [ekj[i] for i in use]
    imin = min(range(len(y)), key=lambda k: y[k]) if y else 0
    if not y or imin == 0 or imin == len(y) - 1:
        raise MorseError("the window has no point on both sides of the minimum; widen it")
    p, rms = fit(x, y)
    De_kj, a, re_, Einf_kj = p
    De_h = De_kj / HARTREE_KJ
    k_si = 2 * De_h * HARTREE_J * (a * 1e10) ** 2           # N/m
    omega = math.sqrt(k_si / (mu_amu * AMU_KG)) / (2 * math.pi * C_CM)   # cm-1
    De_cm = De_h * HARTREE_CM
    wexe = omega * omega / (4 * De_cm)
    D0_cm = De_cm - omega / 2 + wexe / 4
    lo, hi = min(rs), max(rs)
    curve_r = [lo + (hi - lo) * i / (CURVE_POINTS - 1) for i in range(CURVE_POINTS)]
    return {
        "points_used": len(use), "points_total": len(rs),
        "window_kJ_per_mol": top, "window_mode": mode,
        "r_used_A": x,
        "De_hartree": De_h, "De_eV": De_h * HARTREE_EV, "De_kJ_per_mol": De_kj,
        "a_per_A": a, "re_A": re_,
        "Einf_hartree": emin + Einf_kj / HARTREE_KJ,
        "E_min_hartree": emin,
        "k_N_per_m": k_si,
        "mu_amu": mu_amu,
        "omega_e_cm-1": omega, "omega_e_xe_cm-1": wexe,
        "D0_cm-1": D0_cm, "D0_eV": D0_cm / HARTREE_CM * HARTREE_EV,
        "D0_kJ_per_mol": D0_cm / HARTREE_CM * HARTREE_KJ,
        "rms_kJ_per_mol": rms,
        "curve_r_A": curve_r,
        "curve_E_hartree": [emin + morse(v, *p) / HARTREE_KJ for v in curve_r],
    }


SUP = str.maketrans("-0123456789", "⁻⁰¹²³⁴⁵⁶⁷⁸⁹")


def num(x, sig=4):
    """sig significant figures; 1.2 × 10⁻³ instead of e-notation."""
    if x == 0 or not math.isfinite(x):
        return "%g" % x
    ex = int(math.floor(math.log10(abs(x))))
    if -2 <= ex < 5:
        return "%.*f" % (max(0, sig - 1 - ex), x)
    m = x / 10 ** ex
    return "%.*f × 10%s" % (sig - 1, m, str(ex).translate(SUP))


def report(res, name="", pair=""):
    lines = []
    head = "Morse fit" + (" of " + name if name else "")
    if pair:
        head += " (%s, μ = %s u)" % (pair, num(res["mu_amu"], 5))
    lines.append(head)
    lines.append("%d of %d points, up to %s kJ/mol above the lowest (%s)" % (
        res["points_used"], res["points_total"], num(res["window_kJ_per_mol"], 4),
        "all bound points" if res["window_mode"] == "bound" else "chosen window"))
    lines.append("Dₑ = %s eV = %s kJ/mol" % (num(res["De_eV"]), num(res["De_kJ_per_mol"])))
    lines.append("a = %s Å⁻¹" % num(res["a_per_A"]))
    lines.append("rₑ = %s Å" % num(res["re_A"]))
    lines.append("ωₑ = %s cm⁻¹" % num(res["omega_e_cm-1"]))
    lines.append("ωₑxₑ = %s cm⁻¹" % num(res["omega_e_xe_cm-1"]))
    lines.append("D₀ = Dₑ − ωₑ/2 + ωₑxₑ/4 = %s eV = %s kJ/mol"
                 % (num(res["D0_eV"]), num(res["D0_kJ_per_mol"])))
    lines.append("RMS residual = %s kJ/mol" % num(res["rms_kJ_per_mol"], 2))
    return "\n".join(lines)


def run(calc, window=None, atoms=None, masses=None):
    calc = os.path.abspath(calc[len("file://"):] if calc.startswith("file://") else calc)
    r, e = read_scan(calc)
    S = _load("ecce_si_export", "ecce-si-export")
    try:
        els = S._chemsys(calc)[0]
    except OSError:
        els = []
    pair = ""
    if masses:
        m1, m2 = masses
    else:
        idx = atoms if atoms else scanned_atoms(calc, len(els))
        if not els or max(idx) >= len(els) or min(idx) < 0:
            raise MorseError("the scanned atoms are not in the stored structure; "
                             "give --masses M1,M2")
        table = isotope_masses()
        sym = [re.sub(r"[^A-Za-z]", "", els[i]).capitalize() for i in idx]
        if any(s not in table for s in sym):
            raise MorseError("no isotope mass for %s; give --masses M1,M2" % "/".join(sym))
        m1, m2 = table[sym[0]], table[sym[1]]
        pair = "%s%d–%s%d" % (sym[0], idx[0] + 1, sym[1], idx[1] + 1)
    res = analyse(r, e, m1 * m2 / (m1 + m2), window)
    res["calculation"] = os.path.basename(calc)
    res["bond"] = pair
    res["report"] = report(res, res["calculation"], pair)
    return res


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 usage="ecce-morse <calc> [--window KJ_PER_MOL] "
                                       "[--atoms I,J] [--masses M1,M2] [--json]")
    ap.add_argument("calc")
    ap.add_argument("--window", type=float,
                    help="kJ/mol above the lowest point (default: all bound points)")
    ap.add_argument("--atoms", help="the scanned bond, counted from 1 (default: from the scan)")
    ap.add_argument("--masses", help="atomic masses in u (default: most abundant isotopes)")
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args(argv)
    try:
        atoms = [int(v) - 1 for v in a.atoms.split(",")] if a.atoms else None
        masses = [float(v) for v in a.masses.split(",")] if a.masses else None
        if (atoms and len(atoms) != 2) or (masses and len(masses) != 2):
            raise MorseError("--atoms and --masses take two values")
        res = run(a.calc, a.window, atoms, masses)
    except (MorseError, ValueError) as exc:
        if a.json:
            print(json.dumps({"error": str(exc)}))
        sys.stderr.write("error: %s\n" % exc)
        return 1
    if a.json:
        print(json.dumps(res, indent=1, ensure_ascii=False))
    else:
        print(res["report"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
