#!/usr/bin/env python3
"""Workflows (#341): a workflow of linked steps, made as the code buttons
make them, through the Organizer's ECCE_TEST_ORGANIZER hook.

    tests/apps/workflow_test.py [--shots DIR]

An NWChem calculation with a result is the first step; an xTB step is made
from it (final geometry, charge and multiplicity carried), an ORCA step from
that, and a second ORCA step as a branch.  Checks the carried geometry,
charge and multiplicity, the stored "geometry" links, which steps Launch
Ready Steps would launch, and that no link label in the drawing touches a
box, a line, an arrow or another label.  A right-click on each box builds its
context menu without an exception (#681).  --shots keeps the screenshots of
the view and the New-step dialog.

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
CALC = os.path.join(HERE, "fixtures", "calc-water-opt")
# orca-1 twice: the second right-click is on the box already selected.
MENU_STEPS = ("orca-1", "xtb-1", "nw-water", "orca-1")

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def check(what, cond, why):
    if cond:
        print("ok    " + what)
    else:
        fail(what, why)


def steps(info):
    """name -> {field: value} from a workflowinfo answer."""
    table = {}
    body = info.split(" links:")[0]
    for entry in body.split("|"):
        f = entry.split()
        if len(f) < 4:
            continue
        rec = dict(zip(("name", "code", "state", "source"), f[:4]))
        for kv in f[4:]:
            k, _, v = kv.partition("=")
            rec[k] = v
        table[f[0]] = rec
    return table


def run(display, data, work, shots):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "flows")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    shutil.copytree(CALC, os.path.join(project, "water"), symlinks=True)
    url = "file://" + root
    proj = url + "/flows"
    wf = proj + "/Workflow"
    png = os.path.join(shots, "workflow-view.png")
    dlg = os.path.join(shots, "workflow-new-step.png")
    commands = [
        "summary %s/water" % proj,
        "new %s workflow" % proj,
        "workflowcopy %s %s/water nw-water" % (wf, proj),
        "workflowstep %s xtb_es xtb-1 %s/nw-water charge=1 mult=2" % (wf, wf),
        "workflowstep %s orca_es orca-1 %s/xtb-1" % (wf, wf),
        "workflowstep %s orca_es orca-2 %s/xtb-1 nocharge" % (wf, wf),
        "workflowstep %s orca_es orca-3 %s/xtb-1 nogeom" % (wf, wf),
        "workflowinfo %s" % wf,
        "workflowready %s" % wf,
        "workflowshot %s %s %s/orca-1" % (png, wf, wf),
        "workflowdialogshot %s %s orca_es %s/xtb-1" % (dlg, wf, wf),
    ] + ["workflowmenu %s %s/%s" % (wf, wf, n) for n in MENU_STEPS]
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    env = {"ECCE_TEST_ORGANIZER": script}
    result = apps.run(display, "organizer", windowTimeout=60, settle=75,
                      env=env)
    log = result.log or ""
    what = "Workflow"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)

    def answer(prefix):
        got = re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % re.escape(prefix),
                         log, re.M)
        return got

    made = answer("new ")
    check("File > New workflow", made and made[0] == "ok",
          "new answered %r" % made)
    for prefix in ("workflowcopy", "workflowstep"):
        for line in answer(prefix):
            print("      %s -> %s" % (prefix, line[:100]))
    info = answer("workflowinfo")
    if not info or not info[0].startswith("ok "):
        return fail(what, "no workflowinfo answer", log)
    table = steps(info[0][3:])
    links = info[0].split(" links:")[1].split()
    check("five steps", set(table) == {"nw-water", "xtb-1", "orca-1", "orca-2",
                                       "orca-3"}, "steps %s" % sorted(table))
    if not {"nw-water", "xtb-1", "orca-1", "orca-2", "orca-3"} <= set(table):
        return
    nw, xtb, o1, o2, o3 = (table[k] for k in
                           ("nw-water", "xtb-1", "orca-1", "orca-2", "orca-3"))
    check("codes", (nw["code"], xtb["code"], o1["code"]) ==
          ("NWChem", "xtb", "ORCA"), "%s %s %s" % (nw["code"], xtb["code"],
                                                   o1["code"]))
    check("links stored with the name geometry",
          sorted(links) == sorted(["nw-water>xtb-1:geometry",
                                   "xtb-1>orca-1:geometry",
                                   "xtb-1>orca-2:geometry",
                                   "xtb-1>orca-3:geometry"]), "links %s" % links)
    check("source has a result geometry", "final0" in nw, "nw-water: %r" % nw)
    check("xTB step starts from the final geometry",
          xtb.get("xyz0") == nw.get("final0") and xtb.get("xyz0") != nw.get("xyz0"),
          "xtb %s, nw final %s input %s" % (xtb.get("xyz0"), nw.get("final0"),
                                            nw.get("xyz0")))
    check("atoms carried", xtb["atoms"] == nw["atoms"] != "0",
          "%s vs %s" % (xtb["atoms"], nw["atoms"]))
    check("ORCA step starts from the xTB step's input geometry",
          o1.get("xyz0") == xtb.get("xyz0"),
          "orca-1 %s xtb-1 %s" % (o1.get("xyz0"), xtb.get("xyz0")))
    check("charge and multiplicity carried (xtb-1 -> orca-1)",
          (o1["charge"], o1["mult"]) == ("1", "2") == (xtb["charge"], xtb["mult"]),
          "xtb %s/%s orca-1 %s/%s" % (xtb["charge"], xtb["mult"],
                                      o1["charge"], o1["mult"]))
    check("charge and multiplicity left out on request",
          o2["charge"] == "0" and o2["mult"] != "2" and o2.get("xyz0") == xtb.get("xyz0"),
          "orca-2 %s/%s" % (o2["charge"], o2["mult"]))
    check("geometry left out on request", o3["atoms"] in ("0", "-1"),
          "orca-3 atoms %s" % o3["atoms"])

    ready = answer("workflowready")
    check("Launch Ready Steps: only steps whose source is Complete",
          bool(ready) and "waiting: orca-1 orca-2 orca-3" in ready[0] and
          "xtb-1" in ready[0].split("waiting:")[0],
          "ready answered %r (states %s)" % (ready, [(k, v["state"]) for k, v in table.items()]))

    shot = answer("workflowshot")
    check("view saved, five boxes, four links, labels clear",
          bool(shot) and shot[0].startswith("ok boxes=5 links=4 label-overlaps=0"),
          "workflowshot answered %r" % shot)
    check("view screenshot exists", os.path.getsize(png) > 5000
          if os.path.exists(png) else False, png)
    menus = answer("workflowmenu")
    check("right-click menu on every box (%s)" % " ".join(MENU_STEPS),
          len(menus) == len(MENU_STEPS) and
          all(m.startswith("ok ") and "Rename" in m for m in menus),
          "workflowmenu answered %r" % menus)
    ds = answer("workflowdialogshot")
    check("New-step dialog screenshot", bool(ds) and ds[0] == "ok" and
          os.path.exists(dlg), "answered %r" % ds)


def main():
    keep = None
    if "--shots" in sys.argv:
        keep = sys.argv[sys.argv.index("--shots") + 1]
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
    work = tempfile.mkdtemp(prefix="workflow-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    shots = keep or os.path.join(work, "shots")
    os.makedirs(shots, exist_ok=True)
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work, shots)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
