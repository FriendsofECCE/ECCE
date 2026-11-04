#!/usr/bin/env python3
"""Property switches preserve the Builder side pane width at laptop sizes.

Known failing until #843 is fixed. Use --png DIR for screenshots and logs.
Runs the installed Builder through the isolated app harness.
"""
import argparse
import os
import re
import sys
from types import SimpleNamespace

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps
import builder_overlay_test as bo
import fixture
import isolate
import pbc_ux_test
import xdisplay

PANELS = ("MOs", "Density Profile", "Vibrational Frequencies", "MOs", "Surfaces")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--png", required=True)
    parser.add_argument("--sizes", default="1366x768,1024x600")
    args = parser.parse_args()
    os.makedirs(args.png, exist_ok=True)
    if not apps.installed("builder"):
        print("SKIP no installed builder")
        return 77
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["HOME"] = settings["ECCE_REALUSERHOME"]
    session = SimpleNamespace(state=fixture.stateHome(), home=os.environ["ECCE_HOME"])
    failures = []
    for size in args.sizes.split(","):
        with xdisplay.Display(screen=size + "x24") as display:
            log = []
            apps.startServices(display, log)
            try:
                if not all(apps.serviceState(display).values()):
                    print("FAIL services: %s" % log)
                    return 1
                fixture.ensureRealUserAccount()
                restore = fixture.settleUpgradeNotices()
                try:
                    url, error = pbc_ux_test.install("property-width")
                    if error:
                        print("FAIL fixture: " + error)
                        return 1
                    auth = fixture.authFile(os.path.join(fixture.stateHome(),
                                                       ".ECCE", "auth.pipe"),
                                            user=fixture.USER)
                    lines = ["wait 3000", "list Density Profile", "wait 1000",
                             "columnwidth 300", "wait 1000",
                             "panesize Panel Tabs", "panesize Property Index", "shot width-set"]
                    for i, panel in enumerate(PANELS):
                        lines += ["list " + panel, "wait 1500", "panesize MOs"
                                  if panel == "Surfaces" else "panesize " + panel,
                                  "shot panel-%d" % i]
                    lines.append("quit")
                    text, shots = bo.runScript(
                        session, display, display.env(), apps.WRAPPERS,
                        ["-pipe", auth, "-context", url], lines, args.png, size,
                        {"ECCE_PANEL_MODE": "detail", "ECCE_PANEL_FULLSCREEN": "1"})
                    with open(os.path.join(args.png, size + ".log"), "w") as out:
                        out.write(text)
                    reports = re.findall(r"PANESIZE (.*?): (shown|hidden).*? "
                                         r"width=(\d+) client_width=(\d+) "
                                         r"virtual_width=(\d+)", text)
                    reports = [r for r in reports if r[0] != "Panel Tabs"]
                    widths = [int(r[2]) for r in reports]
                    found = []
                    if len(reports) != len(PANELS) + 1 or len(shots) != len(PANELS) + 1:
                        found.append("missing pane reports or screenshots")
                    if any(r[1] != "shown" for r in reports):
                        found.append("a selected pane is hidden")
                    if widths and abs(widths[0] - 300) > 2:
                        found.append("saved width was not restored: %d" % widths[0])
                    if widths and (min(widths) < 100 or max(widths) - min(widths) > 2):
                        found.append("width changed: %s" % widths)
                    mo = [r for r in reports[:-1] if r[0] == "MOs"]
                    if not mo or not all(int(r[4]) > int(r[3]) for r in mo):
                        found.append("wide MO content has no horizontal scroll area")
                    if any(int(r[3]) > int(r[2]) + 2 for r in reports):
                        found.append("a panel window exceeds its pane width")
                    if re.search(r"ECCE_BUILDER_SCRIPT: .*: (?!ok$).+", text, re.M):
                        found.append("script command failed")
                    if any(m in text for m in pbc_ux_test.MARKERS):
                        found.append("Builder crashed")
                    print("%s %s widths=%s%s" % (
                        "FAIL" if found else "PASS", size, widths,
                        "; " + "; ".join(found) if found else ""), flush=True)
                    failures += found
                    fixture.remove("property-width")
                finally:
                    restore()
            finally:
                apps.stopServices(display)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
