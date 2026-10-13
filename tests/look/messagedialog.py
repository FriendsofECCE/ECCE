#!/usr/bin/env python3
"""Screenshot ECCE's message dialog (F26).

    tests/look/messagedialog.py OUTDIR [--include DIR] [--libdir DIR] [--tag NAME]

Builds messagedialog.C against a tree, runs it on a private Xvfb at a typical
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

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "apps"))
import xdisplay  # noqa: E402

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
    ap.add_argument("--display", default=None,
                    help="X display number to use (default: a free one)")
    o = ap.parse_args()
    os.makedirs(o.outdir, exist_ok=True)
    work = tempfile.mkdtemp(prefix="messagedialog")
    binary = os.path.join(work, "messagedialog")
    cxx = out(["wx-config", "--cxxflags"]).stdout.split()
    libs = out(["wx-config", "--libs", "core,base,adv,html,aui,gl,xml,richtext"]).stdout.split()
    cmd = (["nice", "-n", "19", "g++", "-std=c++17", "-o", binary,
            os.path.join(HERE, "messagedialog.C"), "-I" + o.include] + cxx
           + ["-L" + o.libdir] + ["-l" + lib for lib in LIBS]
           + libs + out(["pkg-config", "--libs", "gtk+-3.0"]).stdout.split() + ["-lxerces-c", "-lssh", "-lmosquitto", "-lssl", "-lcrypto"])
    b = out(cmd)
    if b.returncode:
        print(b.stderr[-3000:])
        return 1
    xvfb = xdisplay.Display(number=int(o.display.lstrip(":")) if o.display else None,
                              screen="1000x600x24").__enter__()
    o.display = xvfb.name
    env = dict(os.environ, DISPLAY=o.display, HOME=work,
               ECCE_REALUSERHOME=work, ECCE_HOME=ROOT)
    try:
        time.sleep(2)
        shots = os.path.join(o.outdir, o.tag)
        os.makedirs(shots, exist_ok=True)
        r = out([binary, shots], env=env, timeout=60)
        print(r.stdout + r.stderr[-2000:])
    finally:
        xvfb.__exit__()
        shutil.rmtree(work, True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
