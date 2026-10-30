#!/usr/bin/env python3
"""README screenshots from the shipped examples, headless, in local mode.

    ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<bin> \\
        tools/screenshots/readme_examples.py --out <dir>

Writes organizer.png (the Organizer on the finished ORCA Opt+Freq of
ethylene, Progress box in view) and viewer.png (the Builder with the HOMO of
ethene on a white background, MO Diagram docked, Properties showing).  The
examples are imported by the Organizer's own `examples organic` command;
nothing is clicked.  Needs Xvfb, xdotool and ImageMagick.
"""
import argparse
import os
import shutil
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)), "tests", "apps"))

import apps           # noqa: E402
import examples_arm   # noqa: E402
import isolate        # noqa: E402
import readme         # noqa: E402
import xdisplay       # noqa: E402

PROJECT = "Examples/Organic_Chemistry"


def find(root, name):
    for base, dirs, _ in os.walk(root):
        if name in dirs and os.path.isdir(os.path.join(base, name, "Props")):
            return os.path.join(base, name)
    return None


def organizer(display, data, work, out):
    root = os.path.join(data, "users", "local", PROJECT)
    cmds = os.path.join(work, "commands")
    open(cmds, "w").write("examples organic\n")
    shot = os.path.join(work, "organizer-raw.png")
    known = set(w for w, _ in display.windows())
    proc = readme.launch(display, "ecce-organizer", [],
                         {"ECCE_TEST_ORGANIZER": cmds, "ECCE_NO_EXAMPLE_IMPORT": ""},
                         log=os.path.join(work, "organizer.log"))
    try:
        win = readme.waitWindow(display, known, 90, "ECCE Organizer")
        if not win:
            return "no Organizer window"
        if not examples_arm.waitStable(root, 21, 600):
            return "import did not finish"
        calc = find(root, "ethylene")
        if not calc:
            return "no ethylene calculation"
        with open(cmds, "a") as h:
            h.write("size 1300 800\nsummary file://%s\n" % calc)
        time.sleep(8)
        with open(cmds, "a") as h:
            h.write("progressbox\nshowbox\nsnap %s\n" % shot)
        time.sleep(10)
    finally:
        readme.stop(proc)
    if not os.path.exists(shot):
        return "no snapshot"
    shutil.copy(shot, os.path.join(out, "organizer.png"))
    return None


def viewer(display, data, work, out):
    calc = find(os.path.join(data, "users", "local", PROJECT), "ecceqm-ethene")
    if not calc:
        return "no ethene calculation"
    scene = os.path.join(work, "viewer.scene")
    with open(scene, "w") as h:
        h.write("style Ball And Stick\nviewall\nbackground 1 1 1\n"
                "mo 8 0.04 50\nrotate 35\n")
    known = set(w for w, _ in display.windows())
    env = {"ECCE_VIEWER_SCENE": scene, "ECCE_VIEWER_SCENE_OUT": work,
           "ECCE_VIEWER_SCENE_SIZE": "800x600", "ECCE_VIEWER_SCENE_HOLD": "150",
           "ECCE_OPEN_PANEL": "MO Diagram", "ECCE_TRANSPARENCY_FALLBACK_MS": "0"}
    proc = readme.launch(display, "ecce-builder", ["-context", "file://" + calc],
                         env, log=os.path.join(work, "builder.log"))
    try:
        win = readme.waitWindow(display, known, 90, "ECCE Viewer")
        if not win:
            return "no Builder window"
        wid = win[0]
        readme.xdo(display, "windowmove", wid, "0", "0")
        time.sleep(40)
        readme.xdo(display, "windowsize", wid, "1300", "800")
        time.sleep(25)
        raw = os.path.join(work, "viewer-raw.png")
        import subprocess
        subprocess.run(["import", "-display", display.name, "-window", "root", raw],
                       check=True)
        readme.png(raw, os.path.join(out, "viewer.png"), "1300x800+0+0")
    finally:
        readme.stop(proc)
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(os.path.expanduser("~"), "readme-shots"))
    ap.add_argument("--only", choices=["organizer", "viewer"])
    opts = ap.parse_args()
    os.makedirs(opts.out, exist_ok=True)
    settings = isolate.apply(apps.INSTALL)
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    xdisplay.SCREEN = "1700x1100x24"
    work = tempfile.mkdtemp(prefix="readme-ex-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    errors = []
    with xdisplay.Display(number=176) as display:
        import subprocess
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            for name, fn in (("organizer", organizer), ("viewer", viewer)):
                if opts.only in (None, name):
                    err = fn(display, data, work, opts.out)
                    print(name, err or "ok")
                    if err:
                        errors.append(name)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    print("work dir kept:", work, "out:", os.path.abspath(opts.out))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
