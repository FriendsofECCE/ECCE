#!/usr/bin/env python3
"""Pictures of every plot panel in the real Builder, headless.

    tests/plots/capture.py --png DIR --tag before|after [case ...]

Needs the private install (ECCE_TEST_HOME / ECCE_TEST_WRAPPERS, as
tests/apps/run_tests.py).  For each case a calculation is built from the
real parser scripts over a real output file (makecalc.py), loaded in the
real Builder on a private Xvfb, and the Builder's scene command `plotshot`
floats each plot panel at two sizes and saves the pixels on screen.  No
synthetic input.

Writes DIR/<tag>-<case>-<panel>-<size>.png; sizes are `large` (880x520)
and `small` (500x320, what a docked plot gets at 1024x600).  Also with
--screen 1024x600 the Xvfb is that size.
"""
import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
sys.path.insert(0, HERE)

import makecalc                                               # noqa: E402

CASES = ("g16-h2o-optfreq", "orca-h2o-opt", "nwchem-h2o-opt", "orca-h2o-sym",
         "synthetic")
#  orca-h2o-sym is the case with orbital symmetries: the MO panel plots.
MO_ONLY = ("orca-h2o-sym",)
PANELS = ("Geometry Trace", "Geometry Step Plots", "Wave Step Plots",
          "Vibrational Frequencies")
MO_PANELS = ("MOs:plot", "MOs:plotsym")
#  Kinetics and metadynamics: no parser fixture has them, so makecalc.py
#  writes smooth made-up numbers in their layouts (a look test only).
SYNTHETIC_PANELS = ("Equilibrium Constant Plot",
                    "Forward Rate Constant Plot--TST",
                    "Forward Rate Constant Plot--TST and CVT",
                    "Forward Rate Constant Plot--TST/CVT Ratio",
                    "1D Metadynamics Potential", "Reaction Trace")
SIZES = (("large", 880, 520), ("small", 500, 320))


def slug(text):
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def sceneFor(case, panel, size):
    name, w, h = size
    return ("plotshot @TAG@-%s-%s-%s %d %d %s\nhold 1\n"
            % (case, slug(panel), name, w, h, panel))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--png", required=True)
    ap.add_argument("--tag", default="after")
    ap.add_argument("--screen", default="1280x1024")
    ap.add_argument("--panel", action="append")
    ap.add_argument("cases", nargs="*")
    args = ap.parse_args()

    import apps
    import fixture
    import isolate
    import xdisplay
    xdisplay.SCREEN = args.screen + "x24"
    os.makedirs(args.png, exist_ok=True)
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"
    cases = args.cases or CASES
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
                for case in cases:
                    work = os.path.join(state, "plots-" + case)
                    if os.path.exists(work):
                        shutil.rmtree(work)
                    os.makedirs(work)
                    name = "pl-" + case
                    if case == "synthetic":
                        makecalc.make_synthetic(os.path.join(work, name))
                    else:
                        makecalc.make(case, os.path.join(work, name))
                    url, error = fixture.install(
                        name, source_dir=os.path.join(work, name))
                    if error:
                        problems.append("%s: install: %s" % (case, error))
                        continue
                    cargs = ("-context", url)
                    if (os.path.realpath(state)
                            != os.path.realpath(os.path.expanduser("~"))):
                        auth = fixture.authFile(
                            os.path.join(state, ".ECCE", "auth.pipe"),
                            user=fixture.USER)
                        cargs = ("-pipe", auth) + cargs
                    #  One Builder per panel and size: a panel is made at the size
                    #  it is shown at, as when docked, and one window never
                    #  sits over the next.
                    panels = args.panel or (
                        MO_PANELS if case in MO_ONLY else
                        SYNTHETIC_PANELS if case == "synthetic" else PANELS)
                    for panel, size in [(p, s) for p in panels for s in SIZES]:
                        scene = os.path.join(work, "scene")
                        with open(scene, "w") as h:
                            h.write(sceneFor(case, panel, size).replace(
                                "@TAG@", args.tag))
                        result = apps.run(display, "builder", args=cargs,
                                          windowTimeout=120, settle=300,
                                          env={"ECCE_VIEWER_SCENE": scene,
                                               "ECCE_VIEWER_SCENE_OUT": work,
                                               "ECCE_REALUSER": fixture.USER})
                        for line in (result.log or "").splitlines():
                            if line.startswith("PLOTSHOT"):
                                print("  " + line)
                    fixture.remove(name)
                    shots = [f for f in os.listdir(work) if f.endswith(".png")]
                    for f in shots:
                        shutil.copy(os.path.join(work, f), args.png)
                    failed = os.path.join(work, "FAILED")
                    if os.path.exists(failed):
                        problems.append("%s: %s" % (case, open(failed).read()))
                    print("%s: %d pictures" % (case, len(shots)))
                    if not args.panel and len(shots) < len(panels) * len(SIZES):
                        tail = (result.log or "").splitlines()[-12:]
                        problems.append("%s: only %d of %d pictures\n  | %s"
                                        % (case, len(shots),
                                           len(panels) * len(SIZES),
                                           "\n  | ".join(tail)))
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for p in problems:
        print("FAIL  " + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
