#!/usr/bin/env python3
"""Make the screenshots of the help (help/src/img) headlessly, in local data mode.

    ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<install bin> \\
        tools/screenshots/help_shots.py --out help/src/img [--only NAME ...] \\
        [--tmp DIR]

Names: organizer-first-start, organizer-project, builder-water, calced-water,
launcher-localhost, viewer-geometry-trace, viewer-imported.

Everything runs on a private Xvfb, in the light theme, against local data
(ECCE_LOCAL_DATA, no data server) holding the NWChem water optimisation of
tests/apps/fixtures/calc-water-opt, in the states the tutorial shows at each
step.  Nothing is clicked or typed: the Organizer opens on its target
through ECCE_ORGANIZER_OPEN, the Builder opens a panel through
ECCE_OPEN_PANEL, the Launcher is filled and photographed by its script hook.
Windows are only sized and placed with xdotool, there is no window manager.
Also checks that an import leaves the original output file unchanged
(--only import-unchanged).
"""

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

import apps        # noqa: E402
import isolate     # noqa: E402
import xdisplay    # noqa: E402

FIXTURE = os.path.join(ROOT, "tests", "apps", "fixtures", "calc-water-opt")
OUTPUT = os.path.join(FIXTURE, "Outputs", "ecce.out")
PROJECT_META = os.path.join(HERE, "data", "project.ecce-meta")
NAMES = ["organizer-first-start", "organizer-project", "builder-water",
         "calced-water", "launcher-localhost", "viewer-geometry-trace",
         "viewer-imported", "import-unchanged", "check-energies",
         "check-summary"]
RUNDIR = "/home/user/ecce-runs"


def xdo(display, *args):
    return subprocess.run(["xdotool"] + list(args), env=display.env(),
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT).stdout.decode().strip()


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


def waitWindow(display, known, seconds=90, title=None):
    end = time.time() + seconds
    while time.time() < end:
        found = []
        for wid, name in display.windows():
            if wid in known or not name:
                continue
            w, h = windowSize(display, wid)
            if w > 60 and h > 60 and (title is None or name.startswith(title)):
                found.append((wid, name, w, h))
        if found:
            return max(found, key=lambda f: f[2] * f[3])
        time.sleep(1)
    return None


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


def png(src, dest, crop=None):
    argv = ["convert", src]
    if crop:
        argv += ["-crop", crop, "+repage"]
    argv += ["-strip", dest]
    subprocess.run(argv, check=True)
    if shutil.which("optipng"):
        subprocess.run(["optipng", "-quiet", "-o5", dest], check=False)


# ---- the data the apps see -------------------------------------------------

class Data(object):
    """A fresh local-data folder, and the calculation in the state a step needs."""

    def __init__(self, folder):
        self.folder = folder
        self.home = os.path.join(folder, "users", "local")

    def url(self, *parts):
        return "file://" + os.path.join(self.home, *parts) + "/"

    def reset(self):
        shutil.rmtree(self.folder, ignore_errors=True)
        os.makedirs(self.home)

    def project(self, name="tutorial"):
        """The project: the folder with the metadata of the fixture's project."""
        path = os.path.join(self.home, name)
        os.makedirs(path)
        shutil.copy(PROJECT_META, os.path.join(path, ".ecce-meta"))
        return path

    def calc(self, state, props=True, setup=True, project="tutorial",
             name="water-opt", user=None):
        """Copy the fixture into the project as <name>, in run state `state`."""
        path = os.path.join(self.home, project, name)
        shutil.rmtree(path, ignore_errors=True)
        shutil.copytree(FIXTURE, path, symlinks=True)
        if not props:
            shutil.rmtree(os.path.join(path, "Props"), ignore_errors=True)
        if not setup:
            for sub in ("Inputs", "Outputs"):
                shutil.rmtree(os.path.join(path, sub), ignore_errors=True)
            for fname in ("SetupParams", "BasisSet.ecce_basisset"):
                try:
                    os.unlink(os.path.join(path, "Parameters", fname))
                except OSError:
                    pass
        for dirpath, _, files in os.walk(path):
            if ".ecce-meta" in files:
                meta = os.path.join(dirpath, ".ecce-meta")
                text = open(meta, encoding="utf-8").read()
                text = re.sub(r"(ecce:state\t[^\t\n]*\t)[^\t\n]*",
                              r"\g<1>" + state, text, count=1)
                if user is not None:
                    text = re.sub(r"(ecce:launch_user\t[^\t\n]*\t)[^\t\n]*",
                                  r"\g<1>" + user, text, count=1)
                open(meta, "w", encoding="utf-8").write(text)
        return path


