#!/usr/bin/env python3
"""Run Mgmt > Re-Parse Outputs (#651): the parsers run again over the outputs
stored with a calculation, with no login, and the run state, dates and status
reason stay as they were.

    tests/apps/reparse_outputs_test.py       own Xvfb, services, local data

A MOPAC and an ORCA output are imported (the import stores their outputs and
parses them). The parsed properties are then deleted from the data folder, and
the Organizer hook `reparseoutputs URL...` -- what the menu item calls -- runs
the job store over the stored outputs. The properties must come back, and the
run-state, start/completion-date and status-reason metadata must be exactly
what they were before.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS). Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
FIXTURES = os.path.join(HERE, "..", "parsers", "fixtures")
# CP2K and Quantum ESPRESSO have no importer: no stored calculation of
# theirs can be made here.
OUTPUTS = ["mopac/ch4_opt.out", "orca/h2o_sym.out"]
STATE_KEYS = re.compile(r"ecce:(state|startdate|completiondate|"
                        r"runStatusReason)\b")
failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def calculations(project):
    """Directories below the project that hold a Props folder."""
    found = []
    for base, dirs, _files in os.walk(project):
        if "Props" in dirs:
            found.append(base)
            dirs[:] = []
    return sorted(found)


def props(calc):
    path = os.path.join(calc, "Props")
    return sorted(os.listdir(path)) if os.path.isdir(path) else []


def stateLines(calc):
    with open(os.path.join(calc, ".ecce-meta")) as handle:
        return sorted(l for l in handle.read().split("\n")
                      if STATE_KEYS.search(l))


def waitFor(test, seconds):
    end = time.time() + seconds
    while time.time() < end:
        if test():
            return True
        time.sleep(1)
    return False


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "session-test")
    first = os.path.join(work, "import.commands")
    with open(first, "w") as handle:
        for name in OUTPUTS:
            handle.write("import %s\n" % os.path.abspath(
                os.path.join(FIXTURES, name)))

    def waitImport(_display):
        waitFor(lambda: len(calculations(project)) == len(OUTPUTS) and
                all(props(c) for c in calculations(project)), 120)
        time.sleep(20)

    apps.run(display, "organizer", windowTimeout=60, settle=20,
             env={"ECCE_TEST_ORGANIZER": first}, inspect=waitImport)

    calcs = calculations(project)
    if len(calcs) != len(OUTPUTS):
        return fail("setup", "imported %d of %d calculations: %s"
                    % (len(calcs), len(OUTPUTS), calcs))
    before = {c: (props(c), stateLines(c)) for c in calcs}
    for calc, (have, _state) in before.items():
        if not have:
            return fail("setup", "the import of %s stored no properties" % calc)
        shutil.rmtree(os.path.join(calc, "Props"))
        os.makedirs(os.path.join(calc, "Props"))

    second = os.path.join(work, "reparse.commands")
    with open(second, "w") as handle:
        handle.write("reparseoutputs %s\n"
                     % " ".join("file://" + c for c in calcs))

    def waitReparse(_display):
        waitFor(lambda: all(props(c) for c in calcs), 120)
        time.sleep(15)

    result = apps.run(display, "organizer", windowTimeout=60, settle=10,
                      env={"ECCE_TEST_ORGANIZER": second},
                      inspect=waitReparse)
    log = result.log or ""
    if result.crashed:
        return fail("Re-Parse Outputs", "CRASHED (%s)" % result.signalName, log)
    if "REPARSE: Re-parse finished: %d done" % len(calcs) not in log:
        fail("finished message", "no 'Re-parse finished: %d done' in the "
             "Organizer's log" % len(calcs), log)
    for calc in calcs:
        name = os.path.basename(calc)
        had, state = before[calc]
        got = props(calc)
        if not got:
            fail(name, "no properties after the re-parse", log)
        elif sorted(got) != sorted(had):
            fail(name, "properties differ: before %s, after %s" % (had, got))
        elif stateLines(calc) != state:
            fail(name, "state/date/reason changed: %s -> %s"
                 % (state, stateLines(calc)))
        else:
            print("ok    %s: %d properties, state and dates unchanged"
                  % (name, len(got)))


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb") or not shutil.which("perl"):
        print("SKIP: no Xvfb or perl")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings), flush=True)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    os.environ.pop("DISPLAY", None)
    work = tempfile.mkdtemp(prefix="reparse-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
