#!/usr/bin/env python3
"""Generate ECCE's application icon: benzene's in-plane electron density.

    ./make-icon.py            writes ecce.svg and ecce-{48,64,128,256}.png here

The density is a model (Slater-like terms on each nucleus), not ECCE
output; once ECCE draws plane contour maps itself (#215) the contours
should come from its own benzene calculation instead.

Needs numpy, matplotlib (contouring only) and, for the PNGs, librsvg via
PyGObject -- the renderer GNOME draws the icon with.
"""
import os

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
SIZES = (48, 64, 128, 256)

# Bohr: C-C 1.40 a.u. from the centre is a stylised ring, not the real 2.64.
RING_C, RING_H = 1.40, 2.48
LEVELS = np.logspace(np.log10(0.08), np.log10(4), 7)
SCALE = 14.5            # px per bohr on the 128 px canvas


def density(X, Y):
    rho = np.zeros_like(X)
    for k in range(6):
        t = np.pi / 6 + k * np.pi / 3
        for radius, weight, zeta in ((RING_C, 6.0, 1.6), (RING_H, 1.2, 1.24)):
            r = np.hypot(X - radius * np.cos(t), Y - radius * np.sin(t))
            rho += weight * np.exp(-2 * zeta * r)
    return rho


def contour_paths():
    x = np.linspace(-4.2, 4.2, 600)
    X, Y = np.meshgrid(x, x)
    cs = plt.contour(X, Y, density(X, Y), LEVELS)
    paths = []
    for i, segs in enumerate(cs.allsegs):
        colour = matplotlib.colors.to_hex(
            matplotlib.colormaps["viridis"](0.15 + 0.85 * i / (len(LEVELS) - 1)))
        for s in segs:
            pts = " L".join("%.1f,%.1f" % (64 + px * SCALE, 64 - py * SCALE)
                            for px, py in s[::2])
            paths.append('<path d="M%s Z" fill="none" stroke="%s" '
                         'stroke-width="%.2f" stroke-linejoin="round"/>'
                         % (pts, colour, 3.2 - 0.15 * i))
    return "".join(paths)


def svg():
    return (
        '<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" '
        'viewBox="0 0 128 128"><defs>'
        '<linearGradient id="tile" x1="0" y1="0" x2="0" y2="1">'
        '<stop offset="0" stop-color="#2a2f45"/>'
        '<stop offset="1" stop-color="#11131c"/></linearGradient>'
        '<clipPath id="clip"><rect x="8" y="8" width="112" height="108" '
        'rx="24"/></clipPath></defs>'
        '<rect x="8" y="12" width="112" height="108" rx="24" fill="#07080c"/>'
        '<rect x="8" y="8" width="112" height="108" rx="24" fill="url(#tile)"/>'
        '<g clip-path="url(#clip)">%s</g></svg>\n' % contour_paths())


def write_pngs(svg_path):
    import gi
    gi.require_version("Rsvg", "2.0")
    from gi.repository import Rsvg
    import cairo
    handle = Rsvg.Handle.new_from_file(svg_path)
    for size in SIZES:
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, size, size)
        viewport = Rsvg.Rectangle()
        viewport.x = viewport.y = 0
        viewport.width = viewport.height = size
        handle.render_document(cairo.Context(surface), viewport)
        surface.write_to_png(os.path.join(HERE, "ecce-%d.png" % size))


if __name__ == "__main__":
    path = os.path.join(HERE, "ecce.svg")
    with open(path, "w") as f:
        f.write(svg())
    write_pngs(path)
