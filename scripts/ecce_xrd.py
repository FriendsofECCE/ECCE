#!/usr/bin/env python3
"""ecce-xrd: calculated X-ray diffraction patterns (#971).

    ecce-xrd fibre STRUCTURE [options]     fibre pattern: molecules aligned on
                                           one axis, randomly rotated about it

The structure is an .xyz, .pdb or .mvm file; --select takes atom numbers from
1 ("1-40,57").  numpy only (no scipy, no wx).  Subcommands for powder (#587)
and layer patterns (#972) are added to SUBCOMMANDS below.

Units: reciprocal space is S = 1/d in 1/Angstrom, with R the component
perpendicular to the fibre axis and Z the component along it.  A flat film
perpendicular to the beam, the fibre axis in the film plane perpendicular to
the beam (optionally tilted), maps a film point to (R, Z) through the Ewald
sphere; no iteration is needed.
"""
import argparse
import csv
import json
import math
import os
import struct
import sys
import time
import zlib

import numpy as np

# International Tables for Crystallography Vol. C, Table 6.1.1.4 (Cromer-Mann,
# the "IT92" set): f(s) = sum a_i exp(-b_i s^2) + c, s = sin(theta)/lambda.
_FF = {
    "H": ((0.493002, 0.322912, 0.140191, 0.040810),
          (10.5109, 26.1257, 3.14236, 57.7997), 0.003038),
    "C": ((2.31000, 1.02000, 1.58860, 0.865000),
          (20.8439, 10.2075, 0.568700, 51.6512), 0.215600),
    "N": ((12.2126, 3.13220, 2.01250, 1.16630),
          (0.005700, 9.89330, 28.9975, 0.582600), -11.529),
    "O": ((3.04850, 2.28680, 1.54630, 0.867000),
          (13.2771, 5.70110, 0.323900, 32.9089), 0.250800),
    "Na": ((4.76260, 3.17360, 1.26740, 1.11280),
           (3.28500, 8.84220, 0.313600, 129.424), 0.676000),
    "Mg": ((5.42040, 2.17350, 1.22690, 2.30730),
           (2.82750, 79.2611, 0.380800, 7.19370), 0.858400),
    "P": ((6.43450, 4.17910, 1.78000, 1.49080),
          (1.90670, 27.1570, 0.526000, 68.1645), 1.11490),
    "S": ((6.90530, 5.20340, 1.43790, 1.58630),
          (1.46790, 22.2151, 0.253600, 56.1720), 0.866900),
    "Cl": ((11.4604, 7.19640, 6.25560, 1.64550),
           (0.010400, 1.16620, 18.5194, 47.7784), -9.5574),
    "K": ((8.21860, 7.43980, 1.05190, 0.865900),
          (12.7949, 0.774800, 213.187, 41.6841), 1.42280),
    "Ca": ((8.62660, 7.38730, 1.58990, 1.02110),
           (10.4421, 0.659900, 85.7484, 178.437), 1.37510),
    "Fe": ((11.7695, 7.35730, 3.52220, 2.30450),
           (4.76110, 0.307200, 15.3535, 76.8805), 1.03690),
    "Zn": ((14.0743, 7.03180, 5.16520, 2.41000),
           (3.26550, 0.233300, 10.3163, 58.7097), 1.30410),
}


def form_factor(symbol, s):
    """Atomic form factor at s = sin(theta)/lambda = 1/(2d), s an array."""
    try:
        a, b, c = _FF[symbol]
    except KeyError:
        raise ValueError("no form factor for element %r (have %s)"
                         % (symbol, " ".join(sorted(_FF))))
    s2 = np.asarray(s, dtype=float) ** 2
    return sum(ai * np.exp(-bi * s2) for ai, bi in zip(a, b)) + c


# ----------------------------------------------------------------- structure

def _norm_symbol(text):
    t = "".join(ch for ch in text if ch.isalpha())
    if not t:
        raise ValueError("no element in %r" % text)
    t = t[0].upper() + t[1:2].lower()
    return t if t in _FF else t[0]


