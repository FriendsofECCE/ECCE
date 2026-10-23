#!/usr/bin/env python3
"""The Density Map panel in the real Builder, headless (#215): drives every
control through the dm* scene commands, saves the 3-D viewer with the plane
drawn in it (white background), the map the panel draws, and the CSV.

    ECCE_TEST_HOME=<DESTDIR>/usr/local \\
        tests/builder/densitymap_viewer_capture.py --out DIR [case ...]

Cases use the tests/modiagram fixtures through the data server, as
tests/modiagram/capture.py does.  Needs a private install (tests/apps).
"""
import argparse
import importlib.util
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

FIXTURES = os.path.join(ROOT, "tests", "modiagram", "fixtures")

#  case -> (fixture, scene).  Every line is a scene command; `hold` waits.
CASES = {
    "benzene-above-ring": ("orca-benzene", """
background 1 1 1
dmoffset 0.8
rotatex -50
viewall
hold 1
snap viewer-benzene-plane
densitymap map-benzene-0.8A 900 700 0 alpha 0
dmcsv benzene-0.8A
dmstate
"""),
    "ethene-pi": ("ecceqm-c2h4", """
background 1 1 1
dmpick 1 2
hold 1
dmstate
rotatex 40
viewall
snap viewer-ethene-plane
densitymap map-ethene-pi 900 700 8 alpha 2 1 2
dmstate
dmpick 1 2 3
dmoffset 0.5 drag
dmoffset 0.5
dmstate
dmviewcurrent
dmstate
dmclose
dmstate
"""),
}


def loadModiagramCapture():
    spec = importlib.util.spec_from_file_location(
        "modiagram_capture", os.path.join(ROOT, "tests", "modiagram", "capture.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True)
    ap.add_argument("cases", nargs="*")
    args = ap.parse_args()

    import apps
    import fixture
    import isolate
    import xdisplay
    moc = loadModiagramCapture()
    os.makedirs(args.out, exist_ok=True)
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"
    problems = []
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % log)
                return 1
            fixture.ensureRealUserAccount()
            restore = fixture.settleUpgradeNotices()
            try:
                for case in args.cases or CASES:
                    fixtureName, scene = CASES[case]
                    source = os.path.join(FIXTURES, fixtureName)
                    work = os.path.join(state, "dm-" + case)
                    shutil.rmtree(work, ignore_errors=True)
                    os.makedirs(work)
                    name = "dm-" + case
                    url, error = fixture.install(name, source_dir=source)
                    if error:
                        problems.append("%s: install: %s" % (case, error))
                        continue
                    marker = os.path.join(source, ".davprops")
                    if os.path.exists(marker):
                        import json
                        moc.setProperties(url, json.load(open(marker)))
                    cargs = ("-context", url)
                    if (os.path.realpath(state)
                            != os.path.realpath(os.path.expanduser("~"))):
                        auth = fixture.authFile(
                            os.path.join(state, ".ECCE", "auth.pipe"),
                            user=fixture.USER)
                        cargs = ("-pipe", auth) + cargs
                    path = os.path.join(work, "scene")
                    with open(path, "w") as h:
                        h.write(scene.strip() + "\n")
                    result = apps.run(display, "builder", args=cargs,
                                      windowTimeout=120, settle=300,
                                      env={"ECCE_VIEWER_SCENE": path,
                                           "ECCE_VIEWER_SCENE_OUT": work,
                                           "ECCE_OPEN_PANEL": "Density Map",
                                           "ECCE_DEBUG_PANELS": "1",
                                           "ECCE_REALUSER": fixture.USER})
                    for line in (result.log or "").splitlines():
                        if line.startswith(("DENSITYMAP", "have ", "title ",
                                            "plane", "offset ", "range ",
                                            "grid ", "shown ")):
                            print("  " + line)
                    fixture.remove(name)
                    for f in sorted(os.listdir(work)):
                        src = os.path.join(work, f)
                        if f.endswith(".ppm"):
                            subprocess.run(["convert", src, os.path.join(
                                args.out, f[:-4] + ".png")], check=True)
                        elif f.endswith((".png", ".csv")):
                            shutil.copy(src, args.out)
                    failed = os.path.join(work, "FAILED")
                    if os.path.exists(failed):
                        print("\n".join((result.log or "").splitlines()[-30:]))
                        problems.append("%s: %s" % (case, open(failed).read()))
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for p in problems:
        print("FAIL  " + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
