#!/usr/bin/env python3
"""Gaussian 16 traces line up point for point with their energies (#947).

    tests/parsers/g16_trace_align_test.py

results.h5 keeps a trace's energies only when there is one per structure, and
the views index structures by energy point.  Each check reads the .log with
regexes of this file's own:

* optimisation: one GEOMTRACE frame and one TEVEC per "SCF Done" before
  "Optimization completed", the frame being the last "(Angstroms)" table
  printed before that energy;
* IRC, in run order as the points arrive (#948): the TS first (the starting
  structure, same interatomic distances, coordinate 0), then each CURRENT
  STRUCTURE with its signed coordinate and the "SCF Done" before it; at the
  end the summary's values replace these in the same order; a log cut in
  the forward and in the reverse path gives the points so far.  The path
  order and the TS fit are the readers' (tests/resultsh5/trace_vib_test.py);
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
SN2 = os.path.join(REPO, "data", "client", "examples", "workflows", "sn2-cl-ch3br")
sys.path.insert(0, HERE)
from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output)

OPT = [os.path.join(FIX, f) for f in ("h2o_optfreq.log", "h2o_freq_thermo.log",
                                      "h2co_nosymm_opt.log", "octane_pcm.log")]
OPT += [os.path.join(SN2, d, n) for d, n in (
    ("g16-reactant", "sn2_reactant.out"), ("g16-product", "sn2_product.out"),
    ("g16-qst3", "sn2_qst3.out"))]
IRC = [os.path.join(FIX, "hcn_irc.log"), os.path.join(SN2, "g16-irc", "sn2_irc.out")]
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


def irc_log(lines):
    """Per CURRENT STRUCTURE: (signed NET coordinate, SCF energy before it,
    structure); and the structure the path starts from."""
    pts, energy, start, sign = [], None, None, 1
    for i, ln in enumerate(lines):
        m = SCF.search(ln)
        if m:
            energy = float(m.group(1))
        if "Start new reaction path calculation" in ln and start is None:
            start = [k for k in range(i) if "Coordinates (Angstroms)" in lines[k]][-1]
        if "path direction" in ln:
            sign = -1 if "REVERSE" in ln else 1
        if "CURRENT STRUCTURE" in ln:
            hdr = next(k for k in range(i, len(lines)) if "Coordinates (Angstroms)" in lines[k])
            net = next((k for k in range(i, len(lines))
                        if "NET REACTION COORDINATE" in lines[k]), None)
            if net is None:
                break
            x = float(lines[net].split("=")[1])
            pts.append((sign * x, energy, flat(table_at(lines, hdr))))
    return pts, (flat(table_at(lines, start)) if start is not None else None)


def check_run_order(name, recs, pts, ts_geom, bad):
    """IRCGEOM/IRCCOORD/IRCENERGY in run order (#948): the TS at coordinate 0,
    then one point per CURRENT STRUCTURE so far; frame i is energy i's."""
    frames = values(recs, "IRCGEOM")
    coord = (values(recs, "IRCCOORD") or [[]])[-1]
    energy = (values(recs, "IRCENERGY") or [[]])[-1]
    if not (len(frames) == len(coord) == len(energy) == len(pts) + 1):
        bad.append("%s: %d IRCGEOM, %d IRCCOORD, %d IRCENERGY for %d points and the TS"
                   % (name, len(frames), len(coord), len(energy), len(pts)))
        return None
    if coord[0] != 0 or not close(distances(frames[0]), distances(ts_geom), 1e-4):
        bad.append("%s: IRCGEOM 1 is not the TS the path starts from" % name)
    for i, (x, e, g) in enumerate(pts):
        if not close(frames[i + 1], g):
            bad.append("%s: IRCGEOM %d is not CURRENT STRUCTURE %d" % (name, i + 2, i + 1))
        if abs(coord[i + 1] - x) > 1e-5:
            bad.append("%s: IRCCOORD %d is %g, the log's point %g" % (name, i + 2, coord[i + 1], x))
        if abs(energy[i + 1] - e) > 6e-6:
            bad.append("%s: IRCENERGY %d is %.6f, the point's SCF %.6f"
                       % (name, i + 2, energy[i + 1], e))
    return frames, coord, energy


def check_irc(path, bad):
    name = os.path.basename(path)
    text = open(path, errors="replace").read()
    lines = text.splitlines()
    pts, ts_geom = irc_log(lines)
    ts_e = float(re.search(r"relative to the TS energy of\s+(%s)" % FLOAT, text).group(1))
    summary = text.split("Summary of reaction path following")[-1].split("Total number of points")[0]
    rows = [(float(e), float(x)) for e, x in
            re.findall(r"^\s+\d+\s+(%s)\s+(%s)\s*$" % (FLOAT, FLOAT), summary, re.M)]
    recs = records(path, "IRC")
    got = check_run_order(name, recs, pts, ts_geom, bad)
    if got is None:
        return
    _frames, coord, energy = got
    if len(coord) != len(rows):
        bad.append("%s: %d IRC points for %d summary rows" % (name, len(coord), len(rows)))
        return
    # The summary's values, matched to the run-order points by coordinate.
    for i, (x, e) in enumerate(zip(coord, energy)):
        if not any(abs(x - rx) <= 1e-5 and abs(e - (ts_e + rel)) <= 6e-6 for rel, rx in rows):
            bad.append("%s: IRC point %d (%g, %.6f) is no summary row" % (name, i + 1, x, e))


def check_truncated(path, bad):
    """A running IRC: the log cut in the middle of the forward path and of the
    reverse path gives the TS and every point finished so far (#948)."""
    name = os.path.basename(path)
    lines = open(path, errors="replace").read().splitlines()
    nets = [i for i, ln in enumerate(lines) if "NET REACTION COORDINATE" in ln]
    rev = next(i for i, ln in enumerate(lines) if "REVERSE path direction" in ln)
    fwd_n = sum(1 for i in nets if i < rev)
    for where, at in (("forward", nets[fwd_n // 2] + 40),
                      ("reverse", nets[fwd_n + (len(nets) - fwd_n) // 2] + 40)):
        cut = lines[:at]
        workdir = tempfile.mkdtemp(prefix="ecce-g16cut-")
        atexit.register(shutil.rmtree, workdir, True)
        cutfile = os.path.join(workdir, "cut.log")
        open(cutfile, "w").write("\n".join(cut) + "\n")
        pts, ts_geom = irc_log(cut)
        label = "%s cut in the %s path (%d points)" % (name, where, len(pts))
        got = check_run_order(label, records(cutfile, "IRC"), pts, ts_geom, bad)
        if got and where == "reverse" and not any(x < 0 for x in got[1]):
            bad.append("%s: no reverse point has a negative coordinate" % label)
        print("%s: %s" % (label, "checked" if got else "FAILED"))


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
        check_truncated(p, bad)
    for p in FORCES:
        check_forces(p, bad)
    for b in bad:
        print("FAIL " + b)
    print("ok" if not bad else "%d failures" % len(bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
