#!/usr/bin/env python3
"""Displace a structure along a normal mode (#618): the way off a saddle point.

Wx-free; the Vibrations panel and the command line both use it.  Reads the
modes file of tests/vibanalysis (N, N lines "El x y z" in Angstrom, then
"modes M" and per mode a frequency and N lines "dx dy dz" in the frame of
the structure) and prints the displaced structures as JSON.

Usage: ecce_displace.py <modes-file> (--mode N | --imaginary)
           [--amplitude A] [--sign +|-|both] [--source NAME]

The stored vectors are Cartesian displacements of any normalisation, so the
mode is scaled until its largest atom displacement is A Angstrom (default
0.1).  --imaginary sums every mode with a negative frequency, each scaled to
A first, and rescales the sum the same way.  --sign both gives two
structures.  Modes are numbered from 1; a mode with all-zero vectors is
refused.
"""
import argparse
import json
import math
import sys


def read_modes(path):
    """-> (elements, coords [N][3], freqs, modes [M][N][3])."""
    tok = open(path).read().split()
    pos = 0
    n = int(tok[pos]); pos += 1
    els, xyz = [], []
    for _ in range(n):
        els.append(tok[pos])
        xyz.append([float(t) for t in tok[pos + 1:pos + 4]])
        pos += 4
    freqs, modes = [], []
    while pos < len(tok):
        word = tok[pos]; pos += 1
        if word == "bonds":
            pos += 1 + 3 * int(tok[pos])
        elif word == "modes":
            m = int(tok[pos]); pos += 1
            for _ in range(m):
                freqs.append(float(tok[pos])); pos += 1
                v = [float(t) for t in tok[pos:pos + 3 * n]]
                modes.append([v[3 * a:3 * a + 3] for a in range(n)])
                pos += 3 * n
    return els, xyz, freqs, modes


def largest(vec):
    return max(math.sqrt(sum(c * c for c in a)) for a in vec)


def scaled(vec, amplitude):
    """vec scaled so its largest atom displacement is `amplitude`."""
    m = largest(vec)
    if m < 1e-12:
        raise ValueError("the mode has no displacement")
    return [[c * amplitude / m for c in a] for a in vec]


def direction(modes, freqs, which, amplitude):
    """-> (vector with largest atom displacement = amplitude, description)."""
    if which is None:
        idx = [i for i, f in enumerate(freqs) if f < 0]
        if not idx:
            raise ValueError("no imaginary mode")
        tot = None
        for i in idx:
            s = scaled(modes[i], amplitude)
            tot = s if tot is None else [[a + b for a, b in zip(x, y)]
                                         for x, y in zip(tot, s)]
        names = ", ".join("%di" % round(-freqs[i]) for i in idx)
        what = ("all %d imaginary modes (%s cm-1)" % (len(idx), names)
                if len(idx) > 1 else
                "mode %d (%s cm-1)" % (idx[0] + 1, names))
        return scaled(tot, amplitude), what
    if not 1 <= which <= len(modes):
        raise ValueError("mode %d does not exist (1..%d)" % (which, len(modes)))
    f = freqs[which - 1]
    fs = "%di" % round(-f) if f < 0 else "%d" % round(f)
    return scaled(modes[which - 1], amplitude), "mode %d (%s cm-1)" % (which, fs)


def displace(xyz, modes, freqs, which, amplitude=0.1, signs=("+",), source=""):
    vec, what = direction(modes, freqs, which, amplitude)
    out = []
    for s in signs:
        k = 1.0 if s == "+" else -1.0
        coords = [[x + k * d for x, d in zip(a, v)] for a, v in zip(xyz, vec)]
        note = "displaced %s%.2f Å along %s" % (s, amplitude, what)
        if source:
            note += " of " + source
        out.append({"sign": s, "coords": coords, "note": note})
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("modes")
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--mode", type=int)
    g.add_argument("--imaginary", action="store_true")
    ap.add_argument("--amplitude", type=float, default=0.1)
    ap.add_argument("--sign", choices=["+", "-", "both"], default="+")
    ap.add_argument("--source", default="")
    a = ap.parse_args(argv)
    if a.amplitude <= 0:
        ap.error("amplitude must be positive")
    els, xyz, freqs, modes = read_modes(a.modes)
    signs = ("+", "-") if a.sign == "both" else (a.sign,)
    try:
        res = displace(xyz, modes, freqs, None if a.imaginary else a.mode,
                       a.amplitude, signs, a.source)
    except ValueError as e:
        print("ecce-displace: %s" % e, file=sys.stderr)
        return 1
    json.dump({"elements": els, "results": res}, sys.stdout)
    print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
