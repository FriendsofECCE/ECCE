#!/usr/bin/env python3
"""The Organizer next to a second window, active and in :backdrop (#210).

    tests/look/organizer_backdrop.py OUTDIR      (an installed tree: ECCE_TEST_HOME,
                                                  ECCE_TEST_WRAPPERS, as tests/apps)

Starts the real Organizer and the periodic table on a private Xvfb with an
isolated data server (tools/screenshots/capture.py's setup), and writes
three screenshots: Organizer active (focused.png), Organizer in :backdrop
with the theme's own fading (backdrop-theme.png, ECCE_BACKDROP=theme) and
with the fix (backdrop-fixed.png).  A headless display has no window
manager, so ECCE_TEST_BACKDROP=1 holds the Organizer in :backdrop.
Screenshots are PNG when ImageMagick's `import` exists, else .xwd.
"""
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "screenshots"))
import capture  # noqa: E402
from capture import apps, fixture, isolate, xdisplay, xdo  # noqa: E402


def launch(display, wrapper, args, extra):
    env = dict(display.env(), **extra)
    return subprocess.Popen([os.path.join(apps.WRAPPERS, wrapper)] + args,
                            env=env, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL, start_new_session=True)


def grab(display, path):
    if subprocess.run(["import", "-display", display.name, "-window", "root",
                       path], stderr=subprocess.DEVNULL).returncode == 0:
        return path
    path = path[:-4] + ".xwd"
    with open(path, "wb") as f:
        subprocess.run(["xwd", "-root", "-silent", "-display", display.name],
                       stdout=f)
    return path


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    settings = isolate.apply(apps.INSTALL)
    print(isolate.describe(settings))
    with xdisplay.Display() as display:
        apps.startServices(display)
        fixture.ensureRealUserAccount()
        fixture.install()
        auth = capture.realUserAuth(os.path.join(out, ".auth"))
        for name, extra in (("focused", {}),
                            ("backdrop-theme", {"ECCE_TEST_BACKDROP": "1",
                                                "ECCE_BACKDROP": "theme"}),
                            ("backdrop-fixed", {"ECCE_TEST_BACKDROP": "1"})):
            org = launch(display, "ecce-organizer", ["-pipe", auth], extra)
            time.sleep(12)
            capture.dismissAuth(display)
            time.sleep(10)
            wid = capture.windowId(display, "ECCE Organizer")
            if wid:
                xdo(display, "windowmove", wid, "0", "0")
                xdo(display, "windowsize", wid, "1000", "700")
            other = launch(display, "ecce-pertable", [], {})
            time.sleep(8)
            pid = capture.windowId(display, "Periodic Table") or \
                capture.windowId(display, "ECCE Periodic Table")
            if pid:
                xdo(display, "windowmove", pid, "1020", "0")
                xdo(display, "windowfocus", pid)
            time.sleep(3)
            print(name, grab(display, os.path.join(out, name + ".png")),
                  "organizer" if wid else "NO ORGANIZER",
                  "pertable" if pid else "no pertable")
            for p in (other, org):
                p.terminate()
            time.sleep(3)
        os.unlink(auth)
        apps.stopServices(display)
    return 0


if __name__ == "__main__":
    sys.exit(main())
