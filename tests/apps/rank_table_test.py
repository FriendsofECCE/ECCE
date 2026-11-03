#!/usr/bin/env python3
"""The Organizer's Rank table has the deltas and order of ecce-si-export --rank (#703).

    tests/apps/checks_table_test.py       own Xvfb, services, local data

A folder with two calculations (one given an unsuccessful state and a
not-converged progress snapshot) is opened in the Organizer; the "checks"
test hook writes the dialog's rows as CSV, which must equal the CLI's CSV for
the same folders (numbers compared as numbers: the GUI reads the data layer,
the CLI the stored files).  Exit 77 (skip) without an install or Xvfb.
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
import fixture    # noqa: E402
import run_tests  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data", "project.ecce-meta")
CLI = os.path.join(ROOT, "scripts", "ecce_si_export.py")
SRC = os.path.join(fixture.FIXTURES, "calc-water-opt")
NS = "http://www.emsl.pnl.gov/ecce:"
failures = []


def check(ok, what, detail=""):
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else "  " + str(detail)))
    if not ok:
        failures.append(what)
    return ok


def rows(text):
    return {r["name"]: r for r in csv.DictReader(io.StringIO(text))}


def same(a, b):
    try:
        return abs(float(a) - float(b)) < 1e-9
    except ValueError:
        return a == b


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-rnk-data-")
    work = tempfile.mkdtemp(prefix="ecce-rnk-")
    try:
        project = os.path.join(data, "users", "local", "chk")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        for n, te in (("water_a", "-76.40"), ("water_b", "-76.30"), ("water_x", "-90.0")):
            d = os.path.join(project, n)
            shutil.copytree(SRC, d, symlinks=True)
            open(os.path.join(d, "Props", "TE"), "w").write(
                '<?xml version="1.0"?>\n<value name="TE" units="Hartree">%s</value>\n' % te)
        with open(os.path.join(project, "water_x", ".ecce-meta"), "a") as h:
            h.write(".\t%sexcluded\t\tbad run\n" % NS)
        missing = os.path.join(project, "water_missing")
        shutil.copytree(SRC, missing, symlinks=True)
        shutil.rmtree(os.path.join(missing, "Props"))
        os.makedirs(os.path.join(missing, "Props"))
        cli = subprocess.run([sys.executable, CLI, "--rank", "csv", project],
                             capture_output=True, text=True, check=True).stdout
        out = os.path.join(work, "gui.csv")
        out2 = os.path.join(work, "gui-ref.csv")
        out3 = os.path.join(work, "gui-rejected.csv")
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as h:
            h.write("rank %s file://%s\n" % (out, project))
            h.write("rank %s file://%s water_b\n" % (out2, project))
            h.write("rank %s file://%s water_b water_x\n" % (out3, project))
        result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + project,
                               "ECCE_LOCAL_DATA": data})
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        if not check(os.path.exists(out), "the hook wrote the grid rows",
                     (result.log or "")[-1500:]):
            return
        gui, ref = rows(open(out).read()), rows(cli)
        check(set(gui) == set(ref) == {"water_a", "water_b", "water_x", "water_missing"}, "same rows", (list(gui), list(ref)))
        for name, r in ref.items():
            g = gui.get(name, {})
            bad = [k for k in ("dE_kJ", "dE_eV", "dG_kJ", "dG_eV", "excluded")
                   if not same(g.get(k, "?"), r[k])]
            check(not bad, name + ": the deltas equal the CLI's",
                  [(k, g.get(k), r[k]) for k in bad])
        check(gui.get("water_a", {}).get("dE_kJ") == "0.0000" and
              abs(float(gui["water_b"]["dE_kJ"]) - 0.1 * 2625.499638) < 1e-2,
              "the deltas are the hand-computed ones", gui)
        check(gui.get("water_x", {}).get("dE_kJ") == "-", "the excluded row has no delta", gui)
        # A chosen reference (the dialog's Reference choice) = ecce-si-export --ref.
        if check(os.path.exists(out2), "the reference run wrote its rows"):
            cref = subprocess.run([sys.executable, CLI, "--rank", "csv", "--ref", "water_b", project],
                                  capture_output=True, text=True, check=True).stdout
            g2, r2 = rows(open(out2).read()), rows(cref)
            check(g2.get("water_b", {}).get("dE_kJ") == "0.0000" and
                  all(same(g2[n]["dE_kJ"], r2[n]["dE_kJ"]) for n in r2),
                  "against water_b the deltas equal the CLI's --ref", (g2, r2))
        answers = re.findall(r"ECCE_TEST_ORGANIZER: rank .*?: (.*)$", result.log or "", re.M)
        check(len(answers) == 3 and all(
              set(x.strip() for x in re.findall(r"choice=([^|]+)", a)) ==
              {"Lowest", "water_a", "water_b"} for a in answers),
              "reference chooser omits excluded and energy-less calculations", answers)
        check(len(answers) == 3 and "rejected | reference=water_b" in answers[2],
              "a rejected reference restores the chooser to water_b", answers)
        check(os.path.exists(out3) and open(out3).read() == open(out2).read(),
              "a rejected reference preserves the table and CSV")
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("rank", "rank table wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
