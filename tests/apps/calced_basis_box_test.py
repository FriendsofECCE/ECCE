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
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402


def box(log):
    m = re.findall(r"CALCED: basisbox=(\w+)", log)
    return m[-1] if m else None


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
        if sym.failures:
            results.fail("calced-basis-box", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
