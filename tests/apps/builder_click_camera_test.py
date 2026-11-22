#!/usr/bin/env python3
"""Builder: a click in the zoomed-in 3-D view selects or clears the selection
and leaves the camera where it is; import and View All still fit (#1260).

    tests/apps/builder_click_camera_test.py      own Xvfb and services

181L is imported and zoomed into until atoms lie outside the view; then the
commands a click on an atom and a click on empty space run in Select mode,
and Edit > Select All, must not move the camera.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                          # noqa: E402
import fixture                       # noqa: E402
import builder_import_pdb_test as ip  # noqa: E402

SCRIPT = [
    "wait 2000", "import " + ip.STRUCTURE, "wait 1000", "mode select",
    "inview", "zoom 0.2", "inview", "camerastate",
    "pickatom 100", "camerastate",
    "emptyclick", "camerastate",
    "menu Edit|Select All", "camerastate",
    "inview", "viewall", "inview", "camerastate", "quit",
]


def problemsIn(log):
    inview = re.findall(r"BUILDER: inview outside=(\d)", log)
    cams = re.findall(r"BUILDER: camerastate (.*)", log)
    if len(inview) != 4 or len(cams) != 5:
        return ["script did not finish: inview %s, camerastate %d lines"
                % (inview, len(cams))]
    found = []
    if inview[0] != "0":
        found.append("after the import an atom is outside the view")
    if inview[1] != "1":
        found.append("zoom 0.2 left every atom in view: nothing to test")
    for step, cam in zip(("atom click", "empty-space click", "Select All"),
                         cams[1:4]):
        if cam != cams[0]:
            found.append("%s moved the camera: %s -> %s"
                         % (step, cams[0], cam))
    if inview[2] != "1":
        found.append("after the clicks every atom is in view again")
    if inview[3] != "0" or cams[4] == cams[0]:
        found.append("View All did not fit the structure")
    return found


def main():
    import isolate
    import xdisplay

    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"

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
                out = ip.run(display, SCRIPT, "builder-click-camera")
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for line in out.splitlines():
        if re.search(r"BUILDER: (inview|camerastate)", line):
            print("      | " + line)
    problems = problemsIn(out)
    if problems:
        print("FAIL  builder click camera: %s" % "; ".join(problems))
        return 1
    print("PASS  builder click camera")
    return 0


if __name__ == "__main__":
    sys.exit(main())
