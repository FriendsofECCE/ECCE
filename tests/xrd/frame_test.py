#!/usr/bin/env python3
"""scripts/ecce_xrd.py rotating-crystal detector frame against independent
checks (#985, #972).

    tests/xrd/frame_test.py

 1. Every recorded NaCl (Mo Ka) reflection obeys Bragg's law at its 2theta.
 2. Its detector position is the scattered-beam direction k + R(phi) U G, with
    the rotation, orientation and G recomputed here from the CSV and the cell.
 3. A full turn about a cell axis records every reflection that a brute-force
    scan over phi finds on the detector (sign changes of |k + g|^2 - k^2).
 4. KCl: the all-odd reflections are below 1 % of 200.
 5. Same --seed gives the same image, another seed another noise; overlays
    change only the PNG, not the .npy or .csv.
Exit 77 (skip) without numpy.
"""
import csv
import math
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
try:
    import numpy as np
except ImportError:
    print("SKIP: numpy is not installed")
    sys.exit(77)

TOOL = os.path.join(ROOT, "scripts", "ecce_xrd.py")
CIF = os.path.join(ROOT, "tests", "fixtures", "cif")
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import ecce_xrd  # noqa: E402
LAM = ecce_xrd.DOUBLETS["mo"][0]   # Mo Ka1, the frame's default; read, not copied
A_NACL = 5.6402
failures = []


def check(ok, what, detail=""):
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else "   " + str(detail)))
    if not ok:
        failures.append(what)


def run(tmp, name, *args):
    p = subprocess.run([sys.executable, TOOL, "frame", os.path.join(CIF, name + ".cif")] + list(args),
                       capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr)
    return p.stdout


def read_csv(path):
    with open(path) as f:
        r = list(csv.DictReader(f))
    return {"hkl": np.array([[int(x["h"]), int(x["k"]), int(x["l"])] for x in r]),
            **{k: np.array([float(x[k]) for x in r]) for k in ("x", "y", "phi", "tt", "d", "I")}}


def rot(axis, deg):
    ax = np.array(axis, float)
    ax /= np.linalg.norm(ax)
    t = math.radians(deg)
    c, s = math.cos(t), math.sin(t)
    x, y, z = ax
    return np.array([[c + x * x * (1 - c), x * y * (1 - c) - z * s, x * z * (1 - c) + y * s],
                     [y * x * (1 - c) + z * s, c + y * y * (1 - c), y * z * (1 - c) - x * s],
                     [z * x * (1 - c) - y * s, z * y * (1 - c) + x * s, c + z * z * (1 - c)]])


def euler(a, b, c):
    return rot([0, 0, 1], a) @ rot([1, 0, 0], b) @ rot([0, 0, 1], c)


