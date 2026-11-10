#!/usr/bin/env python3
"""ORCA IRC points arrive while the IRC runs (#949).

    tests/parsers/orca_irc_live_test.py

Each output is replayed through orca.desc as the monitor feeds it.  The
expected path is read from the output with regexes of this file's own: the
TS energy is the last FINAL SINGLE POINT ENERGY before the first IRC banner,
and row k of the FORWARD/BACKWARD IRC table is the point at coordinate
+(k+1)/-(k+1).  Checked:

* the whole output: the live IRCCOORD/IRCENERGY just before the summary hold
  every point in path order; the summary's vectors (the last ones, which a
  view reads) are the same points with the summary's numbers;
* the output cut in the forward and in the backward path: the TS and every
  row printed so far, in path order;
* a run that is not an IRC gives no IRC points.
"""
import atexit
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, "scripts", "parsers")
EX = os.path.join(REPO, "data", "client", "examples", "organic", "reactions")
sys.path.insert(0, HERE)
from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output)

OUTPUTS = [os.path.join(HERE, "fixtures", "orca", "hcn_irc.out"),
           os.path.join(EX, "orca-sn2_irc", "sn2_irc.out"),
           os.path.join(EX, "orca-diels_alder_irc", "diels_alder_irc.out")]
ROW = re.compile(r"^\s+(\d+)\s+(-\d+\.\d+)\s+-?\d+\.\d+\s+\d+\.\d+\s+\d+\.\d+\s*$")
BANNER = re.compile(r"^\s*\*\s+(FORWARD|BACKWARD) IRC\s+\*")
SUMMARY = "IRC PATH SUMMARY"


def expected(lines):
    """[(coordinate, energy)] in path order, from the output text."""
    pts, ts, sign = [], None, None
    for ln in lines:
        if SUMMARY in ln:
            break
        m = re.search(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", ln)
        if m and sign is None:
            ts = float(m.group(1))
        m = BANNER.match(ln)
        if m:
            sign = 1 if m.group(1) == "FORWARD" else -1
            continue
        m = ROW.match(ln)
        if m and sign is not None:
            pts.append((sign * (int(m.group(1)) + 1), float(m.group(2))))
    if sign is None:
        return []
    return sorted([(0, ts)] + pts)


def records(path, runtype="IRC"):
    """[(begin line, script, key, values)] for the IRC vectors, in feed order."""
    workdir = tempfile.mkdtemp(prefix="ecce-orcairc-")
    atexit.register(shutil.rmtree, workdir, True)
    desc = read_desc(os.path.join(SCRIPTS, "orca.desc"))
    result = replay(desc, path)
    delivered = sorted(((b.begin_line, e, b) for e in desc.live_entries()
                        for b in result.delivered_for(e)), key=lambda t: t[0])
    out = []
    for line, entry, block in delivered:
        text, err, rc = run_parser(SCRIPTS, entry, block,
                                   (".", runtype, "SCF", "DFT", "0"), workdir=workdir)
        if rc != 0:
            sys.exit("%s failed: %s" % (entry.script, err))
        for r in parse_parser_output(text):
            if r["key"] in ("IRCCOORD", "IRCENERGY"):
                out.append((line, entry.script, r["key"],
                            [float(x) for x in r["flat"].get("values", "").split()]))
    return out


def last(recs, key, script=None):
    got = [r[3] for r in recs if r[2] == key and (script is None or r[1] == script)]
    return got[-1] if got else []


def same(label, coord, energy, want, tol, bad):
    if len(coord) != len(energy) or len(coord) != len(want):
        bad.append("%s: %d IRCCOORD and %d IRCENERGY values for %d points"
                   % (label, len(coord), len(energy), len(want)))
        return False
    for i, (x, e, (wx, we)) in enumerate(zip(coord, energy, want)):
        if x != wx or abs(e - we) > tol:
            bad.append("%s: point %d is (%g, %.8f), the output's (%g, %.8f)"
                       % (label, i + 1, x, e, wx, we))
            return False
    return True


def check_whole(path, bad):
    name = os.path.basename(path)
    lines = open(path, errors="replace").read().splitlines()
    want = expected(lines)
    recs = records(path)
    ok = same(name + " live", last(recs, "IRCCOORD", "orca.ircpoint"),
              last(recs, "IRCENERGY", "orca.ircpoint"), want, 1e-9, bad)
    # The summary (6 decimals, the TS rounded) comes last and is what is kept.
    if recs and recs[-1][1] != "orca.irc":
        bad.append("%s: the summary is not the last IRC record" % name)
    ok = same(name + " summary", last(recs, "IRCCOORD"), last(recs, "IRCENERGY"),
              want, 6e-7, bad) and ok
    print("%s: %d points %s" % (name, len(want), "checked" if ok else "FAILED"))


def check_cut(path, bad):
    name = os.path.basename(path)
    lines = open(path, errors="replace").read().splitlines()
    banners = [i for i, ln in enumerate(lines) if BANNER.match(ln)]
    for where, start in zip(("forward", "backward"), banners):
        rows = [i for i in range(start, len(lines)) if ROW.match(lines[i])]
        rows = rows[:next((k for k in range(1, len(rows)) if rows[k] != rows[k - 1] + 1),
                          len(rows))]
        cut = lines[:rows[len(rows) // 2] + 1]
        workdir = tempfile.mkdtemp(prefix="ecce-orcacut-")
        atexit.register(shutil.rmtree, workdir, True)
        cutfile = os.path.join(workdir, "cut.out")
        open(cutfile, "w").write("\n".join(cut) + "\n")
        want = expected(cut)
        recs = records(cutfile)
        label = "%s cut in the %s path" % (name, where)
        ok = same(label, last(recs, "IRCCOORD"), last(recs, "IRCENERGY"), want, 1e-9, bad)
        if where == "backward" and not any(x < 0 for x, _e in want):
            bad.append("%s: no backward point" % label)
        print("%s (%d points): %s" % (label, len(want), "checked" if ok else "FAILED"))


def main():
    bad = []
    for p in OUTPUTS:
        check_whole(p, bad)
        check_cut(p, bad)
    if records(OUTPUTS[0], "Geometry"):
        bad.append("a run that is not an IRC gives IRC points")
    for b in bad:
        print("FAIL " + b)
    print("ok" if not bad else "%d failures" % len(bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