# ---- shots -------------------------------------------------------------------

def shootWindow(display, tmp, out, name, wrapper, args, env, title=None,
                size=None, settle=25, crop=None, logname=None):
    known = set(w for w, _ in display.windows())
    proc = launch(display, wrapper, args, env,
                  log=os.path.join(tmp, (logname or name) + ".log"))
    try:
        win = waitWindow(display, known, 90, title)
        if not win:
            return "no window for %s; windows: %s" % (name, display.windows())
        wid = win[0]
        xdo(display, "windowmove", wid, "0", "0")
        time.sleep(min(settle, 40) / 2)
        if size:
            xdo(display, "windowsize", wid, str(size[0]), str(size[1]))
        time.sleep(settle / 2)
        full = os.path.join(tmp, name + "-full.png")
        subprocess.run(["import", "-display", display.name, "-window", wid,
                        full], check=True)
        png(full, os.path.join(out, name + ".png"), crop)
        print("  %-24s %s %s" % (name, win[1], windowSize(display, wid)))
    finally:
        stop(proc)
    return None


def shootBuilder(display, tmp, out, name, context, panel=None, size=(1400, 900),
                 env=None, dest=None, turn=0, extra=""):
    """The Builder on `context`, one panel open, the molecule fitted to the
    final window size.  The scene script waits (`hold`) for the resize and
    then fits the view; the marker snapshot says it has."""
    scene = os.path.join(tmp, name + ".scene")
    with open(scene, "w") as handle:
        handle.write("style Ball And Stick\nhold 25\nviewall\n%s%ssnap %s-ready\n"
                     "hold 120\n" % (extra + "\n" if extra else "",
                                     "rotatex %d\n" % turn if turn else "",
                                     name))
    marker = os.path.join(tmp, name + "-ready.ppm")
    if os.path.exists(marker):
        os.unlink(marker)
    environment = {"ECCE_TRANSPARENCY_FALLBACK_MS": "0",
                   "ECCE_VIEWER_SCENE": scene, "ECCE_VIEWER_SCENE_OUT": tmp,
                   "ECCE_VIEWER_SCENE_SIZE": "640x480",
                   "ECCE_VIEWER_SCENE_HOLD": "130"}
    if panel:
        environment["ECCE_OPEN_PANEL"] = panel
    environment.update(env or {})
    known = set(w for w, _ in display.windows())
    proc = launch(display, "ecce-builder", ["-context", context], environment,
                  log=os.path.join(tmp, name + ".log"))
    try:
        win = waitWindow(display, known, 90, "ECCE")
        if not win:
            return "no Builder window; windows: %s" % display.windows()
        wid = win[0]
        xdo(display, "windowmove", wid, "0", "0")
        time.sleep(20)
        xdo(display, "windowsize", wid, str(size[0]), str(size[1]))
        for _ in range(120):
            if os.path.exists(marker) or proc.poll() is not None:
                break
            time.sleep(1)
        time.sleep(6)
        if not os.path.exists(marker):
            return "the scene never reached its marker"
        full = os.path.join(tmp, name + "-full.png")
        subprocess.run(["import", "-display", display.name, "-window", wid,
                        full], check=True)
        png(full, os.path.join(dest or out, name + ".png"))
        print("  %-24s %s %s" % (name, win[1], windowSize(display, wid)))
    finally:
        stop(proc)
    return None


def shootLauncher(display, tmp, out, data):
    script = os.path.join(tmp, "launcher.script")
    shot = os.path.join(tmp, "launcher-localhost.png")
    with open(script, "w") as handle:
        handle.write("machine localhost\nrundir %s\nwait 2500\nshot %s\n"
                     "quit\n" % (RUNDIR, shot))
    if os.path.exists(shot):
        os.unlink(shot)
    proc = launch(display, "ecce-launcher",
                  ["-context", data.url("tutorial", "water-opt")],
                  {"ECCE_LAUNCHER_SCRIPT": script},
                  log=os.path.join(tmp, "launcher-localhost.log"))
    try:
        for _ in range(120):
            if os.path.exists(shot) or proc.poll() is not None:
                break
            time.sleep(1)
        time.sleep(2)
    finally:
        stop(proc)
    if not os.path.exists(shot):
        return "the Launcher wrote no snapshot"
    png(shot, os.path.join(out, "launcher-localhost.png"))
    return None


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as handle:
        h.update(handle.read())
    return h.hexdigest()


