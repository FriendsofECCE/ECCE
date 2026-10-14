#!/usr/bin/env python3
"""Writes the Builder's bond-order and geometry icons (#323) as SVG into
data/client/pixmaps/svg: shaded spheres and sticks in the viewer's
ball-and-stick style, carbon centre, hydrogen-white ligands.

    tools/screenshots/build_icons.py [OUTDIR]

The files are committed; rerun this to change the drawing.
"""
import math
import os
import sys

# Gradients are per sphere in user space: wx's NanoSVG renders the
# objectBoundingBox form flat.
STOPS = {"c": ("#e4e4e4", "#909090", "#4a4a4a"),
         "h": ("#ffffff", "#ececec", "#a8a8a8")}
STICK = "#9a9a9a"
HILITE = "#d4d4d4"


def f(v):
    return ("%.2f" % v).rstrip("0").rstrip(".")


_n = [0]


def sphere(x, y, r, grad):
    _n[0] += 1
    gid = "g%d" % _n[0]
    c0, c1, c2 = STOPS[grad]
    rim = ' stroke="#6a6a6a" stroke-width="0.6"' if grad == "h" else ""
    gx, gy = x - 0.35 * r, y - 0.4 * r
    return ('<radialGradient id="%s" gradientUnits="userSpaceOnUse" cx="%s" '
            'cy="%s" r="%s" fx="%s" fy="%s"><stop offset="0" stop-color="%s"/>'
            '<stop offset="0.5" stop-color="%s"/><stop offset="1" '
            'stop-color="%s"/></radialGradient>'
            '<circle cx="%s" cy="%s" r="%s" fill="url(#%s)"%s/>' % (
                gid, f(gx), f(gy), f(1.5 * r), f(gx), f(gy), c0, c1, c2,
                f(x), f(y), f(r), gid, rim))


def stick(x1, y1, x2, y2, w=1.8, dash=None):
    d = ' stroke-dasharray="%s"' % dash if dash else ""
    s = ('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="%s" stroke-width="%s"'
         '%s/>' % (f(x1), f(y1), f(x2), f(y2), STICK, f(w), d))
    # A lighter line along the upper edge gives the stick its round shading.
    n = math.hypot(x2 - x1, y2 - y1) or 1
    ox, oy = (y2 - y1) / n * w * 0.22, -(x2 - x1) / n * w * 0.22
    if oy > 0:
        ox, oy = -ox, -oy
    s += ('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="%s" stroke-width="%s"'
          '%s/>' % (f(x1 + ox), f(y1 + oy), f(x2 + ox), f(y2 + oy), HILITE,
                    f(w * 0.3), d))
    return s


def svg(body):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" '
            'viewBox="0 0 24 24">\n%s\n</svg>\n' % body)


# ---- bond orders -----------------------------------------------------------
def bond(kind):
    xa, xb, y, r = 4.6, 19.4, 12, 3.6
    sticks = []
    if kind == "single":
        sticks.append(stick(xa, y, xb, y, 2.2))
    elif kind == "double":
        for dy in (-2.5, 2.5):
            sticks.append(stick(xa, y + dy, xb, y + dy, 1.8))
    elif kind == "triple":
        for dy in (-3.4, 0, 3.4):
            sticks.append(stick(xa, y + dy, xb, y + dy, 1.5))
    elif kind == "aromatic":
        sticks.append(stick(xa, y - 2.5, xb, y - 2.5, 1.8))
        sticks.append(stick(xa, y + 2.5, xb, y + 2.5, 1.8, "2.4 1.8"))
    elif kind == "metal":
        sticks.append(stick(xa, y, xb, y, 2.2, "2.6 1.8"))
    return svg("\n".join(sticks + [sphere(xa, y, r, "c"),
                                   sphere(xb, y, r, "c")]))


# ---- geometries ------------------------------------------------------------
def tetra_dirs():
    s = math.sqrt(8 / 9.0)
    return [(0, 1, 0)] + [(s * math.cos(a), -1 / 3.0, s * math.sin(a))
                          for a in (0, 2 * math.pi / 3, 4 * math.pi / 3)]


