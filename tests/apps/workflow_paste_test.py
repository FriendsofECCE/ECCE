#!/usr/bin/env python3
"""Copy, cut and Paste of workflow steps in the Organizer (#935), through
the GUI path (Copy/Cut + Paste, Resource::copy/move), not ecce-copy.

    tests/apps/workflow_paste_test.py

A workflow of two linked steps; step 1 is copied to the project folder, step
2 cut to it, and an ordinary calculation pasted into the workflow, which is
refused.  Same installed tree and isolation as workflow_test.py.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402
import workflow_test as wt   # noqa: E402

SKIP = 77


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "flows")
    os.makedirs(project)
    shutil.copy(wt.PROJECT_META, os.path.join(project, ".ecce-meta"))
    shutil.copytree(wt.CALC, os.path.join(project, "water"), symlinks=True)
    proj = "file://" + root + "/flows"
    wf = proj + "/Workflow"
    commands = [
        "summary %s/water" % proj,
        "new %s workflow" % proj,
        "workflowcopy %s %s/water nw-water" % (wf, proj),
        "workflowstep %s xtb_es xtb-1 %s/nw-water" % (wf, wf),
        "workflowinfo %s" % wf,
        "server paste copy %s %s/nw-water" % (proj, wf),
        "workflowinfo %s" % wf,
        "server paste cut %s %s/xtb-1" % (proj, wf),
        "workflowinfo %s" % wf,
        "server paste copy %s %s/water" % (wf, proj),
        "workflowinfo %s" % wf,
    ]
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=75,
                      env={"ECCE_TEST_ORGANIZER": script})
    log = result.log or ""
    what = "Workflow paste"
    if result.crashed:
        return wt.fail(what, "CRASHED (%s)" % result.signalName, log)

    def answer(prefix):
        return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % re.escape(prefix),
                          log, re.M)

    info = answer("workflowinfo")
    paste = answer("server paste")
    print("      paste answers: %s" % paste)
    if len(info) != 4 or len(paste) != 3:
        return wt.fail(what, "answers: info %s paste %s" % (info, paste), log)
    flow = os.path.join(root, "flows")

    def exists(*p):
        return os.path.exists(os.path.join(flow, *p))

    def names(i):
        return set(wt.steps(info[i][3:]))

    def links(i):
        return info[i].split(" links:")[1].split()

    wt.check("the workflow has two linked steps",
             names(0) == {"nw-water", "xtb-1"} and
             any("nw-water>xtb-1" in l for l in links(0)),
             "%s links %s" % (names(0), links(0)))
    wt.check("copying step 1 to the folder is accepted", paste[0] == "ok",
             paste[0])
    wt.check("the copy is a calculation in the folder with its files",
             exists("nw-water", ".ecce-meta") and
             len(os.listdir(os.path.join(flow, "nw-water"))) >=
             len(os.listdir(os.path.join(flow, "Workflow", "nw-water"))) - 2,
             str(os.listdir(flow)))
    wt.check("the workflow keeps both steps and its link",
             names(1) == {"nw-water", "xtb-1"} and
             any("nw-water>xtb-1" in l for l in links(1)),
             "%s links %s" % (names(1), links(1)))
    wt.check("cutting step 2 to the folder is accepted", paste[1] == "ok",
             paste[1])
    wt.check("the moved step is in the folder and gone from the workflow",
             exists("xtb-1", ".ecce-meta") and not exists("Workflow", "xtb-1"),
             str(os.listdir(flow)))
    wt.check("the workflow has one step and no link to the moved one",
             names(2) == {"nw-water"} and
             not any("xtb-1" in l for l in links(2)),
             "%s links %s" % (names(2), links(2)))
    wt.check("pasting a calculation into the workflow is refused with New step",
             "New step" in paste[2], paste[2])
    wt.check("nothing was made in the workflow",
             not exists("Workflow", "water") and names(3) == {"nw-water"},
             "%s" % sorted(os.listdir(os.path.join(flow, "Workflow"))))


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
    work = tempfile.mkdtemp(prefix="wfpaste-",
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
    print("FAILED: " + ", ".join(wt.failures) if wt.failures else "PASSED")
    return 1 if wt.failures else 0


if __name__ == "__main__":
    sys.exit(main())
