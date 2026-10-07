#!/usr/bin/env python3
"""A calculation whose step vectors come from the real parser scripts.

    makecalc.py <case> <out-dir>        case: a tests/parsers case name

Starts from tests/apps/fixtures/calc-water-vib (orbitals, vibrations) and
writes GEOMTRACE, the geometry-step vectors (TEVEC, ESCFVEC) and the
wave-step vectors (DELTAE, RMSDP, DEWVEC) that the parsers produced for
<case>, in the format the property store uses.  Used by
tests/plots/capture.py.
"""
import importlib.util
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(ROOT, "tests", "parsers")
BASE = os.path.join(ROOT, "tests", "apps", "fixtures", "calc-water-vib")
sys.path.insert(0, PARSERS)

from eccejobmonitor_sim import (read_desc, replay, run_parser,  # noqa: E402
                                parse_parser_output)

GEOMETRY_STEP = ("TEVEC", "ESCFVEC")
WAVE_STEP = ("DELTAE", "RMSDP", "DEWVEC")


def records(caseName):
    """{key: [flat record, ...]} in the order the parsers delivered them."""
    spec = importlib.util.spec_from_file_location(
        "parser_cases", os.path.join(PARSERS, "cases.py"))
    cases = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cases)
    case = [c for c in cases.CASES if c["name"] == caseName][0]
    desc = read_desc(os.path.join(ROOT, "scripts", "parsers", case["desc"]))
    result = replay(desc, os.path.join(PARSERS, "fixtures", case["fixture"]))
    found = {}
    for entry in desc.live_entries():
        for block in result.delivered_for(entry):
            out, _err, _rc = run_parser(
                os.path.join(ROOT, "scripts", "parsers"), entry, block,
                case.get("parse_args"))
            for rec in parse_parser_output(out):
                found.setdefault(rec["key"], []).append(rec["flat"])
    return found


def geomtrace(steps):
    first = steps[0]
    _v, rows, columns = first["size"].split()
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
           '<tsvectable columnLabel="Atom" columnLabels="%s" columns="%s" '
           'name="GEOMTRACE" rowLabel="Geometry Step" rowLabels="%s" '
           'rows="%s" units="%s" vectorLabel="Coordinate" vectors="1">\n'
           % (first.get("columnlabels", "X Y Z"), columns,
              first.get("rowlabels", ""), rows, first.get("units", "Angstrom"))]
    for i, step in enumerate(steps):
        out.append('  <step number="%d">%s</step>\n'
                   % (i + 1, " ".join(step["values"].split())))
    out.append("</tsvectable>\n")
    return "".join(out)


def tsvector(name, rowLabel, steps):
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
           '<tsvector name="%s" rowLabel="%s" units="%s">\n'
           % (name, rowLabel, steps[0].get("units", ""))]
    for i, step in enumerate(steps):
        out.append('  <step number="%d">%s</step>\n'
                   % (i + 1, step["values"].split()[0]))
    out.append("</tsvector>\n")
    return "".join(out)


def make(caseName, out):
    found = records(caseName)
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(BASE, out)
    props = os.path.join(out, "Props")
    template = os.path.join(props, ".DAV", "VIBFREQ")
    wrote = []

    def put(name, text):
        with open(os.path.join(props, name), "w") as handle:
            handle.write(text)
        if name != "GEOMTRACE":
            shutil.copy(template, os.path.join(props, ".DAV", name))
        wrote.append(name)

    if len(found.get("GEOMTRACE", [])) >= 2:
        put("GEOMTRACE", geomtrace(found["GEOMTRACE"]))
    for key in GEOMETRY_STEP:
        if len(found.get(key, [])) >= 2:
            put(key, tsvector(key, "Geometry Step", found[key]))
    for key in WAVE_STEP:
        if len(found.get(key, [])) >= 2:
            put(key, tsvector(key, "Wave Step", found[key]))
    #  Orbital energies, occupations and symmetries (the MO panel's plots)
    for key, tag in (("ORBENG", "vector"), ("ORBOCC", "vector"),
                     ("ORBSYM", "vecstring")):
        if key not in found:
            continue
        rec = found[key][-1]
        values = rec["values"].split()
        if tag == "vector":
            text = ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                    '<vector name="%s" rowLabel="Basis Function" rows="%d" '
                    'units="%s">%s</vector>\n'
                    % (key, len(values), rec.get("units", "NA"),
                       "   ".join(values)))
        else:
            text = ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                    '<vecstring name="%s" rowLabel="Basis Function" rows="%d" '
                    'units="NA">%s</vecstring>\n'
                    % (key, len(values), "   ".join(values)))
        put(key, text)
    return wrote


def table(name, columnLabels, rows, units="NA"):
    """A PropTable file: rows is a list of equal-length value lists."""
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<table columnLabel="Property" columnLabels="%s" columns="%d" '
            'name="%s" rowLabel="Index" rowLabels="%s" rows="%d" '
            'units="%s">\n%s\n</table>\n'
            % (columnLabels, len(rows[0]), name,
               " ".join(str(i + 1) for i in range(len(rows))), len(rows),
               units, "\n".join(" ".join("%.8g" % v for v in r)
                                for r in rows)))


