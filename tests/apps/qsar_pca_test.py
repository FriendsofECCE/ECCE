#!/usr/bin/env python3
"""The Organizer's QSAR Analysis (PCA) equals `ecce-qsar pca` on the same folder (#961).

    tests/apps/qsar_pca_test.py           own Xvfb, services, local data

Six calculations with different stored SMILES are opened in the Organizer;
the "qsarpca" test hook runs the dialog's PCA and saves its result files,
whose scores must equal those of the CLI on the stored folders.  Exit 77 (skip) without an install or Xvfb.
"""
import csv
import io
import math
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


def plot_ticks(verification):
    """Read the labels collected by the actual paint paths."""
    labels = {}
    for axis in ("plot-x", "plot-y", "scree-y"):
        match = re.search(r"^" + axis + r"=(.*)$", verification, re.MULTILINE)
        values = [float(v) for v in match[1].split()] if match else []
        assert len(values) >= 2, (axis, values)
        step = values[1] - values[0]
        assert step > 0, (axis, values)
        mantissa = step / 10 ** math.floor(math.log10(step))
        assert any(math.isclose(mantissa, m) for m in (1, 2, 5, 10)), (axis, values)
        assert all(math.isclose(b - a, step) for a, b in zip(values, values[1:])), (axis, values)
        assert all(math.isclose(v / step, round(v / step), abs_tol=1e-8) for v in values), (axis, values)
        labels[axis] = values
    return labels


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
        shots = os.environ.get("ECCE_QSAR_SHOTS")
        png = ""
        if shots:
            os.makedirs(shots, exist_ok=True)
            screen = os.environ.get("ECCE_XVFB_SCREEN", "1280x1024x24").split("x")
            width, height = int(screen[0]), int(screen[1])
            png = " %s %d %d" % (os.path.join(shots, "pca-%sx%s.png" % (width, height)),
                                      width, height)
        with open(cmds, "w") as h:
            h.write("qsarpca %s file://%s uv 2%s\n" % (out, project, png))
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
        with open(out + "/analysis_summary.txt", encoding="utf-8") as f:
            summary = f.read()
        check("%" in summary and "components" in summary, "summary explains variance as percentages", summary)
        check("Left out (no variance in these " in summary and "column " not in summary,
              "omitted descriptors are grouped by reason", summary)
        with open(out + "/analysis_components.csv", encoding="utf-8", newline="") as f:
            table = list(csv.reader(f))
        check(any("R²X" in h for h in table[0]) and any("Q²" in h for h in table[0])
              and any("%" in h for h in table[0]), "component headers have symbols and percent units", table[0])
        check(len(table) > 2, "component table includes computed components")
        # CV folds depend on row positions; use the exported GUI table to
        # compare presentation independently of folder traversal order.
        subprocess.run([sys.executable, CLI, "pca", out + "/table.csv", "--y", "act",
                        "--components", "2", "--outdir", work + "/display-reference"],
                       capture_output=True, text=True, check=True)
        with open(work + "/display-reference/analysis_components.csv", encoding="utf-8", newline="") as f:
            reference_table = list(csv.reader(f))
        check(table == reference_table, "displayed components equal the CLI table",
              (table, reference_table))
        check("* = component kept" in summary and "model plane" in summary,
              "legend explains kept components and model distances", summary)
        with open(out + "/presentation.tsv", encoding="utf-8", newline="") as f:
            copied = list(csv.reader(f, delimiter="\t"))
        check(copied == table, "Copy table copies every displayed value and header", copied[:2])
        with open(out + "/selection.tsv", encoding="utf-8", newline="") as f:
            selection = list(csv.reader(f, delimiter="\t"))
        check(selection == [table[1][:2]], "Ctrl+C copies only the selected cells", selection)
        with open(out + "/presentation-check.txt", encoding="utf-8") as f:
            verification = f.read()
        for action in ("copy", "selection", "hover", "click"):
            check(action + "=PASS" in verification, "GUI " + action + " follows the interactive path", verification)
        ticks = plot_ticks(verification)
        check(True, "score and scree labels use evenly spaced 1-2-5 ticks")
        for axis, column in (("plot-x", 1), ("plot-y", 2)):
            values = [float(r[column]) for r in gui[1:]]
            check(ticks[axis][0] <= min(values) and ticks[axis][-1] >= max(values),
                  axis + " range encloses every score", ticks[axis])
        check(ticks["scree-y"][0] <= 0 and ticks["scree-y"][-1] >= 100,
              "scree ticks enclose the percentage range", ticks["scree-y"])
        with open(out + "/hover.txt", encoding="utf-8") as f:
            hover = f.read()
        check("m0" in hover and "T²" in hover and "DModX" in hover,
              "plot hover contains molecule names and diagnostics", hover[:500])

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
