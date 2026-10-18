#!/usr/bin/env python3
"""The geometry ECCE stores and the frame of the normal modes agree (#312).

A code that reorients the molecule (ORCA with UseSym, Gaussian's standard
orientation, NWChem autosym) prints its modes in its own frame.  The Builder
draws them on the last GEOMTRACE step, so that step must be in the same
frame, or the arrows start in the wrong places.

The oracle is chemistry, not the parser: in a high-frequency mode (> 2500
cm-1) the hydrogen that moves most moves along its own X-H bond.  Taken
from the last geometry block and the VIB property of the real pipeline
(eccejobmonitor_sim + the real scripts), it holds only if the two are in one
frame; the control rotates the geometry and must fail.

    tests/parsers/vib_frame_test.py
"""
import atexit
import math
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

#  (fixture, desc, ECCE runtype, what it covers)
CASES = [
    ("orca/ethanol_sym_freq.out", "orca.desc", "Vibration",
     "ORCA UseSym, frequencies only: one geometry block"),
    ("orca/ethanol_sym_optfreq.out", "orca.desc", "GeoVib",
     "ORCA UseSym, Opt Freq: the final geometry"),
    ("gaussian-16/h2o_optfreq.log", "gaussian-16.desc", "GeoVib",
     "Gaussian 16: input and standard orientation blocks"),
    ("nwchem/h2o_sym_freq.eprint", "nwchem.desc", "Vibration",
     "NWChem autosym, frequencies only (input rotated into the plane)"),
    ("nwchem/h2o_sym_optfreq.eprint", "nwchem.desc", "GeoVib",
     "NWChem autosym, optimisation then frequencies"),
    ("nwchem/h2o_freq.eprint", "nwchem.desc", "Vibration",
     "NWChem, input already in the symmetry frame"),
    ("mopac/ch4_force.out", "mopac.desc", "Vibration",
     "MOPAC FORCE: modes printed in its own force orientation"),
    ("mopac/ch4_force_nothermo.out", "mopac.desc", "GeoVib",
     "MOPAC Opt then FORCE, no THERMO"),
    ("mopac/ch4_geovib.out", "mopac.desc", "GeoVib",
     "MOPAC Opt then FORCE with THERMO"),
    ("mopac/h2o_geovib.out", "mopac.desc", "GeoVib",
     "MOPAC water, input far from the symmetry frame"),
    ("mopac/nh3_geovib.out", "mopac.desc", "GeoVib",
     "MOPAC ammonia, input far from the symmetry frame"),
    (os.path.join(REPO, "tests/progress/fixtures/mopac/gv.out"),
     "mopac.desc", "GeoVib", "MOPAC Opt then FORCE (progress fixture)"),
    (os.path.join(REPO, "data/client/examples/mopac-ammonia/ammonia.out"),
     "mopac.desc", "GeoVib", "MOPAC ammonia example"),
]

#  Largest share of any vibration (mass-weighted) inside the six rigid
#  motions of the stored structure; the modes belong to it when this is small.
MAX_RIGID = 0.02
MASS = {"H": 1.00794, "C": 12.0107, "N": 14.0067, "O": 15.9994}

#  Elements of the codes that do not label geometry rows with them.
GIVEN = {"nwchem/h2o_sym_freq.eprint": "OHH",
         "nwchem/h2o_sym_optfreq.eprint": "OHH",
         "nwchem/h2o_freq.eprint": "OHH"}

MIN_COS = 0.8
HIGH = 2500.0


def records(fixture, descName, runtype):
    """{key: [flat record in file order]}."""
    #  One directory for all the scripts: mopac.geomtrace leaves the input
    #  geometry there for mopac.vib, as in the real monitor.
    workdir = tempfile.mkdtemp(prefix="ecce-vibframe-")
    atexit.register(shutil.rmtree, workdir, True)
    desc = read_desc(os.path.join(SCRIPTS, descName))
    result = replay(desc, os.path.join(HERE, "fixtures", fixture))
    delivered = []
    for entry in desc.live_entries():
        for block in result.delivered_for(entry):
            delivered.append((block.begin_line, entry, block))
    delivered.sort(key=lambda t: t[0])
    found = {}
    for _line, entry, block in delivered:
        out, _err, _rc = run_parser(SCRIPTS, entry, block,
                                    ("." , runtype, "SCF", "RHF", "0"),
                                    workdir=workdir)
        for rec in parse_parser_output(out):
            found.setdefault(rec["key"], []).append(rec["flat"])
    return found


