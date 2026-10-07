#!/usr/bin/env python3
"""The viewer fills its canvas and picks the clicked atom at content scale 2.

    tests/look/hidpi.py BUILD/viewer-scenes

On a Retina Mac (or GDK_SCALE=2) the GL framebuffer has twice the canvas's
logical size; a viewport in logical units drew into the lower-left quarter
(#133).  Runs the viewer at GDK_SCALE=1 and 2: the frame read back must be
480 or 960 pixels square with the molecule centred, and clicks sent as wx
mouse events in logical units (`wxpick`) must select the atom aimed at.
Exit 77 without the binary or Xvfb, or if wx ignores GDK_SCALE.
"""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

SCENE = """style Ball And Stick
viewall
snap frame
wxpick clicks 1 2 3
"""


def readPpm(path):
    data = open(path, "rb").read()
    m = re.match(rb"P6\s+(\d+)\s+(\d+)\s+255\s", data)
    w, h = int(m.group(1)), int(m.group(2))
    return w, h, data[m.end():]


def centroid(w, h, px):
    bg = px[0:3]
    sx = sy = n = 0
    for y in range(0, h, 2):
        row = y * w * 3
        for x in range(0, w, 2):
            i = row + x * 3
            if px[i:i + 3] != bg:
                sx += x
                sy += y
                n += 1
    return (sx / n / w, sy / n / h) if n else (None, None)


def run(binary, scale, display):
    out = tempfile.mkdtemp(prefix="hidpi%d-" % scale)
    scene = os.path.join(out, "hidpi.scene")
    open(scene, "w").write(SCENE)
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", ECCE_HOME=ROOT,
               GDK_SCALE=str(scale), DISPLAY=":%d" % display)
    env.pop("FL_FONT_PATH", None)
    env.pop("GDK_DPI_SCALE", None)
    env["ECCE_REALUSERHOME"] = tempfile.mkdtemp(prefix="hidpi-home")
    r = subprocess.run([binary, out, scene, "water"], env=env,
                       capture_output=True, text=True, timeout=300)
    if r.returncode != 0:
        print("viewer-scenes (scale %d) failed:\n%s" % (scale, (r.stdout + r.stderr)[-1500:]))
        return None
    return out


def check(scale, out):
    ok = True
    clicks = open(os.path.join(out, "clicks.txt")).read()
    print("scale %d clicks:\n%s" % (scale, clicks.rstrip()))
    got = re.findall(r"scale (\S+) .*click atom (\d+) at .*: selected(.*)", clicks)
    if got and float(got[0][0]) != scale:
        return None
    for s, atom, sel in got:
        if sel.split() != [atom]:
            print("FAIL scale %d: clicking atom %s selected [%s]" % (scale, atom, sel.strip()))
            ok = False
    if len(got) != 3:
        print("FAIL scale %d: %d clicks recorded" % (scale, len(got)))
        ok = False
    w, h, px = readPpm(os.path.join(out, "frame.ppm"))
    cx, cy = centroid(w, h, px)
    print("scale %d frame %dx%d, molecule centre at (%s, %s)" % (scale, w, h, cx, cy))
    if (w, h) != (480 * scale, 480 * scale):
        print("FAIL scale %d: frame is %dx%d" % (scale, w, h))
        ok = False
    if cx is None or abs(cx - 0.5) > 0.15 or abs(cy - 0.5) > 0.15:
        print("FAIL scale %d: molecule not centred" % scale)
        ok = False
    return ok


def main():
    binary = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else ""
    if not os.access(binary, os.X_OK):
        print("SKIP: viewer-scenes not built (ninja viewer-scenes)")
        return 77
    import xdisplay
    try:
        xdisplay.findXvfb()
    except xdisplay.DisplayUnavailable as e:
        print("SKIP: %s" % e)
        return 77
    os.environ.setdefault("ECCE_TEST_XDISPLAYS", "170-179")
    results = {}
    with xdisplay.Display() as display:
        for scale in (1, 2):
            out = run(binary, scale, display.number)
            if out is None:
                return 1
            results[scale] = check(scale, out)
    if results[2] is None:
        print("SKIP: wx did not take GDK_SCALE=2 as the content scale")
        return 77 if results[1] else 1
    ok = results[1] and results[2]
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


sys.exit(main())
