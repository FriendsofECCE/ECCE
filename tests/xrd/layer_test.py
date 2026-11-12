#!/usr/bin/env python3
"""scripts/ecce_xrd.py single-crystal layers against independent oracles (#972).

    tests/xrd/layer_test.py

 1. |F| of the layer spots equals gemmi's (own symmetry expansion, own IT92
    tables) for NaCl, CsCl and quartz (skipped without gemmi).
 2. Absences: NaCl and Cu hk0 only h,k both even; CsCl hk0 all present; quartz
    00l only l = 3n (the 3_1 screw axis).
 3. Symmetry of the hk0 layer: NaCl and Cu 4mm, quartz six-fold in intensity.
 4. Layer geometry: offset layers, --normal, resolution; CLI CSV, PNG, --json.
Exit 77 (skip) without numpy.
"""
import json
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

CIF = os.path.join(ROOT, "tests", "fixtures", "cif")
TOOL = os.path.join(ROOT, "scripts", "ecce_xrd.py")
failures = []


def check(ok, what, detail=""):
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else "   " + str(detail)))
    if not ok:
        failures.append(what)
    return ok


def cif(name):
    return os.path.join(CIF, name + ".cif")


def layer(name, zone, offset=0, dmin=1.0, **kw):
    lat, sym, frac, occ = xrd.read_periodic(cif(name))
    return xrd.layer_reflections(lat, sym, frac, occ, zone, offset, dmin, **kw)


def table(lay):
    return {tuple(int(v) for v in h): i for h, i in zip(lay["hkl"], lay["I"])}


def test_gemmi():
    try:
        import gemmi
    except ImportError:
        print("SKIP gemmi comparison: gemmi is not installed")
        return
    B = 1.0
    for name, zone, off in (("nacl", [0, 0, 1], 0), ("cscl", [0, 0, 1], 0),
                            ("quartz", [0, 1, 0], 0), ("quartz", [0, 0, 1], 1),
                            ("nacl", [1, 0, 0], 2)):
        sm = gemmi.read_small_structure(cif(name))
        sites = sm.get_all_unit_cell_sites()
        lay = layer(name, zone, off, 0.9, bfactor=B)
        worst, n = 0.0, 0
        for h, f in zip(lay["hkl"], lay["F"]):
            stol2 = (0.5 / sm.cell.calculate_d([int(v) for v in h])) ** 2
            tot = 0j
            for st in sites:
                ph = 2 * math.pi * (h[0] * st.fract.x + h[1] * st.fract.y + h[2] * st.fract.z)
                tot += (st.occ * st.element.it92.calculate_sf(stol2) *
                        math.exp(-B * stol2) * complex(math.cos(ph), math.sin(ph)))
            if abs(tot) > 1e-3 * lay["F"].max():
                worst = max(worst, abs(f - abs(tot)) / abs(tot))
                n += 1
            else:
                worst = max(worst, abs(f - abs(tot)) / lay["F"].max() * 1e3)
        check(worst < 1e-4 and n > 5, "%s zone %s layer %d: |F| agrees with gemmi on %d spots (worst %.1e)"
              % (name, zone, off, n, worst), worst)


def test_absences():
    for name in ("nacl", "cu"):
        t = table(layer(name, [0, 0, 1]))
        imax = max(t.values())
        ok = all((i > 1e-3 * imax) == (h % 2 == 0 and k % 2 == 0) and
                 (i < 1e-9 * imax or i > 1e-3 * imax) for (h, k, l), i in t.items())
        check(ok and len(t) > 30, "%s hk0: only h,k both even (%d spots)" % (name, len(t)))
    t = table(layer("cscl", [0, 0, 1]))
    imax = max(t.values())
    check(all(i > 1e-3 * imax for i in t.values()), "CsCl hk0: all present")
    t = table(layer("nacl", [1, 0, 0], 1))
    imax = max(t.values())
    odd = [(k, l) for (h, k, l), i in t.items() if i > 1e-3 * imax]
    check(all(k % 2 == l % 2 for k, l in odd) and len(odd) > 5,
          "NaCl 1kl layer: strong spots only k,l both even or both odd (%d)" % len(odd))
    lat, sym, frac, occ = xrd.read_periodic(cif("quartz"))
    hkl = np.array([[0, 0, l] for l in range(-8, 9) if l])
    s = np.linalg.norm(hkl @ np.linalg.inv(lat).T, axis=1) / 2
    f2 = xrd.structure_factors2(sym, frac, occ, hkl, s)
    present = [int(h[2]) for h, v in zip(hkl, f2) if v > 1e-6 * f2.max()]
    check(present and all(l % 3 == 0 for l in present) and
          all(l in present for l in (-6, -3, 3, 6)), "quartz 00l: only l = 3n (%s)" % present)
    lay = layer("quartz", [0, 1, 0], 0, 0.8)
    t = table(lay)
    imax = max(t.values())
    col = {l: t[(0, 0, l)] for (h, k, l) in t if h == 0 and k == 0}
    check(all((v > 1e-6 * imax) == (l % 3 == 0) for l, v in col.items()) and len(col) > 6,
          "quartz h0l layer: the 00l column has only l = 3n")


