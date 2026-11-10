#!/usr/bin/env python3
"""Gaussian 16 properties carry the unit their values are in (#946).

    tests/parsers/g16_units_test.py

Every fixture under fixtures/gaussian-16 is run through gaussian-16.desc; each
record's declared unit must be the one in UNITS, so a new property or a copied
script with the wrong unit fails here.  The coordinate and gradient tables
are also read straight from the .log with regexes of this file's own, so the
unit is checked against the numbers and not only against a list:
GEOMTRACE/IRCGEOM against the "(Angstroms)" tables, EGRADVEC against the
negated "Forces (Hartrees/Bohr)" table and EGRAD against "Cartesian Forces: Max".
results.h5 drops a trace whose length unit it does not know, so a wrong label
is not cosmetic.
"""
import atexit
import glob
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, "scripts", "parsers")
sys.path.insert(0, HERE)
from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output)

ENERGY = "Hartree"
UNITS = {
    "GEOMTRACE": "Angstrom", "IRCGEOM": "Angstrom", "SCANGEOM": "Angstrom",
    "IRCCOORD": "sqrt(amu)*bohr", "IRCENERGY": ENERGY,
    "SCANCOORD": ("Angstrom", "Degree"), "SCANENERGY": ENERGY,
    "EGRAD": "Hartree/Bohr", "EGRADVEC": "Hartree/Bohr",
    "DIPOLE": "Debye", "VIB": "Angstrom", "VIBFREQ": "cm-1", "VIBIR": "KM/Mole",
    "VIBRAM": "A^4/AMU", "ANHARMFREQ": "cm-1", "ANHARMIR": "KM/Mole",
    "MULLIKEN": "e", "APT": "e", "ESPFIT": "e", "HIRSHFELD": "e", "CM5": "e",
    "AIM": "e", "NPA": "e",
    "ISOSHIELD": "ppm", "ANISOSHIELD": "ppm", "SHIELDTENSOR": "ppm",
    "SHIELDEIGVAL": "ppm",
    "EXCIENERGY": "eV", "EXCIOSC": "f",
    "THERMOTEMP": "Kelvin", "THERMOPRES": "atm",
    "CODEBASIS": "NA", "SOLVMODEL": "NA", "THERMOSIGMA": "NA",
}
# Energies (and Gaussian's legacy RMSDP label, a density change)
for k in ("ESCF", "ESCFVEC", "TE", "TEVEC", "KE", "NNREPUL", "DELTAE", "RMSDP",
          "EZEROPT", "ETHERM", "ENTHALPY", "EGIBBS", "ENTROPY", "ESOLVCDS",
          "MOENERGIES", "MOBETAENERGIES"):
    UNITS[k] = ENERGY
ENERGY_PREFIX = ("EMP", "ECC", "ECI", "EDFT")

FLOAT = r"-?\d+\.\d+"
ROW = re.compile(r"^\s+\d+\s+(-?\d+)\s+(?:-?\d+\s+)?(%s)\s+(%s)\s+(%s)\s*$"
                 % (FLOAT, FLOAT, FLOAT))


def tables(text, header, end_rule=True):
    """Numeric rows of every table whose header line contains `header`."""
    out = []
    lines = text.splitlines()
    for i, ln in enumerate(lines):
        if header not in ln:
            continue
        rows, j = [], i + 1
        while j < len(lines) and not ROW.match(lines[j]):
            j += 1
        while j < len(lines) and ROW.match(lines[j]):
            m = ROW.match(lines[j])
            if int(m.group(1)) >= 0:
                rows.append([float(m.group(k)) for k in (2, 3, 4)])
            j += 1
        out.append(rows)
    return out


def runtype(path):
    name = os.path.basename(path)
    return "IRC" if "irc" in name else "GeoVib"


def records(path):
    workdir = tempfile.mkdtemp(prefix="ecce-g16units-")
    atexit.register(shutil.rmtree, workdir, True)
    desc = read_desc(os.path.join(SCRIPTS, "gaussian-16.desc"))
    result = replay(desc, path)
    delivered = sorted(((b.begin_line, e, b) for e in desc.live_entries()
                        for b in result.delivered_for(e)), key=lambda t: t[0])
    recs = []
    for _l, entry, block in delivered:
        out, _e, _rc = run_parser(SCRIPTS, entry, block,
                                  (".", runtype(path), "SCF", "RHF", "0"),
                                  workdir=workdir)
        recs += parse_parser_output(out)
    return recs


def floats(rec):
    return [float(x) for x in rec["flat"].get("values", "").split()]


def close(a, b, tol=2e-6):
    return len(a) == len(b) and all(abs(x - y) <= tol for x, y in zip(a, b))


def main():
    bad = []
    seen, unlisted = set(), set()
    for path in sorted(glob.glob(os.path.join(HERE, "fixtures", "gaussian-16",
                                              "*.log"))):
        name = os.path.basename(path)
        text = open(path, errors="replace").read()
        recs = records(path)
        for rec in recs:
            key, unit = rec["key"], rec["flat"].get("units")
            if unit is None:
                continue
            want = UNITS.get(key)
            if want is None and key.startswith(ENERGY_PREFIX):
                want = ENERGY
            if want is None:
                unlisted.add("%s=%s" % (key, unit))
                continue
            seen.add(key)
            if unit not in (want if isinstance(want, tuple) else (want,)):
                bad.append("%s: %s declares %r, expected %r" % (name, key, unit, want))

        def frames(key):
            return [floats(r) for r in recs if r["key"] == key]

        # Coordinates: the parsed frames are the "(Angstroms)" tables.
        ang = [sum(t, []) for t in tables(text, "Coordinates (Angstroms)")]
        for key in ("GEOMTRACE", "IRCGEOM"):
            for k, f in enumerate(frames(key)):
                if not any(close(f, a) for a in ang):
                    bad.append("%s: %s frame %d is no Angstrom table of the log"
                               % (name, key, k + 1))
        # Gradient: minus a forces table, Hartree/Bohr.  Which table is
        # taken is the desc's business, not this test's.
        forces = [[-x for x in sum(t, [])] for t in
                  tables(text, "Forces (Hartrees/Bohr)")]
        for f in frames("EGRADVEC"):
            if not any(close(f, t, 1e-9) for t in forces):
                bad.append("%s: EGRADVEC is not the negated Hartree/Bohr forces"
                           % name)
        maxes = [float(m) for m in re.findall(
            r"Cartesian Forces:\s+Max\s+(%s)" % FLOAT, text)]
        for f in frames("EGRAD"):
            if not any(close(f, [m], 1e-9) for m in maxes):
                bad.append("%s: EGRAD is no Cartesian Forces Max of the log" % name)
        print("checked %s: %d records" % (name, len(recs)))
    for key in ("GEOMTRACE", "IRCGEOM", "EGRADVEC", "EGRAD"):
        if key not in seen:
            bad.append("no fixture produced %s" % key)
    if unlisted:
        bad.append("units not in UNITS: " + ", ".join(sorted(unlisted)))
    for b in bad:
        print("FAIL " + b)
    print("ok" if not bad else "%d failures" % len(bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
