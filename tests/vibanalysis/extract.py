"""Real code output -> modes file for vibanalysis_cli (#367).

The text goes through the real .desc files and parser scripts (the same
replay tests/parsers uses), so the geometry, the normal modes and the
code's own VIBSYM labels are what ECCE would store for the job.
"""
import atexit
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "tests", "parsers")
SCRIPTS = os.path.join(REPO, "scripts", "parsers")
sys.path.insert(0, PARSERS)

from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output)


def records(fixture, descName, runtype):
    """{key: [flat record, in file order]} over a fixture path."""
    desc = read_desc(os.path.join(SCRIPTS, descName))
    result = replay(desc, fixture)
    delivered = []
    for entry in desc.live_entries():
        for block in result.delivered_for(entry):
            delivered.append((block.begin_line, entry, block))
    delivered.sort(key=lambda t: t[0])
    found = {}
    #  One directory for all the scripts, as in the real monitor: mopac.vib
    #  reads what mopac.geomtrace left there.
    workdir = tempfile.mkdtemp(prefix="ecce-vibanalysis-")
    atexit.register(shutil.rmtree, workdir, True)
    for _line, entry, block in delivered:
        out, _err, _rc = run_parser(SCRIPTS, entry, block,
                                    (".", runtype, "SCF", "RHF", "0"),
                                    workdir=workdir)
        for rec in parse_parser_output(out):
            found.setdefault(rec["key"], []).append(rec["flat"])
    return found


def floats(text):
    return [float(t) for t in text.replace("D", "e").split()]


def modes_file(fixture, descName, runtype, elements=None, bonds=None,
               geometry=None):
    """(text for vibanalysis_cli, code's VIBSYM labels or [], frequencies)."""
    found = records(fixture, descName, runtype)
    geo = found["GEOMTRACE"][-1]
    xyz = floats(geo["values"])
    labels = geo["rowlabels"].split()
    if elements is None:
        elements = [lab.split("-")[1] for lab in labels]
    if geometry is not None:
        elements, xyz = geometry
    n = len(elements)
    vib = found["VIB"][-1]
    nmodes, natoms, _ = (int(v) for v in vib["size"].split())
    assert natoms == n == len(xyz) // 3, (natoms, n, len(xyz))
    flat = floats(vib["values"])
    freqs = floats(found["VIBFREQ"][-1]["values"])
    assert len(freqs) == nmodes
    lines = [str(n)]
    for a in range(n):
        lines.append("%s %.8f %.8f %.8f" % ((elements[a],) + tuple(xyz[3 * a:3 * a + 3])))
    if bonds:
        lines.append("bonds %d" % len(bonds))
        lines += ["%d %d %g" % b for b in bonds]
    lines.append("modes %d" % nmodes)
    for m in range(nmodes):
        lines.append("%.4f" % freqs[m])
        for a in range(n):
            lines.append(" ".join("%.8f" % v
                                  for v in flat[(m * n + a) * 3:(m * n + a) * 3 + 3]))
    sym = found.get("VIBSYM", [{}])[-1].get("values", "").split()
    return "\n".join(lines) + "\n", sym, freqs
