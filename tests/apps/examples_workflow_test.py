#!/usr/bin/env python3
"""The Workflows example set (#341): File > Open Example Calculations >
Workflows imports Na hydration as a Workflow session, not as loose calculations.

    tests/apps/examples_workflow_test.py [--shots DIR]

Checks the nine steps (three xTB, three ORCA gas, three ORCA water), the
"geometry" links xTB > gas > water, that each water step starts from the Hessian
of its gas step, the charge and multiplicity of every step, and that
no label of the drawing touches a box, a line or an arrow.  --shots keeps the
screenshot of the view.  A right-click on an ORCA and an xTB box builds the
context menu without an exception (#681); the first-start import of the
example sets runs in the same start.

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
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
ARRANGEMENTS = ("6_0", "5_1", "4_2")

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
    for entry in info.split(" links:")[0].split("|"):
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
    wf = "file://" + root + "/Examples/Workflows/Na_hydration"
    png = os.path.join(shots, "workflow-na-hydration.png")
    menus = ["workflowmenu %s %s/na_6_0_%s" % (wf, wf, k)
             for k in ("orca", "xtb", "orca")]
    commands = ["examples workflows"] + menus + ["workflowinfo " + wf,
                "workflowshot %s %s" % (png, wf)]
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=90,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_NO_EXAMPLE_IMPORT": ""})
    log = result.log or ""
    what = "Workflows example"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)

    def answer(prefix):
        return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$"
                          % re.escape(prefix), log, re.M)

    info = answer("workflowinfo")
    if not info or not info[0].startswith("ok "):
        return fail(what, "no workflowinfo answer %r" % info, log)
    table = steps(info[0][3:])
    links = info[0].split(" links:")[1].split()
    expect = {"na_%s_%s" % (n, k) for n in ARRANGEMENTS
              for k in ("xtb", "orca", "orca_water")}
    check("nine steps", set(table) == expect, "steps %s" % sorted(table))
    check("codes", all(table.get("na_%s_xtb" % n, {}).get("code") == "xtb" and
                       table.get("na_%s_%s" % (n, e), {}).get("code") == "ORCA"
                       for n in ARRANGEMENTS for e in ("orca", "orca_water")),
          "%s" % table)
    check("xTB > gas > water links",
          sorted(links) == sorted(
              ["na_%s_xtb>na_%s_orca:geometry" % (n, n) for n in ARRANGEMENTS] +
              ["na_%s_orca>na_%s_orca_water:geometry" % (n, n)
               for n in ARRANGEMENTS]), "links %s" % links)
    check("water steps carry the Hessian of their gas step, no other step does",
          all(table["na_%s_orca_water" % n].get("hess") == "na_%s_orca" % n
              for n in ARRANGEMENTS) and
          not any("hess" in r for k, r in table.items()
                  if not k.endswith("_water")), "%s" % table)
    check("charge 1 and multiplicity 1 on every step",
          all(r.get("charge") == "1" and r.get("mult") == "1"
              for r in table.values()), "%s" % table)
    got = answer("workflowmenu")
    check("right-click menu on ORCA and xTB boxes",
          len(got) == 3 and all(m.startswith("ok ") and "Rename" in m
                                for m in got),
          "workflowmenu answered %r" % got)
    shot = answer("workflowshot")
    check("view saved, nine boxes, six links, labels clear",
          bool(shot) and shot[0].startswith("ok boxes=9 links=6 label-overlaps=0"),
          "workflowshot answered %r" % shot)
    check("view screenshot exists",
          os.path.exists(png) and os.path.getsize(png) > 5000, png)


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
    work = tempfile.mkdtemp(prefix="examples-workflow-",
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
