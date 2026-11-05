#!/usr/bin/env python3
"""Photograph a list of shots headlessly: calculation, panel, selections, size.

    ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<install bin> \\
        tools/screenshots/shots.py LIST.json --out DIR [--only NAME ...] \\
        [--tmp DIR] [--display N]

LIST.json is {"calculations": {"water": "path/to/calculation-folder", ...},
"shots": [...]}.  Every calculation is copied into one local-data project,
nothing is run or sent anywhere.  A Builder shot is

    {"name": "water-charges", "calc": "water",
     "panel": "Mulliken Charges",        # opened as ECCE_OPEN_PANEL (optional)
     "size": [1500, 1000],               # Builder window, default 1400x900
     "script": ["assigncharges Mulliken Charges", "select 1"],
     "capture": "window",                # or "screen": floating panels too
     "crop": "720x560+0+0"}              # ImageMagick geometry (optional)

with "script" lines from the Builder scene script (src/wxviz/viewer/
SceneScript.C, Builder.C, BuilderShotHooks.C: select, style, measure,
assigncharges, gtstep, panelfloat, panelsize ...).  The 3-D background is
white; the window is photographed from outside once the script reaches its
"shot" line, so what is on the screen is what is painted.  An Energy
Calculator shot is

    {"name": "energy-calculator", "kind": "energycalc",
     "sideA": ["water"], "sideB": ["-2*other"]}    # [-][N*]calculation

done through the Organizer's ECCE_TEST_ORGANIZER "comparedialog" command.
Runs on its own Xvfb, in the isolated state of tests/apps/isolate.py; never
touches ~/.ECCE.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

import help_shots  # noqa: E402
import apps        # noqa: E402
import isolate     # noqa: E402
import xdisplay    # noqa: E402


class Calcs(object):
    """The local-data folder holding the calculations of the list."""

    def __init__(self, folder, sources):
        self.folder = folder
        self.sources = sources
        self.home = os.path.join(folder, "users", "local")

    def url(self, name):
        return "file://" + os.path.join(self.home, "shots", name) + "/"

    def make(self):
        shutil.rmtree(self.folder, ignore_errors=True)
        project = os.path.join(self.home, "shots")
        os.makedirs(project)
        shutil.copy(help_shots.PROJECT_META, os.path.join(project, ".ecce-meta"))
        for name, source in self.sources.items():
            shutil.copytree(os.path.join(ROOT, source) if not os.path.isabs(source)
                            else source, os.path.join(project, name),
                            symlinks=True)


def builderShot(display, tmp, out, calcs, shot):
    name = shot["name"]
    width, height = shot.get("size", [1400, 900])
    lines = ["windowsize %d %d" % (width, height), "background 1 1 1",
             "style Ball And Stick", "viewall"]
    lines += shot.get("script", [])
    lines += ["hold 2", "shot " + name]
    scene = os.path.join(tmp, name + ".scene")
    with open(scene, "w") as handle:
        handle.write("\n".join(lines) + "\n")
    ready, go = (os.path.join(tmp, name + ext) for ext in (".ready", ".go"))
    for path in (ready, go):
        if os.path.exists(path):
            os.unlink(path)
    environment = {"ECCE_TRANSPARENCY_FALLBACK_MS": "0",
                   "ECCE_VIEWER_SCENE": scene, "ECCE_VIEWER_SCENE_OUT": tmp,
                   "ECCE_VIEWER_SCENE_SIZE": "640x480",
                   "ECCE_VIEWER_SCENE_HOLD": "300"}
    if shot.get("panel"):
        environment["ECCE_OPEN_PANEL"] = shot["panel"]
    log = os.path.join(tmp, name + ".log")
    proc = help_shots.launch(display, "ecce-builder",
                             ["-context", calcs.url(shot["calc"])], environment,
                             log=log)
    try:
        for _ in range(240):
            if os.path.exists(ready) or proc.poll() is not None:
                break
            time.sleep(1)
        if not os.path.exists(ready):
            return "the script never reached its shot line (see %s)" % log
        time.sleep(2)
        full = os.path.join(tmp, name + "-full.png")
        #  The script sizes the window at 0,0 (no window manager), so the
        #  window is a crop of the screen; its own id is not needed.
        subprocess.run(["import", "-display", display.name, "-window", "root",
                        full], check=True)
        crop = shot.get("crop") or (None if shot.get("capture") == "screen"
                                    else "%dx%d+0+0" % (width, height))
        help_shots.png(full, os.path.join(out, name + ".png"), crop)
        open(go, "w").close()
        text = open(log, errors="replace").read()
        bad = [l for l in text.splitlines() if "ECCE_VIEWER_SCENE:" in l]
        if bad:
            return "script failed: " + bad[0]
    finally:
        help_shots.stop(proc)
    return None


def energyShot(display, tmp, out, calcs, shot):
    name = shot["name"]
    png = os.path.abspath(os.path.join(out, name + ".png"))
    if os.path.exists(png):
        os.unlink(png)

    def urls(names):
        parts = []
        for n in names:
            sign = "-" if n.startswith("-") else ""
            n = n.lstrip("-")
            count = ""
            if "*" in n:
                count, n = n.split("*", 1)
                count += "*"
            parts.append(sign + count + calcs.url(n).rstrip("/"))
        return ",".join(parts)

    commands = os.path.join(tmp, name + ".commands")
    with open(commands, "w") as handle:
        handle.write("comparedialog %s %s %s\n" % (
            urls(shot["sideA"]), urls(shot["sideB"]), png))
    log = os.path.join(tmp, name + ".log")
    proc = help_shots.launch(display, "ecce-organizer", [],
                             {"ECCE_TEST_ORGANIZER": commands,
                              "ECCE_TRANSPARENCY_FALLBACK_MS": "0"}, log=log)
    try:
        for _ in range(120):
            if os.path.exists(png) or proc.poll() is not None:
                break
            time.sleep(1)
        time.sleep(1)
    finally:
        help_shots.stop(proc)
    if not os.path.exists(png):
        return "the Energy Calculator wrote no picture (see %s)" % log
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("list")
    parser.add_argument("--out", required=True)
    parser.add_argument("--only", action="append")
    parser.add_argument("--display", type=int, default=177)
    parser.add_argument("--tmp", default=os.path.join(ROOT, "build-cmake",
                                                      "shots"))
    options = parser.parse_args()
    with open(options.list) as handle:
        spec = json.load(handle)
    shots = [s for s in spec["shots"]
             if not options.only or s["name"] in options.only]
    options.out = os.path.abspath(options.out)
    os.makedirs(options.out, exist_ok=True)
    os.makedirs(options.tmp, exist_ok=True)
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    os.environ["ECCE_REALUSER"] = "student"
    xdisplay.SCREEN = "1700x1100x24"
    calcs = Calcs(os.path.join(os.path.abspath(options.tmp), "data"),
                  spec["calculations"])
    os.environ["ECCE_LOCAL_DATA"] = calcs.folder
    failed = 0
    with xdisplay.Display(number=options.display) as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            calcs.make()
            #  The first start registers this machine; do it before any shot.
            apps.run(display, "organizer", windowTimeout=60, settle=20)
            for shot in shots:
                kind = shot.get("kind", "builder")
                err = (energyShot if kind == "energycalc" else builderShot)(
                    display, options.tmp, options.out, calcs, shot)
                print("  %-28s %s" % (shot["name"], err or "ok"))
                failed += bool(err)
        finally:
            os.environ.pop("ECCE_NO_REAP", None)
            apps.stopServices(display)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
