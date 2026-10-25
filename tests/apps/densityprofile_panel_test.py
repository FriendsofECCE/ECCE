#!/usr/bin/env python3
"""
The Density Profile panel on stored ECCE-QM Ne, Ar, H2 and N2, headless:
Builder scene commands (dp*, plotshot), no input events.  Checked per case:
the default view of an atom (shells about it) and of a molecule (a line
through the first bond), curve labels, the number of maxima and atom marks,
that the contributions sum to the total, the csv and png exports, and that
the docked pane keeps a plot of usable size on a 1366x768 and a 1700x1000
screen.

    ECCE_DP_SCREENS=1366x768x24,1700x1000x24   screens to try
    ECCE_DP_PNG=<dir>   also copy the pictures there
    ECCE_DP_REPORT=1    print the numbers but do not fail
    ECCE_DP_ONLY=ar,hcn only these cases

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
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
import xdisplay   # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

FIX = os.path.join(REPO, "tests", "builder", "radialdist-fixtures")
CALCS = {n: os.path.join(FIX, n) for n in ("ne", "ar", "h2", "n2", "hcn")}
CALCS["hcnmore"] = CALCS["hcn"]     # the folded controls, open, in a session of their own
SCREENS = os.environ.get("ECCE_DP_SCREENS", "1366x768x24,1700x1000x24").split(",")

#  Per calculation: what to set, then the pictured state last (a docked
#  pane left by one shot shows through the next one's picture, hence one
#  session each).
SCENES = {
    "ne": """dpopen
dpstate
dpcsv {t}-ne
dppng {t}-ne-plot 900 600
plotshot {t}-ne 0 0 Density Profile:docked
dpmode line
dpmode atom
dpquantity avg
dpquantity radial
dpunit bohr
dpunit A
hold 1
""",
    "ar": """dpopen
dpstate
plotshot {t}-ar 0 0 Density Profile:docked
dpxaxis sqrt
dpstate
dppng {t}-ar-sqrt 1000 650
dpxaxis log
dppng {t}-ar-logr 1000 650
dpyaxis log
dpstate
dppng {t}-ar-logr-logy 1000 650
dpyaxis linear
dpxaxis linear
dpshow selected
dpmo 10,11
dpstate
hold 1
""",
    "hcn": """dpopen
dpatoms 1 3
dpstate
dppng {t}-hcn-total 1000 650
dpzoom whole
dpstate
dpzoom fit
dpquantity psi2
dpstate
dpmo none
dpshow selected
dpstate
dpmo 6
dpquantity psi
dpstate
dppng {t}-hcn-pi-zero 1000 650
dpshift 0.5
dpstate
dppng {t}-hcn-pi-shifted 1000 650
dpshift 0
dpfollow 3
dpstate
plotshot {t}-hcn 0 0 Density Profile:docked
hold 1
""",
    "hcnmore": """dpopen
dpatoms 1 3
dpmore 1
dpstate
plotshot {t}-hcnmore 0 0 Density Profile:docked
dpmore 0
hold 1
""",
    "h2": """dpopen
dpstate
dpquantity psi2
dpshow selected
dpmo 1,2
dpstate
dpcsv {t}-h2
plotshot {t}-h2 0 0 Density Profile:docked
dpquantity psi
dpmargin 3
dpatoms 2 1
dpstate
hold 1
""",
    "n2": """dpopen
