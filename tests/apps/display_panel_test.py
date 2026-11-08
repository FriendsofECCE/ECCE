#!/usr/bin/env python3
"""The Builder's Display panel (#881), headless through scene hooks.

Opens a stored water calculation with MOs, draws an MO surface, then:
  * sets the lobe colours through the panel and checks the drawn surface's
    pixel colours change (red/green default -> blue/yellow lobes);
  * sets the background white and checks the corner pixel;
  * moves a quality slider (Automatic off) and ticks Automatic again;
  * checks the lobe colours and opacity reached the preferences file.

    ECCE_DISPLAY_PNG=<dir>   also copy the panel picture there

Same installed tree and isolation as orca_mo_test.py.  Exit 77 (skip)
without an install or Xvfb.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                   # noqa: E402
import isolate                # noqa: E402
import orca_mo_test as base   # noqa: E402
import xdisplay               # noqa: E402


def ppm(path):
    from PIL import Image
    return Image.open(path).convert("RGB")


def count(img, test):
    px = img.load()
    w, h = img.size
    return sum(1 for y in range(h) for x in range(w) if test(*px[x, y]))


def info(out, name):
    path = os.path.join(out, name + ".txt")
    return dict(l.split(" ", 1) for l in open(path).read().splitlines() if l)


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    pngdir = os.environ.get("ECCE_DISPLAY_PNG")
    work = tempfile.mkdtemp(prefix="display-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1700x1000x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    scene = ("hold 2\nmopanel compute\nhold 2\ndisplay open\nhold 1\n"
             "display info i0\nsnap s_default\n"
             "display lobes 0 0.2 1 1 1 0\nhold 2\nsnap s_custom\n"
             "display opacity 100\nhold 1\ndisplay info i1\n"
             "display slider sphrq 60\ndisplay info i2\n"
             "display auto sphrq on\ndisplay info i3\n"
             "display background 1 1 1\nhold 2\nsnap s_white\n"
             "display info i4\nhold 2\n")
    shotpath = os.path.join(work, "run", "panel.png")
    try:
        with xdisplay.Display() as display:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-start")],
                           env=display.env(), timeout=180)
            url = base.makeCalc(data, "water")
            out = os.path.join(work, "run")
            res = base.runBuilder(display, url, out, scene, shot=shotpath,
                                  panel="MOs")
        check(not res.crashed, "the Builder did not crash")
        check(not os.path.exists(os.path.join(out, "FAILED")),
              "the scene ran to the end")
        d = ppm(os.path.join(out, "s_default.ppm"))
        c = ppm(os.path.join(out, "s_custom.ppm"))
        red = lambda r, g, b: r > 120 and g < 70 and b < 70
        blue = lambda r, g, b: b > 120 and r < 70 and g < 160
        check(count(d, red) > 200, "default positive lobe is red")
        check(count(c, red) < count(d, red) // 4,
              "red lobe gone after setting lobe colours")
        #  Water's lowest MO has one sign only, so only the positive lobe
        #  colour is on screen.
        check(count(c, blue) > 200,
              "blue lobe drawn after setting lobe colours (%d px)"
              % count(c, blue))
        if os.environ.get("ECCE_DISPLAY_KEEP"): shutil.copy(os.path.join(out, "s_custom.ppm"), os.environ["ECCE_DISPLAY_KEEP"])
        w = ppm(os.path.join(out, "s_white.ppm"))
        check(w.getpixel((2, 2)) == (255, 255, 255),
              "background white: corner pixel %s" % (w.getpixel((2, 2)),))
        check(info(out, "i0")["sphrq"].endswith("auto 1"),
              "sphere quality starts automatic")
        check(info(out, "i2")["sphrq"] == "60 auto 0",
              "moving the slider unticks Automatic: %s" % info(out, "i2")["sphrq"])
        check(info(out, "i3")["sphrq"].endswith("auto 1"),
              "ticking Automatic again")
        check(info(out, "i1")["opacity"] == "100", "opacity set")
        ini = [os.path.join(d, f) for d, _, fs in
               os.walk(os.environ["ECCE_REALUSERHOME"])
               for f in fs if f == "wxbuilder.ini"]
        text = open(ini[0]).read() if ini else ""
        check("PosColor=0.0000,0.2000,1.0000" in text and
              "NegColor=1.0000,1.0000,0.0000" in text and
              "Opacity=100" in text,
              "lobe colours and opacity are in the preferences file (%s)" % (ini,))
        if pngdir and os.path.exists(shotpath):
            os.makedirs(pngdir, exist_ok=True)
            shutil.copy(shotpath, pngdir)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    if failures:
        print("FAILED: %d" % len(failures))
        return 1
    print("display panel: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
