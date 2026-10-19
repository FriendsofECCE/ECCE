#!/usr/bin/env python3
"""Renders previews of the candidate SVG icons with librsvg (no ECCE needed)."""
import sys, os, gi, cairo
gi.require_version("Rsvg", "2.0")
from gi.repository import Rsvg
from PIL import Image, ImageDraw
import io
D = sys.argv[1]; OUT = sys.argv[2]
os.makedirs(OUT, exist_ok=True)
STEPS = ["prepare", "optimize", "equilibrate", "dynamics", "energy"]
def ras(path, s):
    h = Rsvg.Handle.new_from_file(path)
    sf = cairo.ImageSurface(cairo.FORMAT_ARGB32, s, s)
    c = cairo.Context(sf); c.scale(s/64, s/64)
    h.render_cairo(c)
    im = Image.frombuffer("RGBA", (s, s), bytes(sf.get_data()), "raw", "BGRA", 0, 1)
    return im
def tri(s, col=(0, 0, 255)):
    im = Image.new("RGBA", (s, s), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    d.polygon([(s*.2, s*.15), (s*.2, s*.85), (s*.85, s*.5)], fill=col+(255,))
    return im
def paste(bg, im, xy): bg.paste(im, xy, im)
sets = [("A", "a"), ("B", "b")]
# sheet 1: per-size previews on white and dark
for bgname, bgc in (("white", (255,255,255)), ("dark", (0x2d,0x2d,0x2d))):
    sizes = (16, 24, 32)
    rows = [("study", [os.path.join(D, "studygromacs_%s.svg" % k) for k in "abc"])]
    W = 20 + 5*(32*3+40) ; 
    cellw = sum(sizes)+ 40
    H = 20 + (2*len(sets)+1)*50
    im = Image.new("RGB", (20 + 5*cellw, H), bgc); y = 10
    # study row
    x = 10
    for k in "abc":
        for s in sizes:
            paste(im, ras(os.path.join(D, "studygromacs_%s.svg" % k), s), (x, y)); x += s+8
        x += 20
    y += 50
    for _, k in sets:
        for n in STEPS:
            pass
        x = 10
        for n in STEPS:
            for s in sizes:
                paste(im, ras(os.path.join(D, "taskgmx_%s_%s.svg" % (n, k)), s), (x, y)); x += s+8
            x += 20
        y += 50
    im.save(os.path.join(OUT, "sizes_%s.png" % bgname))
# sheet 2: mock tree strips at 1x and 2x
for scale in (1, 2):
    for bgname, bgc, fg in (("white", (255,255,255), (0,0,0)), ("dark", (0x2d,0x2d,0x2d), (230,230,230))):
        lab = ["prepare", "optimize", "equilibrate", "dynamics"]
        S = 16*scale
        rowh = S+6*scale
        blocks = [("A", "a")] + [] 
        im = Image.new("RGB", (4*140*scale//1, 1), bgc)
        total = 0
        parts = []
        for _, k in sets:
            parts.append(k)
        H = (1 + len(parts)*4)*rowh + 20
        im = Image.new("RGB", (180*scale, H), bgc); d = ImageDraw.Draw(im); y = 6
        paste(im, ras(os.path.join(D, "studygromacs_a.svg"), S), (6, y)); d.text((6+S+4*scale, y+2), "gromacs study (candidate A)", fill=fg); y += rowh+6
        for k in parts:
            for n in lab:
                paste(im, ras(os.path.join(D, "taskgmx_%s_%s.svg" % (n, k)), S), (6, y))
                paste(im, tri(S), (6+S, y))
                d.text((6+2*S+4*scale, y+2), n + "  (" + k + ")", fill=fg); y += rowh
            y += 6
        im.save(os.path.join(OUT, "tree_%dx_%s.png" % (scale, bgname)))
