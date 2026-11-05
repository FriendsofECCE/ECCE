#!/usr/bin/env python3
"""Workflows (#341): a workflow of linked steps, made as the code buttons
make them, through the Organizer's ECCE_TEST_ORGANIZER hook.

    tests/apps/workflow_test.py [--recipes-only | --editor-only | --protonation-only] [--shots DIR]

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


CREST = os.path.join(HERE, "fixtures", "crest-butane-ensemble")


def ensemble():
    """(relative energies, [flat xyz per conformer]) of the CREST fixture."""
    rel = open(os.path.join(CREST, "Props", "CONFREL")).read()
    rel = numbers(" ".join(re.findall(r"<step number=\"\d+\">(.*?)</step>", rel)))
    geom = open(os.path.join(CREST, "Props", "GEOMTRACE")).read()
    steps = re.findall(r"<step number=\"\d+\">(.*?)</step>", geom, re.S)
    return rel, [numbers(t) for t in steps]


def run_best(display, data, work, shots):
    """Steps from the best N conformers of a CREST ensemble (#682)."""
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "conf")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    shutil.copytree(CREST, os.path.join(project, "crest"), symlinks=True)
    proj = "file://" + root + "/conf"
    wf = proj + "/Workflow"
    commands = [
        "summary %s/crest" % proj,
        "new %s workflow" % proj,
        "workflowcopy %s %s/crest ens" % (wf, proj),
        "workflowstep %s orca_es best %s/ens best=3" % (wf, wf),
        "workflowstep %s orca_es toomany %s/ens best=7" % (wf, wf),
        "workflowstep %s orca_es plain %s/ens" % (wf, wf),
        "workflowinfo %s" % wf,
        "workflowready %s" % wf,
        "workflowresults %s" % wf,
    ]
    script = os.path.join(work, "commands-best")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=75,
                      env={"ECCE_TEST_ORGANIZER": script})
    log = result.log or ""
    what = "Best conformers"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)

    def answer(prefix):
        return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % re.escape(prefix),
                          log, re.M)

    info = answer("workflowinfo")
    if not info or not info[0].startswith("ok "):
        return fail(what, "no workflowinfo answer", log)
    table = steps(info[0][3:])
    links = info[0].split(" links:")[1].split()
    want = {"best-conf1", "best-conf2", "best-conf3"}
    check("three sibling steps are made", want <= set(table), "steps %s" % sorted(table))
    check("more steps than conformers makes none",
          not any(n.startswith("toomany") for n in table) and
          any("conformers" in l for l in answer("workflowstep")),
          "steps %s, answers %s" % (sorted(table), answer("workflowstep")))
    check("each step is linked from the ensemble under its rank",
          {"ens>best-conf%d:geometry.c%d" % (k, k) for k in (1, 2, 3)} <= set(links),
          "links %s" % links)
    if not want <= set(table):
        return
    rel, frames = ensemble()
    order = sorted(range(len(rel)), key=lambda i: rel[i])
    for k in (1, 2, 3):
        rec = table["best-conf%d" % k]
        got = [float(v) for v in rec["xyz0"].split(",")]
        ref = frames[order[k - 1]][:3]
        check("conformer %d (frame %d, %.3f kcal/mol) is the step's start" %
              (k, order[k - 1] + 1, rel[order[k - 1]]),
              all(abs(a - b) < 1e-4 for a, b in zip(got, ref)), "%s vs %s" % (got, ref))
    check("the conformers differ", len({tuple(frames[order[k]][:6]) for k in range(3)}) == 3,
          "frames equal")
    plain = table.get("plain")
    got = [float(v) for v in plain["xyz0"].split(",")] if plain and plain.get("xyz0") else []
    ref = frames[order[0]][:3]
    check("a plain geometry link from an ensemble gives the lowest conformer (#812)",
          bool(plain) and "ens>plain:geometry" in links and
          len(got) == 3 and all(abs(a - b) < 1e-4 for a, b in zip(got, ref)),
          "plain %s, want %s, links %s" % (got, ref, links))
    note = os.path.join(root, "conf", "Workflow", "best-conf2", ".ecce-meta")
    text = open(note).read() if os.path.exists(note) else ""
    check("the annotation names the conformer and its energy",
          "Conformer 2 of 6" in text and "%.3f kcal/mol" % rel[order[1]] in text,
          text[:300])
    ready = answer("workflowready")
    check("Launch Ready Steps takes all three",
          bool(ready) and all(n in ready[0] for n in want), "ready %s" % ready)
    res = answer("workflowresults")
    check("the CREST box counts its conformers within the default window (#810)",
          bool(res) and "ens: 6 conformers within 6 kcal/mol" in res[0],
          "workflowresults answered %r" % res)


def run_crest_preopt(display, data, work, shots):
    """New step > CREST on a box that is not an optimisation adds an xTB
    optimisation between them (#840)."""
    import ensemble_patterns_test as ep
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "pre")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    ep.fixture(os.path.join(project, "crest"))
    proj = "file://" + root + "/pre"
    wf = proj + "/Workflow"
    commands = [
        "new %s workflow" % proj,
        "workflowcopy %s %s/crest ens" % (wf, proj),
        "workflowstep %s orca_es sp %s/ens nogeom" % (wf, wf),
        "workflowstep %s crest_es cr %s/sp" % (wf, wf),
        "workflowinfo %s" % wf,
    ]
    script = os.path.join(work, "commands-preopt")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=75,
                      env={"ECCE_TEST_ORGANIZER": script})
    log = result.log or ""
    what = "CREST after a non-optimisation"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    info = re.findall(r"ECCE_TEST_ORGANIZER: workflowinfo.*?: (.*)$", log, re.M)
    if not info or not info[0].startswith("ok "):
        return fail(what, "no workflowinfo answer", log)
    links = info[0].split(" links:")[1].split()
    check("an xTB step sits between the box and CREST",
          "sp>xtb-1:geometry" in links and "xtb-1>cr:geometry" in links,
          "links %s" % links)


