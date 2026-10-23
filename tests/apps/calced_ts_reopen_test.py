#!/usr/bin/env python3
"""
A saved or duplicated ORCA NEB-TS calculation shows its atom mapping without
another Save (#464).

The NEB-TS project of calced_neb_test.py: "ts" gets react as reactant, prod as
product, a basis set, and is saved.  Reopened, and then duplicated as the
Organizer does it (Resource::copy), each copy must open ready (no reason
given, Launch on), with the status line reporting the match and 'Show mapping'
listing the product, without the basis set being touched.

Same installed tree, isolation and services as run_tests.py.  Exit 77 (skip)
without an install or Xvfb.
"""

import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                     # noqa: E402
import calced_qst_test as qst   # noqa: E402
import fixture                  # noqa: E402
import run_tests                # noqa: E402
import symmetry_find_test as sym  # noqa: E402

REOPEN = ["wait 15000", "ts-state", "enabled", "ts-mapping"]


def duplicate(folder, url):
    """The copy's URL, made the way the Organizer's Duplicate makes it."""
    env = dict(os.environ, ECCE_HOME=apps.INSTALL, ECCE_LOCAL_DATA=folder,
               ECCE_NO_MESSAGING="1",
               LD_LIBRARY_PATH=os.environ.get(
                   "ECCE_TEST_BUILD", os.path.join(qst.REPO, "build-cmake")))
    done = subprocess.run([os.path.join(fixture.stateDir(), "resourceTest"),
                           "copy", url.rstrip("/")], env=env,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = done.stdout.decode()
    m = re.search(r"^(file://\S+)$", out, re.M)
    return m.group(1) if done.returncode == 0 and m else None, out[-800:]


def checkOpened(log, what):
    tail = run_tests._tail(log, 30)
    st = re.findall(r"CALCED: ts-state (.*)", log)
    sym.check(len(st) >= 1, "%s: ts-state reported" % what, tail)
    if st:
        line = re.search(r" line=(.*) reason=(.*)$", st[0])
        sym.check(line is not None and line.group(1).startswith("Matched"),
                  "%s: the status line reports the match (%s)"
                  % (what, line.group(1) if line else st[0]), tail)
        sym.check(line is not None and line.group(2).strip() == "",
                  "%s: nothing is missing (%s)"
                  % (what, line.group(2) if line else "?"), tail)
    en = re.search(r"ECCE_CALCED_SCRIPT: enabled: (.*)", log)
    sym.check(en is not None and "launch=on" in en.group(1),
              "%s: Launch is on (%s)" % (what, en.group(1) if en else "?"), tail)
    mp = re.findall(r"ECCE_CALCED_SCRIPT: ts-mapping: (.*)", log)
    sym.check(bool(mp) and "Product" in mp[0] and "No atom mapping" not in mp[0],
              "%s: Show mapping lists the product (%s)"
              % (what, mp[0][:120] if mp else "?"), tail)
    return mp


def main():
    os.environ["ECCE_QST_CODE"] = "ORCA"

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "ts-reopen-localdata")
        urls, err = qst.makeProject(folder)
        if urls is None:
            results.fail("calced-ts-reopen", "could not build the project: %s" % err)
            return
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", [
                "wait 9000", "ts-source other", "wait 1500",
                "ts-pick reactant react", "wait 7000",
                "runtype Transition State (NEB-TS)", "wait 3000", "ready",
                "basis 6-31G", "wait 1500", "ts-pick product prod",
                "button save", "wait 6000", "quit"], urls["ts"] + "/")
            sym.check("Calculation saved as" in log, "the first save",
                      run_tests._tail(log, 30))

            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                               REOPEN + ["quit"], urls["ts"] + "/")
            checkOpened(log, "reopened")

            copy, out = duplicate(folder, urls["ts"])
            sym.check(copy is not None, "Duplicate made a copy", out)
            if copy:
                log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                                   REOPEN + ["button save", "wait 4000",
                                             "ts-mapping", "quit"], copy + "/")
                mp = checkOpened(log, "duplicate")
                sym.check(len(mp) == 2 and mp[0] == mp[1],
                          "duplicate: Save keeps the same mapping",
                          run_tests._tail(log, 30))
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-ts-reopen", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