def ring(n, y, off=0.0, rad=1.0):
    return [(rad * math.cos(off + 2 * math.pi * i / n), y,
             rad * math.sin(off + 2 * math.pi * i / n)) for i in range(n)]


A52 = math.radians(52)
SHAPES = {
    # name: (flat, direction vectors)
    "lone": (True, []),
    "terminal": (True, [(1, 0, 0)]),
    "linear": (True, [(-1, 0, 0), (1, 0, 0)]),
    "bent": (True, [(-math.sin(A52), -math.cos(A52), 0),
                    (math.sin(A52), -math.cos(A52), 0)]),
    "trigplanar": (True, [(math.cos(a), math.sin(a), 0) for a in (math.pi / 2, 7 * math.pi / 6, 11 * math.pi / 6)]),
    "pyramidal": (False, tetra_dirs()[1:]),
    "tetrahedral": (False, tetra_dirs()),
    "squareplanar": (True, [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0)]),
    "trigbipyramidal": (False, [(0, 1, 0), (0, -1, 0)] + ring(3, 0)),
    "squarepyramidal": (False, [(0, 1, 0)] + ring(4, -0.25, 0, 0.95)),
    "octahedral": (False, [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0),
                           (0, 0, 1), (0, 0, -1)]),
    "cubic": (False, [(x, y, z) for x in (-.6, .6) for y in (-.6, .6)
                      for z in (-.6, .6)]),
    "decahedral": (False, [(0, 1, 0), (0, -1, 0)] + ring(4, 0.4, 0, 0.9)
                   + ring(4, -0.4, math.pi / 4, 0.9)),
}
# Where the centre sits, to keep the whole figure in the box.
SHIFT = {"pyramidal": (0, -2.5), "bent": (0, -3.0), "terminal": (-3.5, 0)}


def shape(name):
    flat, dirs = SHAPES[name]
    dx, dy = SHIFT.get(name, (0, 0))
    cx, cy, L = 12.0 + dx, 12.0 + dy, 8.6
    ay, ax = math.radians(28), math.radians(24)
    pts = []
    for x, y, z in dirs:
        if not flat:
            x, z = (x * math.cos(ay) + z * math.sin(ay),
                    -x * math.sin(ay) + z * math.cos(ay))
            y, z = (y * math.cos(ax) - z * math.sin(ax),
                    y * math.sin(ax) + z * math.cos(ax))
        pts.append((cx + x * L, cy - y * L, z))
    items = []  # (depth, svg): far parts first
    for x, y, z in pts:
        items.append((z, stick(cx, cy, x, y, 1.6)))
        items.append((z + 0.01, sphere(x, y, 2.3 + 0.3 * z, "h")))
    items.append((0.0, sphere(cx, cy, 3.5, "c")))
    items.sort(key=lambda t: t[0])
    return svg("\n".join(s for _, s in items))


# ---- the periodic table button ---------------------------------------------
def pertab():
    rows = ["N.......N", "MM..SNNNN", "MM..MSNNN", "MMMMMSSNN", "MMMMMMSSN"]
    col = {"M": "#0072B2", "S": "#E69F00", "N": "#009E73"}
    out = []
    for r, row in enumerate(rows):
        for c, k in enumerate(row):
            if k != ".":
                out.append('<rect x="%s" y="%s" width="2.1" height="3.7" '
                           'fill="%s"/>' % (f(2.4 + c * 2.35),
                                            f(2.6 + r * 4.1), col[k]))
    return svg("\n".join(out))


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        here, "..", "..", "data", "client", "pixmaps", "svg")
    os.makedirs(out, exist_ok=True)
    files = {"bond-%s.svg" % k: bond(k) for k in
             ("single", "double", "triple", "aromatic", "metal")}
    files.update({"shape-%s.svg" % n: shape(n) for n in SHAPES})
    files["pertab.svg"] = pertab()
    for name, text in files.items():
        with open(os.path.join(out, name), "w") as h:
            h.write(text)


main()
