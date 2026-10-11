#!/usr/bin/env python3
"""The Calculation Editor's code icons, enabled and disabled (behind a modal).

    tests/look/codebuttons.py OUTDIR [--libdir BUILD] [--thing FILE] [--check]

Builds codebuttons.C against a build tree (ThingToggle.C of this source tree,
or --thing FILE, is compiled in, so a stale library does not matter), runs it
on a private Xvfb and saves enabled.png and disabled.png of a row of code
buttons.  With --check, fails if the disabled row has black squares or its
icons are gone (the disabled icon must be pale, not black, and not blank).
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
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
import xdisplay  # noqa: E402

LIBS = ["eccewxgui", "eccewxplotctrl", "eccewxthings", "eccewxgui",
        "eccecomm", "eccercmd", "eccedsi", "eccedav", "eccecipc",
        "eccefaces", "eccexml", "eccetdat", "ecceutil"]


def out(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def dark(img):
    """Fraction of pixels that are near black."""
    px = list(img.convert("RGB").getdata())
    return sum(1 for p in px if sum(p) < 90) / len(px)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--libdir", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--thing", default=os.path.join(
        ROOT, "src/wxgui/wxthings/ThingToggle.C"))
    ap.add_argument("--theme", default="Adwaita")
    ap.add_argument("--check", action="store_true")
    o = ap.parse_args()
    os.makedirs(o.outdir, exist_ok=True)
    work = tempfile.mkdtemp(prefix="codebuttons")
    binary = os.path.join(work, "codebuttons")
    cxx = out(["wx-config", "--cxxflags"]).stdout.split()
    libs = out(["wx-config", "--libs", "core,base,adv,html,aui,gl"]).stdout.split()
    cmd = (["nice", "-n", "19", "g++", "-std=c++17", "-o", binary,
            os.path.join(HERE, "codebuttons.C"), o.thing,
            "-I" + os.path.join(ROOT, "include")] + cxx
           + ["-L" + o.libdir, "-Wl,-rpath," + o.libdir,
              "-Wl,--start-group"] + ["-l" + lib for lib in LIBS]
           + ["-Wl,--end-group"] + libs
           + ["-lxerces-c", "-lssh", "-lmosquitto", "-lssl", "-lcrypto"]
           + out(["pkg-config", "--libs", "gtk+-3.0"]).stdout.split())
    b = out(cmd)
    if b.returncode:
        print(b.stderr[-3000:])
        return 1
    xvfb = xdisplay.Display(screen="800x600x24").__enter__()
    status = 0
    try:
        time.sleep(2)
        env = dict(os.environ, DISPLAY=xvfb.name, HOME=work,
                   ECCE_REALUSERHOME=work, ECCE_HOME=ROOT,
                   GTK_THEME=o.theme)
        r = out([binary, o.outdir], env=env, timeout=60)
        if r.returncode:
            print(r.stderr[-500:])
            return 1
        for state in ("enabled", "disabled", "modal"):
            img = Image.open(os.path.join(o.outdir, state + ".png"))
            d = dark(img)
            print("%s: %.1f%% near-black pixels" % (state, 100 * d))
            if o.check and state != "enabled" and d > 0.10:
                print("FAIL: the disabled code icons are black squares")
                status = 1
    finally:
        xvfb.__exit__()
        shutil.rmtree(work, True)
    return status


if __name__ == "__main__":
    sys.exit(main())
