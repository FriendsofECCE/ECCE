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

A second workflow (#674) holds a stored ECCE-QM relaxed scan of butane (25
points, stored per-point structures) and two copies of it as reactant and
product.  A link from the scan carries its maximum and its last point; a join
of the three (reactant, product, scan maximum as TS guess) makes a G16 QST3
step and a join of two a QST2.  Checks the link names, the step's structure
(the point's, from the scan's own per-point table), the run type, that the
three structure files TS Search writes list the atoms in one order and the
third is the maximum, and that the drawing with joins has clear labels.

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


SCAN = os.path.join(ROOT, "data", "client", "examples", "organic",
                    "conformations", "ecceqm-butane-scan", "stored",
                    "butane_scan")


def numbers(text):
    return [float(v) for v in re.findall(r"-?\d+\.\d+(?:[eE][-+]?\d+)?", text)]


def scan_points():
    """(energies, [flat xyz per point]) of the stored scan."""
    energy = open(os.path.join(SCAN, "Props", "SCANENERGY")).read()
    energy = energy.split(">", 2)[2]
    geom = open(os.path.join(SCAN, "Props", "SCANGEOM")).read()
    steps = re.findall(r"<step number=\"\d+\">(.*?)</step>", geom, re.S)
    return numbers(energy.split("</vector>")[0]), [numbers(t) for t in steps]


def pair_sum(xyz):
    pts = [xyz[i:i + 3] for i in range(0, len(xyz), 3)]
    return sum(sum((a - b) ** 2 for a, b in zip(p, q)) ** 0.5
               for i, p in enumerate(pts) for q in pts[i + 1:])


def frag_atoms(path):
    rows = open(path).read().split("atom_list:")[1].split("\n")
    out = [r.split() for r in rows if len(r.split()) == 4]
    return [r[0] for r in out], [float(v) for r in out for v in r[1:]]