def run_patterns(display, data, work, shots):
    """One step per coordination pattern of a CREST ensemble around Na+ (#682)."""
    import ensemble_patterns_test as ep
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "pat")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    ep.fixture(os.path.join(project, "crest"))
    proj = "file://" + root + "/pat"
    wf = proj + "/Workflow"
    commands = [
        "new %s workflow" % proj,
        "workflowcopy %s %s/crest ens" % (wf, proj),
        "workflowstep %s orca_es pat %s/ens patterns" % (wf, wf),
        "workflowinfo %s" % wf,
    ]
    script = os.path.join(work, "commands-patterns")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=75,
                      env={"ECCE_TEST_ORGANIZER": script})
    log = result.log or ""
    what = "One step per pattern"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    info = re.findall(r"ECCE_TEST_ORGANIZER: workflowinfo.*?: (.*)$", log, re.M)
    if not info or not info[0].startswith("ok "):
        return fail(what, "no workflowinfo answer", log)
    table = steps(info[0][3:])
    check("three pattern steps are made",
          {"pat-pat1", "pat-pat2", "pat-pat3"} <= set(table), "steps %s" % sorted(table))
    links = info[0].split(" links:")[1].split()
    # Patterns by lowest energy: 6_0 (frame 4, rank 1), 5_1 (frame 2, rank 2),
    # 4_2 (frame 6, rank 5).
    for k, (tag, frame, rank) in enumerate((("6_0", 4, 1), ("5_1", 2, 2), ("4_2", 6, 5)), 1):
        check("pattern %d is linked from conformer %d" % (k, rank),
              "ens>pat-pat%d:geometry.c%d" % (k, rank) in links, "links %s" % links)
        rec = table.get("pat-pat%d" % k)
        if not rec:
            continue
        got = [float(v) for v in rec["xyz0"].split(",")]
        ref = [float(v) for r in ep.atoms(tag) for v in r[1:]][:3]
        check("pattern %d starts from the %s structure (frame %d)" % (k, tag, frame),
              all(abs(a - b) < 1e-4 for a, b in zip(got, ref)), "%s vs %s" % (got, ref))


def run_best_poses(display, data, work, shots):
    """Best N of a docking pose ensemble, ranked by its score (#682).

    The pose fixture is made from the CREST one: the same frames, scored with
    POSESCORE in an order that is not the file's, so ranking matters."""
    scores = [-20.0, -30.0, -10.0, -25.0, -5.0, -15.0]
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "dock")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    poses = os.path.join(project, "poses")
    shutil.copytree(CREST, poses, symlinks=True)
    props = os.path.join(poses, "Props")
    os.remove(os.path.join(props, "CONFREL"))
    with open(os.path.join(props, "POSESCORE"), "w") as handle:
        handle.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                     '<tsvector name="POSESCORE" rowLabel="Geometry Step" rows="1" '
                     'units="kcal/mol">\n\n')
        for i, v in enumerate(scores):
            handle.write('  <step number="%d">%.3f</step>\n' % (i + 1, v))
        handle.write("</tsvector>\n")
    meta = os.path.join(props, ".ecce-meta")
    with open(meta) as handle:
        text = handle.read()
    with open(meta, "w") as handle:
        handle.write(text.replace("CONFREL", "POSESCORE"))
    proj = "file://" + root + "/dock"
    wf = proj + "/Workflow"
    commands = [
        "summary %s/poses" % proj,
        "new %s workflow" % proj,
        "workflowcopy %s %s/poses ens" % (wf, proj),
        "workflowstep %s orca_es best %s/ens best=2" % (wf, wf),
        "workflowinfo %s" % wf,
    ]
    script = os.path.join(work, "commands-poses")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=75,
                      env={"ECCE_TEST_ORGANIZER": script})
    log = result.log or ""
    what = "Best poses"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    info = re.findall(r"ECCE_TEST_ORGANIZER: workflowinfo.*?: (.*)$", log, re.M)
    if not info or not info[0].startswith("ok "):
        return fail(what, "no workflowinfo answer", log)
    table = steps(info[0][3:])
    links = info[0].split(" links:")[1].split()
    check("best=2 makes two steps", {"best-conf1", "best-conf2"} <= set(table)
          and "best-conf3" not in table, "steps %s" % sorted(table))
    check("the steps are linked under their rank",
          {"ens>best-conf1:geometry.c1", "ens>best-conf2:geometry.c2"} <= set(links),
          "links %s" % links)
    _, frames = ensemble()
    order = sorted(range(len(scores)), key=lambda i: scores[i])
    for k in (1, 2):
        rec = table.get("best-conf%d" % k)
        if not rec:
            continue
        got = [float(v) for v in rec["xyz0"].split(",")]
        ref = frames[order[k - 1]][:3]
        check("pose %d (frame %d, score %.1f) is the step's start" %
              (k, order[k - 1] + 1, scores[order[k - 1]]),
              all(abs(a - b) < 1e-4 for a, b in zip(got, ref)), "%s vs %s" % (got, ref))
    note = os.path.join(project, "Workflow", "best-conf1", ".ecce-meta")
    text = open(note).read() if os.path.exists(note) else ""
    check("the annotation names the pose and its score",
          "Pose 1 of 6" in text and "score -30.000 kcal/mol" in text, text[:300])


