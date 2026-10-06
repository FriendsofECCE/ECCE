#!/usr/bin/env python3
"""Make the README screenshots (docs/images/) headlessly, from fixtures.

    ECCE_TEST_HOME=<prefix> ECCE_TEST_WRAPPERS=<bin> \\
    ECCE_TEST_STATE=<scratch state dir> ECCE_DATASERVER_PORT=8396 \\
    ECCE_BROKER_PORT=8388 \\
        tools/screenshots/readme.py --out docs/images [--only NAME ...]

Names: organizer, viewer, orbital-benzene, esp-benzene, vectors-water.
(mo-diagram-water.png comes from the MO diagram panel, not from here.)

Everything runs on a private Xvfb against an isolated data server (tests/apps
isolation), in the light theme.  Nothing is clicked or typed: the Organizer
opens on the calculation through ECCE_ORGANIZER_OPEN, the Builder draws the
scenes through ECCE_VIEWER_SCENE (tools/coin/scenes syntax, see
SceneScript) and keeps its window open with ECCE_VIEWER_SCENE_HOLD.  Windows
are only sized and placed (xdotool windowsize/windowmove), there is no window
manager on the display.

Data: the ORCA benzene run in tools/screenshots/data (RHF/def2-SVP, input
beside it) is imported through the Organizer's own Import Calculation hook;
water is the Gaussian 16 fixture tests/apps/fixtures/calc-water-vib.
Needs Xvfb, xdotool, ImageMagick (import, convert) and curl.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

import apps        # noqa: E402
import fixture     # noqa: E402
import isolate     # noqa: E402
import xdisplay    # noqa: E402

DATA = os.path.join(HERE, "data")
WATER = os.path.join(ROOT, "tests", "apps", "fixtures", "calc-water-vib")
PROJECT = "Examples"
NAMES = ["organizer", "viewer", "orbital-benzene", "esp-benzene",
         "vectors-water"]

#  (scene script, calculation, canvas size) for the viewer-only images.
SCENES = {
    "orbital-benzene": ("style Ball And Stick\nviewall\nmo 21 0.04 50\n"
                        "rotate 35\nsnap orbital-benzene\n",
                        "benzene", "680x450"),
    "esp-benzene": ("style Ball And Stick\nviewall\nesp 40\nrotate 30\n"
                    "snap esp-benzene\n", "benzene", "680x450"),
    "vectors-water": ("style Ball And Stick\nviewall\nnmvect 0\nviewall\n"
                      "snap vectors-water\n", "water-vib", "560x386"),
}


def xdo(display, *args):
    return subprocess.run(["xdotool"] + list(args), env=display.env(),
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT).stdout.decode().strip()


def user():
    return fixture.realUser()


def homeUrl():
    return "http://localhost:%d/Ecce/users/%s" % (
        fixture.dataserverPort(), user())


def dav(method, path, *extra, data=None):
    argv = ["curl", "-s", "-S", "-u", "%s:%s" % (user(), "ecce"),
            "-X", method] + list(extra)
    if data is not None:
        argv += ["--data", data]
    result = subprocess.run(argv + [homeUrl() + "/" + path],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return result.stdout.decode("utf-8", "replace")


def userRoot():
    return os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users", user())


def importBenzene(display):
    """File > Import Calculation from Output File, as the test hook does it."""
    out = os.path.join(DATA, "benzene.out")
    os.environ["ECCE_TEST_CALCIMPORT"] = out
    auth = fixture.authFile(os.path.join(fixture.stateHome(), ".ECCE",
                                         "auth.pipe"), user=user())
    result = apps.run(display, "organizer", args=("-pipe", auth),
                      windowTimeout=60, settle=60)
    del os.environ["ECCE_TEST_CALCIMPORT"]
    if "imported" not in (result.log or ""):
        raise RuntimeError("import failed: %s" % (result.log or "")[-500:])
    #  The importer's job store fills Props/ after the app has gone.
    props = os.path.join(userRoot(), "calcimport-test", "benzene", "Props")
    for _ in range(60):
        if os.path.exists(os.path.join(props, "ORBOCC")):
            time.sleep(3)
            return
        time.sleep(2)
    raise RuntimeError("the imported calculation never got its properties")


def arrange():
    """Project "Examples" holding benzene (ORCA) and water (Gaussian 16),
    both Completed -- an imported calculation is Loaded."""
    print(dav("MOVE", "calcimport-test",
              "-H", "Destination: %s/%s" % (homeUrl(), PROJECT)))
    water = os.path.join(userRoot(), PROJECT, "water-vib")
    shutil.rmtree(water, ignore_errors=True)
    shutil.copytree(WATER, water, symlinks=True)
    ns = "http://www.emsl.pnl.gov/ecce:"
    for calc in ("benzene",):
        body = ('<?xml version="1.0"?><propertyupdate xmlns="DAV:" '
                'xmlns:e="%s"><set><prop><e:state>Complete</e:state><e:launch_totalprocs>4</e:launch_totalprocs>'
                '</prop></set></propertyupdate>' % ns)
        print(dav("PROPPATCH", "%s/%s" % (PROJECT, calc), data=body))


def calcUrl(name):
    return "%s/%s/%s" % (homeUrl(), PROJECT, name)


def windowSize(display, wid):
    out = subprocess.run(["xwininfo", "-display", display.name, "-id", wid],
                         stdout=subprocess.PIPE).stdout.decode()
    w = h = 0
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Width:"):
            w = int(line.split()[1])
        elif line.startswith("Height:"):
            h = int(line.split()[1])
    return w, h


def newWindows(display, known):
    found = []
    for wid, title in display.windows():
        if wid in known or not title:
            continue
        w, h = windowSize(display, wid)
        if w > 60 and h > 60:
            found.append((wid, title, w, h))
    return found


def launch(display, wrapper, args, env=None, log=None):
    environment = display.env()
    environment.update(env or {})
    sink = open(log, "w") if log else subprocess.DEVNULL
    return subprocess.Popen([os.path.join(apps.WRAPPERS, wrapper)] + list(args),
                            env=environment, stdout=sink,
                            stderr=subprocess.STDOUT, start_new_session=True)


def stop(proc):
    for sig in (15, 9):
        try:
            os.killpg(proc.pid, sig)
            proc.wait(timeout=10)
            return
        except Exception:
            pass


def waitWindow(display, known, seconds=60, title=None):
    end = time.time() + seconds
    while time.time() < end:
        wins = [w for w in newWindows(display, known)
                if title is None or w[1] == title]
        if wins:
            return max(wins, key=lambda w: w[2] * w[3])
        time.sleep(1)
    return None


def png(src, dest, crop=None):
    argv = ["convert", src]
    if crop:
        argv += ["-crop", crop, "+repage"]
    argv += ["-strip", dest]
    subprocess.run(argv, check=True)
    if shutil.which("optipng"):
        subprocess.run(["optipng", "-quiet", "-o5", dest], check=False)


def shootOrganizer(display, auth, out):
    """The Organizer on benzene, at 1280x800.

    The code editor and the Launcher open from it, but with no window
    manager on the display they come up undecorated, overlapping and only
    half drawn, so they are not shown."""
    known = set(w for w, _ in display.windows())
    url = calcUrl("benzene")
    env = {"ECCE_ORGANIZER_OPEN": url}
    procs = [launch(display, "ecce-organizer", ["-pipe", auth], env)]
    try:
        org = waitWindow(display, known, 60, "ECCE Organizer")
        if not org:
            return "no Organizer window"
        wid = org[0]
        xdo(display, "windowmove", wid, "0", "0")
        xdo(display, "windowsize", wid, "1280", "800")
        time.sleep(25)
        shots = os.path.join(out, "organizer-full.png")
        subprocess.run(["import", "-display", display.name, "-window", "root",
                        shots])
        png(shots, os.path.join(out, "organizer.png"), "1280x800+0+0")
        os.unlink(shots)
    finally:
        for p in procs:
            stop(p)
    return None


def renderScene(display, auth, name, calc, script, size, outdir):
    for stale in ("FAILED", name + ".ppm"):
        try:
            os.unlink(os.path.join(outdir, stale))
        except OSError:
            pass
    context = calcUrl(calc)
    sceneFile = os.path.join(outdir, name + ".scene")
    with open(sceneFile, "w") as handle:
        handle.write(script)
    env = {"ECCE_VIEWER_SCENE": sceneFile, "ECCE_VIEWER_SCENE_OUT": outdir,
           "ECCE_VIEWER_SCENE_SIZE": size}
    proc = launch(display, "ecce-builder",
                  ["-pipe", auth, "-context", context], env,
                  log=os.path.join(outdir, name + ".log"))
    ppm = os.path.join(outdir, name + ".ppm")
    try:
        for tick in range(600):
            if tick == 90:      # evidence of what a slow run is waiting on
                subprocess.run(["import", "-display", display.name, "-window",
                                "root", os.path.join(outdir, name + "-wait.png")])
            if os.path.exists(ppm) or os.path.exists(
                    os.path.join(outdir, "FAILED")) or proc.poll() is not None:
                break
            time.sleep(1)
    finally:
        stop(proc)
    if not os.path.exists(ppm):
        return "no snapshot for %s" % name
    return None


def shootViewer(display, auth, out, tmp):
    """The Builder in viewer mode on benzene, with its MO and summary
    panels and an orbital drawn in the canvas."""
    script = "style Ball And Stick\nviewall\nmo 21 0.04 50\nrotate 35\n"
    sceneFile = os.path.join(tmp, "viewer.scene")
    with open(sceneFile, "w") as handle:
        handle.write(script)
    known = set(w for w, _ in display.windows())
    env = {"ECCE_VIEWER_SCENE": sceneFile, "ECCE_VIEWER_SCENE_OUT": tmp,
           "ECCE_VIEWER_SCENE_SIZE": "640x480",
           "ECCE_VIEWER_SCENE_HOLD": "120", "ECCE_OPEN_PANEL": "MOs"}
    proc = launch(display, "ecce-builder",
                  ["-pipe", auth, "-context", calcUrl("benzene")], env)
    try:
        win = waitWindow(display, known, 90, "ECCE Viewer")
        if not win:
            return "no Builder window"
        wid = win[0]
        xdo(display, "windowmove", wid, "0", "0")
        #  The Builder restores its own size once the calculation loads, so
        #  size it again after that, then give the canvas time to repaint.
        time.sleep(40)
        xdo(display, "windowsize", wid, "1400", "900")
        time.sleep(20)
        shots = os.path.join(tmp, "viewer-full.png")
        w, h = windowSize(display, wid)
        subprocess.run(["import", "-display", display.name, "-window", "root",
                        shots], check=True)
        png(shots, os.path.join(out, "viewer.png"), "%dx%d+0+0" % (w, h))
    finally:
        stop(proc)
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default=os.path.join(ROOT, "docs", "images"))
    parser.add_argument("--only", action="append", choices=NAMES)
    parser.add_argument("--display", type=int, default=175)
    parser.add_argument("--tmp", default=os.path.join(ROOT, "build-cmake",
                                                      "readme-shots"))
    options = parser.parse_args()
    want = options.only or NAMES
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
    xdisplay.SCREEN = "1700x1100x24"

    failed = 0
    restore = lambda: None
    with xdisplay.Display(number=options.display) as display:
        apps.startServices(display)
        try:
            fixture.ensureRealUserAccount()
            #  Without this the Builder opens on a modal "layout reset"
            #  notice for any calculation older than the build.
            restore = fixture.settleUpgradeNotices()
            if not os.path.isdir(os.path.join(userRoot(), PROJECT)):
                importBenzene(display)
                arrange()
            auth = fixture.authFile(os.path.join(options.tmp, "auth.pipe"),
                                    user=user())
            for name in want:
                if name == "organizer":
                    err = shootOrganizer(display, auth, options.tmp)
                    if not err:
                        shutil.move(os.path.join(options.tmp, "organizer.png"),
                                    os.path.join(options.out, "organizer.png"))
                elif name == "viewer":
                    err = shootViewer(display, auth, options.out, options.tmp)
                else:
                    script, calc, size = SCENES[name]
                    err = renderScene(display, auth, name, calc, script, size,
                                      options.tmp)
                    if not err:
                        png(os.path.join(options.tmp, name + ".ppm"),
                            os.path.join(options.out, name + ".png"))
                print("  %-16s %s" % (name, err or "ok"))
                failed += bool(err)
        finally:
            restore()
            os.environ.pop("ECCE_NO_REAP", None)
            apps.stopServices(display)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
