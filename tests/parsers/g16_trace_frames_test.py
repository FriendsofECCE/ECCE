#!/usr/bin/env python3
"""Gaussian 16 stores one GEOMTRACE frame per optimisation step (#498).

Input, Z-Matrix and Standard orientation blocks all feed GEOMTRACE; the
Geometry Trace panel indexes frames by energy point, so frames must equal
TEVEC points.  Standard wins (modes use it, #312); with symmetry off the
Input block is the frame.

    tests/parsers/g16_trace_frames_test.py [extra.log ...]
"""
import atexit
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, "scripts", "parsers")
sys.path.insert(0, HERE)
from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output)

# (fixture, runtype, steps)
CASES = [
    ("gaussian-16/h2o_optfreq.log", "GeoVib", 4),
    ("gaussian-16/nh3_dummy_zmat_opt.log", "Geometry", 3),
    ("gaussian-16/octane_pcm.log", "Geometry", 8),
    ("gaussian-16/h2co_nosymm_opt.log", "Geometry", 8),
]


def count(path, runtype):
    workdir = tempfile.mkdtemp(prefix="ecce-g16trace-")
    atexit.register(shutil.rmtree, workdir, True)
    desc = read_desc(os.path.join(SCRIPTS, "gaussian-16.desc"))
    result = replay(desc, path)
    delivered = sorted(((b.begin_line, e, b) for e in desc.live_entries()
                        for b in result.delivered_for(e)), key=lambda t: t[0])
    frames, tevec = [], 0
    for _l, entry, block in delivered:
        out, _e, _rc = run_parser(SCRIPTS, entry, block,
                                  (".", runtype, "SCF", "RHF", "0"),
                                  workdir=workdir)
        for rec in parse_parser_output(out):
            if rec["key"] == "GEOMTRACE":
                frames.append(rec["flat"]["values"])
            elif rec["key"] == "TEVEC":
                tevec += 1
    return frames, tevec


def main():
    paths = [(os.path.join(HERE, "fixtures", f), r, n) for f, r, n in CASES]
    paths += [(p, "Geometry", None) for p in sys.argv[1:]]
    bad = 0
    for path, runtype, steps in paths:
        frames, tevec = count(path, runtype)
        # A job still running has printed the next geometry before its energy.
        lead = len(frames) - tevec
        ok = lead == 0 if steps else lead in (0, 1)
        ok = ok and (steps is None or tevec == steps)
        ok = ok and len(set(frames)) == len(frames)
        print("%s %s: %d frames, %d TEVEC points" %
              ("ok  " if ok else "FAIL", os.path.basename(path),
               len(frames), tevec))
        bad += not ok
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