def run_joins(display, data, work, shots):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "joins")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    shutil.copytree(SCAN, os.path.join(project, "butscan"), symlinks=True)
    proj = "file://" + root + "/joins"
    wf = proj + "/Workflow"
    png = os.path.join(shots, "workflow-join.png")
    vertical_png = os.path.join(shots, "workflow-join-vertical.png")
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
        "workflowsash 520",
        "workfloworient vertical",
        "workflowshot %s %s %s/qst3" % (vertical_png, wf, wf),
        # Recreate the context panel to check the saved choice is restored.
        "summary %s" % proj,
        "workflowshot %s %s %s/qst3" % (vertical_png, wf, wf),
        "workflowsash",
        "workflowsash 1",
        "workflowsash 9999",
        "workflowsash 420",
        "workfloworient horizontal",
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
          {"scan>pt-max:geometry.max", "scan>pt-last:geometry.last",
           "react>qst3:reactant", "prod>qst3:product", "scan>qst3:tsguess.max",
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
    check("join views in both orientations: clear labels, separate boxes, increasing ranks",
          len(shot) == 3 and all(
              entry.startswith("ok boxes=7 links=7 label-overlaps=0") and
              "box-overlaps=0 rank-errors=0" in entry for entry in shot) and
          "vertical=0" in shot[0] and all("vertical=1" in entry for entry in shot[1:]),
          "workflowshot answered %r" % shot)
    check("orientation hook switches both ways",
          len(answer("workfloworient")) == 2 and
          "vertical=1" in answer("workfloworient")[0] and
          "vertical=0" in answer("workfloworient")[1],
          "workfloworient answered %r" % answer("workfloworient"))
    sash = [dict(re.findall(r"(sash|saved|top|table|canvas-height)=(-?\d+)", entry))
            for entry in answer("workflowsash")]
    check("sash moves and is restored after reopening the view",
          len(sash) == 5 and all(int(sash[i].get("sash", -1)) == 520 and
                                 int(sash[i].get("saved", -1)) == 520 for i in (0, 1)),
          "workflowsash answered %r" % sash)
    check("sash clamps both panes and can be moved back",
          len(sash) == 5 and int(sash[2].get("canvas-height", -1)) >= 120 and
          int(sash[3].get("table", -1)) >= 80 and
          int(sash[4].get("sash", -1)) == 420,
          "workflowsash answered %r" % sash)
    for image in (png, vertical_png):
        check("orientation screenshot exists: " + image,
              os.path.exists(image) and os.path.getsize(image) > 5000, image)


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
    dlg2 = os.path.join(shots, "workflow-new-first-step.png")
    png2 = os.path.join(shots, "workflow-states.png")
    prog = os.path.join(work, "xtb-progress")
    with open(prog, "w") as handle:
        handle.write("# seq=1 sent=1760000000.0\ncrest stage=sampling mtd=9/14\n")
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
        "workflowdialogshot %s %s orca_es -" % (dlg2, wf),
    ] + ["workflowmenu %s %s/%s" % (wf, wf, n) for n in MENU_STEPS] + [
        # every creation path of a step with a source shows the dialog (#674)
        "workflownew %s orca_es panel %s/orca-1" % (wf, wf),
        "workflownew %s gaussian16_es tree %s/xtb-1" % (wf, wf),
        "workflowlaunchbutton %s" % wf,
        # a Complete and a Running step side by side, as the tree draws them
        "setstate %s/nw-water completed" % wf,
        "setstate %s/xtb-1 running" % wf,
        "workflowshot %s %s %s/xtb-1" % (png2, wf, wf),
        "workflowicons %s" % wf,
        # the result line under each name (#810): a finished step's energy,
        # a running step's progress
        "progress %s/xtb-1 %s" % (wf, prog),
        "workflowresults %s" % wf,
    ]
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
    check("New-step dialog screenshot", len(ds) == 2 and
          ds[0] == "ok carry=1" and os.path.exists(dlg), "answered %r" % ds)
    check("a step without a source gets no Carry-over boxes",
          len(ds) == 2 and ds[1] == "ok carry=0" and os.path.exists(dlg2),
          "answered %r" % ds)
    new = answer("workflownew")
    check("New-step dialog with Carry over from the view's code button and "
          "from the tree's New menu",
          new == ["ok dialogs=carry;", "ok dialogs=carry;"],
          "workflownew answered %r" % new)
    button = answer("workflowlaunchbutton")
    check("Launch Ready Steps button is in the view, shown, inside it",
          bool(button) and button[0] == "ok label=Launch Ready Steps shown=1 "
          "inside=1 enabled=1", "answered %r" % button)
    got = answer("workflowicons")
    icons = {}
    tree = {}
    for key, into in (("icons", icons), ("tree", tree)):
        m = re.search(key + r"=(\S+)", got[0]) if got else None
        for pair in (m.group(1).split(",") if m else []):
            name, _, idx = pair.rpartition(":")
            into[name] = idx
    names = ("nw-water", "xtb-1", "orca-3")
    check("boxes draw the tree's run-state icon (Complete and Running differ)",
          bool(got) and got[0].startswith("ok ") and
          all(n in icons and icons[n] == tree.get(n) for n in names) and
          icons.get("nw-water") != icons.get("xtb-1"),
          "workflowicons answered %r" % got)
    energy = float(re.search(r">(-?[\d.e+-]+)<", open(os.path.join(
        CALC, "Props", "TE")).read()).group(1))
    lines = dict(l.split(": ", 1) for l in
                 (answer("workflowresults") or ["ok "])[0][3:].split("|") if ": " in l)
    check("a finished step says its energy, a running one its progress",
          lines.get("nw-water") == "E \u2212%.3f Eh" % -energy and
          lines.get("xtb-1") == "MTD 9 of 14",
          "workflowresults answered %r" % lines)


EXAMPLE = os.path.join(ROOT, "data", "client", "examples", "workflows",
                       "sn2-cl-ch3br")
# step name, example folder, stored calculation, recipe key role, thermo
SN2 = (("reactant", "g16-reactant", "sn2_reactant", "reactant", 1),
       ("scan", "g16-scan", "sn2_scan", "-", 0),
       ("product", "g16-product", "sn2_product", "product", 1),
       ("qst3", "g16-qst3", "sn2_qst3", "ts", 1),
       ("irc", "g16-irc", "sn2_irc", "-", 0))
SN2_LINKS = (("reactant", "scan", "geometry"), ("scan", "product", "geometry.last"),
             ("reactant", "qst3", "reactant"), ("product", "qst3", "product"),
             ("scan", "qst3", "tsguess.max"), ("qst3", "irc", "geometry"))


