#!/usr/bin/env python3
"""scripts/ecce_xrd.py powder patterns against independent oracles (#587).

    tests/xrd/powder_test.py

 1. |F(hkl)| equals gemmi's StructureFactorCalculatorX (own symmetry expansion,
    own IT92 tables) for NaCl, KCl, CsCl, Cu, Si and quartz (skipped without gemmi).
 2. Systematic absences: fcc, bcc, simple cubic, rock salt, KCl.
 3. Published patterns: Si, NaCl, Cu line positions and the NaCl intensity ratio.
 4. Multiplicities of the cubic forms.
 5. CLI: table, CSV, PNG, --json, --measured, .xyz/.mvm input with a lattice,
    the stored Packing in solids examples.
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
EX = os.path.join(ROOT, "data", "client", "examples", "genchem", "packing-in-solids")
TOOL = os.path.join(ROOT, "scripts", "ecce_xrd.py")
failures = []


def check(ok, what, detail=""):
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else "   " + str(detail)))
    if not ok:
        failures.append(what)
    return ok


def cif(name):
    return os.path.join(CIF, name + ".cif")


def allhkl(lat, n):
    r = np.arange(-n, n + 1)
    h = np.stack(np.meshgrid(r, r, r, indexing="ij"), -1).reshape(-1, 3)
    return h[np.any(h != 0, axis=1)]


def f2_of(path, hkl, b=0.0):
    lat, sym, frac, occ = xrd.read_periodic(path)
    s = np.linalg.norm(hkl @ np.linalg.inv(lat).T, axis=1) / 2
    return xrd.structure_factors2(sym, frac, occ, hkl, s, b), lat


def p1_cif(tmp, name, a, atoms):
    path = os.path.join(tmp, name + ".cif")
    with open(path, "w") as f:
        f.write("data_%s\n_cell_length_a %g\n_cell_length_b %g\n_cell_length_c %g\n"
                "_cell_angle_alpha 90\n_cell_angle_beta 90\n_cell_angle_gamma 90\n"
                "loop_\n_atom_site_label\n_atom_site_fract_x\n_atom_site_fract_y\n"
                "_atom_site_fract_z\n" % (name, a, a, a))
        for el, p in atoms:
            f.write("%s %s\n" % (el, p))
    return path


def test_gemmi():
    try:
        import gemmi
    except ImportError:
        print("SKIP gemmi comparison: gemmi is not installed")
        return
    B = 1.0
    for name in ("nacl", "kcl", "cscl", "cu", "si", "quartz"):
        sm = gemmi.read_small_structure(cif(name))
        # gemmi's own symmetry expansion and IT92 tables, summed by hand: its
        # calculate_sf_from_small_structure counts a special position once per
        # operation.
        sites = sm.get_all_unit_cell_sites()
        lat, sym, frac, occ = xrd.read_periodic(cif(name))
        hkl = allhkl(lat, 5)
        s = np.linalg.norm(hkl @ np.linalg.inv(lat).T, axis=1) / 2
        mine = np.sqrt(xrd.structure_factors2(sym, frac, occ, hkl, s, B))
        ref = np.empty(len(hkl))
        for i, h in enumerate(hkl):
            stol2 = float(s[i]) ** 2
            tot = 0j
            for st in sites:
                ph = 2 * math.pi * (h[0] * st.fract.x + h[1] * st.fract.y + h[2] * st.fract.z)
                tot += (st.occ * st.element.it92.calculate_sf(stol2) *
                        math.exp(-B * stol2) * complex(math.cos(ph), math.sin(ph)))
            ref[i] = abs(tot)
        check(len(sites) == len(sym), "%s: %d atoms in the cell as gemmi expands it" % (name, len(sym)),
              (len(sites), len(sym)))
        sg = np.array([0.5 / sm.cell.calculate_d([int(v) for v in h]) for h in hkl])
        big = ref > 1e-3 * ref.max()
        rel = float(np.max(np.abs(mine[big] - ref[big]) / ref[big]))
        small = float(np.max(np.abs(mine[~big] - ref[~big]))) if (~big).any() else 0.0
        check(rel < 1e-4 and small < 1e-3 * ref.max() and np.abs(s - sg).max() < 1e-9,
              "%s: |F| agrees with gemmi on %d reflections (worst relative %.1e, s %.1e)"
              % (name, big.sum(), rel, np.abs(s - sg).max()),
              (rel, small))


def test_absences(tmp):
    lat = None
    f2, lat = f2_of(cif("cu"), allhkl(None, 6))
    h = allhkl(None, 6)
    par = np.array([len(set(int(v) % 2 for v in r)) == 1 for r in h])
    check(np.all(f2[par] > 1e-3 * f2.max()) and np.all(f2[~par] < 1e-9 * f2.max()),
          "fcc Cu: only all-even or all-odd hkl")
    na = p1_cif(tmp, "na", 4.2906, [("Na", "0 0 0"), ("Na", "0.5 0.5 0.5")])
    f2, _ = f2_of(na, h)
    ev = h.sum(axis=1) % 2 == 0
    check(np.all(f2[ev] > 1e-3 * f2.max()) and np.all(f2[~ev] < 1e-9 * f2.max()),
          "bcc Na: h+k+l even")
    po = p1_cif(tmp, "po", 3.35, [("Po", "0 0 0")])
    f2, _ = f2_of(po, h)
    check(np.all(f2 > 1e-3 * f2.max()), "sc Po: all hkl present")
    q = np.array([[1, 1, 1], [2, 0, 0], [3, 1, 1]])
    f2, _ = f2_of(cif("nacl"), q)
    check(f2[0] < 0.05 * f2[1] and f2[2] < 0.05 * f2[1],
          "NaCl: (111) and (311) weak, (200) strong (%s)" % np.round(np.sqrt(f2), 1))
    f2, _ = f2_of(cif("kcl"), q)
    check(f2[0] < 0.01 * f2[1] and f2[2] < 0.01 * f2[1],
          "KCl: (111) and (311) near zero (|F|^2 relative to (200): %.1e, %.1e)"
          % (f2[0] / f2[1], f2[2] / f2[1]))


def line(r, hkl):
    for k in range(len(r["d"])):
        if tuple(r["hkl"][k]) == tuple(hkl):
            return k
    raise KeyError(hkl)


def pattern(name, **kw):
    lat, sym, frac, occ = xrd.read_periodic(cif(name))
    return xrd.powder_reflections(lat, sym, frac, occ, **kw)


def test_published():
    r = pattern("si")
    for hkl, tt in (((1, 1, 1), 28.44), ((2, 2, 0), 47.30), ((3, 1, 1), 56.12)):
        got = r["tt"][line(r, hkl)]
        check(abs(got - tt) <= 0.02, "Si %s at %.2f (%.3f)" % (hkl, tt, got))
    n = int((r["I_rel"] > 1e-3).sum())
    check(n == 6 and r["tt"].max() <= 90, "Si: six allowed lines to 90 degrees (up to 422)", n)
    r = pattern("nacl", bfactor=1.5)
    got = r["tt"][line(r, (2, 0, 0))]
    check(abs(got - 31.69) <= 0.02, "NaCl (200) at 31.69 (%.3f)" % got)
    ratio = r["I"][line(r, (2, 2, 0))] / r["I"][line(r, (2, 0, 0))]
    check(0.55 <= ratio <= 0.65, "NaCl (220)/(200) intensity ratio %.3f in 0.55-0.65" % ratio)
    r = pattern("cu")
    for hkl, tt in (((1, 1, 1), 43.30), ((2, 0, 0), 50.43)):
        got = r["tt"][line(r, hkl)]
        check(abs(got - tt) <= 0.02, "Cu %s at %.2f (%.3f)" % (hkl, tt, got))


def test_multiplicity(tmp):
    po = p1_cif(tmp, "po", 3.35, [("Po", "0 0 0")])
    lat, sym, frac, occ = xrd.read_periodic(po)
    r = xrd.powder_reflections(lat, sym, frac, occ, tt_max=120)
    for hkl, m in (((1, 0, 0), 6), ((1, 1, 0), 12), ((1, 1, 1), 8), ((2, 1, 0), 24)):
        k = line(r, hkl)
        check(r["mult"][k] == m, "multiplicity of {%d%d%d} is %d" % (hkl + (m,)), r["mult"][k])


def cli(*args):
    p = subprocess.run([sys.executable, TOOL, "powder"] + list(args),
                       capture_output=True, text=True)
    return p.returncode, p.stdout, p.stderr


def test_cli(tmp):
    png, csvp, tab = (os.path.join(tmp, n) for n in ("p.png", "p.csv", "t.csv"))
    rc, out, err = cli(cif("nacl"), "--B", "1.5", "--out", png, "--out", csvp, "--table", tab)
    check(rc == 0 and "reflections" in out.splitlines()[0], "CLI nacl.cif runs", err)
    check(open(png, "rb").read(8) == b"\x89PNG\r\n\x1a\n", "PNG is written")
    rows = [ln.split(",") for ln in open(csvp).read().splitlines()]
    tt = np.array([float(r[0]) for r in rows[1:]])
    y = np.array([float(r[1]) for r in rows[1:]])
    check(rows[0] == ["two_theta", "intensity"] and abs(tt[1] - tt[0] - 0.01) < 1e-9
          and abs(y.max() - 100) < 1e-3 and abs(tt[y.argmax()] - 31.70) < 0.02,
          "CSV: 0.01 degree grid, maximum 100 at the NaCl (200) line")
    # Width: the FWHM of the (200) peak is the requested 0.1 degrees.
    k = y.argmax()
    above = np.nonzero(y > y[k] / 2)[0]
    seg = above[(above > k - 30) & (above < k + 30)]
    fw = (seg[-1] - seg[0] + 1) * 0.01
    check(abs(fw - 0.1) < 0.02, "peak FWHM %.2f is the requested 0.1" % fw)
    trows = [ln.split(",") for ln in open(tab).read().splitlines()]
    check(trows[0][:3] == ["h", "k", "l"] and trows[1][:3] == ["1", "1", "1"] and
          not any(r[:3] == ["1", "0", "0"] for r in trows), "table has the allowed lines only")
    rc, out, err = cli(cif("nacl"), "--json", "--tt-max", "60")
    j = json.loads(out)
    check(rc == 0 and [t["hkl"] for t in j["table"]][:2] == [[1, 1, 1], [2, 0, 0]]
          and j["table"][1]["mult"] == 6 and abs(j["table"][1]["I_rel"] - 100) < 1e-6,
          "--json table")
    meas = os.path.join(tmp, "m.txt")
    with open(meas, "w") as f:
        f.write("# 2theta I\n")
        for a, b in zip(tt[::10] + 0.3, y[::10] * 7 + 3):
            f.write("%.3f  %.3f\n" % (a, b))
    out2 = os.path.join(tmp, "m.csv")
    rc, _, err = cli(cif("nacl"), "--measured", meas, "--out", out2, "--out", os.path.join(tmp, "m.png"))
    rows = [ln.split(",") for ln in open(out2).read().splitlines()]
    mv = np.array([float(r[2]) for r in rows[1:] if len(r) > 2 and r[2]])
    check(rc == 0 and rows[0][2] == "measured" and abs(mv.max() - 100) < 1e-6,
          "--measured is a third column scaled to the same maximum", err)
    # The Builder's route: .xyz with Lattice=, and the stored .mvm, give the same lines.
    lat = xrd.read_periodic(cif("nacl"))
    L, sym, frac, occ = lat
    xyz = os.path.join(tmp, "nacl.xyz")
    with open(xyz, "w") as f:
        f.write('%d\nLattice="%s"\n' % (len(sym), " ".join("%.6f" % v for v in L.ravel())))
        for s_, p in zip(sym, frac @ L):
            f.write("%s %.6f %.6f %.6f\n" % ((s_,) + tuple(p)))
    rc, o1, _ = cli(xyz, "--json")
    rc2, o2, _ = cli(cif("nacl"), "--json")
    t1, t2 = json.loads(o1)["table"], json.loads(o2)["table"]
    check(rc == 0 and rc2 == 0 and len(t1) == len(t2) and
          max(abs(a["I_rel"] - b["I_rel"]) for a, b in zip(t1, t2)) < 1e-4,
          ".xyz with Lattice= equals the CIF pattern")
    flat = os.path.join(tmp, "flat.xyz")
    with open(flat, "w") as f:
        f.write("2\nno lattice\nO 0 0 0\nH 0 0 1\n")
    rc, _, err = cli(flat)
    check(rc == 1 and "not periodic" in err, "a structure without a lattice is refused", err)


def test_examples():
    first = {"po_sc": [(1, 0, 0), (1, 1, 0)], "na_bcc": [(1, 1, 0), (2, 0, 0)],
             "al_fcc": [(1, 1, 1), (2, 0, 0)], "cu_fcc": [(1, 1, 1), (2, 0, 0)],
             "nacl": [(1, 1, 1), (2, 0, 0)], "cscl": [(1, 0, 0), (1, 1, 0)],
             "mg_hcp": [(1, 0, 0), (0, 0, 2), (1, 0, 1)]}
    for name, lines in first.items():
        mvm = os.path.join(EX, "qe-" + name, "stored", name, "Parameters", "chemsys.mvm")
        lat, sym, frac, occ = xrd.read_periodic(mvm)
        r = xrd.powder_reflections(lat, sym, frac, occ, tt_min=5, tt_max=90)
        keep = r["I_rel"] > 0.01
        got = [tuple(int(v) for v in h) for h in r["hkl"][keep]][:len(lines)]
        got = [tuple(sorted(map(abs, g), reverse=True)) for g in got]
        want = [tuple(sorted(map(abs, w), reverse=True)) for w in lines]
        # Hexagonal (hkl) and (khl) share d, so compare as sorted indices.
        check(got == want, "%s: the first lines are %s (got %s)" % (name, want, got))


def main():
    with tempfile.TemporaryDirectory() as tmp:
        test_gemmi()
        test_absences(tmp)
        test_published()
        test_multiplicity(tmp)
        test_cli(tmp)
        test_examples()
    print("%d failure(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