def read_structure(path):
    """(symbols, coordinates[n, 3] in Angstrom) of an .xyz, .pdb or .mvm."""
    ext = os.path.splitext(path)[1].lower()
    sym, xyz = [], []
    with open(path) as f:
        lines = f.read().splitlines()
    if ext == ".pdb":
        for ln in lines:
            if ln.startswith(("ATOM", "HETATM")):
                el = ln[76:78].strip() or ln[12:16].strip()
                sym.append(_norm_symbol(el))
                xyz.append([float(ln[30:38]), float(ln[38:46]), float(ln[46:54])])
    elif ext == ".mvm":
        on = False
        for ln in lines:
            w = ln.split()
            if ln.startswith("atom_list:"):
                on = True
            elif on and len(w) >= 4:
                sym.append(_norm_symbol(w[0]))
                xyz.append([float(v) for v in w[1:4]])
            elif on:
                break
    else:
        n = int(lines[0].split()[0])
        for ln in lines[2:2 + n]:
            w = ln.split()
            sym.append(_norm_symbol(w[0]))
            xyz.append([float(v) for v in w[1:4]])
    if not sym:
        raise ValueError("no atoms read from %s" % path)
    return sym, np.array(xyz, dtype=float)


def parse_selection(text, natoms):
    """'1-40,57' -> zero-based index array; empty text selects everything."""
    if not text:
        return np.arange(natoms)
    idx = []
    for part in text.replace(" ", "").split(","):
        if not part:
            continue
        a, _, b = part.partition("-")
        lo, hi = int(a), int(b or a)
        if lo < 1 or hi > natoms or hi < lo:
            raise ValueError("selection %r is outside 1-%d" % (part, natoms))
        idx.extend(range(lo - 1, hi))
    return np.array(sorted(set(idx)))


def fibre_axis(xyz):
    """Principal axis of the atoms with the largest spread (the helix axis)."""
    c = xyz - xyz.mean(axis=0)
    w, v = np.linalg.eigh(c.T @ c)
    return v[:, np.argmax(w)]


class Model:
    """Atoms in cylindrical coordinates about the fibre axis."""

    def __init__(self, symbols, xyz, axis=None):
        xyz = np.asarray(xyz, dtype=float)
        ax = fibre_axis(xyz) if axis is None else np.asarray(axis, float)
        ax = ax / np.linalg.norm(ax)
        c = xyz - xyz.mean(axis=0)
        helper = np.array([1.0, 0, 0]) if abs(ax[0]) < 0.9 else np.array([0, 1.0, 0])
        e1 = np.cross(ax, helper)
        e1 /= np.linalg.norm(e1)
        e2 = np.cross(ax, e1)
        self.axis = ax
        self.z = c @ ax
        x, y = c @ e1, c @ e2
        self.r = np.hypot(x, y)
        self.phi = np.arctan2(y, x)
        self.symbols = list(symbols)
        self.elements = sorted(set(symbols))
        self.groups = [np.array([i for i, s in enumerate(symbols) if s == e])
                       for e in self.elements]
        for e in self.elements:
            form_factor(e, 0.0)


# ------------------------------------------------------------- calculation

def bessel_table(x, nmax):
    """J_n(x) for n = -nmax..nmax as an array [len(x), 2*nmax+1] (columns in
    order n = 0..nmax, -nmax..-1, as numpy's FFT lays them out).

    exp(i x sin t) = sum_n J_n(x) exp(i n t), so J_n is the FFT of the
    integrand over one period; the trapezoid rule on a periodic integrand is
    exact up to J_{n+K}, which is negligible for K well beyond n + x.
    """
    x = np.asarray(x, dtype=float)
    k = 1 << int(math.ceil(math.log2(2 * (nmax + float(np.max(np.abs(x))) + 16))))
    return _jn_from_fft(x, 2 * np.pi * np.arange(k) / k, k, nmax)


def _jn_from_fft(x, t, k, nmax):
    spec = np.fft.fft(np.exp(1j * x[:, None] * np.sin(t)[None, :]), axis=1) / k
    cols = np.concatenate([np.arange(0, nmax + 1), np.arange(k - nmax, k)])
    return spec[:, cols].real


def _orders(xmax):
    return int(math.ceil(xmax + 4 * xmax ** (1 / 3.0) + 12))