def test_symmetry():
    for name in ("nacl", "cu"):
        t = table(layer(name, [0, 0, 1], 0, 0.9))
        ok = all(abs(i - t.get((k, h, l), -1)) < 1e-6 * i + 1e-9 and
                 abs(i - t.get((-h, k, l), -1)) < 1e-6 * i + 1e-9 for (h, k, l), i in t.items())
        check(ok, "%s hk0: 4mm, |F|(hk0) = |F|(kh0) = |F|(-hk0)" % name)
    t = table(layer("quartz", [0, 0, 1], 0, 0.9))
    imax = max(t.values())
    # rotation by 60 degrees of the hexagonal net, and inversion
    ok = all(abs(i - t[(h + k, -h, 0)]) < 1e-6 * imax and abs(i - t[(-h, -k, 0)]) < 1e-6 * imax
             for (h, k, l), i in t.items() if (h + k, -h, 0) in t)
    nmatch = sum(1 for (h, k, l) in t if (h + k, -h, 0) in t)
    check(ok and nmatch > 20, "quartz hk0: six-fold in intensity (%d spots compared)" % nmatch)


def test_geometry():
    lat, sym, frac, occ = xrd.read_periodic(cif("nacl"))
    a = lat[0, 0]
    l1 = layer("nacl", [0, 0, 1], 2, 1.0)
    check(np.all(l1["hkl"][:, 2] == 2) and abs(l1["height"] - 2 / a) < 1e-9,
          "layer l=2: all spots have l = 2, height 2/a")
    ln = layer("nacl", [1, 1, 1], 0, 1.2)
    check(np.all(ln["hkl"].sum(axis=1) == 0) and np.all(ln["d"] >= 1.2 - 1e-9),
          "normal [111] layer 0: h+k+l = 0, d >= d_min")
    lw = layer("nacl", [0, 0, 1], 0, 0.5, wavelength=1.5406)
    check(lw["gmax"] <= 2 / 1.5406 + 1e-12 and lw["d"].min() >= 1.5406 / 2 - 1e-9,
          "a wavelength limits 1/d to 2/lambda")
    try:
        layer("nacl", [0, 0, 1], 9, 1.0)
        check(False, "a layer beyond the resolution is refused")
    except ValueError:
        check(True, "a layer beyond the resolution is refused")
    lh = layer("quartz", [0, 1, 0], 0, 1.0)
    a1 = [np.linalg.norm(v) for v in lh["axes"]]
    check(a1[0] > 0 and a1[2] > 0 and a1[1] == 0, "h0l: a* and c* drawn, b* (out of the plane) not")


def cli(*args):
    p = subprocess.run([sys.executable, TOOL, "layer"] + list(args), capture_output=True, text=True)
    return p.returncode, p.stdout, p.stderr


def test_cli(tmp):
    png, csvf = os.path.join(tmp, "l.png"), os.path.join(tmp, "l.csv")
    rc, out, err = cli(cif("nacl"), "--zone", "l=0", "--show-absent", "--out", png, "--out", csvf)
    check(rc == 0 and open(png, "rb").read(8) == b"\x89PNG\r\n\x1a\n", "CLI writes a PNG", err)
    rows = [ln.split(",") for ln in open(csvf).read().splitlines()]
    check(rows[0][:3] == ["h", "k", "l"] and rows[0][5:8] == ["d", "F", "I"] and len(rows) > 50,
          "CSV has h k l, d, |F|, I")
    rc, out, err = cli(cif("nacl"), "--zone", "l=0", "--json")
    j = json.loads(out)
    check(rc == 0 and j["reflections"] == len(rows) - 1 and j["absent"] > 0 and
          len(j["table"]) == j["reflections"], "--json agrees with the CSV")
    rc, out, err = cli(cif("quartz"), "--normal", "0", "1", "0", "--offset", "1", "--json")
    check(rc == 0 and all(r["hkl"][1] == 1 for r in json.loads(out)["table"]),
          "--normal 0 1 0 --offset 1 is the k = 1 layer", err)
    rc, _, err = cli(cif("nacl"), "--zone", "x=0")
    check(rc == 1 and "--zone" in err, "a bad --zone is refused", err)
    flat = os.path.join(tmp, "flat.xyz")
    with open(flat, "w") as f:
        f.write("2\nno lattice\nO 0 0 0\nH 0 0 1\n")
    rc, _, err = cli(flat)
    check(rc == 1 and "not periodic" in err, "a structure without a lattice is refused", err)


def main():
    with tempfile.TemporaryDirectory() as tmp:
        test_gemmi()
        test_absences()
        test_symmetry()
        test_geometry()
        test_cli(tmp)
    print("%d failure(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
