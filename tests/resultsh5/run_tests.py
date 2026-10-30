#!/usr/bin/env python3
"""results.h5 converters: invariants, round trips, and the size rule.

    tests/resultsh5/run_tests.py <ecce-results> [fixture-name ...]

For every fixture: convert -> trace(P S) = N and C'SC = 1 from the
canonical basis -> h5 write/read identical -> Molden export and re-import
(coefficients to 1e-10) -> results.h5 no larger than the sources it was
made from, plus TOLERANCE.

TOLERANCE: HDF5 spends roughly 0.2 kB on every group, dataset and
attribute list however small it is, about 4 kB for the ~20 objects of a
three-atom file.  Past that the compressed arrays win.  Source files that
are not parsed (the .chk/.gbw a fixture replaces) are only sized.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
FX = os.path.join(HERE, "fixtures")
TOLERANCE = 4096

# name -> (parsed inputs, extra files the h5 must also be smaller than)
FIXTURES = {
    "xtb_water": (["xtb_water.molden"], []),
    "xtb_ch2fcl": (["xtb_ch2fcl.molden"], []),
    "orca_water": (["orca_water.molden", "orca_water.hess"], []),
    "orca_hoi": (["orca_hoi.molden"], []),          # I: ECP, d, f
    "orca_hocl": (["orca_hocl.molden"], []),        # def2-TZVP: f on O and Cl
    "g16_water_b3lyp_freq": (["g16_water_b3lyp_freq.fchk"], []),
    "g16_water_hf_6d": (["g16_water_hf_6d.fchk"], []),
    "g16_hoi_def2svp": (["g16_hoi_def2svp.fchk"], []),
    "g16_hocl_def2tzvp_trimmed": (["g16_hocl_def2tzvp_trimmed.fchk"], []),
}


def main():
    exe = os.path.abspath(sys.argv[1])
    names = sys.argv[2:] or sorted(FIXTURES)
    bad = 0
    for name in names:
        ins, refs = FIXTURES[name]
        with tempfile.TemporaryDirectory() as tmp:
            cmd = [exe, "selftest", tmp, str(TOLERANCE)] + [os.path.join(FX, f) for f in ins]
            if refs:
                cmd += ["--"] + [os.path.join(FX, f) for f in refs]
            r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        print("== %s (exit %d)\n%s" % (name, r.returncode, r.stdout))
        if r.returncode != 0:
            bad += 1
    print("%d of %d fixtures failed" % (bad, len(names)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
