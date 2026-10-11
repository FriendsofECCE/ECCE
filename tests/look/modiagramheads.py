#!/usr/bin/env python3
"""The MO diagram's column headings must not overlap, docked or floating.

    tests/look/modiagramheads.py [OUTDIR] [--libdir BUILD] [--left H --right F]

Paints the real MoDiagramCanvas (tests/look/modiagramheads.C) at several
widths, three times each with one heading blanked, and takes each heading's
box from the pixels that change.  Fails if two boxes meet, or a box leaves
the picture.  Needs wx for a bitmap DC, no display.
"""
import argparse
import os
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LIBS = ["eccewxgui", "eccewxplotctrl", "eccewxthings", "eccewxgui",
        "eccecomm", "eccercmd", "eccedsi", "eccedav", "eccecipc",
        "eccefaces", "eccexml", "eccetdat", "ecceutil"]
WIDTHS = (300, 330, 400, 500, 600, 800, 1100)


def out(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def box(a, b):
    diff = ImageChops.difference(a.convert("RGB"), b.convert("RGB"))
    return diff.getbbox()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir", nargs="?")
    ap.add_argument("--libdir", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--left", default="H")
    ap.add_argument("--right", default="F")
    ap.add_argument("--canvas-dir", default=os.path.join(ROOT, "src/apps/builder"),
                    help="where MoDiagramCanvas.H is read from (an old copy "
                    "shows the overlap this test is about)")
    o = ap.parse_args()
    work = tempfile.mkdtemp(prefix="heads")
    cxx = out(["wx-config", "--cxxflags"]).stdout.split()
    libs = out(["wx-config", "--libs", "core,base,adv,html,aui,gl"]).stdout.split()
    binary = os.path.join(work, "heads")
    b = out(["nice", "-n", "19", "g++", "-std=c++17", "-o", binary,
             os.path.join(HERE, "modiagramheads.C"),
             "-I" + os.path.join(ROOT, "include"),
             "-I" + o.canvas_dir] + cxx +
            ["-L" + o.libdir, "-Wl,-rpath," + o.libdir,
             "-Wl,--start-group"] + ["-l" + l for l in LIBS] +
            ["-Wl,--end-group"] + libs +
            ["-lxerces-c", "-lssh", "-lmosquitto", "-lssl", "-lcrypto"] +
            out(["pkg-config", "--libs", "gtk+-3.0"]).stdout.split())
    if b.returncode:
        print(b.stderr[-3000:])
        return 1
    env = dict(os.environ, HOME=work, ECCE_HOME=ROOT, ECCE_REALUSERHOME=work)
    status = 0
    L, C, R = o.left, "Molecular orbitals", o.right

    def paint(name, width, l, c, r):
        path = os.path.join(work, "%s-%d.png" % (name, width))
        res = out([binary, path, str(width), l, c, r], env=env, timeout=60)
        if res.returncode:
            print(res.stderr[-500:])
            sys.exit(1)
        return Image.open(path)

    for width in WIDTHS:
        full = paint("full", width, L, C, R)
        boxes = {"left": box(full, paint("nol", width, "~", C, R)),
                 "centre": box(full, paint("noc", width, L, "~", R)),
                 "right": box(full, paint("nor", width, L, C, "~"))}
        # The heading row only: levels and labels are below it.
        boxes = {k: v for k, v in boxes.items() if v and v[1] < 80}
        problems = []
        names = sorted(boxes, key=lambda k: boxes[k][0])
        for i in range(len(names) - 1):
            a, b2 = boxes[names[i]], boxes[names[i + 1]]
            if a[2] + 3 > b2[0]:
                problems.append("%s heading (to x=%d) meets %s (from x=%d)"
                                % (names[i], a[2], names[i + 1], b2[0]))
        for k, v in boxes.items():
            if v[0] < 0 or v[2] > width:
                problems.append("%s heading leaves the picture" % k)
        print("%4d px  %s  %s" % (width, "OK  " if not problems else "FAIL",
                                  "; ".join(problems) or
                                  " ".join("%s %d-%d" % (k, boxes[k][0],
                                                         boxes[k][2])
                                           for k in names)))
        if problems:
            status = 1
        if o.outdir:
            os.makedirs(o.outdir, exist_ok=True)
            full.save(os.path.join(o.outdir, "heads-%d.png" % width))
    # A click puts the level's text under the diagram: it must be drawn.
    if o.outdir and os.path.exists(os.path.join(o.outdir, "heads-600.png")):
        for width in (330, 600):
            path = os.path.join(o.outdir, "clicked-%d.png" % width)
            res = out([binary, path, str(width), L, C, R, "click"], env=env,
                      timeout=60)
            if res.returncode:
                print(res.stderr[-500:])
                status = 1
                continue
            plain = Image.open(os.path.join(o.outdir, "heads-%d.png" % width))
            changed = box(plain, Image.open(path))
            ok = changed is not None and changed[3] > 400
            print("%4d px  %s  clicked level's text drawn under the diagram "
                  "(changed rows from %s)" % (width, "OK  " if ok else "FAIL",
                                              changed[1] if changed else "-"))
            if not ok:
                status = 1
    return status


if __name__ == "__main__":
    sys.exit(main())
