#!/usr/bin/env python3
"""
Builder script "constraints scan <steps> <step>" (#1053): Geometry Constraints >
Scan... on the selected atoms, through the dialog's own code.  On benzene, a
ring torsion scan and a bond scan are stored (identical in form to what the dialog
writes), listed, and refused for a 1-atom selection and for bad numbers.
Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402

PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")
SOURCE = os.path.join(HERE, "fixtures", "calc-polar-c6h6-g16")
failures = []


def check(ok, what, detail=""):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
        if detail:
            print(detail)


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "scanscript-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "p")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "meth")
        shutil.copytree(SOURCE, calc, symlinks=True)
        gcst = os.path.join(calc, "Parameters", "GeomConstraintModel.gcst")
        if os.path.exists(gcst):
            os.remove(gcst)
        meta = os.path.join(calc, ".ecce-meta")      # not run, so it may be edited
        text = open(meta, errors="surrogateescape").read()
        text = re.sub(r"(ecce:state\t\t)[^\n]*", r"\g<1>Ready", text)
        text = re.sub(r"(ecce:runtype\t\t)[^\n]*", r"\g<1>Geometry", text)
        open(meta, "w", errors="surrogateescape").write(text)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT", [
                "wait 4000", "constraints show",
                "constraints select 1 2 3 4", "constraints scan 36 10",   # C-C-C-C torsion
                "constraints select 1 2", "constraints scan 5 0.1",       # C-C bond
                "constraints select 1", "constraints scan 5 0.1",         # one atom
                "constraints select 1 2", "constraints scan 0 0.1",       # no steps
                "constraints select 1 2", "constraints scan 5 0",         # no step size
                "constraints list", "save", "wait 2500", "constraints stored",
                "wait 500", "quit"], "file://" + calc + "/")
            tail = run_tests._tail(log, 30)
            res = re.findall(r"BUILDER: constraints scan: (.*)", log)
            check(len(res) == 5 and res[0].startswith("Added Torsion") and
                  res[1].startswith("Added Bond"), "torsion and bond scans added: %s" % res, tail)
            check(all("Select 2, 3 or 4" in r or "Scan steps" in r or "step size" in r
                      for r in res[2:]) and len(res) == 5,
                  "1 atom, 0 steps and 0 step size are refused with the dialog's text", tail)
            check(len(re.findall(r"ECCE_BUILDER_SCRIPT.*constraints scan.*FAIL", log)) == 3 or
                  log.count("FAIL: ") >= 3, "the refused script lines FAIL", tail)
            rows = re.findall(r"BUILDER: constraint row: (.*)", log)
            check(len(rows) == 2 and any("scan 36 × 10°" in r for r in rows) and
                  any("scan 5 × 0.1 Å" in r for r in rows), "list reports both scans: %s" % rows, tail)
            text = open(gcst).read() if os.path.exists(gcst) else ""
            # Same writer as the dialog's Save; the scan lines are what matters.
            check(re.search(r'<Constraint type="Torsion">\s*<Group>0 1 2 3\s*</Group>\s*<Value>0\s*</Value>'
                            r'\s*<Scan steps="36" step="10"/>', text) is not None
                  and re.search(r'<Constraint type="Bond">\s*<Group>0 1\s*</Group>\s*<Value>[0-9.]+\s*</Value>'
                                r'\s*<Scan steps="5" step="0.1"/>', text) is not None,
                  "the stored .gcst has both scans in the dialog's format", text)
            m = re.search(r"constraints stored=(-?\d+)", log)
            check(m is not None and m.group(1) == "2", "the calculation stored 2 constraints", tail)
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if failures:
            results.fail("builder-scan-script", "%d wrong: %s" % (len(failures), "; ".join(failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
