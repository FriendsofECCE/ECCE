#!/usr/bin/env python3
"""The Organizer's QSAR Table has the rows of `ecce-qsar table` (#961).

    tests/apps/qsar_table_test.py         own Xvfb, services, local data

A folder with two calculations (one with a stored SMILES and a measured
value, one without a SMILES) is opened in the Organizer; the "qsartable" test
hook writes the dialog's rows as CSV, which must equal the CLI's CSV for the
same folders (numbers compared as numbers: the GUI reads the data layer, the
CLI the stored files).  Exit 77 (skip) without an install or Xvfb.
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
        for n in ("water", "nosmiles"):
            shutil.copytree(SRC, os.path.join(project, n), symlinks=True)
        with open(os.path.join(project, "water", ".ecce-meta"), "a") as f:
            f.write(".\t%sqsar.smiles\t\tO\n" % NS)
            f.write(".\t%sqsar.measured\t\t{\"IC50 (nM)\": 3.5}\n" % NS)
            f.write(".\t%sqsar.source\t\tassay.csv\n" % NS)
        cli = subprocess.run([sys.executable, CLI, "table", "--csv", "-",
                              project + "/water", project + "/nosmiles"],
                             capture_output=True, text=True, check=True).stdout
        out = os.path.join(work, "gui.csv")
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as h:
            h.write("qsartable %s file://%s\n" % (out, project))
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
        definitions = subprocess.run([sys.executable, CLI, "table", "--describe", "--json",
                                      project + "/water", project + "/nosmiles"],
                                     capture_output=True, text=True, check=True).stdout
        import json
        sys.path.insert(0, os.path.join(ROOT, "scripts"))
        import ecce_qsar_chem as C
        info = json.loads(definitions)["columns"]
        with open(out + ".key", encoding="utf-8") as f:
            key = f.read()
        check(key == "".join(d["name"] + ": " + C.definition_text(d) + "\n\n" for d in info),
              "Key dialog contains every current column with the CLI's definitions")
        with open(out + ".copied", encoding="utf-8") as f:
            check(f.read() == key, "Copy All copies the complete Key dialog text")
        with open(out + ".tips", encoding="utf-8") as f:
            tips = dict(line.rstrip("\n").split(": ", 1) for line in f)
        for name in ("a_nH", "SlogP_VSA3", "qm_HOMO", "IC50 (nM)"):
            definition = next(d for d in info if d["name"] == name)
            check(tips.get(name) == C.definition_text(definition),
                  name + ": header motion installs the library tooltip", tips.get(name))
        check("from assay.csv" in tips["IC50 (nM)"], "measured provenance reaches header")
        gui, ref = rows(open(out).read()), rows(cli)
        check(set(gui) == set(ref) and len(ref) == 2, "same rows", (list(gui), list(ref)))
        for name, r in ref.items():
            g = gui.get(name, {})
            bad = [k for k in r if not same(g.get(k, "?"), r[k])]
            check(not bad, name + ": every column equals the CLI's",
                  [(k, g.get(k), r[k]) for k in bad])
        check(gui.get("nosmiles", {}).get("problems") == "no SMILES stored",
              "the row without a SMILES is listed with the reason", gui.get("nosmiles"))
        check(gui.get("water", {}).get("weight", "").startswith("18.0") and
              gui.get("water", {}).get("IC50 (nM)") == "3.5" and
              gui.get("water", {}).get("qm_HOMO", "") != "",
              "descriptors, measured value and HOMO reach the grid", gui.get("water"))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("qsar", "QSAR table wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
