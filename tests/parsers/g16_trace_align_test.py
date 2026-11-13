#!/usr/bin/env python3
"""Gaussian 16 traces line up point for point with their energies (#947).

    tests/parsers/g16_trace_align_test.py

results.h5 keeps a trace's energies only when there is one per structure, and
the views index structures by energy point.  Each check reads the .log with
regexes of this file's own:

* optimisation: one GEOMTRACE frame and one TEVEC per "SCF Done" before
  "Optimization completed", the frame being the last "(Angstroms)" table
  printed before that energy;
* IRC: one IRCGEOM structure per summary row; each non-TS structure is a
  CURRENT STRUCTURE table whose energy (the "SCF Done" before it) is that
  row's, and the TS row holds the starting structure (same interatomic
  distances), placed between its two neighbours;
* EGRADVEC: the last "Forces (Hartrees/Bohr)" table, negated.
"""
import atexit
import math
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, "scripts", "parsers")
FIX = os.path.join(HERE, "fixtures", "gaussian-16")
sys.path.insert(0, HERE)
from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output)

OPT = [os.path.join(FIX, f) for f in ("h2o_optfreq.log", "h2o_freq_thermo.log",
                                      "h2co_nosymm_opt.log", "octane_pcm.log")]
IRC = [os.path.join(FIX, "hcn_irc.log")]
FORCES = [os.path.join(FIX, f) for f in ("h2o_anharm.log", "h2o_freq_thermo.log",
                                         "h2o_optfreq.log")]

FLOAT = r"-?\d+\.\d+"
ROW = re.compile(r"^\s+\d+\s+(-?\d+)\s+(?:-?\d+\s+)?(%s)\s+(%s)\s+(%s)\s*$"
                 % (FLOAT, FLOAT, FLOAT))
SCF = re.compile(r"SCF Done:\s+E\(\S+\)\s+=\s+(%s)" % FLOAT)


def table_at(lines, i):
    """Rows of the numeric table that follows header line i."""
    rows, j = [], i + 1
    while j < len(lines) and not ROW.match(lines[j]):
        j += 1
    while j < len(lines) and ROW.match(lines[j]):
        m = ROW.match(lines[j])
        if int(m.group(1)) >= 0:
            rows.append([float(m.group(k)) for k in (2, 3, 4)])
        j += 1
    return rows


def records(path, rt):
    workdir = tempfile.mkdtemp(prefix="ecce-g16align-")
    atexit.register(shutil.rmtree, workdir, True)
    desc = read_desc(os.path.join(SCRIPTS, "gaussian-16.desc"))
    result = replay(desc, path)
    delivered = sorted(((b.begin_line, e, b) for e in desc.live_entries()
                        for b in result.delivered_for(e)), key=lambda t: t[0])
    recs = []
    for _l, entry, block in delivered:
        out, _e, _rc = run_parser(SCRIPTS, entry, block,
                                  (".", rt, "SCF", "DFT", "0"),
                                  workdir=workdir)
        recs += parse_parser_output(out)
    return recs


def values(recs, key):
    return [[float(x) for x in r["flat"].get("values", "").split()]
            for r in recs if r["key"] == key]


def flat(rows):
    return [x for r in rows for x in r]


def close(a, b, tol=2e-6):
    return len(a) == len(b) and all(abs(x - y) <= tol for x, y in zip(a, b))


def rmsd(a, b):
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)) / (len(a) / 3))


def distances(xyz):
    p = [xyz[k:k + 3] for k in range(0, len(xyz), 3)]
    return [math.dist(p[i], p[j]) for i in range(len(p)) for j in range(i)]


