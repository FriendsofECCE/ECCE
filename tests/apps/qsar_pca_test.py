#!/usr/bin/env python3
"""The Organizer's QSAR Analysis (PCA) equals `ecce-qsar pca` on the same folder (#961).

    tests/apps/qsar_pca_test.py           own Xvfb, services, local data

Six calculations with different stored SMILES are opened in the Organizer;
the "qsarpca" test hook runs the dialog's PCA and saves its result files,
whose scores must equal those of the CLI on the stored folders.  Exit 77 (skip) without an install or Xvfb.
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
        smi = ["O", "CCO", "c1ccccc1", "CC(=O)O", "CCN", "OCC(O)CO"]
        for i, sm in enumerate(smi):
            d = os.path.join(project, "m%d" % i)
            shutil.copytree(SRC, d, symlinks=True)
            with open(os.path.join(d, ".ecce-meta"), "a") as f:
                f.write(".\t%sqsar.smiles\t\t%s\n" % (NS, sm))
                f.write(".\t%sqsar.measured\t\t{\"act\": %d}\n" % (NS, i))
        tab = os.path.join(work, "cli.csv")
        subprocess.run([sys.executable, CLI, "table", "--csv", tab, project],
                       capture_output=True, text=True, check=True)
        cli = subprocess.run([sys.executable, CLI, "pca", tab, "--y", "act",
                              "--components", "2", "--outdir", work + "/cli"],
                             capture_output=True, text=True, check=True).stdout
        out = os.path.join(work, "gui")
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as h:
            h.write("qsarpca %s file://%s uv 2 %s\n" % (out, project, os.environ.get("ECCE_QSAR_SHOT", "")))
        result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + project,
                               "ECCE_LOCAL_DATA": data})
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        if not check(os.path.exists(out + "/pca_scores.csv"), "the hook wrote the PCA files",
                     (result.log or "")[-1500:]):
            return
        gui = list(csv.reader(open(out + "/pca_scores.csv")))
        ref = list(csv.reader(open(work + "/cli/pca_scores.csv")))
        check(len(gui) == len(ref) == 7 and gui[0] == ref[0], "same rows and columns", (gui[:1], ref[:1]))
        refd = {r[0]: r for r in ref[1:]}
        bad = [(g[0], g[1], refd.get(g[0], [0, "missing"])[1]) for g in gui[1:]
               if g[0] not in refd or abs(float(g[1]) - float(refd[g[0]][1])) > 1e-6 * (1 + abs(float(g[1])))]
        check(not bad, "scores equal the CLI's", bad)
        check("PCA: 6 rows" in cli, "CLI report", cli[:200])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("qsar", "QSAR analysis wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