def run_recipes(display, data, work, shots):
    """Apply Recipe (#710): both entry points, boxes and links, key data."""
    root = os.path.join(data, "users", "local")
    for name in ("recipes", "keydata"):
        os.makedirs(os.path.join(root, name))
        shutil.copy(PROJECT_META, os.path.join(root, name, ".ecce-meta"))
    for step, folder, stored, _, _ in SN2:
        shutil.copytree(os.path.join(EXAMPLE, folder, "stored", stored),
                        os.path.join(root, "keydata", stored), symlinks=True)
    proj = "file://" + root + "/recipes"
    kproj = "file://" + root + "/keydata"
    wf, kwf = proj + "/Workflow", kproj + "/Workflow"
    recdir = os.path.join(work, "userrecipes")
    # A hand-written recipe in the user's folder, and a broken one beside it.
    userdir = os.path.join(os.environ["ECCE_REALUSERHOME"], ".ECCE", "Recipes")
    os.makedirs(userdir, exist_ok=True)
    with open(os.path.join(userdir, "handmade.recipe"), "w") as f:
        f.write("# my own recipe\nrecipe: handmade\ntitle: Handmade\n"
                "role: geom | Geometry | PBE | def2-SVP | my choice\n"
                "variant: main | xtb then ORCA | ORCA\n"
                "step: main | first | xtb | | GeoVib | xtb Opt | structure\n"
                "step: main | second | ORCA | geom | GeoVib | ORCA Opt |\n"
                "link: main | first | second | geometry\n")
    # One ORCA step: the role's basis set is stored once it has a structure.
    with open(os.path.join(userdir, "single.recipe"), "w") as f:
        f.write("recipe: single\ntitle: Single\n"
                "role: geom | Geometry | B3LYP | 6-31G(d) | test\n"
                "variant: main | ORCA | ORCA\n"
                "step: main | opt | ORCA | geom | GeoVib | ORCA Opt | structure\n")
    with open(os.path.join(userdir, "singleqm.recipe"), "w") as f:
        f.write("recipe: singleqm\ntitle: Single QM\n"
                "role: geom | Geometry | PBE0 | cc-pVDZ | test\n"
                "variant: main | ECCE-QM | ECCE-QM\n"
                "step: main | opt | ECCE-QM | geom | GeoVib | ECCE-QM Opt | structure\n")
    # A step flagged charge=+1 takes its source's charge plus one (#808).
    with open(os.path.join(userdir, "shifted.recipe"), "w") as f:
        f.write("recipe: shifted\ntitle: Shifted\n"
                "role: geom | Geometry | PBE | def2-SVP | test\n"
                "variant: main | ORCA | ORCA\n"
                "step: main | cation | ORCA | geom | GeoVib | ORCA Opt | charge=+1\n")
    # The anion rule: a structure stored later turns 6-31G(d) into 6-31+G(d).
    with open(os.path.join(userdir, "anion.recipe"), "w") as f:
        f.write("recipe: anion\ntitle: Anion\n"
                "role: geom | Geometry | PBE0 | 6-31G(d) | test\n"
                "rules: anion-diffuse\n"
                "variant: main | ORCA | ORCA\n"
                "step: main | start | ORCA | geom | GeoVib | ORCA Opt | structure\n")
    with open(os.path.join(userdir, "broken.recipe"), "w") as f:
        f.write("recipe: broken\nthis line has no colon\n")
    shots_panel = os.path.join(shots, "recipe-panel.png")
    shots_done = os.path.join(shots, "recipe-keydata.png")
    shots_vertical = os.path.join(shots, "recipe-keydata-vertical.png")
    commands = [
        "summary %s" % proj, "summary %s" % kproj,
        "new %s workflow" % proj, "new %s workflow" % kproj,
        "workflowrecipelist",
        "workflowrecipepanel - %s ts-irc orca -" % wf,
        "workflowrecipepanel %s %s ts-irc g16 - solvent=DMSO" % (shots_panel, wf),
        "workflowrecipecreate %s" % wf,
        # a second recipe appended after the TS box
        "workflowrecipepanel - %s conformers orca %s/ts solvent=DMSO" % (wf, wf),
        "workflowrecipecreate %s" % wf,
        "workflowsaverecipe %s mine My-recipe" % wf,
    ]
    for step, folder, stored, key, thermo in SN2:
        commands.append("workflowcopy %s %s/%s %s" % (kwf, kproj, stored, step))
    for a, b, name in SN2_LINKS:
        commands.append("workflowlink %s %s/%s %s/%s %s" % (kwf, kwf, a, kwf, b, name))
    commands.append("workflowkeydata %s" % kwf)       # not a recipe workflow yet
    for step, folder, stored, key, thermo in SN2:
        commands.append("workflowmark %s %s/%s %s %d" % (kwf, kwf, step, key, thermo))
    commands += ["workflowkeydata %s" % kwf,
                 "workflowshot %s %s" % (shots_done, kwf),
                 "workfloworient vertical",
                 "workflowshot %s %s" % (shots_vertical, kwf),
                 "workfloworient horizontal",
                 "workflowrecipe %s handmade main - solvent=chloroform" % wf,
                 "workflowrecipe %s single main %s/reactant" % (kwf, kwf),
                 "workflowrecipe %s singleqm main %s/reactant solvent=water" % (kwf, kwf),
                 "workflowrecipe %s anion main -" % wf,
                 "workflowrecipe %s shifted main %s/reactant" % (kwf, kwf),
                 "workflowstructure %s %s/start %s/reactant" % (wf, wf, kwf),
                 "workflowinfo %s" % wf,
                 "workflowrecipehere %s %s/start" % (wf, wf),
                 "workflowrecipehere %s %s/first" % (wf, wf)]
    script = os.path.join(work, "commands-recipes")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    env = {"ECCE_TEST_ORGANIZER": script, "ECCE_RECIPE_DIRS": recdir}
    os.makedirs(recdir)
    result = apps.run(display, "organizer", windowTimeout=60, settle=75, env=env)
    log = result.log or ""
    what = "Recipes"
    with open(os.path.join(shots, "recipe-dialog.log"), "w") as handle:
        handle.write(log)
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)

    def answers(prefix):
        return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % re.escape(prefix),
                          log, re.M)

    listed = answers("workflowrecipelist")
    check("both built-in recipes are found",
          listed and "ts-irc:g16,orca" in listed[0] and "conformers:orca,g16" in listed[0],
          "workflowrecipelist answered %r" % listed)
    check("a hand-written user recipe is listed, a broken one is reported with its line",
          listed and "handmade:main" in listed[0] and "broken.recipe: line 2" in listed[0]
          and "ts-irc:" in listed[0], "workflowrecipelist answered %r" % listed)
    hand = answers("workflowrecipe ")
    single = [h for h in hand if h.startswith("ok") and "|opt ORCA" in h]
    check("the role's basis set is stored in a step that has a structure",
          single and re.search(r"(^|\|| )opt ORCA [^|]*basis=6-31G\*", single[0]),
          "workflowrecipe answered %r" % hand[-1:])
    check("the hand-written recipe builds its two boxes and the link",
          hand and "first xtb" in hand[0] and "second ORCA" in hand[0] and
          "first>second:geometry" in hand[0], "workflowrecipe answered %r" % hand[:1])
    check("ORCA gets the role's method as its theory and functional",
          single and re.search(r"(^|\|)opt ORCA [^|]*theory=RDFT[^|]*xc=B3LYP( |$|\|)", single[0]),
          "workflowrecipe answered %r" % hand[-1:])
    # The step's name is made unique ("opt-2") when an "opt" already exists.
    qmStep = r"(^|\|)opt(-\d+)? ECCE-QM [^|]*"
    qm = [h for h in hand if h.startswith("ok") and re.search(qmStep, h)]
    check("ECCE-QM gets the role's method as its functional (PBE0) and basis",
          qm and re.search(qmStep + r"theory=DFT[^|]*xc=PBE0( |$|\|)", qm[0])
          and re.search(qmStep + r"basis=cc-pVDZ", qm[0]),
          "workflowrecipe answered %r" % hand[-2:])
    here = answers("workflowrecipehere")
    check("the box menu's Apply Recipe from here opens the panel after that box",
          len(here) == 2 and here[0].startswith("ok") and "apply=after" in here[0],
          "workflowrecipehere answered %r" % here)
    stored = answers("workflowstructure")
    check("a structure stored later re-applies the rules (anion: diffuse basis)",
          stored and stored[0].startswith("ok") and "start=PBE0/6-31+G(d)" in stored[0],
          "workflowstructure answered %r" % stored)
    infos = answers("workflowinfo")
    after = steps(infos[-1].replace("ok ", "", 1)).get("start", {}) if infos else {}
    check("the re-applied level is stored in the step (6-31+G*, PBE0)",
          after.get("basis") == "6-31+G*" and after.get("xc") == "PBE0"
          and after.get("theory") == "RDFT", "start: %r" % after)
    panels = answers("workflowrecipepanel")
    check("the set-up panel previews the ORCA variant (4 boxes, 3 links, no overlap)",
          len(panels) >= 2 and "recipe=ts-irc variant=orca boxes=4 links=3 overlaps=0" in panels[0],
          "panel answered %r" % panels[:1])
    check("the set-up panel previews the G16 variant (5 boxes, 6 links, no overlap)",
          len(panels) >= 2 and "variant=g16 boxes=5 links=6 overlaps=0" in panels[1]
          and "ok" in panels[1][:3], "panel answered %r" % panels[1:2])
    check("the panel offers the recommendation and its reason",
          len(panels) >= 2 and "B3LYP/6-31+G(d)" in panels[1] and "Why: the usual starting level" in panels[1],
          "panel answered %r" % panels[1:2])
    check("recipe dialog has no GTK minimum-size warnings",
          "for_size smaller than min-size" not in log,
          "\n".join(line for line in log.splitlines() if "for_size" in line))
    check("method and basis column headings, full wrapped role label and step names",
          len(panels) >= 2 and "headings=Method;Basis set;" in panels[1]
          and "Optimisations, TS search, frequencies, IRC" in panels[1]
          and "Applies to: reactant, scan, product, ts, irc" in panels[1],
          "panels: %r" % panels)
    check("new workflow hides section 3; selected box shows the named choices",
          len(panels) >= 3 and all("where-visible=0" in p for p in panels[:2])
          and "where-visible=1" in panels[2]
          and "where-heading=3  Where the new steps go" in panels[2]
          and "new-label=Start a new workflow" in panels[2]
          and 'after-label=Continue after the selected box "ts"' in panels[2],
          "panels: %r" % panels)
    check("rebuilt role rows fit without overlap or clipped labels",
          len(panels) >= 3 and all("role-overlaps=0 clipped-labels=0" in p for p in panels),
          "panels: %r" % panels)
    check("each built-in recipe has one solvent row applying to every step",
          len(panels) >= 3 and all(p.count("row[solvent]=Solvent;") == 1
          and "applies to every step" in p for p in panels)
          and "Solvent model: xtb: ALPB; CREST: ALPB; ORCA: CPCM" in panels[2],
          "panels: %r" % panels)
    created = answers("workflowrecipecreate")
    info = created[0] if created else ""
    st = steps(info.replace("ok ", "", 1))
    check("Create boxes makes the five G16 steps",
          sorted(st) == ["irc", "product", "reactant", "scan", "ts"],
          "info %r" % info[:300])
    check("solvent choice sets PCM and the accepted solvent name in every G16 step",
          len(st) == 5 and all(v.get("solvent-model") == "PCM"
          and v.get("solvent") == "Dimethylsulfoxide"
          and v.get("solvent-toggle-type") == "toggle_input" for v in st.values()),
          "steps: %r" % st)
    conf = steps(created[1].replace("ok ", "", 1)) if len(created) > 1 else {}
    check("Conformers applies ALPB to CREST/xTB and CPCM to ORCA",
          all(conf.get(n, {}).get("solvent-model") == model
          and conf.get(n, {}).get("solvent") == name
          for n, model, name in (("crest", "ALPB", "dmso"),
                                 ("xtb", "ALPB", "dmso"), ("dft", "CPCM", "DMSO"))),
          "steps: %r" % conf)
    check("solvent checkbox restores as a checkbox in the ORCA theory editor",
          conf.get("dft", {}).get("solvent-toggle-type") == "toggle_input",
          "DFT step: %r" % conf.get("dft"))
    check("a user recipe without a solvent role gets solvent in all steps, with aliases",
          hand and "solvent-model=ALPB solvent=chcl3" in hand[0]
          and "solvent-model=CPCM solvent=chloroform" in hand[0],
          "user recipe: %r" % hand[:1])
    check("ECCE-QM uses its CPCM solvent setting",
          qm and "solvent-model=CPCM solvent=Water" in qm[0], "QM: %r" % qm)
    check("none is gas phase for a user recipe",
          single and "solvent-model=None" in single[0], "single: %r" % single)
    shift = [h for h in hand if h.startswith("ok") and "|cation ORCA" in h]
    qr = re.search(r"(^|\|| )reactant G16 \S+ \S+ charge=(-?\d+)", shift[0]) if shift else None
    qc = re.search(r"(^|\|)cation ORCA \S+ \S+ charge=(-?\d+)", shift[0]) if shift else None
    check("a charge=+1 step after a source gets the source's charge plus one",
          qr and qc and int(qc.group(2)) == int(qr.group(2)) + 1,
          "shifted: %r" % shift)
    check("only the first box needs a structure",
          [n for n in st if st[n].get("needs") == "1"] == ["reactant"],
          "needs: %r" % [(n, st[n].get("needs")) for n in st])
    want = ("reactant>scan:geometry scan>product:geometry.last reactant>ts:reactant "
            "product>ts:product scan>ts:tsguess.max ts>irc:geometry")
    check("the links are the recipe's (roles and scan points)",
          all(l in info for l in want.split()), "links in %r" % info[-300:])
    check("G16 gets the role's method (B3LYP) as its functional",
          st.get("reactant", {}).get("xc") == "B3LYP_(hybrid)"
          and st.get("reactant", {}).get("theory") == "RDFT", "reactant: %r" % st.get("reactant"))
    check("the TS step is the QST3 run type",
          st.get("ts", {}).get("code") == "G16", "ts: %r" % st.get("ts"))
    check("appended after a box: the chain starts from it",
          len(created) > 1 and "preopt>crest:geometry" in created[1]
          and "ts>preopt:geometry" in created[1] and "crest>xtb:geometry" in created[1] and "xtb>dft:geometry" in created[1],
          "second info %r" % (created[1][-300:] if len(created) > 1 else ""))
    saved = answers("workflowsaverecipe")
    check("Save Workflow as Recipe writes a recipe file",
          saved and saved[0].startswith("ok ") and os.path.exists(saved[0][3:]),
          "answered %r" % saved)
    if saved and saved[0].startswith("ok "):
        text = open(saved[0][3:]).read()
        check("the saved recipe keeps its shared solvent default",
              "role: solvent | Solvent |  | DMSO |" in text, text[:300])
        check("the saved recipe keeps links and drops structures",
              "link: main | reactant | scan | geometry" in text and "xyz" not in text.lower(),
              text[:300])

    keys = answers("workflowkeydata")
    check("no key data for a workflow made without a recipe",
          keys and keys[0] == "ok ", "answered %r" % keys[:1])
    text = keys[1] if len(keys) > 1 else ""
    check("key data: one code, one level, shown with progress",
          "5 of 5 steps complete" in text and "Gaussian-16  B3LYP" in text, text)
    check("key data: reaction energy -9.6 kJ/mol and barrier +27.3 kJ/mol",
          "E(product) - E(reactant): -9.6 kJ/mol" in text and
          "E(TS) - E(reactant): +27.3 kJ/mol" in text, text)
    check("key data: free-energy barrier +25.6 kJ/mol",
          "G(TS) - G(reactant): +25.6 kJ/mol" in text, text)
    shot = answers("workflowshot")
    check("both orientations with the key-data box have clear labels",
          len(shot) == 2 and all(entry.startswith("ok ") and
          "label-overlaps=0" in entry and "keybox=1 keyshown=1" in entry for entry in shot)
          and "vertical=0" in shot[0] and "vertical=1" in shot[1],
          "workflowshot answered %r" % shot)
    shutil.rmtree(recdir, ignore_errors=True)


