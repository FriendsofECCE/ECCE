#!/usr/bin/env python3
"""Vibrational frequencies and intensities read straight from the codes' own
output files, with no ECCE parser involved.

This is the oracle the spectrum viewer is checked against (#214): the
numbers in `tests/parsers/fixtures/*` as the program printed them, not
what `scripts/parsers/*` turned them into.

    oracle.py <fixtures-dir> <out-dir>      write one .spec per fixture

A .spec file is what tools/spectrum/render reads.
"""
import os
import re
import sys

NUM = r"[-+]?\d+\.\d+(?:[eE][-+]?\d+)?"


def gaussian(path):
    """Frequencies -- / IR Inten -- / Raman Activ -- blocks, with the
    irrep labels on the line above Frequencies."""
    lines = open(path, errors="replace").read().splitlines()
    freq, ir, raman, irrep = [], [], [], []
    for i, line in enumerate(lines):
        if re.match(r"\s*Frequencies --", line):
            freq += [float(x) for x in re.findall(NUM, line.split("--")[1])]
            labels = lines[i - 1].split()
            irrep += labels
            for j in range(i + 1, i + 8):
                tag = lines[j]
                if re.match(r"\s*IR Inten\s+--", tag):
                    ir += [float(x) for x in re.findall(NUM, tag.split("--")[1])]
                elif re.match(r"\s*Raman Activ --", tag):
                    raman += [float(x) for x in re.findall(NUM, tag.split("--")[1])]
    return dict(freq=freq, irrep=irrep, ir=ir, raman=raman,
                ir_units="KM/Mole", raman_units="A^4/AMU")


def orca(path):
    text = open(path, errors="replace").read()

    def block(title):
        m = list(re.finditer(r"^" + title + r"\s*$", text, re.M))
        return text[m[-1].end():] if m else ""

    freq = {}
    for m in re.finditer(r"^\s*(\d+):\s+(" + NUM + r") cm\*\*-1",
                         block("VIBRATIONAL FREQUENCIES"), re.M):
        freq[int(m.group(1))] = float(m.group(2))
    n = max(freq) + 1
    ir = [0.0] * n
    raman = [0.0] * n
    irBlock = block("IR SPECTRUM").split("RAMAN SPECTRUM")[0]
    for m in re.finditer(r"^\s*(\d+):\s+(" + NUM + r")\s+" + NUM + r"\s+(" +
                         NUM + r")\s", irBlock, re.M):
        ir[int(m.group(1))] = float(m.group(3))
    ramBlock = block("RAMAN SPECTRUM").split("The first frequency")[0]
    for m in re.finditer(r"^\s*(\d+):\s+(" + NUM + r")\s+(" + NUM + r")\s",
                         ramBlock, re.M):
        raman[int(m.group(1))] = float(m.group(3))
    return dict(freq=[freq[i] for i in range(n)], irrep=[], ir=ir, raman=raman,
                ir_units="KM/Mole", raman_units="A^4/AMU")


def nwchemEprint(path):
    """ecce_print arrays: 'projected frequencies' and
    'projected intensities (KM/mol)', as the .desc selects them."""
    text = open(path, errors="replace").read()

    def array(name):
        m = re.search(r"%begin%" + re.escape(name) + r"%\d+%double\n(.*?)\n"
                      r"[^\n]*%end%" + re.escape(name), text, re.S)
        return [float(x) for x in m.group(1).split()] if m else []

    return dict(freq=array("projected frequencies"), irrep=[],
                ir=array("projected intensities (KM/mol)"), raman=[],
                ir_units="KM/Mole", raman_units="")


CASES = [
    ("g16-h2o-optfreq", gaussian, "gaussian-16/h2o_optfreq.log"),
    ("g16-co-freq", gaussian, "gaussian-16/co_freq.log"),
    ("orca-h2o-freq-raman", orca, "orca/h2o_freq_raman.out"),
    ("nwchem-h2o-freq", nwchemEprint, "nwchem/h2o_freq.eprint"),
]


def write(spec, path):
    n = len(spec["freq"])
    with open(path, "w") as f:
        f.write("ir_units %s\n" % spec["ir_units"])
        if spec["raman"]:
            f.write("raman_units %s\n" % spec["raman_units"])
        for i in range(n):
            irrep = spec["irrep"][i] if i < len(spec["irrep"]) else "-"
            ir = "%.10g" % spec["ir"][i] if i < len(spec["ir"]) else "-"
            ra = "%.10g" % spec["raman"][i] if i < len(spec["raman"]) else "-"
            f.write("%.10g %s %s %s\n" % (spec["freq"][i], irrep, ir, ra))


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    fixtures, out = sys.argv[1:]
    os.makedirs(out, exist_ok=True)
    for name, reader, rel in CASES:
        spec = reader(os.path.join(fixtures, rel))
        write(spec, os.path.join(out, name + ".spec"))
        print("%s: %d modes, ir %d, raman %d"
              % (name, len(spec["freq"]), len(spec["ir"]), len(spec["raman"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