def make_synthetic(out):
    """Kinetics and metadynamics properties with SYNTHETIC numbers.

    There is no Polyrate or metadynamics output among the parser fixtures,
    so these are smooth made-up curves in the layouts the real parsers
    (scripts/parsers/poly.rates, metadyn.desc) write: an Arrhenius pair
    with error bounds, an equilibrium constant, a double-well free energy,
    and a reaction path.  They test how the plots look, not any chemistry.
    """
    import math
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(BASE, out)
    props = os.path.join(out, "Props")
    template = os.path.join(props, ".DAV", "VIBFREQ")

    def put(name, text):
        with open(os.path.join(props, name), "w") as handle:
            handle.write(text)
        shutil.copy(template, os.path.join(props, ".DAV", name))

    temps = [200.0 + 100.0 * i for i in range(9)]

    def lnk(t, ea, a=30.0):
        return a - ea * 1000.0 / t / 1.987 * 1.0

    tst = [lnk(t, 12.0) for t in temps]
    cvt = [lnk(t, 12.4) + 0.15 * (1000.0 / t) for t in temps]
    rows9, rows5, rows9b = [], [], []
    for i, t in enumerate(temps):
        inv = 1000.0 / t
        d1, d2 = 0.15 * inv + 0.1, 0.3 * inv + 0.2
        rows5.append([t, inv, tst[i], cvt[i], tst[i] - cvt[i]])
        rows9.append([t, inv, tst[i], tst[i] - d1, tst[i] + d1, cvt[i],
                      cvt[i] - d1, cvt[i] + d1, tst[i] - cvt[i]])
        rows9b.append([t, inv, tst[i], tst[i] - d2, tst[i] + d2, cvt[i],
                       cvt[i] - d2, cvt[i] + d2, tst[i] - cvt[i]])
    five = "T 1000/T ln(k_TST) ln(k_CVT) ln(k_TST/k_CVT)"
    nine = ("T 1000/T ln(k_TST) ln(k_TST-lower) ln(k_TST-upper) ln(k_CVT) "
            "ln(k_CVT-lower) ln(k_CVT-upper) ln(k_TST/k_CVT)")
    for direction in ("FORWARD", "REVERSE"):
        put(direction + "_RATE", table(direction + "_RATE", five, rows5))
        put(direction + "_RATE1", table(direction + "_RATE1", nine, rows9))
        put(direction + "_RATE2", table(direction + "_RATE2", nine, rows9b))
    eq = []
    for t in temps:
        lnK = 4.0 + 5.0 * 1000.0 / t / 1.987 * 0.8 - 8.0
        eq.append([t, math.exp(lnK), lnK, 1000.0 / t])
    put("EQUILIBRIUM_CONST",
        table("EQUILIBRIUM_CONST", "T K ln(K) 1000/T", eq))

    positions = [0.5 + 0.1 * i for i in range(61)]
    free = [-(8.0 * math.exp(-((x - 2.0) ** 2) / 0.08)
              + 5.0 * math.exp(-((x - 4.2) ** 2) / 0.2)
              - 0.5 * x) for x in positions]
    put("METAVEC",
        '<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
        '<vector name="METAVEC" rowLabel="Position" rowLabels="%s" rows="%d" '
        'units="Kcal/mole">%s</vector>\n'
        % (" ".join("%.2f" % x for x in positions), len(positions),
           "   ".join("%.6f" % f for f in free)))
    put("IKCPVEC",
        '<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
        '<tsvector name="IKCPVEC" rowLabel="Geometry Step" units="Hartree">\n'
        + "".join('  <step number="%d">%.8f</step>\n' % (i + 1, 0.0012)
                  for i in range(20)) + "</tsvector>\n")

    #  A reaction path: back from the saddle to reactants, then forward.
    coord = [0.0, -0.3, -0.6, -0.9, -1.2, 0.0, 0.3, 0.6, 0.9, 1.2]
    energy = [12.0 - 1.2 * c * c - (0.8 if c > 0 else 0.0) * c for c in coord]
    for name, rowLabel, vals in (("RXCOORD", "Reaction Step", coord),
                                 ("RXENERGY", "Reaction Step", energy)):
        put(name, tsvector(name, rowLabel,
                           [{"values": "%.6f" % v, "units": "Bohr"
                             if name == "RXCOORD" else "kcal/mol"}
                            for v in vals]))
    base = records("g16-h2o-optfreq")
    steps = base["GEOMTRACE"]
    trace = geomtrace((steps * 3)[:len(coord)]).replace(
        'name="GEOMTRACE" rowLabel="Geometry Step"',
        'name="RXTRACE" rowLabel="Reaction Step"')
    put("RXTRACE", trace)
    return os.listdir(props)


if __name__ == "__main__":
    if sys.argv[1] == "synthetic":
        print(" ".join(make_synthetic(sys.argv[2])))
    else:
        print(" ".join(make(sys.argv[1], sys.argv[2])))
