#!/usr/bin/env python3
"""ecce-xrd: calculated X-ray diffraction patterns (#971).

    ecce-xrd fibre STRUCTURE [options]     fibre pattern: molecules aligned on
                                           one axis, randomly rotated about it
    ecce-xrd powder STRUCTURE [options]    powder pattern of a periodic structure

The structure is an .xyz, .pdb or .mvm file; --select takes atom numbers from
1 ("1-40,57").  numpy only (no scipy, no wx).  Powder takes a .cif, an .mvm
with lattice_vectors or an .xyz whose comment line has Lattice="ax ay az bx by
bz cx cy cz" (extended xyz, as the Builder writes it).  Layer patterns (#972)
are added to the parser below.

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
import re
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
    "Li": ((1.1282, 0.7508, 0.6175, 0.4653),
          (3.9546, 1.0524, 85.3905, 168.261), 0.0377),
    "Be": ((1.5919, 1.1278, 0.5391, 0.7029),
          (43.6427, 1.8623, 103.483, 0.542), 0.0385),
    "B": ((2.0545, 1.3326, 1.0979, 0.7068),
          (23.2185, 1.021, 60.3498, 0.1403), -0.1932),
    "F": ((3.5392, 2.6412, 1.517, 1.0243),
          (10.2825, 4.2944, 0.2615, 26.1476), 0.2776),
    "Ne": ((3.9553, 3.1125, 1.4546, 1.1251),
          (8.4042, 3.4262, 0.2306, 21.7184), 0.3515),
    "Al": ((6.4202, 1.9002, 1.5936, 1.9646),
          (3.0387, 0.7426, 31.5472, 85.0886), 1.1151),
    "Si": ((6.2915, 3.0353, 1.9891, 1.541),
          (2.4386, 32.3337, 0.6785, 81.6937), 1.1407),
    "Ti": ((9.7595, 7.3558, 1.6991, 1.9021),
          (7.8508, 0.5, 35.6338, 116.105), 1.2807),
    "V": ((10.2971, 7.3511, 2.0703, 2.0571),
          (6.8657, 0.4385, 26.8938, 102.478), 1.2199),
    "Cr": ((10.6406, 7.3537, 3.324, 1.4922),
          (6.1038, 0.392, 20.2626, 98.7399), 1.1832),
    "Mn": ((11.2819, 7.3573, 3.0193, 2.2441),
          (5.3409, 0.3432, 17.8674, 83.7543), 1.0896),
    "Co": ((12.2841, 7.3409, 4.0034, 2.3488),
          (4.2791, 0.2784, 13.5359, 71.1692), 1.0118),
    "Ni": ((12.8376, 7.292, 4.4438, 2.38),
          (3.8785, 0.2565, 12.1763, 66.3421), 1.0341),
    "Cu": ((13.338, 7.1676, 5.6158, 1.6735),
          (3.5828, 0.247, 11.3966, 64.8126), 1.191),
    "Br": ((17.1789, 5.2358, 5.6377, 3.9851),
          (2.1723, 16.5796, 0.2609, 41.4328), 2.9557),
    "Rb": ((17.1784, 9.6435, 5.1399, 1.5292),
          (1.7888, 17.3151, 0.2748, 164.934), 3.4873),
    "Sr": ((17.5663, 9.8184, 5.422, 2.6694),
          (1.5564, 14.0988, 0.1664, 132.376), 2.5064),
    "Mo": ((3.7025, 17.2356, 12.8876, 3.7429),
          (0.2772, 1.0958, 11.004, 61.6584), 4.3875),
    "Ag": ((19.2808, 16.6885, 4.8045, 1.0463),
          (0.6446, 7.4726, 24.6605, 99.8156), 5.179),
    "Cs": ((20.3892, 19.1062, 10.662, 1.4953),
          (3.569, 0.3107, 24.3879, 213.904), 3.3352),
    "Ba": ((20.3361, 19.297, 10.888, 2.6959),
          (3.216, 0.2756, 20.2073, 167.202), 2.7731),
    "Au": ((16.8819, 18.5913, 25.5582, 5.86),
          (0.4611, 8.6216, 1.4826, 36.3956), 12.0658),
    "Pt": ((27.0059, 17.7639, 15.7131, 5.7837),
          (1.51293, 8.81174, 0.424593, 38.6103), 11.6883),
    "Pb": ((31.0617, 13.0637, 18.442, 5.9696),
          (0.6902, 2.3576, 8.618, 47.2579), 13.4118),
    "Po": ((34.6726, 15.4733, 13.1138, 7.02588),
          (0.700999, 3.55078, 9.55642, 47.0045), 13.677),
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


# -------------------------------------------------------------------- powder

CU_KA1 = 1.5406

# Reflections of one d are merged into one line (their intensities added), so
# the multiplicity is counted and no space-group tables are needed.
_NUM = re.compile(r"([+-]?)(?:(\d+\.?\d*|\.\d+)(?:/(\d+))?\*?)?([xyz])?")


def parse_symop(text):
    """'-y,x-y,z+1/3' -> (rotation[3, 3], translation[3])."""
    parts = text.lower().replace(" ", "").split(",")
    if len(parts) != 3:
        raise ValueError("bad symmetry operation %r" % text)
    rot, tr = np.zeros((3, 3)), np.zeros(3)
    for i, part in enumerate(parts):
        for m in _NUM.finditer(part):
            sign, num, den, var = m.groups()
            if not num and not var:
                continue
            v = float(num) if num else 1.0
            if den:
                v /= float(den)
            v = -v if sign == "-" else v
            if var:
                rot[i, "xyz".index(var)] += v
            else:
                tr[i] += v
    return rot, tr


def cell_vectors(a, b, c, alpha, beta, gamma):
    """Rows a, b, c with a along x and b in the xy plane (the PDB frame)."""
    al, be, ga = (math.radians(v) for v in (alpha, beta, gamma))
    cx = c * math.cos(be)
    cy = c * (math.cos(al) - math.cos(be) * math.cos(ga)) / math.sin(ga)
    return np.array([[a, 0, 0], [b * math.cos(ga), b * math.sin(ga), 0],
                     [cx, cy, math.sqrt(max(c * c - cx * cx - cy * cy, 0.0))]])


def _cif_blocks(text):
    """Items and loops of the first data block: ({tag: value}, [(tags, rows)])."""
    items, loops = {}, []
    toks = []
    for ln in text.splitlines():
        if ln.lstrip().startswith("#"):
            continue
        for t in re.findall(r"'[^']*'|\"[^\"]*\"|\S+", ln):
            toks.append(t[1:-1] if t[0] in "'\"" and len(t) > 1 else t)
    i, seen = 0, False
    while i < len(toks):
        t = toks[i]
        if t.startswith("data_"):
            if seen:
                break
            seen = True
            i += 1
        elif t == "loop_":
            i += 1
            tags = []
            while i < len(toks) and toks[i].startswith("_"):
                tags.append(toks[i].lower())
                i += 1
            vals = []
            while i < len(toks) and not toks[i].startswith("_") and \
                    toks[i] != "loop_" and not toks[i].startswith("data_"):
                vals.append(toks[i])
                i += 1
            n = len(tags)
            loops.append((tags, [vals[k:k + n] for k in range(0, len(vals) - n + 1, n)]))
        elif t.startswith("_") and i + 1 < len(toks):
            items[t.lower()] = toks[i + 1]
            i += 2
        else:
            i += 1
    return items, loops


def _cif_float(v):
    return float(re.sub(r"\(.*?\)", "", v)) if v not in (".", "?") else 0.0


def read_cif(path):
    """(lattice, symbols, fractional[n, 3], occupancy[n]) with the symmetry
    operations of the file applied (P1 when it lists none)."""
    with open(path) as f:
        items, loops = _cif_blocks(f.read())
    try:
        lat = cell_vectors(*(_cif_float(items["_cell_" + k]) for k in
                             ("length_a", "length_b", "length_c",
                              "angle_alpha", "angle_beta", "angle_gamma")))
    except KeyError:
        raise ValueError("%s has no cell (_cell_length_a ...)" % path)
    ops, atoms = [], None
    for tags, rows in loops:
        for key in ("_symmetry_equiv_pos_as_xyz", "_space_group_symop_operation_xyz"):
            if key in tags:
                ops = [parse_symop(r[tags.index(key)]) for r in rows]
        if "_atom_site_fract_x" in tags:
            atoms = (tags, rows)
    if atoms is None:
        raise ValueError("%s has no atom_site_fract_x loop" % path)
    if not ops:
        ops = [(np.eye(3), np.zeros(3))]
    tags, rows = atoms
    col = {t: tags.index(t) for t in tags}
    sym, frac, occ = [], [], []
    for r in rows:
        el = _norm_symbol(r[col["_atom_site_type_symbol"]] if "_atom_site_type_symbol" in col
                          else r[col["_atom_site_label"]])
        x = np.array([_cif_float(r[col["_atom_site_fract_" + k]]) for k in "xyz"])
        o = _cif_float(r[col["_atom_site_occupancy"]]) if "_atom_site_occupancy" in col else 1.0
        mine = []
        for rot, tr in ops:
            y = (rot @ x + tr) % 1.0
            if not any(np.abs((y - z + 0.5) % 1.0 - 0.5).max() < 1e-3 for z in mine):
                mine.append(y)
        for y in mine:
            sym.append(el)
            frac.append(y)
            occ.append(o)
    return lat, sym, np.array(frac), np.array(occ)


def read_periodic(path):
    """(lattice[3, 3] rows in Angstrom, symbols, fractional[n, 3], occupancy[n])
    of a .cif, or of an .xyz / .mvm / .pdb that carries a lattice."""
    ext = os.path.splitext(path)[1].lower()
    if ext == ".cif":
        return read_cif(path)
    lat = None
    with open(path) as f:
        lines = f.read().splitlines()
    if ext == ".xyz" and len(lines) > 1:
        m = re.search(r'lattice\s*=\s*"([^"]*)"', lines[1], re.I)
        if m:
            v = [float(t) for t in m.group(1).split()]
            if len(v) != 9:
                raise ValueError("Lattice= needs nine numbers")
            lat = np.array(v).reshape(3, 3)
    elif ext == ".mvm":
        for k, ln in enumerate(lines):
            if ln.startswith("lattice_vectors:"):
                lat = np.array([[float(t) for t in lines[k + j].split()[:3]] for j in (1, 2, 3)])
    elif ext == ".pdb":
        for ln in lines:
            if ln.startswith("CRYST1"):
                lat = cell_vectors(*(float(ln[s:e]) for s, e in
                                     ((6, 15), (15, 24), (24, 33), (33, 40), (40, 47), (47, 54))))
    if lat is None:
        raise ValueError("%s is not periodic: no lattice in it" % path)
    sym, xyz = read_structure(path)
    return lat, sym, xyz @ np.linalg.inv(lat), np.ones(len(sym))


def structure_factors2(sym, frac, occ, hkl, s, bfactor=0.0):
    """|F(hkl)|^2 = |sum_j occ_j f_j(s) exp(-B s^2) exp(2 pi i h.x_j)|^2, with
    s = sin(theta)/lambda = 1/(2d) per reflection."""
    elements = sorted(set(sym))
    groups = [np.array([i for i, e in enumerate(sym) if e == el]) for el in elements]
    f2 = np.empty(len(s))
    for lo in range(0, len(s), 20000):
        sl = slice(lo, lo + 20000)
        ph = np.exp(2j * np.pi * (frac @ hkl[sl].T))
        tot = 0
        for el, idx in zip(elements, groups):
            tot = tot + form_factor(el, s[sl]) * (occ[idx] @ ph[idx])
        f2[sl] = np.abs(tot * np.exp(-bfactor * s[sl] ** 2)) ** 2
    return f2


def powder_reflections(lat, sym, frac, occ, wavelength=CU_KA1, tt_min=5.0,
                       tt_max=90.0, bfactor=0.0, dtol=1e-4):
    """Reflections with 2theta in range, equal d merged: a dict of arrays hkl
    [n, 3] (one representative), d, tt (degrees), F (|F|, root-mean-square over
    the merged reflections), mult, I (|F|^2 * mult * Lorentz-polarisation) and
    I_rel (largest = 100), sorted by 2theta."""
    recip = np.linalg.inv(lat).T            # rows b_i, a_i . b_j = delta_ij
    gmax = 2 * math.sin(math.radians(tt_max / 2)) / wavelength
    hm = [int(math.floor(gmax * np.linalg.norm(v))) for v in lat]
    ax = [np.arange(-m, m + 1) for m in hm]
    hkl = np.stack(np.meshgrid(*ax, indexing="ij"), -1).reshape(-1, 3)
    g = np.linalg.norm(hkl @ recip, axis=1)
    tt_lo = 2 * np.degrees(np.arcsin(np.clip(wavelength * g / 2, 0, 1)))
    keep = (g > 1e-9) & (g <= gmax * (1 + 1e-12)) & (tt_lo >= tt_min - 1e-9)
    hkl, g = hkl[keep], g[keep]
    f2 = structure_factors2(sym, frac, occ, hkl, g / 2.0, bfactor)
    order = np.argsort(-1.0 / g, kind="stable")      # large d first
    hkl, g, f2 = hkl[order], g[order], f2[order]
    d = 1.0 / g
    start = 0
    out = {k: [] for k in ("hkl", "d", "tt", "F", "mult", "I")}
    for end in range(1, len(d) + 1):
        if end < len(d) and d[start] - d[end] <= dtol:
            continue
        mem = hkl[start:end]
        neg = (mem < 0).sum(axis=1)
        rep = max(range(len(mem)), key=lambda k: (-neg[k],) + tuple(mem[k]))
        dm = float(d[start:end].mean())
        th = math.asin(wavelength / (2 * dm))
        lp = (1 + math.cos(2 * th) ** 2) / (math.sin(th) ** 2 * math.cos(th))
        out["hkl"].append(mem[rep])
        out["d"].append(dm)
        out["tt"].append(math.degrees(2 * th))
        out["F"].append(math.sqrt(f2[start:end].mean()))
        out["mult"].append(end - start)
        out["I"].append(f2[start:end].sum() * lp)
        start = end
    r = {k: np.array(v) for k, v in out.items()}
    if len(r["d"]) == 0:
        raise ValueError("no reflections between %g and %g degrees" % (tt_min, tt_max))
    o = np.argsort(r["tt"])
    r = {k: v[o] for k, v in r.items()}
    r["I_rel"] = 100.0 * r["I"] / max(r["I"].max(), 1e-300)
    return r


def powder_pattern(refl, tt_min=5.0, tt_max=90.0, fwhm=0.1, eta=0.5, step=0.01):
    """(two_theta grid, intensity with the largest value 100): a pseudo-Voigt
    of the same FWHM and mixing eta (1 = Lorentzian) on every reflection."""
    tt = np.arange(tt_min, tt_max + step / 2, step)
    y = np.zeros(len(tt))
    half = 10 * fwhm
    for pos, inten in zip(refl["tt"], refl["I_rel"]):
        if inten < 1e-4:
            continue
        lo, hi = np.searchsorted(tt, [pos - half, pos + half])
        x = tt[lo:hi] - pos
        y[lo:hi] += inten * (eta / (1 + 4 * x * x / fwhm ** 2) +
                             (1 - eta) * np.exp(-4 * math.log(2) * x * x / fwhm ** 2))
    return tt, 100.0 * y / max(y.max(), 1e-300)


def read_measured(path):
    """Two columns 2theta, intensity (blank, comma, or tab separated)."""
    x, y = [], []
    with open(path) as f:
        for ln in f:
            w = re.split(r"[,\s;]+", ln.strip())
            try:
                x.append(float(w[0]))
                y.append(float(w[1]))
            except (ValueError, IndexError):
                continue
    if len(x) < 2:
        raise ValueError("no two-column data in %s" % path)
    o = np.argsort(x)
    return np.array(x)[o], np.array(y)[o]


_DIGITS = {"0": "111101101101111", "1": "010110010010111", "2": "111001111100111",
           "3": "111001111001111", "4": "101101111001001", "5": "111100111001111",
           "6": "111100111101111", "7": "111001001001001", "8": "111101111101111",
           "9": "111101111001111", ".": "000000000000010"}


def write_png_rgb(path, rgb):
    """8-bit RGB PNG of a uint8 array [h, w, 3]."""
    raw = b"".join(b"\x00" + row.tobytes() for row in rgb)

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" +
                chunk(b"IHDR", struct.pack(">IIBBBBB", rgb.shape[1], rgb.shape[0], 8, 2, 0, 0, 0)) +
                chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def plot_powder(path, tt, y, measured=None, refl=None, size=(1000, 480)):
    """The pattern as a PNG with 2theta ticks; calculated in blue, measured in
    red.  Drawn with numpy alone, so tick labels use a 3x5 digit font."""
    w, h = size
    img = np.full((h, w, 3), 255, np.uint8)
    l, r, t, b = 60, w - 20, 20, h - 50
    x0, x1 = float(tt[0]), float(tt[-1])
    ymax = 105.0

    def px(v):
        return int(round(l + (v - x0) / (x1 - x0) * (r - l)))

    def py(v):
        return int(round(b - (v / ymax) * (b - t)))

    def text(s, x, yy, scale=2):
        for ch in s:
            bits = _DIGITS.get(ch)
            if bits:
                for k, c in enumerate(bits):
                    if c == "1":
                        img[yy + (k // 3) * scale:yy + (k // 3 + 1) * scale,
                            x + (k % 3) * scale:x + (k % 3 + 1) * scale] = 60
            x += 4 * scale
    img[t, l:r + 1] = img[b, l:r + 1] = 0
    img[t:b + 1, l] = img[t:b + 1, r] = 0
    step = next(s for s in (1, 2, 5, 10, 20, 50) if (x1 - x0) / s <= 12)
    for v in np.arange(math.ceil(x0 / step) * step, x1 + 1e-9, step):
        img[b:b + 6, px(v)] = 0
        lab = "%g" % v
        text(lab, px(v) - 4 * len(lab), b + 12)
    for v in (0, 25, 50, 75, 100):
        img[py(v), l - 6:l + 1] = 0
        lab = "%d" % v
        text(lab, l - 10 - 8 * len(lab), py(v) - 5)
    for curve, color in ((measured, (205, 40, 40)), (y, (20, 60, 170))):
        if curve is None:
            continue
        cols = np.clip(np.round(l + (tt - x0) / (x1 - x0) * (r - l)).astype(int), l, r)
        for c in np.unique(cols):
            v = curve[(cols == c) & np.isfinite(curve)]
            if len(v):
                img[py(min(v.max(), ymax)):b, c] = color
    write_png_rgb(path, img)


def run_powder(a):
    lat, sym, frac, occ = read_periodic(a.structure)
    t0 = time.time()
    refl = powder_reflections(lat, sym, frac, occ, a.wavelength, a.tt_min, a.tt_max, a.B, a.dtol)
    tt, y = powder_pattern(refl, a.tt_min, a.tt_max, a.fwhm, a.eta, a.step)
    meas = None
    if a.measured:
        mx, my = read_measured(a.measured)
        sel = (mx >= a.tt_min) & (mx <= a.tt_max)
        if not sel.any():
            raise ValueError("%s has no points between %g and %g degrees" % (a.measured, a.tt_min, a.tt_max))
        meas = np.interp(tt, mx, my / my[sel].max() * 100.0, left=np.nan, right=np.nan)
    shown = refl["I_rel"] >= a.min_intensity
    rows = [("h", "k", "l", "d", "two_theta", "F", "mult", "I_rel")]
    for k in np.nonzero(shown)[0]:
        rows.append(tuple(["%d" % v for v in refl["hkl"][k]] +
                          ["%.5f" % refl["d"][k], "%.4f" % refl["tt"][k], "%.4f" % refl["F"][k],
                           "%d" % refl["mult"][k], "%.4f" % refl["I_rel"][k]]))
    if a.table:
        with open(a.table, "w", newline="") as f:
            csv.writer(f).writerows(rows)
    for out in a.out or []:
        ext = os.path.splitext(out)[1].lower()
        if ext == ".png":
            plot_powder(out, tt, y, meas)
        elif ext == ".csv":
            with open(out, "w", newline="") as f:
                w = csv.writer(f)
                w.writerow(["two_theta", "intensity"] + (["measured"] if meas is not None else []))
                for k in range(len(tt)):
                    row = ["%.2f" % tt[k], "%.5g" % y[k]]
                    if meas is not None:
                        row.append("%.5g" % meas[k] if np.isfinite(meas[k]) else "")
                    w.writerow(row)
        else:
            raise ValueError("--out: unknown extension %r (png, csv)" % ext)
    n = int(shown.sum())
    info = {"atoms": len(sym), "wavelength": a.wavelength, "two_theta": [a.tt_min, a.tt_max],
            "fwhm": a.fwhm, "eta": a.eta, "B": a.B, "reflections": n,
            "lattice": [[round(float(v), 6) for v in row] for row in lat],
            "seconds": round(time.time() - t0, 2)}
    if a.json:
        info["table"] = [{"hkl": [int(v) for v in refl["hkl"][k]], "d": float(refl["d"][k]),
                          "two_theta": float(refl["tt"][k]), "F": float(refl["F"][k]),
                          "mult": int(refl["mult"][k]), "I_rel": float(refl["I_rel"][k])}
                         for k in np.nonzero(shown)[0]]
        print(json.dumps(info))
    else:
        print("%d atoms, %d reflections between %g and %g degrees (%g A)" % (
            info["atoms"], n, a.tt_min, a.tt_max, a.wavelength))
        print("%4s %4s %4s %9s %9s %9s %5s %8s" % rows[0])
        for row in rows[1:]:
            print("%4s %4s %4s %9s %9s %9s %5s %8s" % row)
    return 0


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

    q = sub.add_parser("powder", help="powder diffraction pattern of a periodic structure")
    q.add_argument("structure", help=".cif, or .xyz/.mvm/.pdb with a lattice")
    q.add_argument("--wavelength", type=float, default=CU_KA1, help="Angstrom (Cu Ka1)")
    q.add_argument("--tt-min", type=float, default=5.0, help="first 2theta, degrees")
    q.add_argument("--tt-max", type=float, default=90.0, help="last 2theta, degrees")
    q.add_argument("--fwhm", type=float, default=0.1, help="peak width, degrees 2theta")
    q.add_argument("--eta", type=float, default=0.5, help="pseudo-Voigt mixing, 0 Gauss .. 1 Lorentz")
    q.add_argument("--B", type=float, default=0.0, help="isotropic B, Angstrom^2")
    q.add_argument("--step", type=float, default=0.01, help="2theta grid, degrees")
    q.add_argument("--dtol", type=float, default=1e-4, help="reflections closer in d than this (A) are merged")
    q.add_argument("--min-intensity", type=float, default=1e-3,
                   help="reflections below this I_rel are left out of the table")
    q.add_argument("--measured", help="two columns 2theta, intensity: drawn as a second curve")
    q.add_argument("--table", help="reflection table as .csv")
    q.add_argument("--out", action="append", help=".png or .csv pattern (repeatable)")
    q.add_argument("--json", action="store_true")
    q.set_defaults(func=run_powder)
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
