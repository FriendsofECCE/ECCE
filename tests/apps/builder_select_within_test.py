#!/usr/bin/env python3
"""Builder distance selection on three waters, including undo/redo (#662)."""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402

STRUCTURE = os.path.join(HERE, "fixtures", "select-within-waters.car")
SCRIPT = [
    "wait 2000", "expect atoms 9", "select 1", "select_within 3.0", "selection",
    "key viewer undo", "selection",
    "menu Edit|Redo Select Within Distance", "selection",
    "select 1", "select_within 3.0 molecules", "selection",
    "select 1", "select_within 3.0 molecules replace", "selection",
    "select 1", "select_within 3.0 replace", "selection",
    "quit",
]


def run(display):
    work = os.path.join(fixture.stateHome(), "builder-select-within")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    path = os.path.join(work, "select.script")
    with open(path, "w") as handle:
        handle.write("\n".join(SCRIPT) + "\n")
    try:
        #  A private state directory has no saved login: without -pipe the
        #  builder stops at the "ECCE Authentication" dialog.
        auth = fixture.authFile(os.path.join(work, "auth.pipe"),
                                user=fixture.realUser())
        result = apps.run(display, "builder", args=("-pipe", auth, STRUCTURE),
                          windowTimeout=60, settle=90,
                          env={"ECCE_BUILDER_SCRIPT": path,
                               "ECCE_FEEDBACK_STDERR": "1"})
        if "BUILDER:" not in (result.log or ""):
            print("builder: windows=%s rc=%s crashed=%s note=%s"
                  % (result.windows, result.returncode, result.crashed,
                     result.note))
        return result.log or ""
    finally:
        shutil.rmtree(work, ignore_errors=True)


def problemsIn(log):
    problems = []
    for line in log.splitlines():
        if "ECCE_BUILDER_SCRIPT" in line and "FAIL" in line:
            problems.append(line.strip())
    counts = [int(m) for m in re.findall(r"BUILDER: selection atoms=(\d+)", log)]
    expected = [4, 1, 4, 6, 5, 3]
    if counts != expected:
        problems.append("selected atom counts %s, want %s" % (counts, expected))
    print("%s  select within: atoms, undo/redo, molecules, replacement: %s" %
          ("FAIL" if problems else "PASS", counts))
    return problems


def check(display, results):
    results.checks += 1
    problems = problemsIn(run(display))
    if problems:
        results.fail("builder-select-within", "; ".join(problems))


def main():
    import isolate
    import xdisplay

    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"

    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % log)
                return 1
            fixture.ensureRealUserAccount()
            restore = fixture.settleUpgradeNotices()
            try:
                out = run(display)
                problems = problemsIn(out)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    if problems:
        print("\n".join("      | " + l for l in out.splitlines()[-25:]))
        print("FAIL  builder select within")
        return 1
    print("PASS  builder select within")
    return 0


if __name__ == "__main__":
    sys.exit(main())
