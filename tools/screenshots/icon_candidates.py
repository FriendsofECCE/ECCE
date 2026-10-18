#!/usr/bin/env python3
"""Headless Organizer shots of the GROMACS study icon and Builder-thumbnail
frame candidates, light and dark.

    tools/screenshots/icon_candidates.py OUTDIR [--build BUILD] [--display N]

Needs a built tree (ninja organizer launchjob ...), Xvfb, apache-free local
data (#216).  HOME and the caches must already point inside the worktree.
Writes OUTDIR/<variant>-<theme>.png, the Organizer window at 1x with a
GROMACS study (Prepare, Optimize, Equilibrate, Dynamics) expanded and the
Prepare selected, so the detail pane shows its Builder button.
"""
import argparse
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
for sub in ("tests/launch", "tests/apps"):
    sys.path.insert(0, os.path.join(ROOT, sub))
import harness   # noqa: E402
import xdisplay  # noqa: E402

CAND = os.path.join(ROOT, "data", "client", "pixmaps", "candidates")
THUMB = os.path.join(ROOT, "tests", "apps", "fixtures", "calc-water-vib",
                     "Parameters", "Thumbnail.jpeg")
VARIANTS = [   # name, env
    ("icon-current", {"ECCE_ICON_ALT": "studygromacs.xpm=%s/mdstudy_current.xpm"
                      % CAND}),
    ("icon-A", {"ECCE_ICON_ALT": "studygromacs.xpm=%s/studygromacs_a.xpm" % CAND}),
    ("icon-B", {"ECCE_ICON_ALT": "studygromacs.xpm=%s/studygromacs_b.xpm" % CAND}),
    ("icon-C", {"ECCE_ICON_ALT": "studygromacs.xpm=%s/studygromacs_c.xpm" % CAND}),
    ("thumb-current", {}),
    ("thumb-A-bevel", {"ECCE_THUMB_FRAME": "bevel"}),
    ("thumb-B-border", {"ECCE_THUMB_FRAME": "border"}),
]
THEMES = {"light": "Adwaita", "dark": "Adwaita:dark"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--build", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--display", type=int, default=79)
    ap.add_argument("--only", action="append")
    o = ap.parse_args()
    build = os.path.abspath(o.build)
    os.makedirs(o.outdir, exist_ok=True)
    harness.prerequisites(build, ())
    s = harness.Session(build, "icons", {}, local=True)
    try:
        if not s.services(True):
            print("services did not come up")
            return 1
        rc, out = s.driver("gromacsprepstudy", s.userUrl(), "gmxdemo",
                           os.path.join(ROOT, "tests", "gromacs_md",
                                        "fixtures", "1l2y.pdb"), "equilibrate")
        urls = [l.strip() for l in out.splitlines() if "://" in l]
        print(rc, urls)
        for u in urls[1:]:   # the Builder button shows a stored thumbnail
            d = os.path.join(u[len("file://"):].rstrip("/"), "Parameters")
            os.makedirs(d, exist_ok=True)
            shutil.copy(THUMB, os.path.join(d, "Thumbnail.jpeg"))
        wrappers = os.path.join(s.state, "wrappers")
        shutil.copytree(os.path.join(build, "wrappers"), wrappers)
        for e in os.listdir(wrappers):
            os.chmod(os.path.join(wrappers, e), 0o755)
        harness.link(os.path.join(build, "organizer"),
                     os.path.join(s.home, "bin", "organizer"))
        gw = os.path.join(ROOT, "packaging", "gateway")
        for n in os.listdir(gw):
            if n.startswith("ecce-"):
                harness.link(os.path.join(gw, n), os.path.join(s.home, "bin", n))
        for v, env in VARIANTS:
            if o.only and v not in o.only:
                continue
            for theme, gtk in THEMES.items():
                png = os.path.abspath(os.path.join(o.outdir, "%s-%s.png" % (v, theme)))
                run(s, wrappers, urls, env, gtk, png, o.display)
                print(v, theme, os.path.exists(png))
    finally:
        s.stop() if hasattr(s, "stop") else None
    return 0


def run(s, wrappers, urls, env, gtk, png, number):
    cmds = os.path.join(s.state, "organizer-commands")
    open(cmds, "w").close()
    log = os.path.join(s.state, "organizer.log")
    with xdisplay.Display(number=number, screen="1400x900x24") as display:
        e = s.env(dict({"DISPLAY": display.name, "ECCE_TEST_ORGANIZER": cmds,
                        "GTK_THEME": gtk,
                        "PATH": wrappers + os.pathsep + s.env()["PATH"]}, **env))
        proc = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")], env=e,
            cwd=os.path.join(s.home, "bin"), stdout=open(log, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=120):
            with open(cmds, "a") as h:
                h.write(line + "\n")
            want = "ECCE_TEST_ORGANIZER: %s: " % line
            end = time.time() + timeout
            while time.time() < end:
                if proc.poll() is not None:
                    return "ended"
                for t in open(log, errors="replace").read().splitlines():
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"
        try:
            time.sleep(8)
            print("  size", send("size 1000 640"))
            print("  walk", send("walk"))
            print("  summary", send("summary " + urls[1]))
            time.sleep(2)
            print("  snap", send("snap " + png))
        finally:
            for sig in (15, 9):
                try:
                    os.killpg(proc.pid, sig)
                    proc.wait(timeout=10)
                    break
                except Exception:
                    pass


sys.exit(main())
