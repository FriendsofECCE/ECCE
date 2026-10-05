#!/usr/bin/env python3
"""Measure and screenshot the save control of the editors' bars (#210).

    tests/look/save_button.py OUTDIR [--include DIR] [--libdir DIR] [--tag NAME]

Builds save_button.C against a tree, runs it on a private Xvfb at a typical
and a narrow width, prints the layout and writes PNGs.  Run it once per
tree to compare before and after.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LIBS = ["eccewxgui", "eccewxplotctrl", "eccewxthings", "eccewxgui",
        "eccecomm", "eccercmd", "eccedsi", "eccedav", "eccecipc",
        "eccefaces", "eccexml", "eccetdat", "ecceutil"]


def out(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--include", default=os.path.join(ROOT, "include"))
    ap.add_argument("--libdir", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--tag", default="after")
    ap.add_argument("--display", default=":181")
    o = ap.parse_args()
    os.makedirs(o.outdir, exist_ok=True)
    work = tempfile.mkdtemp(prefix="save-button")
    binary = os.path.join(work, "save_button")
    cxx = out(["wx-config", "--cxxflags"]).stdout.split()
    libs = out(["wx-config", "--libs", "core,base,adv,html"]).stdout.split()
    cmd = (["nice", "-n", "19", "g++", "-std=c++17", "-o", binary,
            os.path.join(HERE, "save_button.C"), "-I" + o.include] + cxx
           + ["-L" + o.libdir] + ["-l" + lib for lib in LIBS]
           + libs + ["-lxerces-c", "-lssh", "-lmosquitto"])
    b = out(cmd)
    if b.returncode:
        print(b.stderr[-3000:])
        return 1
    xvfb = subprocess.Popen(["Xvfb", o.display, "-screen", "0", "1400x400x24"],
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    env = dict(os.environ, DISPLAY=o.display, HOME=work,
               ECCE_REALUSERHOME=work, ECCE_HOME=ROOT)
    try:
        time.sleep(2)
        for width, mode in ((900, "MODIFIED"), (500, "MODIFIED"),
                            (900, "READONLY")):
            png = os.path.join(o.outdir, "%s-%s-%d.png"
                               % (o.tag, mode.lower(), width))
            p = subprocess.Popen([binary, str(width), mode, "3000"], env=env,
                                 stdout=subprocess.PIPE, text=True)
            time.sleep(2)
            subprocess.run(["import", "-window", "root", "-crop",
                            "%dx150+0+0" % width, png], env=env)
            try:
                text = p.communicate(timeout=30)[0]
            except subprocess.TimeoutExpired:
                p.kill()
                text = "TIMEOUT: " + (p.communicate()[0] or "")
            print("== %s width %d -> %s" % (mode, width, png))
            print(text)
    finally:
        xvfb.terminate()
        xvfb.wait()
        shutil.rmtree(work, True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
