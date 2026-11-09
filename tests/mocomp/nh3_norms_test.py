#!/usr/bin/env python3
"""Independent stored NH3 norms and the production composition metric (#927).

MOPAC's GRAPHF inverse matrix supplies the exact Slater overlap. ECCE-QM's
overlap is integrated from its stored Gaussian basis, including spherical d.
Run under Slurm: srun -c 2 nice python3 tests/mocomp/nh3_norms_test.py.
"""
import math
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.append(str(ROOT / "tests/parsers"))
from xtb_trace_ps_test import s1d

MOPAC = ROOT / "data/client/examples/getting-started/mopac-ammonia"
QM = ROOT / "data/client/examples/genchem/vsepr/ecceqm-NH3"


def stored_mos(calc):
    table = ET.parse(calc / "Props/MO").getroot()
    return np.fromstring(table.text, sep=" ").reshape(int(table.get("rows")), -1)


def mopac_overlap():
    text = (MOPAC / "ammonia.mgf").read_text()
    raw = np.array([[float(x.replace("D", "E")) for x in re.findall(
        r"-?\d\.\d+D[-+]\d+", block.split("INVERSE_MATRIX")[0])]
        for block in text.split(" ORBITAL ")[1:]])
    values = [float(x.replace("D", "E")) for x in re.findall(
        r"-?\d\.\d+D[-+]\d+", text.split("INVERSE_MATRIX")[1])]
    inverse_half = np.zeros((7, 7))
    k = 0
    for i in range(7):
        for j in range(i + 1):
            inverse_half[i, j] = inverse_half[j, i] = values[k]
            k += 1
    half = np.linalg.inv(inverse_half)
    c = stored_mos(MOPAC / "stored/ammonia")
    assert np.max(abs(raw @ inverse_half - c)) < 1e-8
    assert np.max(abs(raw @ raw.T - np.eye(7))) < 3e-8
    print("MOPAC raw GRAPHF C.C:", np.sum(raw * raw, axis=1))
    print("MOPAC stored C.C:", np.sum(c * c, axis=1))
    return half @ half


def gaussian_overlap():
    calc = QM / "stored/NH3"
    # Use the final output geometry, retaining more digits than the mvm file.
    text = (QM / "NH3.qmout").read_text()
    geom = text.rsplit("begin geometry", 1)[1].split("end geometry")[0]
    atoms = [(t[1], np.array(list(map(float, t[2:5]))) / 0.52917724924)
             for line in geom.splitlines() if len(t := line.split()) == 5]
    basis = ET.parse(calc / "Parameters/BasisSet.ecce_basisset").getroot()
    lines = basis.find("Data").text.splitlines()
    shells = {}
    for i, line in enumerate(lines):
        if line.startswith("element="):
            element = line.split("=")[1]
        if not line.startswith("contraction "):
            continue
        shell = re.search(r"shell=(\w+)", line)[1]
        nprim = int(re.search(r"num_primitives=(\d+)", line)[1])
        data = np.array([list(map(float, row.split()))
                         for row in lines[i + 1:i + 1 + nprim]])
        for col, kind in enumerate(shell):
            shells.setdefault(element, []).append(("SPD".index(kind),
                                                    data[:, 0], data[:, col + 1]))
    polys = [[{(0, 0, 0): 1}],
             [{(1, 0, 0): 1}, {(0, 1, 0): 1}, {(0, 0, 1): 1}],
             [{(1, 1, 0): 1}, {(0, 1, 1): 1},
              {(0, 0, 2): 1, (2, 0, 0): -0.5, (0, 2, 0): -0.5},
              {(1, 0, 1): 1}, {(2, 0, 0): 0.5, (0, 2, 0): -0.5}]]
    aos = []
    for element, xyz in atoms:
        # MOAOORDER records shells stably sorted by angular momentum.
        for l, exps, coefs in sorted(shells[element], key=lambda s: s[0]):
            for poly in polys[l]:
                aos.append((xyz, poly, list(zip(exps, coefs * exps**(l/2 + 0.75)))))
    s = np.zeros((len(aos), len(aos)))
    for i, (ra, pa, ga) in enumerate(aos):
        for j, (rb, pb, gb) in enumerate(aos):
            s[i, j] = sum(ca * cb * wa * wb * math.prod(
                s1d(la[k], lb[k], a, b, rb[k] - ra[k]) for k in range(3))
                for a, ca in ga for b, cb in gb
                for la, wa in pa.items() for lb, wb in pb.items())
    d = np.sqrt(np.diag(s))
    return s / np.outer(d, d)


def main(build=None):
    from run_tests import run_mocomp
    build = str(build or ROOT / "build-cmake")
    bad = 0
    for name, calc, s in [
            ("MOPAC NH3", MOPAC / "stored/ammonia", mopac_overlap()),
            ("ECCE-QM NH3", QM / "stored/NH3", gaussian_overlap())]:
        c = stored_mos(calc)
        norms = np.diag(c @ s @ c.T)
        print(name + " independent C.S.C:", " ".join(f"{v:.8f}" for v in norms))
        ok = np.max(abs(c @ s @ c.T - np.eye(len(c)))) < 1e-6
        print(("PASS " if ok else "FAIL ") + name + " independent metric")
        bad += not ok

    # Exercise the same MoAoBasis::build/analyse path as the panel, including
    # every virtual orbital. ORCA covers the ordinary Gaussian metric too.
    for name, calc in [("MOPAC NH3", MOPAC / "stored/ammonia"),
                       ("ECCE-QM NH3", QM / "stored/NH3"),
                       ("ORCA water", HERE / "fixtures/water")]:
        nmo = len(stored_mos(calc))
        rc, out, err = run_mocomp(build, str(calc), range(1, nmo + 1), False, tsv=False)
        norms = [float(v) for v in re.findall(r"c.S.c = ([\d.]+)", out)]
        ok = rc == 0 and len(norms) == nmo and max(abs(v - 1) for v in norms) < 1e-3
        print(("PASS " if ok else "FAIL ") + name + " production norms: " + str(norms))
        if rc:
            print(err)
        bad += not ok
    return int(bad != 0)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else None))
