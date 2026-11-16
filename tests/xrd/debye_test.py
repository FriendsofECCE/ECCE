#!/usr/bin/env python3
"""Debye sum: analytic dimers, distance bins, NaCl peaks and CLI exports."""
import json
import math
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
try:
    import numpy as np
except ImportError:
    print("SKIP: numpy is not installed")
    sys.exit(77)
import ecce_xrd as xrd

failures = []


def check(ok, what):
    print(("PASS: " if ok else "FAIL: ") + what)
    if not ok:
        failures.append(what)


def test_analytic():
    q = np.array([0, 0.1, 1, math.pi / 2.82, 4, 8])
    # Independently tabulated IT92 sodium/chlorine coefficients, s=q/(4*pi).
    s2 = (q / (4 * np.pi)) ** 2
    na = sum(a * np.exp(-b * s2) for a, b in zip(
        (4.7626, 3.1736, 1.2674, 1.1128), (3.285, 8.8422, .3136, 129.424))) + .676
    cl = sum(a * np.exp(-b * s2) for a, b in zip(
        (11.4604, 7.1964, 6.2556, 1.6455), (.0104, 1.1662, 18.5194, 47.7784))) - 9.5574
    r = 2.82
    sinc = np.array([1 if v == 0 else math.sin(v * r) / (v * r) for v in q])
    want = na**2 + cl**2 + 2 * na * cl * sinc
    got = xrd.debye_intensity(["Na", "Cl"], [[0, 0, 0], [r, 0, 0]], q)
    check(np.allclose(got, want, rtol=1e-13), "mixed two-atom analytic formula including self and q=0")
    check(np.allclose(xrd.debye_intensity(["Na", "Cl"], np.zeros((2, 3)), q), (na + cl)**2),
          "coincident atoms use sinc(0)=1")
    check(np.allclose(xrd.debye_intensity(["Na"], [[0, 0, 0]], q), na**2), "single atom self term")
    for method in ("direct", "histogram"):
        got = xrd.debye_intensity(["Na", "Cl"], [[0, 0, 0], [r, 0, 0]], q, [.5, .8], 1.5, method)
        want_occ = ((.5*na)**2 + (.8*cl)**2 + 2*.5*.8*na*cl*sinc) * np.exp(-3*s2)
        check(np.allclose(got, want_occ, rtol=1e-13), method + ": occupancy and B match powder amplitudes")
    got = xrd.debye_intensity(["Na", "Cl"], [[0, 0, 0], [2.824, 0, 0]], q, method="histogram")
    check(np.allclose(got, want, rtol=1e-13), "histogram rounds distances to nearest 0.01 A centre")
    line = np.array([[0, 0, 0], [.25, 0, 0], [.8, 0, 0], [1.31, 0, 0]])
    species = ["Na", "Cl", "Na", "Cl"]
    direct = xrd.debye_intensity(species, line, q)
    binned = xrd.debye_intensity(species, line, q, method="histogram")
    check(np.allclose(direct, binned, rtol=1e-13), "direct and histogram count same-element and mixed pairs once")
    shifted = line[:, [1, 2, 0]] + [12, -7, 3]
    check(np.allclose(direct, xrd.debye_intensity(species, shifted, q), rtol=1e-13),
          "Debye intensity is invariant under rigid translation and rotation")
    for badq in ([float("nan")], [-1]):
        try:
            xrd.debye_intensity(["Na"], [[0, 0, 0]], badq)
        except ValueError:
            check(True, "invalid q rejected: " + str(badq))
        else:
            check(False, "invalid q rejected: " + str(badq))


def test_cluster():
    lat, sym, frac, occ = xrd.read_periodic(os.path.join(ROOT, "tests", "fixtures", "cif", "nacl.cif"))
    shifts = np.stack(np.meshgrid(*([np.arange(8)] * 3), indexing="ij"), -1).reshape(-1, 3)
    xyz = (frac[None, :, :] + shifts[:, None, :]).reshape(-1, 3) @ lat
    symbols = sym * len(shifts)
    tt = np.arange(29, 59.001, .05)
    q = 4 * np.pi * np.sin(np.radians(tt / 2)) / xrd.CU_KA1
    intensity = xrd.debye_intensity(symbols, xyz, q)
    powder = xrd.powder_reflections(lat, sym, frac, occ, tt_min=29, tt_max=59)
    # Published NaCl pattern, DOI 10.3390/compounds6030048, abstract:
    # https://www.mdpi.com/2673-6918/6/3/48 . Finite 8-cell edges broaden peaks.
    for hkl, published in (((2, 0, 0), 31.7), ((2, 2, 0), 45.5), ((2, 2, 2), 56.5)):
        zone = np.flatnonzero(abs(tt - published) < 1)
        peak = tt[zone[np.argmax(intensity[zone])]]
        powder_tt = next(v for h, v in zip(powder["hkl"], powder["tt"]) if tuple(h) == hkl)
        check(abs(peak - published) < .35 and abs(peak - powder_tt) < .35,
              "4096-atom NaCl %s maximum %.2f approaches powder %.2f and published %.1f deg" %
              (hkl, peak, powder_tt, published))
    check(len(symbols) > xrd.DEBYE_DIRECT_LIMIT and np.isfinite(intensity).all(),
          "large cluster automatically uses distance histogram")


