#!/usr/bin/env python3
"""A finished Vina run is imported through vina.expt (Organizer > File >
Import) and must give back what ECCE stored when it ran the same job.  The
anchor is the three real runs under data/client/examples/docking: each has
the raw output beside the calculation ECCE itself stored from it.  Checked:
the importer's structure is the stored one (pose 1), and the parse
specification run over the imported output delivers the stored POSESCORE and
GEOMTRACE, pose for pose.  A run that parses nothing fails.
"""
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from eccejobmonitor_sim import read_desc, replay, run_parser, parse_parser_output  # noqa: E402

REPO = os.path.dirname(os.path.dirname(HERE))
EXPT = os.path.join(REPO, "scripts", "parsers", "vina.expt")
PARSERS = os.path.join(REPO, "scripts", "parsers")
DOCK = os.path.join(REPO, "data", "client", "examples", "docking")

failures = []


def check(ok, msg):
    print(("ok   " if ok else "FAIL ") + msg)
    if not ok:
        failures.append(msg)


def floats(s):
    return [float(x) for x in re.findall(r"-?\d+\.?\d*(?:[eE][-+]?\d+)?", s)]


def stored_poses(calc):
    pdir = os.path.join(calc, "Props")
    ps = open(os.path.join(pdir, "POSESCORE")).read()
    scores = [float(x) for x in re.findall(r"<step number=\"\d+\">(\S+)</step>", ps)]
    gt = open(os.path.join(pdir, "GEOMTRACE")).read()
    labels = re.search(r'rowLabels="([^"]*)"', gt).group(1).split()
    steps = [floats(b) for b in re.findall(r"<step number=\"\d+\">(.*?)</step>", gt, re.S)]
    return scores, labels, steps


def imported_poses(out_path):
    desc = read_desc(os.path.join(PARSERS, "vina.desc"))
    res = replay(desc, out_path)
    scores, labels, steps = [], None, []
    for e in desc.live_entries():
        for blk in res.delivered_for(e):
            so, _, rc = run_parser(PARSERS, e, blk,
                                   parse_args=('.', 'Docking (Vina)', 'SE', 'Vina', '0'))
            check(rc == 0, "dock.poses exit 0 for pose %d" % blk.count)
            for rec in parse_parser_output(so):
                sec = rec["sections"]
                if rec["key"] == "POSESCORE":
                    scores.append(floats(" ".join(sec["values"]))[0])
                elif rec["key"] == "GEOMTRACE":
                    labels = " ".join(sec["rowlabels"]).split()
                    steps.append(floats(" ".join(sec["values"])))
    return scores, labels, steps


def mvm_atoms(path):
    rows = []
    seen = False
    for line in open(path):
        if line.startswith("atom_list"):
            seen = True
            continue
        if seen:
            if line.startswith("attr_list"):
                break
            t = line.split()
            if len(t) == 4:
                rows.append((t[0], [float(x) for x in t[1:]]))
    return rows


def run(example):
    d = os.path.join(DOCK, example)
    out = glob.glob(os.path.join(d, "*.out"))[0]
    calc = glob.glob(os.path.join(d, "stored", "*", "Props"))[0][:-6]
    name = os.path.basename(out)
    work = tempfile.mkdtemp(prefix="vinaimp-", dir=os.environ.get("TMPDIR"))
    try:
        dest = os.path.join(work, name)
        os.symlink(out, dest)
        r = subprocess.run([sys.executable, EXPT, name], cwd=work,
                           capture_output=True, text=True)
        check(r.returncode == 0, "%s: importer exit 0 (%s)" % (example, (r.stdout + r.stderr).strip()))
        if r.returncode:
            return
        stem = name[:-4]
        frag = open(os.path.join(work, stem + ".frag")).read()
        param = open(os.path.join(work, stem + ".param")).read()
        check("Theory:\tVina" in param and "RunType:\tDocking (Vina)" in param,
              "%s: theory and run type" % example)

        # structure: the stored system, atom for atom
        want = mvm_atoms(glob.glob(os.path.join(calc, "Parameters", "chemsys.mvm"))[0])
        got = [(l.split()[0], [float(x) for x in l.split()[1:]])
               for l in frag.split("atom_list:\n", 1)[1].splitlines()]
        check(len(want) > 0, "%s: stored structure read (%d atoms)" % (example, len(want)))
        check(len(got) == len(want), "%s: atom count %d == stored %d" % (example, len(got), len(want)))
        # chemsys.mvm is the system as launched (ligand before docking); the
        # output only has the docked poses, so the import carries pose 1.
        # Everything outside the ligand must be identical, and the ligand
        # must be the stored pose 1.
        vin = open(glob.glob(os.path.join(calc, "Inputs", "vina.vinain"))[0]).read()
        lo, hi = [int(x) for x in re.search(r"LIGAND_ATOMS=(\d+)-(\d+)", vin).groups()]

        def same(g, w):
            return g[0] == w[0] and max(abs(a - b) for a, b in zip(g[1], w[1])) < 1e-4
        rec_ok = all(same(g, w) for i, (g, w) in enumerate(zip(got, want), 1) if not lo <= i <= hi)
        check(rec_ok, "%s: all %d non-ligand atoms equal chemsys.mvm" % (example, len(want) - (hi - lo + 1)))
        ws0 = stored_poses(calc)[2][0]
        pose1 = [x for g in got for x in g[1]]
        check(len(pose1) == len(ws0) and max(abs(a - b) for a, b in zip(pose1, ws0)) < 1e-4,
              "%s: imported structure == stored pose 1" % example)

        # poses and scores
        ws, wl, wsteps = stored_poses(calc)
        gs, gl, gsteps = imported_poses(dest)
        check(len(ws) > 0 and len(gs) > 0, "%s: poses parsed (stored %d, imported %d)" % (example, len(ws), len(gs)))
        check(gs == ws, "%s: POSESCORE %s == stored %s" % (example, gs, ws))
        check(gl == wl, "%s: GEOMTRACE row labels equal" % example)
        check(len(gsteps) == len(wsteps) and all(
            len(a) == len(b) and max(abs(x - y) for x, y in zip(a, b)) < 1e-4
            for a, b in zip(gsteps, wsteps)), "%s: GEOMTRACE coordinates of every pose equal" % example)
    finally:
        shutil.rmtree(work, True)


for ex in ("mpro-nirmatrelvir", "hiv-protease-cyclic-urea", "acetylcholinesterase-huprine"):
    run(ex)

# an output with no ECCE frames carries no system: refused, nothing written
work = tempfile.mkdtemp(prefix="vinaimp-")
try:
    src = open(glob.glob(os.path.join(DOCK, "mpro-nirmatrelvir", "*.out"))[0]).read()
    open(os.path.join(work, "x.out"), "w").write(src.split("ECCE-DOCK-FRAME")[0])
    r = subprocess.run([sys.executable, EXPT, "x.out"], cwd=work, capture_output=True, text=True)
    check(r.returncode == 1 and not os.path.exists(os.path.join(work, "x.frag")),
          "output without poses refused")
finally:
    shutil.rmtree(work, True)

print("%d failure(s)" % len(failures))
sys.exit(1 if failures else 0)