def numbers(text):
    return [float(t) for t in text.replace("D", "e").split()]


def symbols(rec, given):
    """Element per row: from the labels when the code writes "1-O", else from
    the case (NWChem's labels are bare indices)."""
    labels = rec["rowlabels"].split()
    if all("-" in lab for lab in labels):
        return [lab.split("-")[1] for lab in labels]
    return list(given)


def worst(found, given):
    """Smallest cos(H displacement, X-H bond) over the high-frequency modes,
    using the last geometry; and the same with the geometry rotated."""
    geo = found["GEOMTRACE"][-1]
    xyz = numbers(geo["values"])
    atoms = [xyz[i:i + 3] for i in range(0, len(xyz), 3)]
    sym = symbols(geo, given)
    vib = found["VIB"][-1]
    nmodes, natoms, _three = (int(v) for v in vib["size"].split())
    assert natoms == len(atoms), (natoms, len(atoms))
    flat = numbers(vib["values"])
    freqs = numbers(found["VIBFREQ"][-1]["values"])
    assert len(freqs) == nmodes

    def cosines(geometry):
        out = []
        for m in range(nmodes):
            if freqs[m] < HIGH:
                continue
            vec = [flat[(m * natoms + a) * 3:(m * natoms + a) * 3 + 3]
                   for a in range(natoms)]
            h = max((a for a in range(natoms) if sym[a] == "H"),
                    key=lambda a: sum(v * v for v in vec[a]))
            heavy = min((a for a in range(natoms) if sym[a] != "H"),
                        key=lambda a: math.dist(geometry[a], geometry[h]))
            bond = [geometry[h][k] - geometry[heavy][k] for k in range(3)]
            dot = sum(vec[h][k] * bond[k] for k in range(3))
            norm = math.sqrt(sum(v * v for v in vec[h]) *
                             sum(b * b for b in bond))
            out.append(abs(dot) / norm)
        return out

    #  A quarter turn about z: no molecule is invariant under it for every
    #  mode, so a frame disagreement of this size must be seen.
    turned = [[-a[1], a[0], a[2]] for a in atoms]
    return min(cosines(atoms)), min(cosines(turned)), nmodes


def rigid_share(found, given):
    """Largest share of a vibration inside rotations and translations of the
    last stored geometry, mass-weighted."""
    import numpy as np
    geo = found["GEOMTRACE"][-1]
    xyz = np.array(numbers(geo["values"])).reshape(-1, 3)
    mass = np.array([MASS[e] for e in symbols(geo, given)])
    vib = found["VIB"][-1]
    nmodes, natoms, _ = (int(v) for v in vib["size"].split())
    modes = np.array(numbers(vib["values"])).reshape(nmodes, natoms, 3)
    root = np.sqrt(mass)[:, None]
    centre = (xyz * mass[:, None]).sum(axis=0) / mass.sum()
    rigid = []
    for k in range(3):
        e = np.zeros(3)
        e[k] = 1.0
        rigid.append((root * e).ravel())
        rigid.append((root * np.cross(e, xyz - centre)).ravel())
    basis, _r = np.linalg.qr(np.array(rigid).T)
    shares = []
    for m in modes:
        u = (root * m).ravel()
        shares.append(float(np.sum((basis.T @ u) ** 2) / np.sum(u ** 2)))
    return max(shares)


def main():
    bad = 0
    for fixture, descName, runtype, what in CASES:
        given = GIVEN.get(fixture, "")
        found = records(fixture, descName, runtype)
        missing = [k for k in ("GEOMTRACE", "VIB", "VIBFREQ")
                   if k not in found]
        if missing:
            print("FAIL %s: no %s" % (fixture, " ".join(missing)))
            bad += 1
            continue
        good, control, nmodes = worst(found, given)
        #  The other codes list the rigid motions among their modes.
        share = rigid_share(found, given) if "mopac" in fixture else 0.0
        ok = good >= MIN_COS and control < MIN_COS and share <= MAX_RIGID
        print("%s %s (%s): X-H stretch alignment %.3f, rotated control "
              "%.3f, rigid-motion share %.2e, %d modes, %d geometry steps"
              % ("ok  " if ok else "FAIL", fixture, what, good, control,
                 share, nmodes, len(found["GEOMTRACE"])))
        bad += not ok
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