def test_cli(tmp):
    structure = os.path.join(tmp, "pair.xyz")
    with open(structure, "w") as f:
        f.write('2\nLattice="6 0 0 0 6 0 0 0 6"\nNa 0 0 0\nCl 2.82 0 0\n')
    tool = [sys.executable, os.path.join(ROOT, "scripts", "ecce_xrd.py"), "debye", structure]
    csvp, png = (os.path.join(tmp, name) for name in ("d.csv", "d.png"))
    p = subprocess.run(tool + ["--tt-min", "0", "--tt-max", "90", "--step", ".5", "--out", csvp,
                               "--out", png, "--json"], text=True, capture_output=True)
    check(p.returncode == 0, "CLI exports PNG, CSV and JSON: " + p.stderr)
    if p.returncode:
        return
    data = json.loads(p.stdout)
    rows = np.loadtxt(csvp, delimiter=",", skiprows=1)
    check(data["method"] == "direct" and data["bin_width"] == 0 and data["atoms"] == 2,
          "CLI reports direct method for periodic cell contents")
    check(open(csvp).readline().strip() == "two_theta,intensity" and
          np.allclose(rows[:, 1], data["intensity"]) and abs(rows[:, 1].max() - 100) < 1e-9,
          "CSV matches sampled JSON and powder normalization")
    check(open(png, "rb").read(8) == b"\x89PNG\r\n\x1a\n", "CLI writes PNG")
    for source in ("cu", "mo", "co"):
        p = subprocess.run(tool + ["--source", source, "--tt-min", "0", "--step", "5", "--json"],
                           text=True, capture_output=True)
        data = json.loads(p.stdout)
        angles = np.array(data["angles"])
        q1 = 4*np.pi*np.sin(np.radians(angles/2)) / xrd.DOUBLETS[source][0]
        q2 = q1*xrd.DOUBLETS[source][0]/xrd.DOUBLETS[source][1]
        want = (xrd.debye_intensity(["Na", "Cl"], [[0, 0, 0], [2.82, 0, 0]], q1) +
                .5*xrd.debye_intensity(["Na", "Cl"], [[0, 0, 0], [2.82, 0, 0]], q2))/1.5
        check(p.returncode == 0 and data["doublet"] and np.allclose(data["raw_intensity"], want),
              source + ": source wavelengths and 2:1 doublet")
    p = subprocess.run(tool + ["--source", "mo", "--no-doublet", "--step", "5", "--json"],
                       text=True, capture_output=True)
    data = json.loads(p.stdout)
    check(p.returncode == 0 and not data["doublet"] and data["wavelength"] == xrd.DOUBLETS["mo"][0],
          "--no-doublet retains the selected source Ka1 wavelength")
    large = os.path.join(tmp, "large.xyz")
    with open(large, "w") as f:
        f.write("3001\ncoincident test atoms\n" + "Na 0 0 0\n" * 3001)
    p = subprocess.run([*tool[:2], "debye", large, "--tt-min", "0", "--tt-max", "10", "--step", "10", "--json"],
                       text=True, capture_output=True)
    data = json.loads(p.stdout)
    check(p.returncode == 0 and data["method"] == "histogram" and data["bin_width"] == .01 and
          abs(data["raw_intensity"][0] / (3001*xrd.form_factor("Na", 0))**2 - 1) < 1e-12,
          "CLI reports histogram above 3000 atoms and retains q=0 forward scattering")
    p = subprocess.run(tool + ["--select", "1", "--step", "5"], text=True, capture_output=True)
    check(p.returncode == 0 and "1 atoms, Debye direct sum" in p.stdout, "selection and method summary")
    for args in (("--step", "0"), ("--wavelength", "nan"), ("--tt-max", "180"), ("--doublet",)):
        p = subprocess.run(tool + list(args), text=True, capture_output=True)
        check(p.returncode == 1 and "error:" in p.stderr, "CLI rejects " + " ".join(args))


def main():
    test_analytic()
    test_cluster()
    with tempfile.TemporaryDirectory() as tmp:
        test_cli(tmp)
    print("%d failure(s)" % len(failures))
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
