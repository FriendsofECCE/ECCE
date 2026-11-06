#!/usr/bin/env python3
"""ecce-spectrum (#707): the IR curves of two calculations, their difference
and an experimental spectrum on one grid.

Wx-free.  The Vibrations panel imports experimental files through this tool
(--parse-exp) and its plot and CSV must equal this tool's output.

Usage: ecce-spectrum <calcA> [<calcB>] [--exp FILE] [--exp-kind K]
           [--fwhm W] [--scale S] [--shape gaussian|lorentzian]
           [--lo X --hi X --step D] [--csv | --json]
       ecce-spectrum --parse-exp FILE [--exp-kind K]

Curves are the area-normalised broadening of the stick spectrum in km/mol per
cm-1, as the panel draws them, with the frequency factor S applied to both
calculations.  The difference is B minus A.  The experimental curve (cm-1
against absorbance or transmittance, comma, semicolon, tab or space separated,
other lines ignored) is converted to absorbance, then scaled so its largest
value in [lo, hi] equals the largest value of A's curve on the grid, and is
empty outside the data.  The factor applies to the computed curves only.
Transmittance is chosen when the baseline lies nearer the maximum than the
minimum (bands are dips); values above 1.5 are percent.  --exp-kind
absorbance|transmittance overrides this.
"""
import argparse
import json
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    import ecce_bandshift as B
except ImportError:
    import importlib.machinery
    import importlib.util
    _l = importlib.machinery.SourceFileLoader(
        "ecce_bandshift", os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                       "ecce-bandshift"))
    _s = importlib.util.spec_from_loader("ecce_bandshift", _l)
    B = importlib.util.module_from_spec(_s)
    _l.exec_module(B)

# Match util/VibFrequency.H: below this (cm-1) a "mode" is a numerical
# translation/rotation, and an imaginary mode is not drawn.
IMAG_THRESHOLD = 1.0


def line(shape, off, fwhm):
    if not fwhm > 0:
        return 0.0
    if shape == "lorentzian":
        g = 0.5 * fwhm
        return (g / math.pi) / (off * off + g * g)
    ln2 = math.log(2.0)
    return 2.0 * math.sqrt(ln2 / math.pi) / fwhm * math.exp(-4.0 * ln2 * (off / fwhm) ** 2)


class Curve:
    """The sticks of one calculation and what the panel draws from them."""
    def __init__(self, calc, scale, fwhm, shape):
        _el, _xyz, freqs, ir, _modes = B.read_modes(calc)
        self.sticks = []
        for i, f in enumerate(freqs):
            if f < IMAG_THRESHOLD or not ir:
                continue
            self.sticks.append((f * scale, ir[i]))
        self.fwhm, self.shape = fwhm, shape

    def at(self, x):
        return sum(h * line(self.shape, x - c, self.fwhm) for c, h in self.sticks)


