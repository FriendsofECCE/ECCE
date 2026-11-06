#!/usr/bin/env python3
"""
Compare Energies > Reaction overview (#709), headless.

Two water frequency calculations with constructed soft modes (the thermo
fixtures) and a reactions file.  The ECCE_TEST_ORGANIZER hook
"reactionoverview <file> <treatment> <png> <csv>" opens the dialog and prints
its chart points, what a click on reaction 1 lists, and writes the exports.
The chart's dE, dG and dG range must equal `ecce-thermo --reactions --format
csv`, the click must list the soft modes below 100 cm-1, the exported CSV
must be the CLI's reaction table, and the PNG must exist.
ECCE_OVERVIEW_PNG=<file> keeps the PNG.  Exit 77 (skip) without an install
or Xvfb.
"""

import csv
import io
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402
import compare_energies_test as ce  # noqa: E402

failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)), "tests", "launch"))
    import thermo_panel_test as tp
    data = tempfile.mkdtemp(prefix="ecce-overview-data-")
    work = tempfile.mkdtemp(prefix="ecce-overview-")
    try:
        project = os.path.join(data, "users", "local", "overview-test")
        os.makedirs(project)
        shutil.copy(ce.PROJECT_META, os.path.join(project, ".ecce-meta"))
        f1 = [25.0, 60.0, 150.0, 1645.0, 3866.0, 3965.0]
        f2 = [40.0, 90.0, 300.0, 1600.0, 3800.0, 3900.0]
        a = tp.makeThermoCalc(project, "soft-a", f1)
        b = tp.makeThermoCalc(project, "soft-b", f2)
        ua, ub = "file://" + a, "file://" + b
        with open(os.path.join(b, "Props", "TE"), "w") as f:
            f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                    '<value name="TE" units="Hartree">-75.0000000000</value>\n')
        rx = os.path.join(work, "reactions.txt")
        open(rx, "w").write("%s -> %s\n2*%s -> 2*%s\n" % (ua, ub, ub, ua))
        png = os.environ.get("ECCE_OVERVIEW_PNG") or os.path.join(work, "chart.png")
        csvOut = os.path.join(work, "chart.csv")
        cmds = os.path.join(work, "commands")
        open(cmds, "w").write("reactionoverview %s qrrho-grimme:100 %s %s\n" % (rx, png, csvOut))
        home = tempfile.mkdtemp(prefix="ecce-overview-home-")
        try:
            env = dict(os.environ, ECCE_HOME=apps.INSTALL, ECCE_REALUSERHOME=home)
            cli = subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-thermo"), "--reactions",
                                  rx, "--format", "csv", "--chosen", "qrrho-grimme:100"],
                                 env=env, capture_output=True, text=True).stdout
        finally:
            shutil.rmtree(home, ignore_errors=True)
        result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + a,
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        got = re.findall(r"ECCE_TEST_ORGANIZER: reactionoverview .*?: (ok.*)", log)
        if not check(len(got) == 1, "the command answered", run_tests._tail(log, 30)):
            return
        lines = got[0].split(" | ")
        rows = list(csv.DictReader(io.StringIO(cli.split("\n\n")[0])))
        check(len(rows) == 2 and all(r["status"] == "ok" for r in rows), "CLI: two reactions", cli[:300])
        pts = {}
        for l in lines:
            m = re.match(r"([01]) (\d+) (\d+) (\S+) (\S+) (\S+) (\S+)$", l)
            if m:
                pts[(int(m.group(1)), int(m.group(3)))] = m.groups()[4:]
        check(len(pts) == 4, "the chart has dE and dG for both reactions", str(lines[:12]))
        for n, r in enumerate(rows, 1):
            e = pts.get((0, n)); g = pts.get((1, n))
            if not e or not g:
                check(False, "points for reaction %d" % n)
                continue
            check(abs(float(e[0]) - float(r["dE_el_kJ_mol"])) < 1e-3, "reaction %d: dE equals the CLI" % n)
            check(abs(float(g[0]) - float(r["dG_kJ_mol"])) < 1e-3 and
                  abs(float(g[1]) - float(r["dG_min_kJ_mol"])) < 1e-3 and
                  abs(float(g[2]) - float(r["dG_max_kJ_mol"])) < 1e-3,
                  "reaction %d: dG and its range equal the CLI" % n, str(g))
        click = [l for l in lines if "cm-1" in l and "soft-" in l]
        check(len(click) == 4, "a click lists the four soft modes (a: 25, 60; b: 40, 90)",
              " | ".join(lines))
        check(any(l.startswith("Soft modes (< 100 cm-1)") for l in lines), "the click names the 100 cm-1 limit")
        exp = open(csvOut).read() if os.path.exists(csvOut) else ""
        check(exp.split("\n\n")[0].strip() == cli.split("\n\n")[0].strip(), "the CSV export is the CLI's table")
        check(os.path.exists(png) and os.path.getsize(png) > 2000, "the PNG exists")
        if os.environ.get("ECCE_OVERVIEW_PNG") is None and os.path.exists(png):
            pass
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("overview", "reaction overview wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
