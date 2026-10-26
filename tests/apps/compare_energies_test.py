#!/usr/bin/env python3
"""
Tools > Compare Energies in the Organizer (#573), headless.

Three local-data calculations made from fixtures/calc-water-opt (final
energy -76.0107465054306 Hartree): A as it is; B with another energy and
another code; C without a final energy.  The ECCE_TEST_ORGANIZER hook
"compareenergies <urlA> <urlB>" prints what the dialog shows and
"comparedialog <urlA> <urlB> <png>" opens the dialog itself:

- A against B: both energies, E(B) - E(A) in Hartree, kJ/mol, kcal/mol and
  eV, and the note that the code differs;
- A against C: no difference, the note that C has no final energy;
- the dialog preset to A and B shows the same text in its two areas.

ECCE_COMPARE_PNGS=<dir> keeps the capture of the dialog there.  Same
installed tree and isolation as run_tests.py.  Exit 77 (skip) without an
install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")
NS = "http://www.emsl.pnl.gov/ecce:"
E_A = -76.0107465054306
E_B = -75.5
KJ, KCAL, EV = 2625.4996394799, 2625.4996394799 / 4.184, 27.211386245988

failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def makeCalc(project, name, energy, code):
    calc = os.path.join(project, name)
    shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-opt"), calc,
                    symlinks=True)
    te = os.path.join(calc, "Props", "TE")
    if energy is None:
        os.remove(te)
    else:
        text = open(te).read().replace("-7.60107465054306e+01", "%.14e" % energy)
        open(te, "w").write(text)
    meta = os.path.join(calc, ".ecce-meta")
    lines = []
    for line in open(meta).read().splitlines():
        if line.startswith(".\t" + NS + "application\t"):
            line = ".\t%sapplication\t\t%s" % (NS, code)
        lines.append(line)
    open(meta, "w").write("\n".join(lines) + "\n")
    return calc


def answers(log, command):
    return re.findall(r"ECCE_TEST_ORGANIZER: %s .*?: (ok.*)" % re.escape(command), log)


def number(text, unit):
    m = re.search(r"\s(-?\d+\.\d+) %s\b" % re.escape(unit), text.split("E(B) - E(A):")[-1])
    return float(m.group(1)) if m else None


def run(display, pngs):
    data = tempfile.mkdtemp(prefix="ecce-compare-data-")
    work = tempfile.mkdtemp(prefix="ecce-compare-")
    try:
        project = os.path.join(data, "users", "local", "compare-test")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        a = makeCalc(project, "water-a", E_A, "NWChem")
        b = makeCalc(project, "water-b", E_B, "ORCA")
        c = makeCalc(project, "water-c", None, "NWChem")
        ua, ub, uc = ("file://" + p for p in (a, b, c))
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("\n".join([
                "compareenergies %s %s" % (ua, ub),
                "compareenergies %s %s" % (ua, uc),
                "comparedialog %s %s %s" % (ua, ub, os.path.join(pngs, "compare-energies.png")),
            ]) + "\n")
        result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + a,
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        ab = answers(log, "compareenergies")
        dlg = answers(log, "comparedialog")
        if not check(len(ab) == 2 and len(dlg) == 1, "every command answered",
                     run_tests._tail(log, 30)):
            return
        for line in ab + dlg:
            print("      " + line)
        withBoth, withNone = ab
        delta = E_B - E_A
        check(number(withBoth, "Hartree") is not None and abs(number(withBoth, "Hartree") - delta) < 1e-7,
              "A against B: E(B) - E(A) in Hartree", withBoth)
        check(number(withBoth, "kJ/mol") is not None and abs(number(withBoth, "kJ/mol") - delta * KJ) < 1e-3,
              "A against B: kJ/mol", withBoth)
        check(number(withBoth, "kcal/mol") is not None and abs(number(withBoth, "kcal/mol") - delta * KCAL) < 1e-3,
              "A against B: kcal/mol", withBoth)
        check(number(withBoth, "eV") is not None and abs(number(withBoth, "eV") - delta * EV) < 1e-3,
              "A against B: eV", withBoth)
        check("E(A) -76.01074651 Hartree" in withBoth and "E(B) -75.50000000 Hartree" in withBoth,
              "A against B: both final energies", withBoth)
        check("Note: The code differs: A NWChem, B ORCA." in withBoth,
              "A against B: the note that the code differs", withBoth)
        check("E(B) - E(A)" not in withNone and "water-c (B) has no final energy" in withNone,
              "A against C: no difference, a note that C has no final energy", withNone)
        check("-76.01074651 Hartree" in dlg[0] and "Notes:" in dlg[0] and
              "code differs" in dlg[0].split("Notes:")[1],
              "the dialog shows the energies and, in its own area, the note", dlg[0])
        png = os.path.join(pngs, "compare-energies.png")
        check(os.path.exists(png) and os.path.getsize(png) > 1000, "saved the dialog's capture")
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        keep = os.environ.get("ECCE_COMPARE_PNGS")
        pngs = keep or tempfile.mkdtemp(prefix="ecce-compare-png-")
        os.makedirs(pngs, exist_ok=True)
        try:
            run(display, pngs)
        finally:
            if not keep:
                shutil.rmtree(pngs, ignore_errors=True)
        if failures:
            results.fail("compare", "compare energies wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
