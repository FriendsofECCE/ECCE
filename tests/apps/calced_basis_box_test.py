#!/usr/bin/env python3
"""
The Calculation Editor leaves the Basis Set box out for a code that has no
basis sets (MOPAC, Quantum ESPRESSO) and keeps it for one that does (#348).

Calculations come from tests/filedsi/resourceTest (as in symmetry_find_test);
the script's "info" prints basisbox=shown|hidden.  The Gaussian calculation
is switched to QuantumESPRESSO with the Code row, the question answered Yes
through ECCE_TEST_DIALOG_ANSWER.

Same installed tree, isolation and services as run_tests.py.  Exit 77
(skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402
import summary_test as summ  # noqa: E402


def box(log):
    m = re.findall(r"CALCED: basisbox=(\w+)", log)
    return m[-1] if m else None


def orcaCase(display):
    """A script-made ORCA RIJCOSX calculation stores no open-shell count and
    keeps the RI-J auxiliary basis in the theory details (#893, #894)."""
    ns = "http://www.emsl.pnl.gov/ecce:"
    setup = ("ES.Theory.DFT.XCFunctionals|PBE0 D3BJ||1|1|menu_input\n"
             "ES.Theory.SCF.RIMode|RIJCOSX||1|1|menu_input\n"
             "ES.Theory.SCF.RIJCOSXAuxBasis|def2/J||1|1|menu_input\n")
    for mult, want in (("Singlet", "0"), ("Triplet", "2")):
        data = tempfile.mkdtemp(prefix="ecce-calced-data-")
        try:
            project = os.path.join(data, "users", "local", "summary-test")
            os.makedirs(project)
            shutil.copy(summ.PROJECT_META, os.path.join(project, ".ecce-meta"))
            calc = os.path.join(project, "water-orca")
            shutil.copytree(os.path.join(fixture.FIXTURES,
                                         "calc-orca-water-ccpvdz"),
                            calc, symlinks=True)
            with open(os.path.join(calc, "Parameters", "SetupParams"), "w") as f:
                f.write(setup)
            meta = os.path.join(calc, ".ecce-meta")
            lines = []
            for line in open(meta).read().splitlines():
                if line.startswith(".\t" + ns + "spinmultiplicity\t"):
                    line = ".\t" + ns + "spinmultiplicity\t\t" + mult
                elif "hasConfigurations" in line:
                    line = line.replace("</rdf:Bag>",
                        '<rdf:li><rdf:href xlink:href="Parameters/SetupParams" '
                        'xlink:type="simple"></rdf:href></rdf:li></rdf:Bag>')
                lines.append(line)
            open(meta, "w").write("\n".join(lines) + "\n")
            os.environ["ECCE_LOCAL_DATA"] = data
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                               ["wait 6000", "info", "quit"],
                               "file://" + calc + "/")
            m = re.findall(r"CALCED: openshells=(\S*) cdfitting=(\S*)", log)
            got = m[-1] if m else None
            sym.check(got == (want, "def2/J"),
                      "ORCA %s RIJCOSX: Open Shells %s, CD Fitting def2/J "
                      "(got %s)" % (mult, want, got), run_tests._tail(log, 60))
        finally:
            shutil.rmtree(data, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "basisbox-localdata")
        error = sym.makeCalculations(folder)
        if error:
            results.fail("calced-basis-box", "cannot make calculations: " + error)
            return
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        try:
            def url(calc):
                return "file://" + os.path.join(folder, "users", "local", calc) + "/"
            for calc, want in (("proj-mopac/ch4", "hidden"),
                               ("proj-g16/w-c2v", "shown")):
                log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                                   ["wait 3000", "ready", "info", "quit"],
                                   url(calc))
                sym.check(box(log) == want, "%s: Basis Set box %s (got %s)"
                          % (calc, want, box(log)), run_tests._tail(log, 30))
            os.environ["ECCE_TEST_DIALOG_CLOSE"] = "2"
            os.environ["ECCE_TEST_DIALOG_ANSWER"] = "yes"
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                               ["wait 3000", "code QuantumESPRESSO", "wait 4000",
                                "info", "quit"], url("proj-g16/w-c2v"))
            sym.check(box(log) == "hidden",
                      "Gaussian switched to QuantumESPRESSO: box hidden (got %s)"
                      % box(log), "\n".join(l for l in log.splitlines()
                      if "SCRIPT" in l or "DIALOG" in l or "FAIL" in l))
        finally:
            restore()
            for var in ("ECCE_LOCAL_DATA", "ECCE_TEST_DIALOG_CLOSE",
                        "ECCE_TEST_DIALOG_ANSWER"):
                os.environ.pop(var, None)
        restore = fixture.settleUpgradeNotices()
        try:
            orcaCase(display)
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-basis-box", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
