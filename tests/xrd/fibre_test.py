#!/usr/bin/env python3
"""scripts/ecce_xrd.py fibre patterns against independent oracles (#971).

    tests/xrd/fibre_test.py

 1. The cylindrical average by the Bessel expansion equals the direct average
    of |F|^2 over rotation angles (no Bessel functions in the direct one).
 2. An ideal helix has J_l(2 pi R r)^2 on layer line l (Cochran-Crick-Vand).
 3. Form factors equal gemmi's IT92 (skipped without gemmi).
 4. The sample B-DNA has the known layer lines, a meridional 3.4 A reflection,
    a weak layer line 4 and the X; the alpha-helix its 5.4 A pitch and the
    1.5 A meridional reflection.
Exit 77 (skip) without numpy.
"""
import math
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
try:
    import numpy as np
except ImportError:
    print("SKIP: numpy is not installed")
    sys.exit(77)
import ecce_xrd as xrd  # noqa: E402

LIB = os.path.join(ROOT, "data", "client", "StructureLibrary", "Teaching", "Fibres")
failures = []


def check(ok, what, detail=""):
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else "   " + str(detail)))
    if not ok:
        failures.append(what)
    return ok


def jn_quadrature(n, x, k=20000):
    t = (np.arange(k) + 0.5) * math.pi / k
    return float(np.mean(np.cos(n * t - x * np.sin(t))))


def test_average():
    rng = np.random.RandomState(1)
    sym = list(rng.choice(["C", "N", "O", "P", "H"], 40))
    xyz = rng.uniform(-9, 9, (40, 3))
    xyz[:, 2] *= 3
    m = xrd.Model(sym, xyz, axis=[0, 0, 1])
    rs, zs = [0.0, 0.03, 0.11, 0.27, 0.45], [0.0, 0.05, 0.2, 0.37, 0.6]
    a = xrd.intensity_rz(m, rs, zs, bfactor=3.0)
    b = xrd.intensity_rz_bruteforce(m, rs, zs, 720, bfactor=3.0)
    worst = float(np.max(np.abs(a - b) / np.maximum(b, 1e-9 * b.max())))
    check(worst < 1e-6, "Bessel average equals the direct psi average (worst relative %.1e)" % worst, worst)
    x = np.array([0.0, 0.3, 7.5, 31.0])
    t = xrd.bessel_table(x, 40)
    ref = np.array([[jn_quadrature(n, v) for n in list(range(0, 41)) + list(range(-40, 0))]
                    for v in x])
    err = float(np.abs(t - ref).max())
    check(err < 1e-9, "Bessel table equals the integral definition (max error %.1e)" % err, err)
    try:
        from scipy.special import jv
        err = max(abs(t[i, n] - jv(n, x[i])) for i in range(4) for n in range(41))
        check(err < 1e-12, "Bessel table equals scipy.special.jv (%.1e)" % err, err)
    except ImportError:
        print("skip scipy.special.jv comparison (scipy absent)")


def test_ideal_helix():
    r, pitch, turns, per = 7.0, 34.0, 30, 60
    k = np.arange(turns * per)
    z = k * pitch / per
    phi = 2 * np.pi * k / per
    xyz = np.stack([r * np.cos(phi), r * np.sin(phi), z], axis=1)
    m = xrd.Model(["C"] * len(k), xyz, axis=[0, 0, 1])
    rs = np.linspace(0.01, 0.5, 40)
    worst = 0.0
    for l in range(1, 7):
        zl = l / pitch
        i = xrd.intensity_rz(m, rs, [zl])[:, 0]
        f = xrd.form_factor("C", 0.5 * np.hypot(rs, zl))
        expect = np.array([jn_quadrature(l, 2 * math.pi * R * r) ** 2 for R in rs])
        got = i / (f * f * len(k) ** 2)
        worst = max(worst, float(np.abs(got - expect).max()))
    check(worst < 1e-8, "ideal helix: layer lines l=1..6 are J_l(2 pi R r)^2 (max abs error %.1e)" % worst, worst)
    off = xrd.intensity_rz(m, [0.2], [0.5 / pitch, 1.5 / pitch])[0]
    check(off.max() < 1e-4 * xrd.intensity_rz(m, [0.2], [1 / pitch])[0, 0],
          "ideal helix: nothing between the layer lines", off)


def test_form_factors():
    z = {"H": 1, "C": 6, "N": 7, "O": 8, "Na": 11, "Mg": 12, "P": 15, "S": 16, "Cl": 17,
         "K": 19, "Ca": 20, "Fe": 26, "Zn": 30}
    bad = [e for e, n in z.items() if abs(xrd.form_factor(e, 0.0) - n) > 0.02]
    check(not bad, "f(0) equals Z for every tabulated element", bad)
    try:
        import gemmi
    except ImportError:
        print("skip gemmi comparison (gemmi absent)")
        return
    worst = 0.0
    for e in z:
        for s in np.linspace(0, 1.0, 11):
            ref = gemmi.Element(e).it92.calculate_sf(s * s)
            worst = max(worst, abs(float(xrd.form_factor(e, s)) - ref))
    check(worst < 1e-3, "form factors equal gemmi's IT92 (max difference %.1e)" % worst, worst)


def layer(m, l, pitch, rs):
    return xrd.intensity_rz(m, rs, [l / pitch])[:, 0]