dpshow totalselected
dpmo 5,6,7
dpstate
dpcsv {t}-n2
dppng {t}-n2-plot 900 600
plotshot {t}-n2 0 0 Density Profile:docked
dpclip 0
dpcombine 0
dpquantity psi
dpshow selected
dpstate
hold 1
""",
}


def blocks(log, command):
    """The parsed DENSITYPROFILE blocks of one command, in order."""
    out = []
    lines = log.splitlines()
    for i, line in enumerate(lines):
        if line.strip() == "DENSITYPROFILE: " + command:
            b = {"curves": [], "marks": []}
            for item in lines[i + 1:]:
                if item.startswith("DENSITYPROFILE") or item.startswith("PLOTSHOT"):
                    break
                m = re.match(r"curve '(.*)' integral ([-\d.e+]+) maxima (\d+)", item)
                col = re.match(r"colour '(.*)' ([0-9a-f]{6})", item)
                if col:
                    b.setdefault("colours", []).append((col.group(1), col.group(2)))
                elif m:
                    b["curves"].append((m.group(1), int(m.group(3))))
                elif item.startswith("mark "):
                    b["marks"].append(item.split()[1])
                else:
                    k, _, v = item.partition(" ")
                    b["s_" + k] = v
            out.append(b)
    return out


def view(b):
    return [float(v) for v in b["s_view"].split()]


def checkHcn(first, sel):
    p = []
    if sorted(first["marks"]) != ["C1", "H2", "N3"]:
        p.append("marks %s, not C1 H2 N3" % first["marks"])
    cols = [c[1] for c in first.get("colours", [])]
    if len(cols) < 6 or len(cols) != len(set(cols)):
        p.append("legend colours %s" % cols)
    if len(sel) < 7:
        return p + ["only %d states" % len(sel)]
    if view(sel[1])[3] <= 1.5 * view(sel[0])[3]:
        p.append("Whole plot does not zoom out past the clipped peaks (%s then %s)" %
                 (sel[0]["s_view"], sel[1]["s_view"]))
    if sel[2].get("s_ticked") != sel[1].get("s_ticked") and sel[2].get("s_ticked") != sel[0].get("s_ticked"):
        p.append("quantity switch changed the ticked orbitals")
    if sel[3].get("s_ticked") != "7":
        p.append("Selected with nothing ticked is not the HOMO (%s)" % sel[3].get("s_ticked"))
    if sel[4].get("s_ticked") != "6" or sel[4].get("s_message") != "1":
        p.append("pi orbital on the axis: ticked %s, message %s" % (sel[4].get("s_ticked"), sel[4].get("s_message")))
    if sel[5].get("s_message") != "0" or not sel[5].get("s_shift", "").startswith("0.500"):
        p.append("shifted pi orbital: message %s shift %s" % (sel[5].get("s_message"), sel[5].get("s_shift")))
    if sel[6].get("s_ticked") != "3":
        p.append("following an MO: ticked %s" % sel[6].get("s_ticked"))
    return p


def check(case, log, t, out):
    problems = []
    states = blocks(log, "dpstate") + blocks(log, "dpopen")
    if not states:
        return ["no state in the log"]
    first = blocks(log, "dpopen")[0] if blocks(log, "dpopen") else states[0]
    names = [c[0] for c in first["curves"]]
    if case in ("ne", "ar"):
        want = ["1s", "2s", "2p"] + (["3s", "3p"] if case == "ar" else []) + ["Total density"]
        if first.get("s_mode") != "atom":
            problems.append("an atom does not open around the atom")
        if names != want:
            problems.append("curves %s, not %s" % (names, want))
        if float(first["s_sumerror"]) < 0 or float(first["s_sumerror"]) > 1e-9:
            problems.append("contributions do not sum to the total (%s)" % first["s_sumerror"])
        n2s = dict(first["curves"]).get("2s")
        if n2s != 2:
            problems.append("2s has %s maxima, not 2" % n2s)
    elif case in ("h2", "n2"):
        if first.get("s_mode") != "line" or len(first["marks"]) != 2:
            problems.append("a molecule opens on a line through two atoms (%s, marks %s)" %
                            (first.get("s_mode"), first["marks"]))
        if float(first["s_sumerror"]) < 0 or float(first["s_sumerror"]) > 1e-9:
            problems.append("contributions do not sum to the total (%s)" % first["s_sumerror"])
    sel = blocks(log, "dpstate")
    if case == "ar":
        if sel[1].get("s_xaxis") != "sqrt" or sel[2].get("s_yaxis") != "log":
            problems.append("axis choices not applied (%s, %s)" % (sel[1].get("s_xaxis"), sel[2].get("s_yaxis")))
        cols = [c[1] for c in first.get("colours", [])]
        if len(cols) != len(set(cols)):
            problems.append("a colour repeats in the legend: %s" % cols)
        total = dict(first.get("colours", [])).get("Total density")
        if total not in ("000000", "ffffff", "dcdcdc"):
            problems.append("the total is not black (%s)" % total)
        if sel[-1].get("s_ticked") != "10,11":
            problems.append("virtual selection %s" % sel[-1].get("s_ticked"))
        elif [c[0] for c in sel[-1]["curves"]][:1] and "(virtual)" not in sel[-1]["curves"][0][0]:
            problems.append("virtual orbital names %s" % sel[-1]["curves"])
    if case == "hcn":
        problems += checkHcn(first, sel)
    if case == "hcnmore" and sel[0].get("s_more") != "1":
        problems.append("More does not open")
    if case == "h2" and len(sel) > 1:
        if [c[0] for c in sel[1]["curves"]] != ["MO 1", "MO 2"]:
            problems.append("H2 selection %s" % sel[1]["curves"])
    if case == "n2" and sel:
        if [c[0] for c in sel[0]["curves"]] != ["MO 5", "MO 6-7", "Total density"]:
            problems.append("N2 selection %s" % sel[0]["curves"])
        if len(sel) > 1 and len(sel[1]["curves"]) != 3:
            problems.append("N2 psi view has %d curves" % len(sel[1]["curves"]))
    if case not in ("ar", "hcn", "hcnmore"):
        for ext in ("csv",):
            f = os.path.join(out, "%s-%s.%s" % (t, case, ext))
            if not os.path.exists(f):
                problems.append("no " + f)
            elif "Total density" not in open(f).read() and case != "h2":
                problems.append("csv lacks the total")
    png = os.path.join(out, "%s-%s.png" % (t, case))
    if not os.path.exists(png):
        problems.append("no docked picture")
    return problems


def main():
    pngdir = os.environ.get("ECCE_DP_PNG")
    report = bool(os.environ.get("ECCE_DP_REPORT"))

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "dp-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        bad = []
        try:
            t = "dp-%s" % xdisplay.SCREEN.split("x")[1]
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            only = os.environ.get("ECCE_DP_ONLY")
            for case, url in urls.items():
                if only and case not in only.split(","):
                    continue
                scene = os.path.join(folder, "scene")
                with open(scene, "w") as h:
                    h.write(SCENES[case].format(t=t))
                result = apps.run(display, "builder", args=("-context", url),
                                  windowTimeout=240, settle=300,
                                  env={"ECCE_PANEL_FULLSCREEN": "1",
                                       "ECCE_VIEWER_SCENE": scene,
                                       "ECCE_VIEWER_SCENE_OUT": out})
                log = result.log or ""
                problems = check(case, log, t, out)
                m = re.search(r"PLOTSHOT: %s-%s (\d+)x(\d+)" % (t, case), log)
                if not m:
                    problems.append("no docked pane size")
                elif int(m.group(1)) < 300 or int(m.group(2)) < 300:
                    problems.append("docked pane %sx%s" % m.groups())
                if case == "ne" and "DENSITYPROFILE: dpunit" not in log:
                    problems.append("no unit command ran")
                for line in log.splitlines():
                    if line.startswith("PLOTSHOT-PLOT") or (line.startswith("PLOTSHOT: ")
                                                            and "window" in line):
                        print("   " + line)
                for png in os.listdir(out):
                    if png.startswith(t) and pngdir:
                        os.makedirs(pngdir, exist_ok=True)
                        shutil.copy(os.path.join(out, png), pngdir)
                print("%s  %s %s%s" % ("ok  " if not problems else "FAIL", t, case,
                                       "" if not problems else ": " + "; ".join(problems)))
                bad += ["%s: %s" % (case, p) for p in problems]
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if bad and not report:
            results.fail("densityprofile", "; ".join(bad))

    run_tests.checkApp = checkApp
    for screen in SCREENS:
        xdisplay.SCREEN = screen
        argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
        sys.argv = argv[:3] + [a for a in argv[3:] if a not in ("--app", "builder")]
        rc = run_tests.main()
        if rc:
            return rc
    return 0


if __name__ == "__main__":
    sys.exit(main())
