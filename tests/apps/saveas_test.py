#!/usr/bin/env python3
"""
Builder > File > Save As... and File > Open..., without clicks.

The Builder's ECCE_TEST_SAVEAS hook adds a structure, opens the dialog,
picks a type, saves to a path on the Local Filesystem and exits;
ECCE_TEST_OPEN opens a path through File > Open... the same way.

- A calculation type saved to a plain local folder is created there (a
  FileEDSI directory), becomes the Builder's context with the structure,
  is listed by File > Open... as a file, not a folder, and opens again
  with the same structure.
- A chemical file type is written, and becomes the context without a
  "Can't open recently saved" warning.
- Opening the Save As dialog must not raise a Gtk-CRITICAL.

Launching such a calculation is tests/launch/run_tests.py --folder.

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

STRUCTURE = os.path.abspath(os.path.join(HERE, "..", "fragreaders", "data",
                                         "glycine.pdb"))
ATOMS = 10

failures = []


def builder(display, **env):
    for key in ("ECCE_TEST_SAVEAS", "ECCE_TEST_OPEN"):
        os.environ.pop(key, None)
    os.environ.update(env)
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
        user=fixture.realUser())
    try:
        return apps.run(display, "builder", args=("-pipe", authPath),
                        windowTimeout=60, settle=60)
    finally:
        for key in env:
            os.environ.pop(key, None)


def report(label, problem, log):
    if problem is None:
        print("ok    %s" % label)
        return True
    failures.append(label)
    print("FAIL  %s: %s\n%s" % (label, problem, run_tests._tail(log, 25)))
    return False


def common(result, hook):
    """The problem shared by every run, or None; and the log after the
    hook started."""
    log = result.log or ""
    done = re.search(r"%s: done, context (\S+), (\d+) atoms" % hook, log)
    if result.crashed:
        return "CRASHED (%s)" % result.signalName, log, done
    if not done:
        return "never finished", log, done
    if "still open" in log:
        return "dialog did not close", log, done
    if "CRITICAL" in log[log.find(hook):]:
        return "Gtk-CRITICAL with the dialog open", log, done
    if "error parsing" in log or "Can't open recently saved" in log:
        return "the result could not be opened", log, done
    return None, log, done


def saveCalc(display, target):
    calc = os.path.join(target, "calc")
    result = builder(display, ECCE_TEST_SAVEAS="ORCA|%s|%s" % (calc, STRUCTURE))
    problem, log, done = common(result, "ECCE_TEST_SAVEAS")
    if problem is None:
        context, atoms = done.group(1), int(done.group(2))
        if not os.path.isdir(calc):
            problem = "%s is not a directory" % calc
        elif not os.path.isfile(os.path.join(calc, ".ecce-meta")):
            problem = "%s has no .ecce-meta" % calc
        elif "orca" not in open(os.path.join(calc, ".ecce-meta"),
                                errors="replace").read().lower():
            problem = "%s/.ecce-meta does not name the ORCA type" % calc
        elif not context.rstrip("/").endswith(calc):
            problem = "the context is %s, not the new calculation" % context
        elif atoms != ATOMS:
            problem = "%d atoms in the saved calculation, expected %d" % (
                atoms, ATOMS)
    if report("Save As ORCA -> local folder", problem, log):
        return calc
    return None


def openCalc(display, calc):
    result = builder(display, ECCE_TEST_OPEN=calc)
    problem, log, done = common(result, "ECCE_TEST_OPEN")
    listed = re.search(r"ECCE_TEST_OPEN: listed as (\w+)", log)
    if problem is None:
        context, atoms = done.group(1), int(done.group(2))
        if not listed or listed.group(1) != "file":
            problem = "Open... lists the calculation as %s" % (
                listed.group(1) if listed else "nothing")
        elif not context.rstrip("/").endswith(calc):
            problem = "the context is %s, not the calculation" % context
        elif atoms != ATOMS:
            problem = "%d atoms after reopening, expected %d" % (atoms, ATOMS)
    report("Open... the saved calculation", problem, log)


def saveXyz(display, target, name, structure):
    """structure None: the Builder's empty structure, whose file (0 atoms)
    no reader opens; it is written and the context stays, unannounced."""
    spec = "XYZ|%s" % os.path.join(target, name)
    if structure:
        spec += "|" + structure
    result = builder(display, ECCE_TEST_SAVEAS=spec)
    problem, log, done = common(result, "ECCE_TEST_SAVEAS")
    path = os.path.join(target, name + ".xyz")
    if problem is None:
        if not os.path.isfile(path):
            problem = "%s not written" % path
        elif structure and not done.group(1).endswith(name + ".xyz"):
            problem = "the context is %s, not the file" % done.group(1)
    report("Save As XYZ (%s) -> local folder"
           % ("structure" if structure else "empty"), problem, log)


def main():
    def checkApp(display, name, results, verbose=False):
        target = tempfile.mkdtemp(prefix="ecce-saveas-")
        try:
            calc = saveCalc(display, target)
            if calc:
                openCalc(display, calc)
            saveXyz(display, target, "plain", STRUCTURE)
            saveXyz(display, target, "empty", None)
        finally:
            shutil.rmtree(target, ignore_errors=True)
        if failures:
            results.fail("saveas", "Save As/Open cases wrong: %s"
                         % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
