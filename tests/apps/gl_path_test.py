#!/usr/bin/env python3
"""The viewer's software/hardware OpenGL path (#566), headless.

Opens a calculation in the Builder under Xvfb (Mesa llvmpipe, so a software
renderer) with a distance measurement and checks, from the program's error
output and the canvas, that

  * the "[GL] ..." line reports the renderer and path "software (detected)";
  * ECCE_GL_PATH=hardware switches it to "hardware (ECCE_GL_PATH)";
  * the measurement label still draws on both paths: the canvas gets more
    dark pixels once the label is there (white background, black label).

Needs a private install (ECCE_TEST_HOME, ECCE_TEST_WRAPPERS).  Exit 77
(skip) without one or without Xvfb.
"""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

SCENE = """
background 1 1 1
viewall
hold 1
snap before
measure dist 0 1
viewall
hold 1
snap after
"""


def darkPixels(path):
    """Pixels darker than mid grey in a binary PPM."""
    data = open(path, "rb").read()
    m = re.match(rb"P6\s+(\d+)\s+(\d+)\s+(\d+)\s", data)
    w, h = int(m.group(1)), int(m.group(2))
    px = data[m.end():]
    return sum(1 for i in range(0, w * h * 3, 3)
               if px[i] < 100 and px[i + 1] < 100 and px[i + 2] < 100)


def main():
    if "ECCE_TEST_WRAPPERS" not in os.environ:
        print("SKIP: set ECCE_TEST_HOME and ECCE_TEST_WRAPPERS to a private install")
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    import apps
    import fixture
    import isolate
    import xdisplay

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT", "ECCE_GL_PATH"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"
    source = os.path.join(ROOT, "tests", "modiagram", "fixtures", "ecceqm-c2h6")
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

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
                for label, extra, want in (
                        ("default", {}, "software (detected)"),
                        ("hardware", {"ECCE_GL_PATH": "hardware"},
                         "hardware (ECCE_GL_PATH)")):
                    work = os.path.join(state, "glpath-" + label)
                    shutil.rmtree(work, ignore_errors=True)
                    os.makedirs(work)
                    name = "glpath-" + label
                    url, error = fixture.install(name, source_dir=source)
                    if error:
                        failures.append("install: " + error)
                        continue
                    cargs = ("-context", url)
                    if (os.path.realpath(state)
                            != os.path.realpath(os.path.expanduser("~"))):
                        auth = fixture.authFile(
                            os.path.join(state, ".ECCE", "auth.pipe"),
                            user=fixture.USER)
                        cargs = ("-pipe", auth) + cargs
                    path = os.path.join(work, "scene")
                    with open(path, "w") as handle:
                        handle.write(SCENE.strip() + "\n")
                    env = {"ECCE_VIEWER_SCENE": path,
                           "ECCE_VIEWER_SCENE_OUT": work,
                           "ECCE_REALUSER": fixture.USER}
                    env.update(extra)
                    result = apps.run(display, "builder", args=cargs,
                                      windowTimeout=120, settle=300, env=env)
                    text = result.log or ""
                    line = [l for l in text.splitlines() if l.startswith("[GL] ")]
                    check(len(line) == 1 and line[0].endswith("drawing path " + want),
                          "%s: %s" % (label, line[0] if line else "no [GL] line"))
                    if line and label == "default":
                        check("llvmpipe" in line[0].lower() or "swrast" in line[0].lower()
                              or "softpipe" in line[0].lower(),
                              "renderer named in the line")
                    fixture.remove(name)
                    try:
                        before = darkPixels(os.path.join(work, "before.ppm"))
                        after = darkPixels(os.path.join(work, "after.ppm"))
                    except (IOError, AttributeError):
                        failures.append(label + ": no snapshots")
                        print(text[-1500:])
                        continue
                    check(after > before + 20,
                          "%s: measurement label draws (%d -> %d dark pixels)"
                          % (label, before, after))
            finally:
                restore()
        finally:
            apps.stopServices(display)
    print("FAILED" if failures else "ok")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
