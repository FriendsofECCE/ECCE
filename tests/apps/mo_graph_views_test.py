#!/usr/bin/env python3
"""The MO panel's Table / Graph / Graph by Symmetries views (#882).

Water (ORCA, cc-pVDZ) in the Builder's MOs panel, switched Table -> Graph ->
Graph by Symmetries -> Table as the panel's menu does.  Each view must show
its own control, with a real size, and each graph must hold the orbital
levels.  A PNG of each view is kept with --shots DIR.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import argparse
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402
import orca_mo_test as base  # noqa: E402

VIEWS = ("table", "plot", "plotsym", "table2")
SKIP = 77


def state(log, name):
    block = log.split("PLOTSHOT-MOTABLE %s\n" % name, 1)
    if len(block) < 2:
        return None
    text = block[1].split("PLOTSHOT:", 1)[0]
    out = {}
    for key in ("graphReg", "graphSym"):
        m = re.search(r"^%s shown (\d) size (\d+)x(\d+) at -?\d+,-?\d+ curves (\d+) levels "
                      r"(\d+)" % key, text, re.M)
        out[key] = tuple(int(g) for g in m.groups()) if m else None
    for line in re.findall(r"^graph\w+ .*$", text, re.M):
        print("      %s: %s" % (name, line))
    m = re.search(r"^tableShown (\d)", text, re.M)
    out["table"] = int(m.group(1)) if m else None
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shots", help="save a PNG of each view here")
    options = parser.parse_args()
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="mo-graph-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    failures = []
    with xdisplay.Display() as display:
        import subprocess
        subprocess.run([os.path.join(apps.INSTALL, "bin",
                                     "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            url = base.makeCalc(data, "water-views")
            out = os.path.join(work, "out")
            #  One Builder, the views switched in turn (the saved choice
            #  starts as the table).
            scene = "".join(
                "plotshot %s 0 0 MOs:docked%s\n" % (v, v.rstrip("2"))
                for v in VIEWS) + "hold 1\n"
            log = base.runBuilder(display, url, out, scene).log or ""
            if os.environ.get("MO_GRAPH_LOG"):
                open(os.environ["MO_GRAPH_LOG"], "w").write(log)
            for view in VIEWS:
                st = state(log, view)
                png = os.path.join(out, view + ".png")
                if st is None or None in (st["graphReg"], st["graphSym"]):
                    failures.append(view)
                    print("FAIL  %s: no state reported\n%s" %
                          (view, "\n".join(log.splitlines()[-30:])))
                    continue
                want = {"table": "table", "table2": "table",
                        "plot": "graphReg", "plotsym": "graphSym"}[view]
                why = []
                for key in ("graphReg", "graphSym"):
                    shown, w, h, curves, levels = st[key]
                    if key == want:
                        if not shown:
                            why.append("%s not shown" % key)
                        if w < 100 or h < 100:
                            why.append("%s is %dx%d" % (key, w, h))
                        if levels <= 0:
                            why.append("%s holds no levels" % key)
                    elif shown:
                        why.append("%s shown too" % key)
                    print("      %s %s: shown %d size %dx%d curves %d "
                          "levels %d" % (view, key, shown, w, h, curves,
                                         levels))
                if (st["table"] == 1) != (want == "table"):
                    why.append("table shown = %s" % st["table"])
                if not os.path.exists(png):
                    why.append("no picture")
                elif options.shots:
                    os.makedirs(options.shots, exist_ok=True)
                    shutil.copy(png, os.path.join(options.shots,
                                                  "mo-%s.png" % view))
                if why:
                    failures.append(view)
                    print("FAIL  %s: %s" % (view, "; ".join(why)))
                else:
                    print("ok    %s" % view)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
