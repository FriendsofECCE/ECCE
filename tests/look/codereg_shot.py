#!/usr/bin/env python3
"""Render the code-registration Theory/Runtype Details dialogs to PNGs.

    tests/look/codereg_shot.py OUTDIR [--theme light|dark|both] [--tag NAME]
                               [--codes orca,nwchem,...] [--scripts DIR]

Runs the real dialog scripts (unmodified) on a private Xvfb, grabs the
frame once it is mapped, and writes OUTDIR/<tag>-<code>-<theory|runtype>-<theme>.png.
Used to compare the dialogs against the C++ windows (#210).
"""
import argparse
import os
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

# code -> (theory script, runtype script, category, theory, runtype)
CASES = {
    "orca": ("orcatheory.py", "orcaruntype.py", "DFT", "RDFT", "Geometry"),
    "nwchem": ("nedtheory.py", "nedruntype.py", "SCF", "RHF", "Geometry"),
    "gaussian16": ("ged16theory.py", "ged16runtype.py", "DFT", "RDFT",
                   "Geometry"),
    "mopac": ("mopactheory.py", "mopacruntype.py", "SE", "RPM7", "Energy"),
    "qe": ("qetheory.py", "qeruntype.py", "PW", "PW", "Energy"),
    "gromacs": ("gromacstheory.py", "gromacsruntype.py", "MD", "MD", "Energy"),
}

INNER = r'''
import os, sys, subprocess
sys.path.insert(0, os.environ["CODEREG_DIR"])
os.chdir(os.environ["CODEREG_DIR"])
import templates
from templates import wx
out = os.environ["SHOT_OUT"]
def finalize(self):
    import time
    self.Fit()
    self.Show(True)
    flash = os.environ.get("SHOT_FLASH")
    if flash:
        panel = [c for c in self.GetChildren()
                 if isinstance(c, templates.EccePanel)][0]
        panel.SetStatusText("Value out of range: hold the colour for the shot",
                            flash)
        panel.colorTimer.Stop()
        panel.messageTimer.Stop()
    end = time.time() + 1.5
    while time.time() < end:
        wx.GetApp().Yield()
        time.sleep(0.05)
    r = self.GetScreenRect()
    subprocess.run(["import", "-window", "root", out + ".root.png"])
    subprocess.run(["convert", out + ".root.png", "-crop",
                    "%dx%d+%d+%d" % (r.width, r.height, r.x, r.y),
                    "+repage", out])
    os.unlink(out + ".root.png")
templates.EcceFrame.Finalize = finalize
import runpy
runpy.run_path(os.environ["SHOT_SCRIPT"], run_name="__main__")
'''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--theme", default="light")
    ap.add_argument("--tag", default="after")
    ap.add_argument("--codes", default=",".join(CASES))
    ap.add_argument("--scripts", default=os.path.join(ROOT, "scripts/codereg"))
    ap.add_argument("--flash", choices=("warning", "error"), default=None,
                    help="show a status message with that flash colour")
    ap.add_argument("--fontsize", default=None,
                    help="write FONTSIZE:<n> to the test preferences")
    o = ap.parse_args()
    os.makedirs(o.outdir, exist_ok=True)
    sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
    import xdisplay
    work = tempfile.mkdtemp(prefix="codereg-shot")
    os.makedirs(os.path.join(work, ".ECCE"))
    if o.fontsize:
        with open(os.path.join(work, ".ECCE", "EcceGlobal"), "w") as f:
            f.write("FONTSIZE:\t%s\n" % o.fontsize)
    sink = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sink.bind(("127.0.0.1", 0))
    port = str(sink.getsockname()[1])
    status = 0
    with xdisplay.Display(screen="1400x1000x24") as xv:
        time.sleep(1)
        themes = ["light", "dark"] if o.theme == "both" else [o.theme]
        for theme in themes:
            gtk = "Adwaita:dark" if theme == "dark" else "Adwaita"
            for code in o.codes.split(","):
                th, rt, cat, theory, run = CASES[code]
                for kind, script in (("theory", th), ("runtype", rt)):
                    png = os.path.join(o.outdir, "%s-%s-%s-%s.png"
                                       % (o.tag, code, kind, theme))
                    restore = os.path.join(work, "restore.in")
                    open(restore, "w").write("END_GUIValues\n")
                    argv = [restore, port, "GUIValues", "Writable",
                            "DebugOff", cat, theory, run, "shot", "0", "C1",
                            "10", "1", "0", "5", "5", "6"]
                    env = dict(os.environ, DISPLAY=xv.name, GTK_THEME=gtk,
                               ECCE_REALUSERHOME=work, HOME=work,
                               CODEREG_DIR=o.scripts, SHOT_OUT=png,
                               SHOT_SCRIPT=os.path.join(o.scripts, script),
                               PYTHONDONTWRITEBYTECODE="1",
                               SHOT_FLASH=o.flash or "")
                    r = subprocess.run([sys.executable, "-c", INNER] + argv,
                                       env=env, capture_output=True, text=True, errors="replace",
                                       timeout=120)
                    if not os.path.exists(png):
                        print("FAILED %s %s %s rc=%s\n%s%s" % (code, kind, theme, r.returncode, r.stdout,
                                                       r.stderr[-1500:]))
                        status = 1
                    else:
                        print(png)
    return status


if __name__ == "__main__":
    sys.exit(main())
