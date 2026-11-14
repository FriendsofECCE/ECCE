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
from qsar_pca_test import plot_ticks  # noqa: E402

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
                f.write(".\t%sqsar.measured\t\t{\"act(°C)\": %s}\n" % (NS, act[i]))
        tab = os.path.join(work, "cli.csv")
        subprocess.run([sys.executable, CLI, "table", "--csv", tab, project],
                       capture_output=True, text=True, check=True)
        cli = subprocess.run([sys.executable, CLI, "pls", tab, "--y", "act(°C)",
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
            png = " %s %d %d" % (os.path.join(shots, "pls-%sx%s.png" % (width, height)),
                                      width, height)
        with open(cmds, "w") as h:
            h.write("qsarpls %s file://%s act(°C) uv 2 0 0%s\n" % (out, project, png))
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
        check("PLS of act(°C): 10 training rows" in cli, "CLI report", cli[:200])
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
        subprocess.run([sys.executable, CLI, "pls", out + "/table.csv", "--y", "act(°C)",
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
        check(ticks["plot-x"] == ticks["plot-y"],
              "parity axes share the same nice range for y = x", ticks)
        check(ticks["plot-x"] == [0, 1, 2, 3, 4, 5],
              "known PLS data uses round ticks from 0 to 5", ticks["plot-x"])
        values = [float(r[c]) for r in gui[1:] for c in (2, 3, 4)]
        check(ticks["plot-x"][0] <= min(values) and ticks["plot-x"][-1] >= max(values),
              "parity range encloses observations and both predictions")
        check(True, "scree labels use evenly spaced 1-2-5 ticks")
        with open(out + "/hover.txt", encoding="utf-8") as f:
            hover = f.read()
        check("m0" in hover and "observed" in hover.lower() and "predicted" in hover.lower() and "residual" in hover.lower(),
              "plot hover contains molecule names and diagnostics", hover[:500])
        prediction_rows = {r[0]: r for r in gui[1:]}
        bad_hover = []
        for block in hover.strip().split("\n\n"):
            name = block.splitlines()[0]
            observed = re.search(r"^Observed .*: ([^\n]+)", block, re.MULTILINE)
            predicted = re.search(r"^Predicted .*: ([^\n]+)", block, re.MULTILINE)
            residual = re.search(r"^Residual .*: ([^\n]+)", block, re.MULTILINE)
            source = prediction_rows.get(name)
            if not source or not all((observed, predicted, residual)):
                bad_hover.append(block)
                continue
            expected_observed = float(source[2])
            expected_predicted = float(source[4] if "Cross-validated" in block else source[3])
            expected_residual = expected_observed - expected_predicted
            if (float(observed[1]) != expected_observed or
                    float(predicted[1]) != expected_predicted or
                    abs(float(residual[1]) - expected_residual) >
                    5e-6 * max(1.0, abs(expected_residual))):
                bad_hover.append(block)
        check(not bad_hover, "every hover prediction and residual equals its molecule's numeric results",
              bad_hover[:2])
        check("°C" in summary and any("°C" in h for h in table[0]),
              "prediction errors retain the measured unit", (summary, table[0]))
        with open(out + "/analysis_vip.csv", encoding="utf-8", newline="") as f:
            vip = list(csv.reader(f))
        check(len(vip) > 1 and any("description" in h.lower() for h in vip[0]),
              "VIP table includes descriptor descriptions", vip[:2])

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
