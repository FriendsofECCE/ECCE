#!/usr/bin/env python3
"""Writes the three GROMACS study icon candidates (data/client/pixmaps/
candidates/studygromacs_{a,b,c}.xpm) in the 16x16 style of calcorca.xpm.

    tools/screenshots/gmx_study_icons.py [OUTDIR]

Colours are Okabe-Ito vermillion (#D55E00) in three tones for the light edge
and the shade, so the icon reads at the tree's size on light and dark themes.
"""
import os
import sys

MAIN, LIGHT, DARK = "#D55E00", "#F0A060", "#8A3D00"
KEY = {"#": MAIN, "h": LIGHT, "d": DARK}

A = """
................
................
.....######.....
...##########...
..####....####..
.###........###.
.###............
.###.....#######
.###.....#######
.###.........##.
.###.........##.
..####......###.
...##########...
.....######.....
................
................
"""

B = """
................
................
................
................
.###.#...#.#...#
#....##.##..#.#.
#....#.#.#...#..
#.##.#...#...#..
#..#.#...#..#.#.
#..#.#...#.#...#
.###.#...#.#...#
................
.##############.
................
.##############.
................
"""

C = """
................
.hhhhhh.........
hMMMMMMhhhhhhhh.
hMMMMMMMMMMMMMMd
hMMMMMMMMMMMMMMd
hMMdddddddMMMMMd
hMMdMMMMMMMMMMMd
hMMdMMMMMMMMMMMd
hMMdMMddddMMMMMd
hMMdMMMMMdMMMMMd
hMMdMMMMMdMMMMMd
hMMMddddddMMMMMd
hMMMMMMMMMMMMMMd
hMMMMMMMMMMMMMMd
.dddddddddddddd.
................
"""


def rows(g):
    r = [l for l in g.strip("\n").split("\n")]
    assert len(r) == 16 and all(len(l) == 16 for l in r), [len(l) for l in r]
    return r


def xpm(name, grid, key):
    cols = {" ": None}
    sym = {}
    body = []
    for l in rows(grid):
        o = ""
        for ch in l:
            if ch == ".":
                o += " "
            else:
                o += ch
                sym[ch] = key[ch]
        body.append(o)
    out = ['/* XPM */', 'static char * %s_xpm[] = {' % name,
           '"16 16 %d 1",' % (len(sym) + 1), '" \tc None",']
    for s, c in sym.items():
        out.append('"%s\tc %s",' % (s, c))
    out += ['"%s",' % l for l in body]
    out[-1] = out[-1].rstrip(",") + "};"
    return "\n".join(out) + "\n"


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..", "data",
        "client", "pixmaps", "candidates")
    os.makedirs(d, exist_ok=True)
    # A and B: one tone plus light edge / shade; C: folder body in the
    # light tone, glyph and shade in the dark one.
    keyC = {"h": LIGHT, "M": "#E8873A", "d": DARK}
    for n, g, k in (("a", A, {"#": MAIN, "h": LIGHT, "d": DARK}),
                    ("b", B, {"#": MAIN}), ("c", C, keyC)):
        name = "studygromacs_" + n
        open(os.path.join(d, name + ".xpm"), "w").write(xpm(name, g, k))


main()