def intensity_rz(model, rs, zs, bfactor=0.0):
    """Cylindrically averaged |F|^2 on the grid rs x zs (1/Angstrom), shape
    [len(rs), len(zs)]:  I = sum_n |sum_j f_j J_n(2 pi R r_j) e^{-i n phi_j}
    e^{2 pi i Z z_j}|^2.  The form factor depends on |S| = sqrt(R^2 + Z^2)."""
    rs, zs = np.asarray(rs, float), np.asarray(zs, float)
    ez = np.exp(2j * np.pi * zs[:, None] * model.z[None, :])
    out = np.empty((len(rs), len(zs)))
    rmax = float(model.r.max()) if len(model.r) else 0.0
    for ir, rr in enumerate(rs):
        n = _orders(2 * np.pi * rr * rmax)
        jn = bessel_table(2 * np.pi * rr * model.r, n)
        norders = np.concatenate([np.arange(0, n + 1), np.arange(-n, 0)])
        a = jn * np.exp(-1j * norders[None, :] * model.phi[:, None])
        s = 0.5 * np.hypot(rr, zs)
        g = 0
        for el, idx in zip(model.elements, model.groups):
            f = form_factor(el, s) * np.exp(-bfactor * s * s / 2.0)
            g = g + f[:, None] * (ez[:, idx] @ a[idx, :])
        out[ir] = np.sum(np.abs(g) ** 2, axis=1)
    return out


def intensity_rz_bruteforce(model, rs, zs, nangles=720, bfactor=0.0):
    """The same average done directly: |F(S)|^2 of the atoms for S at
    nangles azimuths about the axis, averaged.  No Bessel functions."""
    out = np.zeros((len(rs), len(zs)))
    psi = 2 * np.pi * np.arange(nangles) / nangles
    x, y = model.r * np.cos(model.phi), model.r * np.sin(model.phi)
    for ir, rr in enumerate(rs):
        for iz, zz in enumerate(zs):
            s = 0.5 * math.hypot(rr, zz)
            ph = 2 * np.pi * (rr * (np.cos(psi)[:, None] * x[None, :] +
                                    np.sin(psi)[:, None] * y[None, :]) +
                              zz * model.z[None, :])
            e = np.exp(1j * ph)
            f = np.zeros(len(x))
            for el, idx in zip(model.elements, model.groups):
                f[idx] = form_factor(el, s) * math.exp(-bfactor * s * s / 2.0)
            out[ir, iz] = np.mean(np.abs(e @ f) ** 2)
    return out


def length_broaden(grid, h, length):
    """Gaussian smear along Z (axis 1; the grid starts at Z = 0) for an
    effective fibre length `length` Angstrom: FWHM = 1/length."""
    sigma = 1.0 / (2.3548 * length) / h
    half = int(math.ceil(4 * sigma))
    if half < 1:
        return grid
    k = np.exp(-0.5 * (np.arange(-half, half + 1) / sigma) ** 2)
    k /= k.sum()
    pad = np.pad(grid, ((0, 0), (half, half)), mode="reflect")
    return sum(k[i] * pad[:, i:i + grid.shape[1]] for i in range(len(k)))


class Grid:
    """I(R, Z) on R, Z >= 0 with spacing h, evaluated by bilinear lookup."""

    def __init__(self, data, h):
        self.data, self.h = data, h

    def at(self, r, z):
        fr = np.clip(np.abs(r) / self.h, 0, self.data.shape[0] - 1.000001)
        fz = np.clip(np.abs(z) / self.h, 0, self.data.shape[1] - 1.000001)
        i, j = fr.astype(int), fz.astype(int)
        u, v = fr - i, fz - j
        d = self.data
        return ((1 - u) * (1 - v) * d[i, j] + u * (1 - v) * d[i + 1, j] +
                (1 - u) * v * d[i, j + 1] + u * v * d[i + 1, j + 1])


def film_coordinates(nx, ny, size_x, size_y, distance, wavelength, tilt=0.0):
    """x, y (mm, y up), and (R, Z) for every film pixel.  Lab axes: x along
    the equator, b along the beam, y along the (untilted) fibre."""
    xs = (np.arange(nx) + 0.5) / nx * size_x - size_x / 2
    ys = size_y / 2 - (np.arange(ny) + 0.5) / ny * size_y
    x, y = np.meshgrid(xs, ys)
    rho = np.sqrt(x * x + y * y + distance * distance)
    sx, sb, sy = x / rho / wavelength, (distance / rho - 1) / wavelength, y / rho / wavelength
    t = math.radians(tilt)
    z = sb * math.sin(t) + sy * math.cos(t)
    s2 = sx * sx + sb * sb + sy * sy
    return x, y, np.sqrt(np.maximum(s2 - z * z, 0.0)), z