def test_bdna():
    path = os.path.join(LIB, "B-DNA_20bp.mvm")
    if not os.path.exists(path):
        check(False, "sample B-DNA is installed", path)
        return
    sym, xyz = xrd.read_structure(path)
    m = xrd.Model(sym, xyz)
    pitch = 34.0
    zs = np.arange(0.0, 0.34, 0.001)
    rs = np.arange(0.0, 0.40, 0.005)
    g = xrd.intensity_rz(m, rs, zs)
    prof = (g * np.maximum(rs, 0.005)[:, None]).sum(axis=0)
    # layer lines: local maxima of the R-weighted profile (parabolic refinement)
    peaks = []
    for i in range(1, len(zs) - 1):
        if prof[i] > prof[i - 1] and prof[i] >= prof[i + 1] and zs[i] > 0.02:
            d = 0.5 * (prof[i - 1] - prof[i + 1]) / (prof[i - 1] - 2 * prof[i] + prof[i + 1])
            peaks.append(zs[i] + d * 0.001)
    found = {l: min(peaks, key=lambda p: abs(p - l / pitch)) for l in (2, 3, 5, 6, 7, 8, 9, 10)}
    off = {l: found[l] * pitch / l - 1 for l in found}
    check(all(abs(v) < 0.02 for v in off.values()),
          "layer lines at l/34 A^-1 within 2 percent (relative offsets %s)"
          % {l: round(v, 4) for l, v in off.items()}, off)
    mer = g[0]
    sel = zs > 0.1
    zbest = zs[sel][np.argmax(mer[sel])]
    check(abs(zbest - 10 / pitch) < 0.008,
          "strongest meridional reflection beyond Z=0.1 is layer line 10 (Z=%.4f, 10/34=%.4f)" % (zbest, 10 / pitch), zbest)
    rr = np.arange(0.005, 0.5, 0.005)
    tot = {l: float((layer(m, l, pitch, rr) * rr).sum()) for l in (3, 4, 5)}
    check(tot[4] < tot[3] and tot[4] < tot[5],
          "layer line 4 is weak against 3 and 5 (%s)" % {k: "%.3g" % v for k, v in tot.items()}, tot)
    rmax = {}
    for l in (1, 2, 3):
        rr = np.arange(0.01, 0.30, 0.005)
        rmax[l] = float(rr[np.argmax(layer(m, l, pitch, rr))])
    check(rmax[1] < rmax[2] < rmax[3],
          "the X: the maxima of layer lines 1-3 move outward (%s)" % rmax, rmax)


def test_helix():
    path = os.path.join(LIB, "Alpha-helix_polyAla.mvm")
    if not os.path.exists(path):
        check(False, "sample alpha-helix is installed", path)
        return
    sym, xyz = xrd.read_structure(path)
    m = xrd.Model(sym, xyz)
    nit = [i for i, s in enumerate(sym) if s == "N"]
    zn = m.z[nit]
    ph = np.unwrap(m.phi[nit])
    rise = abs(float(np.polyfit(np.arange(len(nit)), zn, 1)[0]))
    turn = abs(float(np.polyfit(np.arange(len(nit)), ph, 1)[0]))
    pitch = rise * 2 * math.pi / turn
    check(abs(rise - 1.5) < 0.1 and abs(pitch - 5.4) < 0.15,
          "model rise %.2f A, pitch %.2f A" % (rise, pitch), (rise, pitch))
    zs = np.arange(0.0, 0.85, 0.0025)
    rs = np.arange(0.0, 0.5, 0.005)
    g = xrd.intensity_rz(m, rs, zs)
    prof = (g * np.maximum(rs, 0.005)[:, None]).sum(axis=0)
    peaks = [zs[i] for i in range(1, len(zs) - 1)
             if prof[i] > prof[i - 1] and prof[i] >= prof[i + 1] and zs[i] > 0.1]
    check(any(abs(p - 1 / 5.4) < 0.01 for p in peaks),
          "layer line at 1/5.4 A^-1 (peaks %s)" % np.round(peaks, 3), peaks)
    sel = zs > 0.5
    zb = zs[sel][np.argmax(g[0][sel])]
    check(abs(zb - 1 / 1.5) < 0.03, "meridional reflection near 1/1.5 A^-1 (Z=%.3f)" % zb, zb)


def test_cli():
    with tempfile.TemporaryDirectory() as d:
        png, csv = os.path.join(d, "p.png"), os.path.join(d, "p.csv")
        src = os.path.join(LIB, "Alpha-helix_polyAla.mvm")
        cmd = [sys.executable, os.path.join(ROOT, "scripts", "ecce_xrd.py"), "fibre", src,
               "--pixels", "120", "--out", png, "--out", csv, "--json"]
        r = subprocess.run(cmd, capture_output=True, text=True)
        check(r.returncode == 0 and open(png, "rb").read(8) == b"\x89PNG\r\n\x1a\n" and
              len(open(csv).read().splitlines()) == 120 * 120 + 1,
              "CLI writes the PNG and the CSV", r.stderr[-300:])
        r = subprocess.run(cmd[:4] + ["--select", "999999"], capture_output=True, text=True)
        check(r.returncode == 1 and "error:" in r.stderr, "a bad selection is an error, not a traceback")


def main():
    test_average()
    test_ideal_helix()
    test_form_factors()
    test_bdna()
    test_helix()
    test_cli()
    print("FAIL: %s" % ", ".join(failures) if failures else "PASS: xrd fibre")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
