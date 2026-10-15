#!/usr/bin/env python3
"""Every modal dialog is shown without the busy cursor, and presented.

    tests/apps/modal_guard_test.py

ewxModalGuard (a wxModalDialogHook installed by ewxApp) ends any busy
cursor while a modal dialog is up, restores it afterwards, and presents the
dialog once it is shown.  The Organizer's ECCE_TEST_ORGANIZER commands show
two text prompts under a menu action's busy cursor and window disabler:
"busyprompt" (a plain prompt) and "newprompt" (File > New calculation with
the name prompt).  ECCE_DEBUG_MODAL logs what each dialog met.

Under Xvfb this checks the bookkeeping, not what Mutter does with the
dialog on a Wayland session; that is checked by hand.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-20:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def check(what, ok, why, log):
    if ok:
        print("ok    %s" % what)
    else:
        fail(what, why, log)


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "alpha")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    url = "file://" + root
    commands = ["busyprompt", "newprompt %s/alpha NWChem" % url]
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    modal = os.path.join(work, "modal.log")
    result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_DEBUG_MODAL": modal})
    log = result.log or ""
    if result.crashed:
        return fail("Organizer", "CRASHED (%s)" % result.signalName, log)
    trace = open(modal).read() if os.path.exists(modal) else ""
    both = log + "\n--- ECCE_DEBUG_MODAL ---\n" + trace

    done = re.search(r"ECCE_TEST_ORGANIZER: busyprompt: (\S+) busyAfter=(\d)",
                     log)
    up = re.search(r"busyprompt: up busy=(\d) parentEnabled=(-?\d)", log)
    check("busy prompt answered", done and done.group(1) == "ok",
          "no answer", both)
    check("no busy cursor while the busy prompt is up",
          up and up.group(1) == "0", "busy while up: %s" %
          (up.group(0) if up else "not seen"), both)
    check("busy cursor back after the busy prompt",
          done and done.group(2) == "1", "busyAfter=%s" %
          (done.group(2) if done else "?"), both)
    check("guard saw the busy cursor at entry",
          re.search(r"enter 'Busy Prompt' .* busy=1 ", trace),
          "no 'enter ... busy=1' line", both)
    check("busy prompt presented without a busy cursor",
          re.search(r"present 'Busy Prompt' busyNow=0", trace),
          "no 'present ... busyNow=0' line", both)

    check("name prompt answered",
          re.search(r"ECCE_TEST_ORGANIZER: newprompt .*: ok", log),
          "newprompt did not answer ok", both)
    name = re.search(r"enter '(New [^']* Name)' .* parentEnabled=(\d)", trace)
    check("name prompt's parent enabled", name and name.group(2) == "1",
          "parent state: %s" % (name.group(0) if name else "not seen"), both)
    check("name prompt presented without a busy cursor",
          name and re.search(r"present '%s' busyNow=0" %
                             re.escape(name.group(1)), trace),
          "no 'present ... busyNow=0' line", both)
    exits = re.findall(r"exit '[^']*' code=\d+ .* busyAfter=\d", trace)
    check("every dialog left the guard", len(exits) >= 2,
          "%d exit lines" % len(exits), both)


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="modalguard-",
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
