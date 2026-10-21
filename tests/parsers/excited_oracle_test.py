#!/usr/bin/env python3
"""Excited-state properties equal what each code prints itself (#383).

The pipeline (eccejobmonitor_sim + the real parse scripts) gives EXCIENERGY,
EXCIOSC and EXCITRANS; the oracle is the raw output, read here with its own
regular expressions: ORCA's "ABSORPTION SPECTRUM VIA TRANSITION ELECTRIC DIPOLE
MOMENTS" table (eV, nm, fosc) and Gaussian's last "Excited State" lines (eV, nm,
f).  Each code is checked only against its own output.  The wavelength the
panel derives from the energy (hc/E) must reproduce the printed nm to the
rounding of that printout.

    tests/parsers/excited_oracle_test.py
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, "scripts", "parsers")
sys.path.insert(0, HERE)

from eccejobmonitor_sim import (read_desc, replay, run_parser,   # noqa: E402
                                parse_parser_output)

HC = 1239.8419843320026          # eV nm

failures = 0
checks = 0


def check(cond, what):
    global failures, checks
    checks += 1
    if not cond:
        failures += 1
        print("FAIL", what)


def pipeline(fixture, descName):
    desc = read_desc(os.path.join(SCRIPTS, descName))
    result = replay(desc, os.path.join(HERE, "fixtures", fixture))
    found = {}
    for entry in desc.live_entries():
        for block in result.delivered_for(entry):
            out, _err, _rc = run_parser(SCRIPTS, entry, block,
                                        (".", "Energy", "DFT", "B3LYP", "0"))
            for rec in parse_parser_output(out):
                found[rec["key"]] = rec["flat"]
    return found


def floats(rec):
    return [float(v) for v in rec["values"].split()]


def orca_oracle(path):
    rows, inside = [], False
    for line in open(path, errors="replace"):
        if "ABSORPTION SPECTRUM VIA TRANSITION ELECTRIC DIPOLE MOMENTS" in line:
            inside = True
            continue
        if inside:
            m = re.match(r"\s*\d+-\d+\S*\s+->\s+\d+-\d+\S*\s+(\d+\.\d+)\s+"
                         r"\d+\.\d+\s+(\d+\.\d+)\s+(\d+\.\d+)", line)
            if m:
                rows.append(tuple(float(v) for v in m.groups()))
            elif rows:
                break
    return rows


def g16_oracle(path):
    """The last "Excited State" block: lines after the last header."""
    blocks = [[]]
    for line in open(path, errors="replace"):
        if "Excitation energies and oscillator strengths" in line:
            blocks.append([])
        m = re.match(r"\s*Excited State\s+\d+:.*?(\d+\.\d+) eV\s+(\d+\.\d+) nm"
                     r"\s+f=(\d+\.\d+)", line)
        if m:
            blocks[-1].append(tuple(float(v) for v in m.groups()))
    return blocks[-1]


def compare(name, found, rows, nm_tol):
    ev = floats(found["EXCIENERGY"])
    f = floats(found["EXCIOSC"])
    check(len(ev) == len(rows) and len(f) == len(rows),
          "%s: %d states parsed, %d printed" % (name, len(ev), len(rows)))
    for i, (pev, pnm, pf) in enumerate(rows[:len(ev)]):
        check(abs(ev[i] - pev) < 1e-9, "%s state %d energy %s != %s"
              % (name, i + 1, ev[i], pev))
        check(abs(f[i] - pf) < 1e-9, "%s state %d f %s != %s"
              % (name, i + 1, f[i], pf))
        check(abs(HC / ev[i] - pnm) <= nm_tol,
              "%s state %d hc/E = %.3f nm, printed %s" % (name, i + 1,
                                                          HC / ev[i], pnm))
    print("%s: %d states, e.g. state 2 %s eV %s nm f=%s" % (
        name, len(ev), ev[1], rows[1][1], f[1]))


def main():
    fx = os.path.join(HERE, "fixtures")
    # ORCA prints the nm to one decimal and the eV to six.
    path = os.path.join(fx, "orca", "h2co_tddft.out")
    found = pipeline("orca/h2co_tddft.out", "orca.desc")
    rows = [(ev, nm, f) for ev, nm, f in orca_oracle(path)]
    compare("orca-h2co-tddft", found, rows, 0.06)
    # Transition weights: the printed coefficient of the leading root-1 pair
    # (7a -> 8a with ORCA's 0-based numbering) is stored 1-based.
    check(found["EXCITRANS"]["values"].split()[0].startswith("8a>9a:0.99901186"),
          "orca root 1 transition")

    # G16 prints eV to four decimals and nm to two.
    path = os.path.join(fx, "gaussian-16", "h2co_tddft.log")
    found = pipeline("gaussian-16/h2co_tddft.log", "gaussian-16.desc")
    compare("g16-h2co-tddft", found, g16_oracle(path), 0.01)
    check(found["EXCITRANS"]["values"].split()[0] == "8>9:0.70626",
          "g16 root 1 transition: %s" % found["EXCITRANS"]["values"].split()[0])
    # State 8 mixes two pairs, listed in the output's order.
    t = found["EXCITRANS"]["values"].split()
    check(len(t) == 10, "g16 one transition item per root")

    print("%d checks, %d failures" % (checks, failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
