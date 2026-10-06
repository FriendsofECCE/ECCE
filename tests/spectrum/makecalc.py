#!/usr/bin/env python3
"""Build a calculation directory whose vibrational properties come from the
real parser scripts run over a real output file.

    makecalc.py <case> <out-dir>        case: a tests/parsers case name

The directory starts as a copy of tests/apps/fixtures/calc-water-vib (a
Builder-loadable calculation) with VIB, VIBFREQ, VIBIR, VIBRAM and VIBSYM
replaced by what scripts/parsers produced for <case>, written in the format
the property store uses.  For a case with no Raman or no symmetry labels
those files are removed.  Used by tests/spectrum/capture.py.
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(ROOT, "tests", "parsers")
sys.path.insert(0, PARSERS)

import cases as CASEDEFS                                    # noqa: E402
from eccejobmonitor_sim import (read_desc, replay, run_parser,  # noqa: E402
                                parse_parser_output)

BASE = os.path.join(ROOT, "tests", "apps", "fixtures", "calc-water-vib")

CO_MVM = """title:
type: molecule
fragment_attributes:
  point_group: C1
end_fragment_attributes:
num_atoms: 2
atom_info: symbol cart
atom_list:
C 0.00000 0.00000 0.00000
O 0.00000 0.00000 1.12800
attr_list:
 0.000000   1   0   0   0
 0.000000   1   0   0   0
atom_type_list:
        1   2
        1   2
num_bonds: 1
bond_list:
1 2 1.00000 2
"""


def parse(caseName):
    """{key: flat record} from the real parsers over the case's fixture."""
    case = [c for c in CASEDEFS.CASES if c["name"] == caseName][0]
    desc = read_desc(os.path.join(ROOT, "scripts", "parsers", case["desc"]))
    result = replay(desc, os.path.join(PARSERS, "fixtures", case["fixture"]))
    found = {}
    for entry in desc.live_entries():
        for block in result.delivered_for(entry):
            out, _err, _rc = run_parser(
                os.path.join(ROOT, "scripts", "parsers"), entry, block,
                case.get("parse_args"))
            for rec in parse_parser_output(out):
                found[rec["key"]] = rec["flat"]
    return found


def labels(flat, count):
    text = flat.get("rowlabels") or flat.get("columnlabels") or ""
    return text if text else " ".join(str(i + 1) for i in range(count))


def vector(key, flat):
    values = flat["values"].split()
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="%s" rowLabel="Normal Mode" rowLabels="%s" '
            'rows="%d" units="%s">%s</vector>\n'
            % (key, labels(flat, len(values)), len(values),
               flat.get("units", "NA"), "   ".join(values)))


def vecstring(key, flat):
    values = flat["values"].split()
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vecstring name="%s" rowLabel="Normal Mode" rows="%d" '
            'units="NA">%s</vecstring>\n'
            % (key, len(values), "   ".join(values)))


def vectable(flat):
    vectors, rows, columns = [int(x) for x in flat["size"].split()]
    values = flat["values"].split()
    assert len(values) == vectors * rows * columns, "VIB size mismatch"
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
           '<vectable columnLabel="Atom" columnLabels="%s" columns="%d" '
           'name="VIB" rowLabel="Normal Mode" rowLabels="%s" rows="%d" '
           'units="NA" vectorLabel="Coordinate" vectorLabels="%s" '
           'vectors="%d">\n\n  <step number="0"/>\n\n'
           % (flat.get("columnlabels", "X Y Z"), columns,
              flat.get("rowlabels", ""), rows,
              flat.get("vectorlabels", ""), vectors)]
    for v in range(vectors):
        chunk = values[v * rows * columns:(v + 1) * rows * columns]
        out.append('  <matrix number="%d">%s</matrix>\n\n'
                   % (v + 1, "\n".join(chunk)))
    out.append("</vectable>\n")
    return "".join(out)


def make(caseName, out):
    props = parse(caseName)
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(BASE, out)
    pdir = os.path.join(out, "Props")

    def write(key, text):
        with open(os.path.join(pdir, key), "w") as f:
            f.write(text)
        meta = os.path.join(pdir, ".DAV", key)
        if not os.path.exists(meta):
            shutil.copy(os.path.join(pdir, ".DAV", "VIBFREQ"), meta)

    def drop(key):
        for path in (os.path.join(pdir, key), os.path.join(pdir, ".DAV", key)):
            if os.path.exists(path):
                os.remove(path)

    write("VIBFREQ", vector("VIBFREQ", props["VIBFREQ"]))
    write("VIB", vectable(props["VIB"]))
    for key in ("VIBIR", "VIBRAM"):
        if key in props:
            write(key, vector(key, props[key]))
        else:
            drop(key)
    if "VIBSYM" in props:
        write("VIBSYM", vecstring("VIBSYM", props["VIBSYM"]))
    if props["VIB"]["size"].split()[1] == "2":
        with open(os.path.join(out, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(CO_MVM)
    return props


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(2)
    got = make(sys.argv[1], sys.argv[2])
    print("%s: %s" % (sys.argv[1],
                      ", ".join(k for k in sorted(got) if k.startswith("VIB"))))