def tree_of(path):
    out = set()
    for d, _, files in os.walk(path):
        for f in files:
            out.add(os.path.relpath(os.path.join(d, f), path))
    return out


def run_make(display, data, work, shots):
    """Make Workflow / Move into Workflow (#807): the calculation is moved,
    not copied, and keeps its files; refused when already in a workflow."""
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "mk")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    for name in ("water", "water2"):
        shutil.copytree(CALC, os.path.join(project, name), symlinks=True)
    proj = "file://" + root + "/mk"
    wf = proj + "/Workflow"
    commands = [
        "summary %s/water" % proj,
        "workflowmake %s/water" % proj,
        "workflowmake %s/water again" % proj,
        "new %s workflow" % proj,
        "workflowmove %s/water2 %s" % (proj, wf),
        "workflowmake %s/Workflow/water2 nope" % proj,
        "workflowmove %s/water %s" % (proj, wf),
        "workflowinfo %s/water_workflow" % proj,
        "workflowinfo %s" % wf,
    ]
    script = os.path.join(work, "commands-make")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script})
    log = result.log or ""
    what = "Make workflow"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)

    def answers(prefix):
        return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % re.escape(prefix),
                          log, re.M)

    made = answers("workflowmake %s/water" % proj)
    check("make: ok, step selected",
          bool(made) and made[0].startswith("ok ") and made[0].endswith(" selected"),
          str(made))
    check("make: calculation now inside the new workflow",
          os.path.isdir(os.path.join(project, "water_workflow", "water")),
          "no water_workflow/water")
    check("make: original location gone",
          not os.path.exists(os.path.join(project, "water")), "water still there")
    check("make: files and results intact",
          tree_of(os.path.join(project, "water_workflow", "water"))
          >= tree_of(CALC),
          "missing: %s" % sorted(tree_of(CALC) - tree_of(
              os.path.join(project, "water_workflow", "water"))))
    check("make: second make of the moved path fails",
          len(made) > 1 and made[1].startswith("error"), str(made))
    info = answers("workflowinfo %s/water_workflow" % proj)
    check("make: workflow lists the step",
          bool(info) and info[0].startswith("ok water "), str(info))
    moved = answers("workflowmove %s/water2" % proj)
    check("move: into the existing workflow",
          bool(moved) and moved[0].startswith("ok ") and
          os.path.isdir(os.path.join(project, "Workflow", "water2")) and
          not os.path.exists(os.path.join(project, "water2")), str(moved))
    nope = answers("workflowmake %s/Workflow/water2" % proj)
    check("refused: already in a workflow",
          bool(nope) and "already in a workflow" in nope[0], str(nope))
    fin = answers("workflowinfo %s/Workflow" % proj)
    check("move: workflow lists the moved step",
          bool(fin) and "water2" in fin[0], str(fin))