def importOutput(display, tmp, data):
    """File > Import Calculation from Output File, as the test hook does it."""
    os.environ["ECCE_TEST_CALCIMPORT"] = os.path.join(tmp, "water-opt.out")
    shutil.copy(OUTPUT, os.environ["ECCE_TEST_CALCIMPORT"])
    before = sha256(os.environ["ECCE_TEST_CALCIMPORT"])
    result = apps.run(display, "organizer", windowTimeout=60, settle=60)
    path = os.environ.pop("ECCE_TEST_CALCIMPORT")
    log = result.log or ""
    if "imported" not in log:
        raise RuntimeError("import failed: %s" % log[-800:])
    props = os.path.join(data.home, "calcimport-test", "water-opt", "Props")
    for _ in range(60):
        if os.path.exists(os.path.join(props, "GEOMTRACE")):
            time.sleep(3)
            break
        time.sleep(2)
    return before, sha256(path), log[-300:]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default=os.path.join(ROOT, "help", "src",
                                                      "img"))
    parser.add_argument("--only", action="append", choices=NAMES)
    parser.add_argument("--display", type=int, default=176)
    parser.add_argument("--tmp", default=os.path.join(ROOT, "build-cmake",
                                                      "help-shots"))
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
    os.environ["ECCE_REALUSER"] = "student"
    xdisplay.SCREEN = "1700x1100x24"
    #  A short path: the Builder lists the calculation's URL.
    data = Data("/tmp/ecce-help")
    os.environ["ECCE_LOCAL_DATA"] = data.folder
    env = {"ECCE_TRANSPARENCY_FALLBACK_MS": "0"}

    failed = 0
    with xdisplay.Display(number=options.display) as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            #  The first start registers this machine and says so in the
            #  Organizer's message pane, with the build host's name in it.
            data.reset()
            apps.run(display, "organizer", windowTimeout=60, settle=20)
            for name in want:
                err = None
                data.reset()
                if name == "organizer-first-start":
                    err = shootWindow(display, options.tmp, options.out, name,
                                      "ecce-organizer", [], env,
                                      title="ECCE Organizer", size=(1280, 800))
                elif name == "organizer-project":
                    data.project()
                    err = shootWindow(
                        display, options.tmp, options.out, name,
                        "ecce-organizer", [],
                        dict(env, ECCE_ORGANIZER_OPEN=data.url("tutorial")),
                        title="ECCE Organizer", size=(1280, 800))
                elif name == "builder-water":
                    data.project()
                    data.calc("Created", props=False, setup=False)
                    err = shootBuilder(display, options.tmp, options.out, name,
                                       data.url("tutorial", "water-opt"),
                                       size=(1500, 1090))
                elif name == "calced-water":
                    data.project()
                    data.calc("Ready", props=False)
                    err = shootWindow(
                        display, options.tmp, options.out, name,
                        "ecce-calced",
                        ["-context", data.url("tutorial", "water-opt")],
                        env, settle=30)
                elif name == "launcher-localhost":
                    data.project()
                    data.calc("Ready", props=False, user="")
                    err = shootLauncher(display, options.tmp, options.out, data)
                elif name in ("viewer-geometry-trace", "check-energies",
                              "check-summary"):
                    data.project()
                    data.calc("Complete")
                    panel = {"viewer-geometry-trace": "Geometry Trace",
                             "check-energies": "Energies",
                             "check-summary": "Calculation Summary"}[name]
                    #  The plot opens on the energy gradient; the energy is
                    #  the panel's own menu choice, set here as it saves it.
                    ini = os.path.join(settings["ECCE_REALUSERHOME"], ".ECCE",
                                       "wxbuilder.ini")
                    with open(ini, "w") as handle:
                        handle.write("[GeomTrace]\nProp=TEVEC\n")
                    try:
                        err = shootBuilder(
                            display, options.tmp, options.out, name,
                            data.url("tutorial", "water-opt"), panel,
                            dest=options.out if name == NAMES[5]
                            else options.tmp, turn=90)
                    finally:
                        os.unlink(ini)
                elif name in ("viewer-imported", "import-unchanged"):
                    before, after, tail = importOutput(display, options.tmp,
                                                       data)
                    print("  import: sha256 before %s after %s: %s"
                          % (before[:16], after[:16],
                             "unchanged" if before == after else "CHANGED"))
                    if name == "viewer-imported":
                        err = shootBuilder(
                            display, options.tmp, options.out, name,
                            data.url("calcimport-test", "water-opt"),
                            "Energies", turn=90)
                if err:
                    print("  %s: %s" % (name, err))
                    failed += 1
        finally:
            os.environ.pop("ECCE_NO_REAP", None)
            apps.stopServices(display)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