def scale_display(i, smin, beamstop, mode, decades, gamma):
    """Intensities to 0..1 for display.  The centre (|S| < beamstop) is
    masked first: it holds the (sum of f)^2 peak and would set the scale."""
    s = np.sqrt(smin)
    keep = s >= beamstop
    ref = np.percentile(i[keep], 99.9) if keep.any() else i.max()
    v = np.where(keep, i / max(ref, 1e-300), 0.0)
    if mode == "log":
        v = np.clip(1 + np.log10(np.maximum(v, 1e-300)) / decades, 0, 1)
    elif mode == "sqrt":
        v = np.sqrt(np.clip(v, 0, 1))
    else:
        v = np.clip(v, 0, 1)
    return v ** gamma


def write_png(path, gray):
    """8-bit greyscale PNG of an array in 0..1 (zlib only, no imaging library)."""
    g = np.round(np.clip(gray, 0, 1) * 255).astype(np.uint8)
    raw = b"".join(b"\x00" + row.tobytes() for row in g)

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" +
                chunk(b"IHDR", struct.pack(">IIBBBBB", g.shape[1], g.shape[0], 8, 0, 0, 0, 0)) +
                chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


# --------------------------------------------------------------------- fibre

def fibre_pattern(model, wavelength=1.5418, distance=30.0, film=40.0, pixels=400,
                  bfactor=20.0, length=100.0, misalignment=8.0, tilt=0.0,
                  map_kind="film", rmax=None, zmax=None, step=0.005):
    """The pattern as a dict: x, y (axes of the image), R, Z, I, all [ny, nx]."""
    if map_kind == "film":
        x, y, r, z = film_coordinates(pixels, pixels, film, film, distance,
                                      wavelength, tilt)
        smax = float(np.sqrt(r * r + z * z).max())
        if misalignment <= 0:
            rg, zg = float(np.abs(r).max()), float(np.abs(z).max())
        else:
            rg = zg = smax
    else:
        rg, zg = rmax or 0.5, zmax or 0.5
        xs = np.linspace(-rg, rg, pixels)
        ys = np.linspace(zg, -zg, pixels)
        x, y = np.meshgrid(xs, ys)
        r, z = np.abs(x), y
    n_r, n_z = int(math.ceil(rg / step)) + 2, int(math.ceil(zg / step)) + 2
    grid = intensity_rz(model, np.arange(n_r) * step, np.arange(n_z) * step, bfactor)
    if length > 0:
        grid = length_broaden(grid, step, length)
    g = Grid(grid, step)
    if misalignment > 0:
        sig = math.radians(misalignment)
        d = np.linspace(-3 * sig, 3 * sig, 15)
        w = np.exp(-0.5 * (d / sig) ** 2)
        s = np.hypot(r, z)
        a = np.arctan2(r, z)
        inten = sum(wk * g.at(s * np.sin(a + dk), s * np.cos(a + dk))
                    for dk, wk in zip(d, w)) / w.sum()
    else:
        inten = g.at(r, z)
    return {"x": x, "y": y, "R": r, "Z": z, "I": inten}


# ----------------------------------------------------------------------- CLI

def _axis_arg(text):
    if text in ("auto", "selection"):
        return text
    v = [float(p) for p in text.split(",")]
    if len(v) != 3 or not any(v):
        raise argparse.ArgumentTypeError("--axis is auto, selection or 'x,y,z'")
    return v