def run_joins(display, data, work, shots):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "joins")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    shutil.copytree(SCAN, os.path.join(project, "butscan"), symlinks=True)
    proj = "file://" + root + "/joins"
    wf = proj + "/Workflow"
    png = os.path.join(shots, "workflow-join.png")
    g16 = "gaussian16_es"
    commands = [
        "summary %s/butscan" % proj,
        "new %s workflow" % proj,
        "workflowcopy %s %s/butscan scan" % (wf, proj),
        "workflowcopy %s %s/butscan react" % (wf, proj),
        "workflowcopy %s %s/butscan prod" % (wf, proj),
        "workflowstep %s %s pt-max %s/scan point=max" % (wf, g16, wf),
        "workflowstep %s %s pt-last %s/scan point=last" % (wf, g16, wf),
        "workflowstep %s %s pt-bad %s/scan point=99" % (wf, g16, wf),
        "workflowjoin %s %s qst3 %s/react=reactant %s/prod=product "
        "%s/scan=tsguess@max" % (wf, g16, wf, wf, wf),
        "workflowjoin %s %s qst2 %s/react=reactant %s/prod=product" % (wf, g16, wf, wf),
        "workflowjoin %s %s noprod %s/react=reactant %s/prod=reactant" % (wf, g16, wf, wf),
        "workflowjoin %s xtb_es xtbjoin %s/react=reactant %s/prod=product" % (wf, wf, wf),
        "workflowinfo %s" % wf,
        "workflowready %s" % wf,
        "workflowqst %s %s/qst3" % (wf, wf),
        "workflowqst %s %s/qst2" % (wf, wf),
        "workflowshot %s %s %s/qst3" % (png, wf, wf),
    ]
    script = os.path.join(work, "commands-joins")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=75,
                      env={"ECCE_TEST_ORGANIZER": script})
    log = result.log or ""
    what = "Joins"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)

    def answer(prefix):
        return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % re.escape(prefix),
                          log, re.M)

    for prefix in ("workflowstep", "workflowjoin"):
        for line in answer(prefix):
            print("      %s -> %s" % (prefix, line[:160]))
    info = answer("workflowinfo")
    if not info or not info[0].startswith("ok "):
        return fail(what, "no workflowinfo answer", log)
    table = steps(info[0][3:])
    links = info[0].split(" links:")[1].split()
    check("steps made", {"scan", "react", "prod", "pt-max", "pt-last", "qst3",
                         "qst2"} <= set(table), "steps %s" % sorted(table))
    check("a scan point that does not exist makes no step",
          "pt-bad" not in table and any("pt-bad" in l or "points 1 to 25" in l
                                        for l in answer("workflowstep")),
          "steps %s, answers %s" % (sorted(table), answer("workflowstep")))
    check("a join needs a reactant and a product, one code, G16 or ORCA",
          "noprod" not in table and "xtbjoin" not in table,
          "steps %s" % sorted(table))
    check("links carry the point and the role",
          {"scan>pt-max:geometry@max", "scan>pt-last:geometry@last",
           "react>qst3:reactant", "prod>qst3:product", "scan>qst3:tsguess@max",
           "react>qst2:reactant", "prod>qst2:product"} <= set(links),
          "links %s" % links)
    if not {"pt-max", "pt-last", "qst3"} <= set(table):
        return
    energy, frames = scan_points()
    top = max(range(len(energy)), key=lambda i: energy[i])

    def atom0(rec, key):
        return [float(v) for v in rec[key].split(",")]

    for name, frame in (("pt-max", top), ("pt-last", len(frames) - 1)):
        got = atom0(table[name], "xyz0")
        want = frames[frame][:3]
        check("%s starts from scan point %d" % (name, frame + 1),
              all(abs(a - b) < 1e-4 for a, b in zip(got, want)),
              "%s vs %s" % (got, want))
    check("the maximum is not the last point",
          pair_sum(frames[top]) != pair_sum(frames[-1]) and top != len(frames) - 1,
          "max at %d of %d" % (top + 1, len(frames)))
    q = table["qst3"]
    check("QST3 step is G16 with the reactant's structure",
          q["code"] == "G16" and q["atoms"] == "14" and
          q.get("xyz0") == table["react"].get("final0"),
          "qst3 %s react %s" % (q, table["react"]))
    ready = answer("workflowready")
    check("a join is ready when all its sources are complete",
          bool(ready) and "qst3" in ready[0].split("waiting:")[0],
          "ready answered %r" % ready)

    asked = answer("workflowqst")
    check("TS Search structure files written for QST3 and QST2",
          len(asked) == 2 and all(a.startswith("ok ") for a in asked),
          "workflowqst answered %r" % asked)
    if len(asked) == 2 and all(a.startswith("ok ") for a in asked):
        d3, d2 = (a.split()[1] for a in asked)
        check("QST3 run type", "runtype=Transition State (QST) + Frequencies" in asked[0]
              and "Matched 14 atoms" in asked[0], asked[0])
        frags = [os.path.join(d3, "ts.qst%d.frag" % k) for k in (1, 2, 3)]
        check("three structure files for QST3, two for QST2",
              all(os.path.exists(f) for f in frags) and
              os.path.exists(os.path.join(d2, "ts.qst2.frag")) and
              not os.path.exists(os.path.join(d2, "ts.qst3.frag")),
              "%s %s" % (os.listdir(d3), os.listdir(d2)))
        if all(os.path.exists(f) for f in frags):
            atoms = [frag_atoms(f) for f in frags]
            check("identical atom order in the three structures",
                  atoms[0][0] == atoms[1][0] == atoms[2][0] and
                  len(atoms[0][0]) == 14, "%s" % [a[0] for a in atoms])
            check("the third structure is the scan maximum",
                  abs(pair_sum(atoms[2][1]) - pair_sum(frames[top])) < 1e-3 and
                  abs(pair_sum(atoms[2][1]) - pair_sum(frames[-1])) > 1e-2,
                  "%.4f vs max %.4f last %.4f" % (pair_sum(atoms[2][1]),
                                                  pair_sum(frames[top]),
                                                  pair_sum(frames[-1])))
        for d in (d3, d2):
            shutil.rmtree(d, ignore_errors=True)
    shot = answer("workflowshot")
    check("join view saved, seven boxes, seven links, labels clear",
          bool(shot) and shot[0].startswith("ok boxes=7 links=7 label-overlaps=0"),
          "workflowshot answered %r" % shot)


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
            run_joins(display, data, work, shots)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