def run_editor(display, data, work, shots):
    """A recipe successor must be editable before Launch Ready Steps."""
    project = os.path.join(data, "users", "local", "editor")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    shutil.copytree(CALC, os.path.join(project, "water"), symlinks=True)
    userdir = os.path.join(os.environ["ECCE_REALUSERHOME"], ".ECCE", "Recipes")
    os.makedirs(userdir, exist_ok=True)
    with open(os.path.join(userdir, "editor.recipe"), "w") as f:
        f.write("recipe: editor\ntitle: Editor\n"
                "variant: main | xTB | xtb\n"
                "step: main | first | xtb | | Geometry | First | structure\n"
                "step: main | next | xtb | | Geometry | Next |\n"
                "link: main | first | next | geometry\n")
    proj = "file://" + project
    wf = proj + "/Workflow"

    def organizer(tag, commands):
        script = os.path.join(work, "editor-" + tag)
        with open(script, "w") as f:
            f.write("\n".join(commands) + "\n")
        result = apps.run(display, "organizer", windowTimeout=60, settle=35,
                          env={"ECCE_TEST_ORGANIZER": script,
                               "ECCE_TEST_ORGANIZER_NOLAUNCH": "1"})
        with open(os.path.join(shots, "editor-" + tag + ".log"), "w") as f:
            f.write(result.log or "")
        check("recipe editor %s Organizer survives" % tag,
              not result.crashed, result.log or "")
        return result.log or ""

    created = organizer("create", [
        "new %s workflow" % proj,
        "workflowcopy %s %s/water seed" % (wf, proj),
        "workflowstep %s xtb_es charged %s/seed geomfrom=%s/water charge=1 mult=2"
        % (wf, wf, proj),
        "workflowrecipe %s editor main %s/charged" % (wf, wf),
        "start CalculationEditor %s/next" % wf,
        "workflowinfo %s" % wf,
    ])
    check("a recipe successor waits for its source to complete",
          "next xtb Created first charge=-99 mult=0 atoms=-1" in created, created)
    # Supply a completed optimisation's final geometry, as a monitor would.
    first = os.path.join(project, "Workflow", "first")
    shutil.copytree(os.path.join(CALC, "Props"), os.path.join(first, "Props"),
                    dirs_exist_ok=True)
    log = organizer("open", [
        "setstate %s/first completed" % wf,
        "start CalculationEditor %s/next" % wf,
        "workflowstep %s xtb_es manual %s/first" % (wf, wf),
        "workflowinfo %s" % wf,
    ])
    info = re.findall(r"ECCE_TEST_ORGANIZER: workflowinfo.*?: ok (.*)$", log, re.M)
    table = steps(info[-1]) if info else {}
    for name in ("next", "manual"):
        rec = table.get(name, {})
        check(name + " stores atoms, charge and multiplicity before launch",
              (rec.get("atoms"), rec.get("charge"), rec.get("mult")) ==
              ("3", "1", "2"), str(rec))
        check(name + " stores its own chemical system",
              os.path.isfile(os.path.join(project, "Workflow", name,
                                           "Parameters", "chemsys.mvm")), str(rec))
        commands = os.path.join(work, "calced-" + name)
        with open(commands, "w") as f:
            f.write("state\nsave\nstate\n")
        result = apps.run(display, "calced", args=("-context", wf + "/" + name),
                          windowTimeout=60, settle=25,
                          env={"ECCE_TEST_CALCED": commands, "ECCE_TMPDIR": work})
        text = result.log or ""
        with open(os.path.join(shots, "editor-" + name + ".log"), "w") as f:
            f.write(text)
        check(name + " editor loads theory and saves launch input",
              not result.crashed and "theory=SE/GFN2-xTB" in text and
              "ECCE_TEST_CALCED: save: kept " in text, text)
        kept = re.search(r"ECCE_TEST_CALCED: save: kept (.+)", text)
        if kept:
            folder = kept.group(1).strip()
            with open(os.path.join(folder, name + ".frag")) as f:
                frag = f.read()
            with open(os.path.join(folder, name + ".param")) as f:
                params = f.read()
            check(name + " generated input has the chemical system",
                  "num_atoms: 3" in frag and "charge: 1" in frag and
                  "Charge: 1\n" in params and "ChemSys.Multiplicity: 2\n" in params
                  and "NumElectrons: 9\n" in params, frag + params)
            if name == "next":
                check("the recipe run type survives opening and saving",
                      "RunType: Geometry\n" in params, params)


