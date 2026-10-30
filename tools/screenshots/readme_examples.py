#!/usr/bin/env python3
"""README screenshots from the shipped examples, headless, in local mode.

    ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<bin> \\
        tools/screenshots/readme_examples.py --out <dir>

Writes organizer.png (the Organizer on the finished ORCA Opt+Freq of
ethylene, Progress box in view) and viewer.png (the Builder with the HOMO of
ethene on a white background, MO Diagram docked, Properties showing).  The
examples are imported by the Organizer's own `examples organic` command;
nothing is clicked.  Needs only Xvfb; the viewer image is the whole display, to be cropped.
"""
import argparse
import os
import shutil
import sys
import tempfile
import threading
import subprocess
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
                         {"ECCE_TEST_ORGANIZER": cmds, "ECCE_NO_EXAMPLE_IMPORT": "",
                          "ECCE_NO_UPDATE_CHECK": "1"},
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
            h.write("size 1300 1060\nsummary file://%s\n" % calc)
        time.sleep(10)
        with open(cmds, "a") as h:
            h.write("snap %s\n" % shot)
        time.sleep(10)
    finally:
        readme.stop(proc)
    if not os.path.exists(shot):
        return "no snapshot"
    shutil.copy(shot, os.path.join(out, "organizer.png"))
    return None


def viewer(display, data, work, out):
    calc = find(os.path.join(data, "users", "local", PROJECT), "ethene")
    if not calc:
        return "no ethene calculation"
    scene = os.path.join(work, "viewer.scene")
    with open(scene, "w") as h:
        h.write("style Ball And Stick\nviewall\nbackground 1 1 1\n"
                "mo 8 0.04 50 opaque\nrotate 35\nrotatex -60\nwindowsize 1300 800\n"
                "hold 20\nscreen readme-viewer\nrotatex 30\nhold 5\nscreen readme-viewer-b\n")
    known = set(w for w, _ in display.windows())
    env = {"ECCE_VIEWER_SCENE": scene, "ECCE_VIEWER_SCENE_OUT": work,
           "ECCE_VIEWER_SCENE_SIZE": "800x600", "ECCE_VIEWER_SCENE_HOLD": "1",
           "ECCE_OPEN_PANEL": "MOs", "ECCE_NO_UPDATE_CHECK": "1", "ECCE_TRANSPARENCY_FALLBACK_MS": "0"}
    proc = readme.launch(display, "ecce-builder", ["-context", "file://" + calc],
                         env, log=os.path.join(work, "builder.log"))
    try:
        raw = os.path.join(work, "readme-viewer.png")
        for _ in range(200):
            if os.path.exists(raw) or proc.poll() is not None:
                break
            time.sleep(1)
        time.sleep(15)
        if not os.path.exists(raw):
            return "no viewer snapshot"
        for suffix, name in (("", "viewer.png"), ("-b", "viewer-b.png")):
            src = os.path.join(work, "readme-viewer%s.png" % suffix)
            if os.path.exists(src):
                shutil.copy(src, os.path.join(out, name))
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
