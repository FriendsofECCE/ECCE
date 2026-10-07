#!/usr/bin/env python3
"""
The Organizer's calculation summary shows the molecule and the basis set.

A calculation keeps its formula, atom and electron counts, symmetry and
basis-set figures on its Parameters documents (the molecule and the basis
set), not on itself; the summary panel reads them from the calculation, so
the data layer has to gather them from below it.  The Organizer's
ECCE_TEST_ORGANIZER hook selects a calculation and prints the panel's
fields ("summary <url>") and saves the window ("snap <png>"), for:

- a data-server calculation: fixtures/calc-water-vib (Gaussian-16,
  def2-SVP) in the user's account;
- a local-data calculation (ECCE_LOCAL_DATA): fixtures/calc-water-opt
  (NWChem, 6-31G*).

ECCE_SUMMARY_PNGS=<dir> keeps the two window captures there.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
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

#  The panel's values, from the fixtures' stored metadata.
EXPECT = {
    "server": {"Formula": "H2O", "Atoms": "3", "Electrons": "10",
               "Symmetry": "C2V", "Basis": "def2-svp",
               "Functions": "24", "Primitives": "38",
               "Theory": "DFT/RDFT", "Runtype": "Vibration"},
    "local": {"Formula": "H2O", "Atoms": "3", "Electrons": "10",
              "Symmetry": "C2v", "Basis": "6-31G*", "Polarization": "Cartesian",
              "Functions": "19", "Primitives": "36",
              "Theory": "SCF/RHF", "Runtype": "Geometry"},
}
FIELDS = ("Formula", "Atoms", "Electrons", "Symmetry", "Basis",
          "Polarization", "Functions", "Primitives", "Theory", "Runtype")

failures = []


def parse(outcome):
    """'ok Formula=H2O Atoms=3 ...' -> dict; values hold no spaces here."""
    values = {}
    for name in FIELDS:
        m = re.search(r"\b%s=(\S*)" % name, outcome)
        if m:
            values[name] = m.group(1)
    return values


def organizer(display, url, png, env, args=()):
    work = tempfile.mkdtemp(prefix="ecce-summary-")
    cmds = os.path.join(work, "commands")
    with open(cmds, "w") as handle:
        handle.write("summary %s\nsnap %s\n" % (url, png))
    try:
        result = apps.run(display, "organizer", args=args, windowTimeout=60,
                          settle=40,
                          env=dict(env, ECCE_TEST_ORGANIZER=cmds,
                                   ECCE_ORGANIZER_OPEN=url))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return result


def check(label, result, expect):
    log = result.log or ""
    done = re.search(r"ECCE_TEST_ORGANIZER: summary \S+: (.*)", log)
    problem = None
    if result.crashed:
        problem = "CRASHED (%s)" % result.signalName
    elif not done:
        problem = "the summary command never answered"
    else:
        got = parse(done.group(1))
        wrong = ["%s=%r (want %r)" % (k, got.get(k), v)
                 for k, v in sorted(expect.items()) if got.get(k) != v]
        if wrong:
            problem = "summary fields wrong: " + ", ".join(wrong)
    if problem is None:
        print("ok    %s: %s" % (label, done.group(1)))
        return
    failures.append(label)
    print("FAIL  %s: %s\n%s" % (label, problem, run_tests._tail(log, 25)))


def serverCase(display, pngs):
    user = fixture.realUser()
    userRoot = os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users",
                            user)
    project = os.path.join(userRoot, "summary-test")
    shutil.rmtree(project, ignore_errors=True)
    os.makedirs(project)
    shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-vib"),
                    os.path.join(project, "calc-water-vib"), symlinks=True)
    url = "http://localhost:%d/Ecce/users/%s/summary-test/calc-water-vib" % (
        fixture.dataserverPort(), user)
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"), user=user)
    try:
        result = organizer(display, url, os.path.join(pngs, "server.png"), {},
                           args=("-pipe", authPath))
        check("data-server calculation (Gaussian-16)", result,
              EXPECT["server"])
    finally:
        shutil.rmtree(project, ignore_errors=True)


def localCase(display, pngs):
    data = tempfile.mkdtemp(prefix="ecce-summary-data-")
    try:
        project = os.path.join(data, "users", "local", "summary-test")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "water-opt")
        shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-opt"),
                        calc, symlinks=True)
        result = organizer(display, "file://" + calc,
                           os.path.join(pngs, "local.png"),
                           {"ECCE_LOCAL_DATA": data})
        check("local-data calculation (NWChem)", result, EXPECT["local"])
    finally:
        shutil.rmtree(data, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        keep = os.environ.get("ECCE_SUMMARY_PNGS")
        pngs = keep or tempfile.mkdtemp(prefix="ecce-summary-png-")
        os.makedirs(pngs, exist_ok=True)
        try:
            serverCase(display, pngs)
            localCase(display, pngs)
        finally:
            if not keep:
                shutil.rmtree(pngs, ignore_errors=True)
        if failures:
            results.fail("summary", "summary panel wrong: %s"
                         % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