def run_protonation(display, data, work, shots):
    """Protonation Sites recipe (#851): CREST sees the xTB optimisation that
    feeds it, and the step after CREST gets the lowest protomer, which has one
    atom more than CREST's own input."""
    project = os.path.join(data, "users", "local", "proton")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    shutil.copytree(CALC, os.path.join(project, "water"), symlinks=True)
    proj = "file://" + project
    wf = proj + "/Workflow"
    folder = os.path.join(project, "Workflow")

    def organizer(tag, commands):
        script = os.path.join(work, "proton-" + tag)
        with open(script, "w") as f:
            f.write("\n".join(commands) + "\n")
        result = apps.run(display, "organizer", windowTimeout=60, settle=35,
                          env={"ECCE_TEST_ORGANIZER": script,
                               "ECCE_TEST_ORGANIZER_NOLAUNCH": "1"})
        log = result.log or ""
        with open(os.path.join(shots, "proton-" + tag + ".log"), "w") as f:
            f.write(log)
        check("protonation %s Organizer survives" % tag, not result.crashed, log)
        return log

    def answer(log, command):
        got = re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % command, log, re.M)
        return got[-1] if got else ""

    log = organizer("create", [
        "new %s workflow" % proj,
        "workflowcopy %s %s/water seed" % (wf, proj),
        "workflowrecipe %s protonation-sites orca %s/seed" % (wf, wf),
        "workflowcrestcheck %s %s/crest" % (wf, wf),
        "workflowinfo %s" % wf,
    ])
    check("the recipe links preopt to crest",
          "preopt>crest:geometry" in answer(log, "workflowinfo"),
          answer(log, "workflowinfo"))
    check("Launch sees the xTB optimisation feeding CREST",
          answer(log, "workflowcrestcheck") == "ok fromopt=1",
          answer(log, "workflowcrestcheck"))

    # preopt completes with water's optimised geometry, as a monitor would leave it
    shutil.copytree(os.path.join(CALC, "Props"), os.path.join(folder, "preopt", "Props"),
                    dirs_exist_ok=True)
    organizer("crest", [
        "setstate %s/preopt completed" % wf,
        "start CalculationEditor %s/crest" % wf,
    ])
    # CREST --protonate keeps the input order and appends the proton
    # (protonated.xyz); two protomers, the second lowest.
    frames = [("0.0 0.0 0.0 0.77 0.64 0.0 -0.77 0.64 0.0 0.0 -0.3 0.95", 2.5),
              ("0.0 0.0 0.1 0.80 0.60 0.0 -0.80 0.60 0.0 0.0 -0.5 -0.80", 0.0)]
    props = os.path.join(folder, "crest", "Props")
    os.makedirs(props, exist_ok=True)
    with open(os.path.join(props, "GEOMTRACE"), "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                '<tsvectable columnLabel="Atom" columnLabels="X Y Z" columns="3" '
                'name="GEOMTRACE" rowLabel="Geometry Step" rowLabels="1-O 2-H 3-H 4-H" '
                'rows="4" units="Angstrom" vectorLabel="Coordinate" vectors="1">\n')
        for i, (xyz, _e) in enumerate(frames):
            f.write('  <step number="%d">%s</step>\n' % (i + 1, xyz))
        f.write("</tsvectable>\n")
    with open(os.path.join(props, "CONFREL"), "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                '<tsvector name="CONFREL" rowLabel="Geometry Step" rows="1" '
                'units="kcal/mol">\n')
        for i, (_xyz, e) in enumerate(frames):
            f.write('  <step number="%d">%.3f</step>\n' % (i + 1, e))
        f.write("</tsvector>\n")
    log = organizer("next", [
        "setstate %s/crest completed" % wf,
        "start CalculationEditor %s/xtb" % wf,
        "workflowinfo %s" % wf,
    ])
    rec = steps(answer(log, "workflowinfo").partition("ok ")[2]).get("xtb", {})
    check("the step after CREST gets the protomer and the charge plus one",
          (rec.get("atoms"), rec.get("charge")) == ("4", "1"), str(rec))
    chem = os.path.join(folder, "xtb", "Parameters", "chemsys.mvm")
    text = open(chem).read() if os.path.isfile(chem) else ""
    check("the protomer stored is the lowest one",
          "-0.5" in text and "-0.8" in text, text)