def check_opt(path, bad):
    name = os.path.basename(path)
    lines = open(path, errors="replace").read().splitlines()
    want_geom, want_e, geom = [], [], None
    for i, ln in enumerate(lines):
        if "Optimization completed" in ln:
            break
        if "Coordinates (Angstroms)" in ln:
            geom = flat(table_at(lines, i))
        m = SCF.search(ln)
        if m:
            want_geom.append(geom)
            want_e.append(float(m.group(1)))
    rt = "Transition State (QST) + Frequencies" if "qst" in name else "GeoVib"
    recs = records(path, rt)
    frames, tevec = values(recs, "GEOMTRACE"), values(recs, "TEVEC")
    if len(frames) != len(want_geom) or len(tevec) != len(want_e):
        bad.append("%s: %d GEOMTRACE frames and %d TEVEC points, the log has %d steps"
                   % (name, len(frames), len(tevec), len(want_e)))
        return
    for k, (f, g, e, w) in enumerate(zip(frames, want_geom, tevec, want_e)):
        if not close(f, g):
            bad.append("%s: GEOMTRACE frame %d is not step %d's structure" % (name, k + 1, k + 1))
        if not close(e, [w], 1e-7):
            bad.append("%s: TEVEC point %d is not step %d's energy" % (name, k + 1, k + 1))


def check_irc(path, bad):
    name = os.path.basename(path)
    text = open(path, errors="replace").read()
    lines = text.splitlines()
    structs, energy, start = [], None, None
    for i, ln in enumerate(lines):
        m = SCF.search(ln)
        if m:
            energy = float(m.group(1))
        if "Start new reaction path calculation" in ln and start is None:
            start = [k for k in range(i) if "Coordinates (Angstroms)" in lines[k]][-1]
        if "CURRENT STRUCTURE" in ln:
            hdr = next(k for k in range(i, len(lines)) if "Coordinates (Angstroms)" in lines[k])
            structs.append((energy, flat(table_at(lines, hdr))))
    ts_e = float(re.search(r"relative to the TS energy of\s+(%s)" % FLOAT, text).group(1))
    summary = text.split("Summary of reaction path following")[-1].split("Total number of points")[0]
    rows = [(float(e), float(x)) for e, x in
            re.findall(r"^\s+\d+\s+(%s)\s+(%s)\s*$" % (FLOAT, FLOAT), summary, re.M)]
    ts_geom = flat(table_at(lines, start))

    recs = records(path, "IRC")
    frames = values(recs, "IRCGEOM")
    if len(frames) != len(rows) or len(values(recs, "IRCENERGY")[0]) != len(rows):
        bad.append("%s: %d IRCGEOM structures for %d summary points"
                   % (name, len(frames), len(rows)))
        return
    t = min(range(len(rows)), key=lambda i: abs(rows[i][1]))
    for i, ((rel, _x), f) in enumerate(zip(rows, frames)):
        if i == t:
            continue
        if not any(close(f, g) and abs(e - (ts_e + rel)) <= 6e-6 for e, g in structs):
            bad.append("%s: IRCGEOM %d is no CURRENT STRUCTURE of that point's energy"
                       % (name, i + 1))
    f = frames[t]
    if not close(distances(f), distances(ts_geom), 1e-4):
        bad.append("%s: IRCGEOM %d (TS) is not the starting structure" % (name, t + 1))
    if 0 < t < len(rows) - 1:
        a, b = frames[t - 1], frames[t + 1]
        if max(rmsd(f, a), rmsd(f, b)) >= rmsd(a, b):
            bad.append("%s: IRCGEOM %d (TS) is not between its neighbours" % (name, t + 1))


def check_forces(path, bad):
    name = os.path.basename(path)
    lines = open(path, errors="replace").read().splitlines()
    last = [i for i, ln in enumerate(lines) if "Forces (Hartrees/Bohr)" in ln][-1]
    want = [-x for x in flat(table_at(lines, last))]
    got = values(records(path, "GeoVib"), "EGRADVEC")
    if not got or not close(got[-1], want, 1e-9):
        bad.append("%s: EGRADVEC is not the last Forces table" % name)


def main():
    bad = []
    for p in OPT:
        check_opt(p, bad)
    for p in IRC:
        check_irc(p, bad)
    for p in FORCES:
        check_forces(p, bad)
    for b in bad:
        print("FAIL " + b)
    print("ok" if not bad else "%d failures" % len(bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