def run_fibre(a):
    sym, xyz = read_structure(a.structure)
    idx = parse_selection(a.select, len(sym))
    t0 = time.time()
    axis = a.axis
    if axis == "auto":
        axis = None
    elif axis == "selection":
        if len(idx) < 2:
            raise ValueError("--axis selection needs at least two atoms")
        axis = xyz[idx[-1]] - xyz[idx[0]]
    model = Model([sym[i] for i in idx], xyz[idx], axis)
    pat = fibre_pattern(model, a.wavelength, a.distance, a.film_size, a.pixels, a.B,
                        a.length_broadening, a.misalignment, a.tilt, a.map,
                        a.rmax, a.zmax, a.step)
    s2 = pat["R"] ** 2 + pat["Z"] ** 2
    shown = scale_display(pat["I"], s2, a.beamstop, a.scale, a.decades, a.gamma)
    if a.invert:
        shown = 1 - shown
    for out in a.out or []:
        ext = os.path.splitext(out)[1].lower()
        if ext == ".png":
            write_png(out, shown)
        elif ext == ".npy":
            np.save(out, pat["I"])
        elif ext == ".csv":
            with open(out, "w", newline="") as f:
                w = csv.writer(f)
                w.writerow(["x", "y", "R", "Z", "I"])
                for row in zip(*(pat[k].ravel() for k in ("x", "y", "R", "Z", "I"))):
                    w.writerow(["%.6g" % v for v in row])
        else:
            raise ValueError("--out: unknown extension %r (png, npy, csv)" % ext)
    info = {"atoms": int(len(idx)), "axis": [round(float(v), 6) for v in model.axis],
            "map": a.map, "pixels": a.pixels, "wavelength": a.wavelength,
            "distance_mm": a.distance, "film_mm": a.film_size,
            "max_intensity": float(pat["I"].max()), "seconds": round(time.time() - t0, 2)}
    if a.json:
        print(json.dumps(info))
    else:
        print("%d atoms, axis %s, %dx%d %s map in %.1f s" % (
            info["atoms"], info["axis"], a.pixels, a.pixels, a.map, info["seconds"]))
    return 0


def build_parser():
    p = argparse.ArgumentParser(prog="ecce-xrd", description=__doc__.splitlines()[0])
    sub = p.add_subparsers(dest="command", required=True)
    f = sub.add_parser("fibre", help="fibre diffraction pattern")
    f.add_argument("structure")
    f.add_argument("--select", default="", help="atom numbers from 1, e.g. 1-40,57")
    f.add_argument("--axis", type=_axis_arg, default=None, help="auto (principal axis), selection (first to last selected atom) or x,y,z")
    f.add_argument("--wavelength", type=float, default=1.5418, help="Angstrom (Cu Ka)")
    f.add_argument("--distance", type=float, default=30.0, help="sample-film, mm")
    f.add_argument("--film-size", type=float, default=40.0, help="film edge, mm")
    f.add_argument("--pixels", type=int, default=400)
    f.add_argument("--rmax", type=float, help="rz map: R range, 1/Angstrom")
    f.add_argument("--zmax", type=float, help="rz map: Z range, 1/Angstrom")
    f.add_argument("--step", type=float, default=0.005, help="(R,Z) grid step, 1/Angstrom")
    f.add_argument("--B", type=float, default=20.0, help="isotropic B, Angstrom^2")
    f.add_argument("--length-broadening", type=float, default=100.0,
                   help="effective fibre length, Angstrom (0 = off)")
    f.add_argument("--misalignment", type=float, default=8.0, help="axis spread, degrees")
    f.add_argument("--tilt", type=float, default=0.0, help="fibre tilt out of the film plane, degrees")
    f.add_argument("--map", choices=("film", "rz"), default="film")
    f.add_argument("--scale", choices=("log", "sqrt", "linear"), default="log")
    f.add_argument("--decades", type=float, default=2.5, help="log scale range")
    f.add_argument("--gamma", type=float, default=1.0)
    f.add_argument("--beamstop", type=float, default=0.012, help="masked |S| for scaling, 1/Angstrom")
    f.add_argument("--invert", action=argparse.BooleanOptionalAction, default=True,
                   help="dark spots on white (default); --no-invert for light on dark")
    f.add_argument("--out", action="append", help=".png, .npy or .csv (repeatable)")
    f.add_argument("--json", action="store_true")
    f.set_defaults(func=run_fibre)
    return p


def main(argv=None):
    a = build_parser().parse_args(argv)
    try:
        return a.func(a)
    except (OSError, ValueError) as e:
        print("error: %s" % e, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