def run_autorun(display, data, work, shots):
    """Auto-run and the play button (#736): with the setting on, a step that
    completes launches the next; a play button is enabled only for a ready
    step; Launch Step launches just that one."""
    project = os.path.join(data, "users", "local", "autorun")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    shutil.copytree(CALC, os.path.join(project, "water"), symlinks=True)
    proj = "file://" + project
    wf = proj + "/Workflow"
    script = os.path.join(work, "autorun-commands")
    commands = [
        "new %s workflow" % proj,
        "workflowcopy %s %s/water seed" % (wf, proj),
        "workflowstep %s xtb_es a %s/seed" % (wf, wf),
        "workflowstep %s xtb_es b %s/a" % (wf, wf),
        "workflowstep %s xtb_es c %s/a" % (wf, wf),
        "workflowautorun %s query" % wf,
        "workflowplay %s" % wf,
        "workflowlaunchstep %s %s/b" % (wf, wf),
        "workflowlaunchstep %s %s/a" % (wf, wf),
        "workflowplay %s" % wf,
        "workflowlaunchstep %s %s/a" % (wf, wf),
        "workflowinfo %s" % wf,
    ]
    with open(script, "w") as f:
        f.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script})
    log = result.log or ""
    with open(os.path.join(shots, "autorun-1.log"), "w") as f:
        f.write(log)
    check("autorun Organizer survives", not result.crashed, log)

    def answers(command):
        return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % command, log, re.M)

    check("auto-run is off by default",
          answers("workflowautorun")[-1:] == ["ok autorun=off"], str(answers("workflowautorun")))
    play = answers("workflowplay")
    check("play buttons: only the ready step is enabled", len(play) == 2 and
          "seed:0:already run|a:1:|b:0:waiting for a|c:0:waiting for a" in play[0],
          str(play))
    launch = answers("workflowlaunchstep")
    check("a step that is waiting is not launched",
          len(launch) == 3 and launch[0] == "ok blocked waiting for a" or
          (len(launch) == 3 and "blocked waiting for a" in launch[0]), str(launch))
    check("launching a step launches only that one and not twice",
          len(launch) == 3 and "blocked" not in launch[1] and
          "blocked already run" in launch[2], str(launch))
    check("only the launched step left Created",
          len(play) == 2 and play[1].split("|")[1].startswith("a:0:already run") and
          "b:0:waiting for a" in play[1], str(play))

    # Auto-run on: the first step completes, the next two start by themselves.
    script2 = os.path.join(work, "autorun-commands2")
    commands = [
        "workflowautorun %s on" % wf,
        "setstate %s/a completed" % wf,
        "workflowicons %s" % wf,
        "workflowicons %s" % wf,
        "workflowplay %s" % wf,
        "workflowautorun %s query" % wf,
    ]
    with open(script2, "w") as f:
        f.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script2})
    log = result.log or ""
    with open(os.path.join(shots, "autorun-2.log"), "w") as f:
        f.write(log)
    check("autorun second Organizer survives", not result.crashed, log)
    play = answers("workflowplay")
    check("with auto-run on, the dependants of a completed step were launched",
          play and "b:0:already run" in play[-1] and "c:0:already run" in play[-1],
          str(play))
    check("auto-run stays on", answers("workflowautorun")[-1:] == ["ok autorun=on"],
          str(answers("workflowautorun")))


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
            if "--editor-only" in sys.argv:
                run_editor(display, data, work, shots)
            elif "--protonation-only" in sys.argv:
                run_protonation(display, data, work, shots)
            elif "--autorun-only" in sys.argv:
                run_autorun(display, data, work, shots)
            elif "--make-only" in sys.argv:
                run_make(display, data, work, shots)
            elif "--recipes-only" not in sys.argv:
                run(display, data, work, shots)
                run_joins(display, data, work, shots)
                run_make(display, data, work, shots)
                run_best(display, data, work, shots)
                run_best_poses(display, data, work, shots)
                run_patterns(display, data, work, shots)
                run_crest_preopt(display, data, work, shots)
                run_editor(display, data, work, shots)
                run_protonation(display, data, work, shots)
                run_autorun(display, data, work, shots)
            if not {"--make-only", "--editor-only", "--protonation-only", "--autorun-only"} & set(sys.argv):
                run_recipes(display, data, work, shots)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