def main():
    tmp = tempfile.mkdtemp(prefix="frame_test_")
    base = ["--no-noise", "--pixels", "200"]

    # 1 and 2: a general orientation, 30 degrees about the vertical axis
    f1 = os.path.join(tmp, "a.csv")
    run(tmp, "nacl", "--source", "mo", "--phi", "0", "--osc", "30", "--axis", "y",
        "--orient", "23,17,11", *base, "--out", f1)
    r = read_csv(f1)
    n = len(r["I"])
    check(n > 20, "NaCl frame records reflections", n)
    d = A_NACL / np.sqrt((r["hkl"] ** 2).sum(axis=1))
    bragg = np.degrees(2 * np.arcsin(LAM / (2 * d)))
    check(np.abs(bragg - r["tt"]).max() < 1e-3, "Bragg's law at every recorded 2theta",
          np.abs(bragg - r["tt"]).max())
    check(((r["phi"] >= -1e-3) & (r["phi"] <= 30 + 1e-3)).all(), "crossing angles lie in the oscillation")
    U = euler(23, 17, 11)
    worst = 0.0
    for i in range(n):
        g = U @ (r["hkl"][i] / A_NACL)
        ko = rot([0, 1, 0], r["phi"][i]) @ g + np.array([0, 0, 1 / LAM])
        worst = max(worst, abs(np.linalg.norm(ko) * LAM - 1),
                    abs(90 * ko[0] / ko[2] - r["x"][i]) / 1e3, abs(90 * ko[1] / ko[2] - r["y"][i]) / 1e3)
    check(worst < 1e-4, "detector position is the scattered-beam direction", worst)

    # 3: a full turn about b with the cell axes along the lab axes
    f2 = os.path.join(tmp, "b.csv")
    run(tmp, "nacl", "--source", "mo", "--osc", "360", "--axis", "y", "--orient", "0,0,0",
        *base, "--out", f2)
    r = read_csv(f2)
    got = {tuple(int(v) for v in h) for h in r["hkl"]}
    k = 1 / LAM
    hm = 12
    rng = range(-hm, hm + 1)
    hkl = np.array([(h, kk, l) for h in rng for kk in rng for l in rng
                    if h % 2 == kk % 2 == l % 2 and (h, kk, l) != (0, 0, 0)], float)
    exp = set()
    ph = np.radians(np.arange(0, 360.0, 0.02))
    for hh in hkl:
        g = hh / A_NACL
        if np.linalg.norm(g) > 2 * k:
            continue
        # |g(phi) + k0|^2 - k^2 for the spindle along y: g turns in the x-z plane
        gx = g[0] * np.cos(ph) + g[2] * np.sin(ph)
        gz = -g[0] * np.sin(ph) + g[2] * np.cos(ph)
        fz = (gx ** 2 + g[1] ** 2 + (gz + k) ** 2) - k * k
        for j in np.nonzero(np.sign(fz[:-1]) * np.sign(fz[1:]) <= 0)[0]:
            kx, ky, kz = gx[j], g[1], gz[j] + k
            if kz > 0 and abs(90 * kx / kz) < 75 and abs(90 * ky / kz) < 75:
                exp.add(tuple(int(v) for v in hh))
    missing = exp - got
    extra = got - exp
    check(len(exp) > 50 and not missing and not extra,
          "full turn: every reflection on the detector is recorded, none else",
          (sorted(missing)[:5], sorted(extra)[:5]))

    # 4: KCl all-odd against 200
    f3 = os.path.join(tmp, "c.csv")
    run(tmp, "kcl", "--source", "mo", "--osc", "360", "--axis", "y", "--orient", "0,0,0",
        *base, "--out", f3)
    r = read_csv(f3)
    odd = (r["hkl"] % 2 != 0).all(axis=1)
    h200 = (np.sort(np.abs(r["hkl"]), axis=1) == [0, 0, 2]).all(axis=1)
    check(odd.any() and h200.any() and r["I"][odd].max() < 0.01 * r["I"][h200].mean(),
          "KCl all-odd reflections are below 1 % of 200",
          (r["I"][odd].max() / r["I"][h200].mean()) if odd.any() and h200.any() else "none")

    # 5: determinism
    npys = []
    for seed in (4, 4, 5):
        p = os.path.join(tmp, "s%d_%d.npy" % (seed, len(npys)))
        run(tmp, "nacl", "--seed", str(seed), "--pixels", "120", "--osc", "10", "--out", p)
        npys.append(np.load(p))
    check(np.array_equal(npys[0], npys[1]), "same seed, same image")
    check(not np.array_equal(npys[0], npys[2]), "another seed, other noise")
    a = os.path.join(tmp, "o1.npy")
    b = os.path.join(tmp, "o2.npy")
    run(tmp, "nacl", "--pixels", "120", "--out", a)
    run(tmp, "nacl", "--pixels", "120", "--label", "5", "--rings", "--out", b,
        "--out", os.path.join(tmp, "o.png"))
    check(np.array_equal(np.load(a), np.load(b)), "overlays leave the data alone")
    check(os.path.getsize(os.path.join(tmp, "o.png")) > 500, "PNG written")

    print("%d failure(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
