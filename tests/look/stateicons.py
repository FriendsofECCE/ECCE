#!/usr/bin/env python3
"""Render each run-state icon at the app's sizes, check outlines stay inside.

    tests/look/stateicons.py OUTDIR [--libdir BUILD] [--tag NAME] [--check]

Builds stateicons.C against a build tree (the WxState.C of this source tree
is compiled in, so a stale library does not matter), runs it on a private
Xvfb in light and dark, writes per-icon PNGs and an 8x nearest-neighbour
contact sheet per size.  With --check, fails if any non-background pixel
touches the bitmap edge.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LIBS = ["eccewxgui", "eccewxplotctrl", "eccewxthings", "eccewxgui",
        "eccecomm", "eccercmd", "eccedsi", "eccedav", "eccecipc",
        "eccefaces", "eccexml", "eccetdat", "ecceutil"]
SIZES = [12, 16, 20, 24]


def out(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--libdir", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--tag", default="after")
    ap.add_argument("--display", default=":184")
    ap.add_argument("--check", action="store_true")
    o = ap.parse_args()
    os.makedirs(o.outdir, exist_ok=True)
    work = tempfile.mkdtemp(prefix="stateicons")
    binary = os.path.join(work, "stateicons")
    cxx = out(["wx-config", "--cxxflags"]).stdout.split()
    libs = out(["wx-config", "--libs", "core,base,adv,html"]).stdout.split()
    cmd = (["nice", "-n", "19", "g++", "-std=c++17", "-o", binary,
            os.path.join(HERE, "stateicons.C"),
            os.path.join(ROOT, "src/wxgui/wxtools/WxState.C"),
            "-I" + os.path.join(ROOT, "include")] + cxx
           + ["-L" + o.libdir, "-Wl,-rpath," + o.libdir]
           + ["-l" + lib for lib in LIBS]
           + libs + ["-lxerces-c", "-lssh", "-lmosquitto"])
    b = out(cmd)
    if b.returncode:
        print(b.stderr[-3000:])
        return 1
    xvfb = subprocess.Popen(["Xvfb", o.display, "-screen", "0", "800x600x24"],
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    status = 0
    try:
        time.sleep(2)
        for theme in ("Adwaita", "Adwaita:dark"):
            tn = "dark" if "dark" in theme else "light"
            raw = os.path.join(work, tn)
            os.makedirs(raw)
            env = dict(os.environ, DISPLAY=o.display, HOME=work,
                       ECCE_REALUSERHOME=work, ECCE_HOME=ROOT,
                       GTK_THEME=theme)
            r = out([binary, raw], env=env, timeout=60)
            if r.returncode:
                print(r.stderr[-500:])
                status = 1
                continue
            for size in SIZES:
                files = sorted((f for f in os.listdir(raw)
                                if f.startswith("icon-%d-" % size)),
                               key=lambda f: int(f.split("-")[2][:-4]))
                imgs = [Image.open(os.path.join(raw, f)).convert("RGB")
                        for f in files]
                bg = (255, 255, 255)
                sheet = Image.new("RGB", (size * len(imgs) + len(imgs) + 1,
                                          size + 2), (200, 200, 200))
                for i, im in enumerate(imgs):
                    sheet.paste(im, (1 + i * (size + 1), 1))
                    w, h = im.size
                    edge = [im.getpixel(p) for x in range(w) for p in
                            ((x, 0), (x, h - 1))] + \
                           [im.getpixel(p) for y in range(h) for p in
                            ((0, y), (w - 1, y))]
                    touch = sum(1 for p in edge if p != bg)
                    if touch:
                        print("%s size %d state %d: %d edge pixels non-bg"
                              % (tn, size, i, touch))
                        if o.check:
                            status = 1
                sheet = sheet.resize((sheet.width * 8, sheet.height * 8),
                                     Image.NEAREST)
                sheet.save(os.path.join(o.outdir, "%s-icons%d-%s.png"
                                        % (o.tag, size, tn)))
    finally:
        xvfb.terminate()
        xvfb.wait()
        shutil.rmtree(work, True)
    return status


if __name__ == "__main__":
    sys.exit(main())
