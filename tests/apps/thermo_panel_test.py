#!/usr/bin/env python3
"""Thermochemistry panel (#692), headless, with the tree's binaries.

    tests/apps/thermo_panel_test.py [--build build-cmake] [--png DIR]

A frequency calculation (stored ORCA-labelled results: the water thermochemistry
scalars of the Gaussian fixture, plus constructed low frequencies so that the
treatments differ) is opened in the Builder.  The script command "thermo"
chooses a treatment, temperature and standard state in the panel and prints
what it shows; for each treatment, and for 1 M at another temperature, the
numbers must equal `ecce-thermo --json` run on the same calculation.  The
code's own G stays as stored.  Exit status 77 (skip) without Xvfb, import or
binaries.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

def check(ok, what, detail=""):
    if not bo.check(ok, what) and detail:
        say("      " + detail)
    return ok
FIX = os.path.join(HERE, "fixtures")
TREATMENTS = ["rrho", "qrrho-grimme:50", "qrrho-grimme:100", "qrrho-grimme:200",
              "truhlar", "head-gordon"]
NS = "http://www.emsl.pnl.gov/ecce:"


def makeThermoCalc(project, name, freqs, code="ORCA"):
    """A water frequency calculation whose stored frequencies are `freqs`."""
    calc = os.path.join(project, name)
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(os.path.join(FIX, "calc-water-freq-g16"), calc, symlinks=True)
    shutil.copy(os.path.join(FIX, "calc-water-vib", "Props", "VIBFREQ"),
                os.path.join(calc, "Props", "VIBFREQ"))
    shutil.copy(os.path.join(REPO, "tests", "thermo", "fixtures", "h2o", "Props", "PNTGRP"),
                os.path.join(calc, "Props", "PNTGRP"))
    with open(os.path.join(calc, "Props", "VIBFREQ"), "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                '<vector name="VIBFREQ" rowLabel="Normal Mode" rowLabels="%s" '
                'rows="%d" units="cm-1">%s</vector>\n' % (
                    " ".join("%d-A" % (i + 1) for i in range(len(freqs))),
                    len(freqs), "   ".join("%.4f" % v for v in freqs)))
    meta = os.path.join(calc, ".ecce-meta")
    lines = []
    for line in open(meta).read().splitlines():
        if line.startswith(".\t" + NS + "application\t"):
            line = ".\t%sapplication\t\t%s" % (NS, code)
        lines.append(line)
    open(meta, "w").write("\n".join(lines) + "\n")
    return calc


def cli(env, url, *extra):
    out = subprocess.run([os.path.join(env["ECCE_HOME"], "bin", "ecce-thermo"), url,
                          "--json"] + list(extra), env=env, capture_output=True,
                         text=True).stdout
    return json.loads(out)[0]["treatments"]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI + ("ecce-thermo",):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "thermopanel", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI + ("ecce-thermo",):
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
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = makeThermoCalc(project, "soft", [25.0, 60.0, 150.0, 1645.0, 3866.0, 3965.0])
        url = "file://" + calc
        cenv = dict(env, ECCE_HOME=s.home)

        want = {}
        cmds = ["wait 6000", "shot panel"]
        cases = [(t, 298.15, False) for t in TREATMENTS] + [("qrrho-grimme:100", 350.0, True)]
        for t, T, molar in cases:
            extra = ["--T", "%.2f" % T] + (["--standard-state", "1M"] if molar else [])
            want[(t, T, molar)] = cli(cenv, url, *extra)[t]
            cmds.append("thermo %s %.2f %s" % (t, T, "1M" if molar else "1atm"))
        cmds.append("quit")
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", url + "/"], cmds, png, "thermo",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Thermochemistry"})
        check("segmentation" not in text.lower(), "no crash")
        got = re.findall(r"BUILDER: thermo (\S+) H=(\S+) S=(\S+) G=(\S+) (ok|failed.*)", text)
        check(len(got) == len(cases), "every thermo command answered", text[-1500:])
        for (t, T, molar), g in zip(cases, got):
            w = want[(t, T, molar)]
            ok = (g[0] == t and g[4] == "ok" and abs(float(g[1]) - w["H"]) < 2e-6 and
                  abs(float(g[2]) - w["S"]) < 2e-3 and abs(float(g[3]) - w["G"]) < 2e-6)
            check(ok, "%s at %.2f K%s: H, S, G equal ecce-thermo" % (
                t, T, ", 1 M" if molar else ""), str(g))
        # The treatments must differ for these soft modes, or the test proves nothing.
        gs = set(round(want[(t, 298.15, False)]["G"], 6) for t in TREATMENTS)
        check(len(gs) >= 4, "the treatments give different G for soft modes", str(gs))
        stored = open(os.path.join(calc, "Props", "EGIBBS")).read()
        check(stored == open(os.path.join(FIX, "calc-water-freq-g16", "Props", "EGIBBS")).read(),
              "the code's own G is untouched")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
