#!/usr/bin/env python3
"""
Photo hooks of the Builder scene script (#860), checked on their state and
not on the picture: measure leaves the label drawn and no atom selected,
assigncharges stores the Mulliken charges on the atoms and labels them,
gtstep moves the Geometry Trace plot marker with the frame, panelfloat and
panelsize size the pane.  Water optimisation fixture, local data.

Same installed tree and isolation as atom_numbers_test.py
(ECCE_TEST_HOME, ECCE_TEST_WRAPPERS).  Exit 77 (skip) without them.
"""

import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

CALCS = {"water": os.path.join(HERE, "fixtures", "calc-water-opt")}
SCENE = """windowsize 1500 1000
background 1 1 1
viewall
measure dist 0 1
measure angle 1 0 2
hold 1
assigncharges Mulliken Charges
gtstep 2
hold 1
gtframe 1
gtframe last
panelfloat MO Diagram 700 500
panelsize Geometry Trace 400 300
"""


def check(log):
    p = []
    got = re.findall(r"SHOTHOOK: measure (\w+) labels=(\d+) selected=(\d+)", log)
    if [g[0] for g in got] != ["dist", "angle"]:
        p.append("measure reports %s" % got)
    else:
        if int(got[1][1]) != 2:
            p.append("two measures should be drawn, %s are" % got[1][1])
        if any(int(g[2]) for g in got):
            p.append("atoms are still selected after measure")
    m = re.search(r"SHOTHOOK: assigncharges Mulliken Charges atoms=(\d+) first=(\S+) "
                  r"sum=(\S+) label1=\[([^\]]*)\]", log)
    if not m:
        p.append("no assigncharges line")
    else:
        if abs(float(m.group(2))) < 0.05:
            p.append("atom 1 got no charge (%s)" % m.group(2))
        if abs(float(m.group(3))) > 0.02:
            p.append("charges do not sum to zero (%s)" % m.group(3))
        if m.group(2)[:4].lstrip("-") not in m.group(4):
            p.append("label %s does not show charge %s" % (m.group(4), m.group(2)))
    steps = re.findall(r"GTSTRESS: step (\d+) -> playback (-?\d+), plot marker (-?\d+) of (\d+)", log)
    if [x[0] for x in steps] != ["2", "1", "4"]:
        p.append("gtstep/gtframe report %s" % steps)
    else:
        for x in steps:
            if x[1] != x[0] or x[2] != x[0]:
                p.append("frame %s: playback %s, plot marker %s" % x[:3])
    fl = re.search(r"SHOTHOOK: panelfloat MO Diagram floating=(\d) size=(\d+)x(\d+)", log)
    if not fl or fl.group(1) != "1" or abs(int(fl.group(2)) - 700) > 4:
        p.append("MO Diagram not floated at 700 wide: %s" % (fl.group(0) if fl else None))
    if not re.search(r"SHOTHOOK: panelsize Geometry Trace floating=\d size=(\d+)x", log):
        p.append("no panelsize line")
    if "FAIL" in log:
        p.append("a scene command failed")
    return p


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "shothooks-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        try:
            scene = os.path.join(folder, "scene")
            with open(scene, "w") as h:
                h.write(SCENE)
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            result = apps.run(display, "builder", args=("-context", urls["water"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_OPEN_PANEL": "Geometry Trace",
                                   "ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out,
                                   "ECCE_VIEWER_SCENE_HOLD": "5"})
            log = result.log or ""
            problems = check(log)
            if os.environ.get("SHOT_HOOKS_KEEP"):   # a place to look afterwards
                keep = os.environ["SHOT_HOOKS_KEEP"]
                os.makedirs(keep, exist_ok=True)
                with open(os.path.join(keep, "builder.log"), "w") as h:
                    h.write(log)
                for f in os.listdir(out):
                    shutil.copy(os.path.join(out, f), keep)
            print("%s  shot hooks%s" % ("ok  " if not problems else "FAIL",
                                        "" if not problems else ": " + "; ".join(problems)))
            if problems:
                print("\n".join(l for l in log.splitlines()
                                if "SHOTHOOK" in l or "GTSTRESS" in l or "FAIL" in l or "SCENE" in l or "scene" in l)[-2500:])
                results.fail("shot_hooks", "; ".join(problems))
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
