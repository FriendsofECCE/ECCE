#!/usr/bin/env python3
"""QE relax / vc-relax: one TEVEC per GEOMTRACE frame, each its own energy (#1003).

    tests/parsers/qe_trace_align_test.py [pwout ...]

pw.x prints the ATOMIC_POSITIONS block after an SCF, and it is the geometry of
the next SCF.  The oracle is this file's own reading of the output: frame k
is the k-th ATOMIC_POSITIONS block (the final repeat counts only when a
final SCF follows it, as in vc-relax), its energy the (k+1)-th "!" energy (the first "!" belongs to the starting
geometry, which has no block), and for vc-relax the k-th CELL_PARAMETERS is
frame k's cell.  Fixtures are the committed ones; any pwout given on the
command line (e.g. a real run) is checked too.
"""
import atexit
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPTS = os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts", "parsers")
FIX = os.path.join(HERE, "fixtures", "qe")
sys.path.insert(0, HERE)
from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output)

NUM = r"-?\d+\.\d+"
ENERGY = re.compile(r"^!\s+total energy\s+=\s+(%s) Ry" % NUM)
ROW = re.compile(r"^\s*[A-Za-z]{1,2}\d*\s+(%s)\s+(%s)\s+(%s)" % (NUM, NUM, NUM))
CELL = re.compile(r"^\s*(%s)\s+(%s)\s+(%s)\s*$" % (NUM, NUM, NUM))
DEFAULT = [os.path.join(FIX, "h2o_relax.pwout"), os.path.join(FIX, "si_vcrelax.pwout")]


def oracle(path):
    lines = open(path, errors="replace").read().splitlines()
    energies, frames, cells = [], [], []
    for i, ln in enumerate(lines):
        m = ENERGY.match(ln)
        if m:
            energies.append(float(m.group(1)) / 2)
        if ln.startswith("ATOMIC_POSITIONS"):
            rows = []
            for r in lines[i + 1:]:
                m = ROW.match(r)
                if not m:
                    break
                rows += [float(x) for x in m.groups()]
            frames.append(rows)
        elif ln.startswith("CELL_PARAMETERS"):
            cells.append([float(x) for r in lines[i + 1:i + 4]
                          for x in CELL.match(r).groups()])
    return energies, frames, cells


def parsed(path):
    workdir = tempfile.mkdtemp(prefix="ecce-qealign-")
    atexit.register(shutil.rmtree, workdir, True)
    desc = read_desc(os.path.join(SCRIPTS, "qe.desc"))
    result = replay(desc, path)
    delivered = sorted(((b.begin_line, e, b) for e in desc.live_entries()
                        for b in result.delivered_for(e)), key=lambda t: t[0])
    out = {}
    for _l, entry, block in delivered:
        text, _e, _rc = run_parser(SCRIPTS, entry, block,
                                   (".", "GeomOpt", "PW", "PW", "0"), workdir=workdir)
        for r in parse_parser_output(text):
            if r["key"] not in ("GEOMTRACE", "TEVEC", "LATTICEVEC"):
                continue
            out.setdefault(r["key"], []).append(
                [float(x) for x in r["flat"].get("values", "").split()])
    return out


def close(a, b, tol):
    return len(a) == len(b) and all(abs(x - y) <= tol for x, y in zip(a, b))


def check(path, bad):
    name = os.path.basename(path)
    energies, frames, cells = oracle(path)
    got = parsed(path)
    geom, tevec, lat = (got.get(k, []) for k in ("GEOMTRACE", "TEVEC", "LATTICEVEC"))
    n = min(len(frames), len(energies) - 1)
    if len(geom) != len(tevec):
        bad.append("%s: %d GEOMTRACE frames, %d TEVEC" % (name, len(geom), len(tevec)))
    if len(geom) != max(n, 0):
        bad.append("%s: %d frames, expected %d" % (name, len(geom), max(n, 0)))
        return
    if n < 1:
        print("%s: no optimisation steps, no frames" % name)
        return
    if cells and len(lat) != len(geom):
        bad.append("%s: %d LATTICEVEC for %d frames" % (name, len(lat), len(geom)))
    for k in range(n):
        if not close(geom[k], frames[k], 1e-8):
            bad.append("%s: frame %d is not ATOMIC_POSITIONS block %d" % (name, k + 1, k + 1))
        if not close(tevec[k], [energies[k + 1]], 1e-8):
            bad.append("%s: TEVEC %d is not the energy of frame %d" % (name, k + 1, k + 1))
        if cells and k < len(lat) and not close(lat[k], cells[k], 1e-8):
            bad.append("%s: cell %d is not frame %d's" % (name, k + 1, k + 1))
    print("%s: %d frames, %d TEVEC, first TEVEC %.8f Ha" % (name, len(geom), len(tevec), tevec[0][0]))


def main():
    bad = []
    for p in (sys.argv[1:] or DEFAULT):
        check(p, bad)
    for b in bad:
        print("FAIL", b)
    print("ok" if not bad else "FAILED")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
