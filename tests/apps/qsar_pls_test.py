#!/usr/bin/env python3
"""The Organizer's QSAR Analysis (PLS) equals `ecce-qsar pls` on the same folder (#961).

    tests/apps/qsar_pls_test.py           own Xvfb, services, local data

Ten calculations with different stored SMILES are opened in the Organizer;
the "qsarpls" test hook runs the dialog's PLS and saves its result files,
whose predictions must equal those of the CLI on the stored folders.  Exit 77 (skip) without an install or Xvfb.
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
CLI = os.path.join(ROOT, "scripts", "ecce_qsar.py")
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
    data = tempfile.mkdtemp(prefix="ecce-chk-data-")
    work = tempfile.mkdtemp(prefix="ecce-chk-")
    try:
        project = os.path.join(data, "users", "local", "chk")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        smi = ["O", "CCO", "c1ccccc1", "CC(=O)O", "CCN", "OCC(O)CO", "CCCC", "c1ccncc1", "CC(C)O", "NCCO"]
        act = [1.0, 2.1, 3.9, 2.9, 2.0, 3.2, 4.1, 3.0, 2.6, 2.2]
        for i, sm in enumerate(smi):
            d = os.path.join(project, "m%d" % i)
            shutil.copytree(SRC, d, symlinks=True)
            with open(os.path.join(d, ".ecce-meta"), "a") as f:
                f.write(".\t%sqsar.smiles\t\t%s\n" % (NS, sm))
                f.write(".\t%sqsar.measured\t\t{\"act\": %s}\n" % (NS, act[i]))
        tab = os.path.join(work, "cli.csv")
        subprocess.run([sys.executable, CLI, "table", "--csv", tab, project],
                       capture_output=True, text=True, check=True)
        cli = subprocess.run([sys.executable, CLI, "pls", tab, "--y", "act",
                              "--components", "2", "--outdir", work + "/cli"],
                             capture_output=True, text=True, check=True).stdout
        out = os.path.join(work, "gui")
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as h:
            h.write("qsarpls %s file://%s act uv 2\n" % (out, project))
        result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + project,
                               "ECCE_LOCAL_DATA": data})
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        if not check(os.path.exists(out + "/pls_predictions.csv"), "the hook wrote the PLS files",
                     (result.log or "")[-1500:]):
            return
        gui = list(csv.reader(open(out + "/pls_predictions.csv")))
        ref = list(csv.reader(open(work + "/cli/pls_predictions.csv")))
        check(len(gui) == len(ref) == 11 and gui[0] == ref[0], "same rows and columns", (gui[:1], ref[:1]))
        refd = {r[0]: r for r in ref[1:]}
        bad = [(g[0], g[3], refd.get(g[0], [0, 0, 0, "missing"])[3]) for g in gui[1:]
               if g[0] not in refd or abs(float(g[3]) - float(refd[g[0]][3])) > 1e-6 * (1 + abs(float(g[3])))]
        check(not bad, "predictions equal the CLI's", bad)
        check("PLS of act: 10 training rows" in cli, "CLI report", cli[:200])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("qsar", "QSAR PLS wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
