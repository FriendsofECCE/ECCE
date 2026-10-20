#!/usr/bin/env python3
"""Vibrational Frequencies panel: Export .csv (#392), headless, with the
tree's own binaries.

    tests/apps/spectrum_csv_test.py [--build build-cmake] [--png DIR]

calc-water-vib (frequencies, IR and Raman) is opened in the Builder, which
writes the spectrum through the script command "spectrumcsv".  The file is
read back and compared with the numbers in the calculation's own property
files: the sticks, the header, and a curve whose peak sits at the strongest
stick.  Exit status 77 (skip) without Xvfb, import or binaries.
"""

import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
FIXTURE = os.path.join(HERE, "fixtures", "calc-water-vib")
RAMAN = ("9.2633", "47.7911", "21.5432")
NUMBER = re.compile(r"^[-+]?\d+\.?\d*(e[-+]?\d+)?$", re.I)


def prop(name):
    """The numbers of a vector property file."""
    with open(os.path.join(FIXTURE, "Props", name)) as f:
        body = re.search(r"<vector[^>]*>(.*?)</vector>", f.read(), re.S).group(1)
    return [float(w) for w in body.split()]


def readCsv(path):
    header, sticks, curve, part = [], [], [], "head"
    for line in open(path, encoding="utf-8").read().splitlines():
        if not line:
            part = "gap"
        elif line.startswith("#"):
            header.append(line)
        elif line.startswith("mode,"):
            part = "sticks"
        elif line.startswith("wavenumber_cm-1"):
            part = "curve"
        elif part == "sticks":
            sticks.append(line.split(","))
        elif part == "curve":
            curve.append([float(x) for x in line.split(",")])
    return header, sticks, curve


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
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

    s = harness.Session(build, "spectrumcsv", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binaries: %s" % os.path.realpath(os.path.join(s.home, "bin", "builder")))

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "calc-water-vib")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(FIXTURE, calc, symlinks=True)
        #  calc-water-vib has no .ecce-meta of its own; this one is a
        #  Gaussian-16 water calculation like it.
        shutil.copy(os.path.join(HERE, "fixtures", "calc-water-freq-g16",
                                 ".ecce-meta"), calc)

        #  The fixture's Raman activities are all zero, which the panel
        #  treats as no Raman; give the copy real ones.
        ramanProp = os.path.join(calc, "Props", "VIBRAM")
        with open(ramanProp) as f:
            text = f.read()
        with open(ramanProp, "w") as f:
            f.write(re.sub(r">[^<]*</vector>", ">%s</vector>" % "   ".join(RAMAN),
                           text))

        ir = os.path.join(s.state, "spectrum-ir.csv")
        raman = os.path.join(s.state, "spectrum-raman.csv")
        lines = ["wait 6000", "shot panel", "spectrumcsv %s ir" % ir,
                 "spectrumcsv %s raman" % raman, "quit"]
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            lines, png, "spectrumcsv",
            {"ECCE_PANEL_MODE": "detail",
             "ECCE_OPEN_PANEL": "Vibrational Frequencies"})
        check(text.count("spectrumcsv ok") == 2, "the Builder wrote both files")
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        if not (os.path.exists(ir) and os.path.exists(raman)):
            say(text[-2000:])
            check(False, "both files exist")
            return 1

        freq = [f for f in prop("VIBFREQ") if abs(f) >= 10]
        for path, kind, col in ((ir, "IR", 2), (raman, "Raman", 3)):
            header, sticks, curve = readCsv(path)
            say("%s: %d header lines, %d sticks, %d curve points"
                % (kind, len(header), len(sticks), len(curve)))
            check("# spectrum: " + kind in header, kind + ": spectrum named")
            check(any(h.startswith("# calculation: ") for h in header),
                  kind + ": calculation named")
            check(any(h.startswith("# broadening: ") for h in header),
                  kind + ": broadening named")
            check(len(sticks) == len(freq), kind + ": one row per vibration")
            got = [float(r[1]) for r in sticks]
            check(all(abs(a - b) < 1e-3 for a, b in zip(got, freq)),
                  kind + ": frequencies equal VIBFREQ (scale 1)")
            want = prop("VIBIR") if kind == "IR" else [float(x) for x in RAMAN]
            check(all(NUMBER.match(r[col]) for r in sticks) and
                  all(abs(float(r[col]) - w) < 1e-3 for r, w in zip(sticks, want)),
                  kind + ": intensities equal the property's")
            check(len(curve) > 1000, kind + ": curve has a fine grid")
            peak = max(curve, key=lambda p: p[1])
            top = max(sticks, key=lambda r: float(r[col]))
            check(abs(peak[0] - float(top[1])) < 15,
                  kind + ": the curve peaks at the strongest stick "
                  "(%g vs %s)" % (peak[0], top[1]))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