def read_experimental(path, kind="auto"):
    """-> (x, absorbance, detected kind) from a two-column text file."""
    pts = []
    for ln in open(path, errors="replace"):
        f = [t for t in re.split(r"[,;\s]+", ln.strip().replace("﻿", "")) if t]
        if len(f) < 2:
            continue
        try:
            pts.append((float(f[0]), float(f[1])))
        except ValueError:
            continue
    if len(pts) < 2:
        raise ValueError("%s has no two-column numeric data" % path)
    pts.sort()
    ys = [p[1] for p in pts]
    if kind == "auto":
        lo, hi = min(ys), max(ys)
        med = sorted(ys)[len(ys) // 2]
        kind = "transmittance" if med > 0.5 * (lo + hi) else "absorbance"
    if kind == "transmittance":
        div = 100.0 if max(ys) > 1.5 else 1.0
        ys = [-math.log10(max(y / div, 1e-6)) for y in ys]
    return [p[0] for p in pts], ys, kind


def interp(xs, ys, x):
    if x < xs[0] or x > xs[-1]:
        return None
    lo, hi = 0, len(xs) - 1
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if xs[mid] <= x:
            lo = mid
        else:
            hi = mid
    if xs[hi] == xs[lo]:
        return ys[lo]
    return ys[lo] + (ys[hi] - ys[lo]) * (x - xs[lo]) / (xs[hi] - xs[lo])


def spectrum(a, b=None, exp=None, lo=0.0, hi=4000.0, step=1.0, scale=1.0,
             fwhm=15.0, shape="gaussian"):
    """-> dict of lists: x, a, b, difference, experimental (absent ones None)."""
    ca = Curve(a, scale, fwhm, shape)
    cb = Curve(b, scale, fwhm, shape) if b else None
    n = int(math.floor((hi - lo) / step + 1e-9))
    xs = [lo + i * step for i in range(n + 1)]
    ya = [ca.at(x) for x in xs]
    out = {"x": xs, "a": ya, "b": None, "difference": None, "experimental": None}
    if cb:
        out["b"] = [cb.at(x) for x in xs]
        out["difference"] = [q - p for p, q in zip(ya, out["b"])]
    if exp:
        ex, ey, _k = exp
        vis = [y for x, y in zip(ex, ey) if lo <= x <= hi]
        top = max(vis) if vis else 0.0
        f = max(ya) / top if top > 0 and ya else 0.0
        out["experimental"] = [None if interp(ex, ey, x) is None
                               else f * interp(ex, ey, x) for x in xs]
    return out


def csv_text(res, name_a, name_b):
    cols = [("wavenumber_cm-1", res["x"]), ("intensity_" + name_a, res["a"])]
    if res["b"] is not None:
        cols.append(("intensity_" + name_b, res["b"]))
        cols.append(("difference_%s_minus_%s" % (name_b, name_a), res["difference"]))
    if res["experimental"] is not None:
        cols.append(("experimental", res["experimental"]))
    lines = [",".join(c[0] for c in cols)]
    for i in range(len(res["x"])):
        lines.append(",".join("" if c[1][i] is None else "%.8g" % c[1][i] for c in cols))
    return "\n".join(lines) + "\n"


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("calc_a", nargs="?")
    ap.add_argument("calc_b", nargs="?")
    ap.add_argument("--exp")
    ap.add_argument("--parse-exp")
    ap.add_argument("--exp-kind", choices=["auto", "absorbance", "transmittance"],
                    default="auto")
    ap.add_argument("--fwhm", type=float, default=15.0)
    ap.add_argument("--scale", type=float, default=1.0)
    ap.add_argument("--shape", choices=["gaussian", "lorentzian"], default="gaussian")
    ap.add_argument("--lo", type=float, default=0.0)
    ap.add_argument("--hi", type=float, default=4000.0)
    ap.add_argument("--step", type=float, default=1.0)
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--csv", action="store_true")
    g.add_argument("--json", action="store_true")
    o = ap.parse_args(argv)
    try:
        if o.parse_exp:
            x, y, k = read_experimental(o.parse_exp, o.exp_kind)
            print("kind\t" + k)
            for p, q in zip(x, y):
                print("%.8g\t%.8g" % (p, q))
            return 0
        if not o.calc_a:
            ap.error("a calculation is required")
        if not (o.step > 0 and o.hi > o.lo and o.scale > 0):
            ap.error("need step > 0, hi > lo, scale > 0")
        exp = read_experimental(o.exp, o.exp_kind) if o.exp else None
        res = spectrum(o.calc_a, o.calc_b, exp, o.lo, o.hi, o.step, o.scale,
                       o.fwhm, o.shape)
    except (ValueError, OSError) as e:
        print("ecce-spectrum: %s" % e, file=sys.stderr)
        return 1
    na = os.path.basename(os.path.normpath(o.calc_a))
    nb = os.path.basename(os.path.normpath(o.calc_b)) if o.calc_b else ""
    if o.json:
        json.dump(res, sys.stdout)
        print()
    else:
        sys.stdout.write(csv_text(res, na, nb))
    return 0


if __name__ == "__main__":
    sys.exit(main())
