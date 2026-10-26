#!/usr/bin/env python3
"""Geometry Trace / Geometry Step Plots clicks on an MD run (#560).

    tests/apps/md_trace_click_test.py [--build build-cmake] [--png DIR] [--tag TAG]

The energy series of the NaCl-dissolving example's log (real parse scripts:
a plot over 20 ns) are stored with a 101-frame trace, as the example has.
A click far to the right of the plot must show the structure nearest in
time (not the last of the first 101 points), say so, and move the plot's
marker; a click in Geometry Step Plots must do the same; a selection must
survive Play; the temperature axis must carry its unit.
Exit status 77 (skip) without Xvfb, import, python3-pil/numpy or binaries.
"""
import argparse
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
sys.path.insert(0, os.path.join(REPO, "tests", "parsers"))

import builder_overlay_test as bo  # noqa: E402
import energies_groups_test as eg  # noqa: E402
import harness  # noqa: E402
import md_summary_test as ms  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
EX = os.path.join(REPO, "data", "client", "examples", "genchem",
                  "gases-liquids-solids", "gromacs-nacl-dissolving", "1-dynamics")
FRAMES = 101


def trace(atoms=3):
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
           '<tsvectable columnLabel="Atom" columnLabels="X Y Z" columns="3" '
           'name="GEOMTRACE" rowLabel="Geometry Step" rows="%d" units="Angstrom" '
           'vectorLabel="Coordinate" vectors="1">' % atoms]
    for f in range(FRAMES):
        vals = " ".join("%.3f" % (0.5 * a + 0.01 * f + 0.1 * k)
                        for a in range(atoms) for k in range(3))
        out.append('<step number="%d"> %s</step>' % (f + 1, vals))
    out.append("</tsvectable>\n")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--tag", default="after")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    ms.TSVEC.update(("TIMEVEC", "MDDENSVEC", "MDVOLVEC"))
    props = ms.parsedProps(os.path.join(EX, "prod.log"))
    npts = len(props["TIMEVEC"][1])
    check(npts > FRAMES, "the log has %d points, the trace %d frames" % (npts, FRAMES))
    work = tempfile.mkdtemp(prefix="mdclick-")
    shutil.copytree(os.path.join(HERE, "fixtures", "calc-water-opt"),
                    os.path.join(work, "calc-md"), symlinks=True)
    ms.writeProps(os.path.join(work, "calc-md"), props)
    with open(os.path.join(work, "calc-md", "Props", "GEOMTRACE"), "w") as f:
        f.write(trace())

    s = harness.Session(build, "mdclick", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        url = eg.install(s, "calc-md", work)
        scene = os.path.join(work, "scene")
        with open(scene, "w") as f:
            f.write("\n".join([
                "gtfocus", "gtselect 2", "gtselected",
                "gtseries TEMPVEC",
                "gtclick %d" % (npts - 1),
                "gtclick %d" % (npts // 2),
                "gtclick 30",
                "gtstepclick %d" % (npts - 1),
                "gtselected", "gtplay 1200 20", "gtstop", "gtselected",
                "plotshot %s-temperature 880 520 Geometry Trace" % args.tag,
                "hold 1", ""]))
        text, _ = bo.runScript(
            s, display, env, wrappers, ["-context", url], ["wait 25000", "quit"],
            png, "mdclick",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Geometry Step Plots",
             "ECCE_VIEWER_SCENE": scene,
             "ECCE_VIEWER_SCENE_OUT": os.path.join(s.state, "mdclick")})
        for line in re.findall(r"GTSTRESS: .*|BUILDER: message: .*", text):
            say(line)
        clicks = re.findall(r"click (\d+) -> structure (\d+) of (\d+), plot point (\d+)", text)
        check(len(clicks) == 4, "four clicks reported (%d)" % len(clicks))
        if len(clicks) == 4:
            last, mid, low, step = [tuple(map(int, c)) for c in clicks]
            check(last[1] == FRAMES, "a click on the last point shows the last structure")
            check(abs(mid[1] - (FRAMES + 1) / 2) <= 1, "the middle point shows the middle structure")
            check(low[1] == round(30 * (FRAMES - 1) / (npts - 1)) + 1, "an early point maps by time")
            check(step[1] == FRAMES, "a click in Geometry Step Plots shows the last structure")
            check(abs(last[3] - (npts - 1)) <= npts / (FRAMES - 1),
                  "the marker is at the nearest stored structure's point")
        check("no stored structure" in text.lower() or "no structure is stored" in text.lower(),
              "the nearest-structure message is shown")
        check(re.search(r"series TEMPVEC .*y='[^']*\bK\b", text) is not None,
              "the temperature axis carries K")
        sel = re.findall(r"selected=(\d+)", text)
        check(len(sel) >= 3 and all(v == "2" for v in sel),
              "the selection survives clicks and Play %s" % sel)
        for pic in os.listdir(os.path.join(s.state, "mdclick")):
            if pic.endswith(".png"):
                shutil.copy(os.path.join(s.state, "mdclick", pic), png)
        say("pictures: %s" % png)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
        shutil.rmtree(work, ignore_errors=True)
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
