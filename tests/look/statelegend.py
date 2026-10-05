#!/usr/bin/env python3
"""Render the Organizer's run-state legend in the light and dark themes.

    tests/look/statelegend.py OUTDIR [--libdir BUILD] [--ecce-home DIR] [--tag NAME]

Builds statelegend.C against a build tree and runs it on a private Xvfb,
once with Adwaita and once with Adwaita:dark.  The colours come from
EcceGlobal under --ecce-home (data/client/config), so pointing it at a
tree with an older EcceGlobal renders the older colours.
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
    ap.add_argument("--libdir", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--ecce-home", default=ROOT)
    ap.add_argument("--tag", default="after")
    ap.add_argument("--display", default=":183")
    o = ap.parse_args()
    os.makedirs(o.outdir, exist_ok=True)
    work = tempfile.mkdtemp(prefix="statelegend")
    binary = os.path.join(work, "statelegend")
    cxx = out(["wx-config", "--cxxflags"]).stdout.split()
    libs = out(["wx-config", "--libs", "core,base,adv,html"]).stdout.split()
    cmd = (["nice", "-n", "19", "g++", "-std=c++17", "-o", binary,
            os.path.join(HERE, "statelegend.C"),
            "-I" + os.path.join(ROOT, "include")] + cxx
           + ["-L" + o.libdir, "-Wl,-rpath," + o.libdir]
           + ["-l" + lib for lib in LIBS]
           + libs + ["-lxerces-c", "-lssh", "-lmosquitto"])
    b = out(cmd)
    if b.returncode:
        print(b.stderr[-3000:])
        return 1
    xvfb = subprocess.Popen(["Xvfb", o.display, "-screen", "0", "1600x200x24"],
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    status = 0
    try:
        time.sleep(2)
        for theme in ("Adwaita", "Adwaita:dark"):
            png = os.path.join(o.outdir, "%s-%s.png"
                               % (o.tag, "dark" if "dark" in theme else "light"))
            env = dict(os.environ, DISPLAY=o.display, HOME=work,
                       ECCE_REALUSERHOME=work, ECCE_HOME=o.ecce_home,
                       GTK_THEME=theme)
            r = out([binary, png], env=env, timeout=60)
            print(r.stdout.strip() or r.stderr.strip()[-500:])
            status |= r.returncode
    finally:
        xvfb.terminate()
        xvfb.wait()
        shutil.rmtree(work, True)
    return status


if __name__ == "__main__":
    sys.exit(main())
