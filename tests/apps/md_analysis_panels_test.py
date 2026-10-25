#!/usr/bin/env python3
"""Speed Distribution and Displacement (MSD) panels, headless, with the tree's own binaries.

    tests/apps/md_analysis_panels_test.py [--build build] [--png DIR]

The MDSPEED/MDMSD records of the shipped General Chemistry examples (argon
gas and helium gas, `ecce-parse-md.txt`) go through the real parser
gromacs.md into property files of two calculations in one folder.  The
Builder opens each panel; the script commands mdcsv, mdcompare, mdfit and
mdloglog stand for Export, Compare, Fit and Log-log.  The fit is checked
against a least-squares line computed here from the numbers in the file,
and the compared calculation must reach the CSV.  Exit status 77 (skip)
without Xvfb, import or binaries.
"""

import argparse
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
import irc_panel_test as irc  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
EXAMPLES = os.path.join(REPO, "data", "client", "examples", "genchem")
PARSER = os.path.join(REPO, "scripts", "parsers", "gromacs.md")


def records(name):
    """{key: [record, ...]} from the real parser over a shipped parse file."""
    path = os.path.join(EXAMPLES, "gromacs-" + name, "1-dynamics", "ecce-parse-md.txt")
    out = subprocess.run(["perl", PARSER], stdin=open(path), capture_output=True,
                         text=True, check=True).stdout
    found = {}
    for blk in out.split("END\n"):
        m = re.search(r"key: (\w+)\nsize:\n(\d+)\nrowlabels:\n.*?\nvalues:\n(.*)", blk, re.S)
        if m:
            found[m.group(1)] = m.group(3).split()
            assert len(found[m.group(1)]) == int(m.group(2))
    return found


def fit(rec, lo, hi):
    f = rec.split(";")
    t = [float(v) for v in f[2].split(",")]
    y = [float(v) for v in f[3].split(",")]
    p = [(a, b) for a, b in zip(t, y) if lo <= a <= hi]
    n = len(p)
    sx = sum(a for a, _ in p); sy = sum(b for _, b in p)
    sxx = sum(a * a for a, _ in p); sxy = sum(a * b for a, b in p)
    return (n * sxy - sx * sy) / (n * sxx - sx * sx) / 6 * 1000   # 1e-5 cm2/s


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import", "perl"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    ar, he = records("ar-gas"), records("he-gas")
    s = harness.Session(build, "mdpanels", {}, local=True, keep=args.keep)
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
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        for name, recs in (("ar-gas", ar), ("he-gas", he)):
            calc = os.path.join(project, name)
            shutil.rmtree(calc, ignore_errors=True)
            shutil.copytree(irc.BASE, calc, symlinks=True)
            props = os.path.join(calc, "Props")
            for p in os.listdir(props):
                if p != "ESCF":
                    os.unlink(os.path.join(props, p))
            with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
                f.write(irc.HCN)
            for key, vals in recs.items():
                with open(os.path.join(props, key), "w") as f:
                    f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                            '<vecstring name="%s" rowLabel="Atom Kind" rows="%d" units="NA">%s'
                            '</vecstring>\n' % (key, len(vals), "   ".join(vals)))
        calc = os.path.join(project, "ar-gas")
        sp1, sp2, ms1 = [os.path.join(s.state, n) for n in ("sp1.csv", "sp2.csv", "msd.csv")]
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            ["wait 6000", "shot panel", "mdcsv speed %s" % sp1, "mdcompare speed he-gas",
             "mdcsv speed %s" % sp2, "shot compare", "quit"], png, "mdspeed",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Speed Distribution"})
        check("mdcsv curves=2 ok" in text, "Speed Distribution opened with argon and exported")
        check("mdcompare he-gas curves=4 ok" in text, "Compare added the helium run")
        if os.path.exists(sp1) and os.path.exists(sp2):
            one, two = open(sp1).read(), open(sp2).read()
            check("speed_m/s,Ar_measured,Ar_MB" in one and "He_measured" not in one,
                  "the CSV of the run alone holds argon only")
            check("He_measured" in two and "Ar_measured" in two,
                  "the CSV after Compare holds both, helium labelled")
            mean = float(re.search(r"# Ar .*mean_m/s=([0-9.]+)", one).group(1))
            check(abs(mean - 398.7) < 3, "measured mean argon speed %.1f m/s, theory 398.7" % mean)
        else:
            check(False, "the CSVs exist")
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            ["wait 6000", "shot panel", "mdfit 263 1000", "mdcsv msd %s" % ms1,
             "mdfit 100 500", "mdloglog on", "mdcompare msd he-gas", "shot loglog", "quit"],
            png, "mdmsd", {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Displacement (MSD)"})
        want = fit(ar["MDMSD"][0], 263, 1000)
        m = re.findall(r"mdfit D=([0-9.e+-]+)", text)
        check(len(m) == 2 and abs(float(m[0]) / want - 1) < 2e-3,
              "D over 263-1000 ps is %s, least squares here %.4g (1e-5 cm2/s)" % (m[:1], want))
        want2 = fit(ar["MDMSD"][0], 100, 500)
        check(len(m) == 2 and abs(float(m[1]) / want2 - 1) < 2e-3,
              "after a new fit range D is %s, least squares here %.4g" % (m[1:], want2))
        check("mdcompare he-gas curves=4 ok" in text and "mdloglog on" in text,
              "Compare and Log-log reached the Displacement panel")
        say("screenshots: %s" % shots)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
