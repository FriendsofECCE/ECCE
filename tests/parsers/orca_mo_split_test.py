#!/usr/bin/env python3
"""ORCA's MO coefficients can run together in their fixed-width fields (#108, #719).

    tests/parsers/orca_mo_split_test.py

With diffuse functions a coefficient of 100 or more fills its field, so a
row reads "11.344124150.444498" (two values) or "8.168795-72.789039".  The
rows below are from a real ORCA 6.1.1 output (def2 basis with diffuse
functions); orca.mo must give rows x columns values, each in its place.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPT = os.path.join(REPO, "scripts", "parsers", "orca.mo")

BLOCK = """\
                      0         1         2         3         4         5
                 -19.18750 -19.17312 -10.27080 -10.23542 -10.22561   0.10372
                   2.00000   2.00000   2.00000   2.00000   2.00000   0.00000
                  --------  --------  --------  --------  --------  --------
  0C   1s        -24.884208  8.168795-72.789039 68.593106 11.344124150.444498
  0C   2s        -53.912765-29.386459-74.097237106.293096 20.429675  8.030060
  0C   1pz       -121.740258 12.093634 33.234965173.113034-65.916144 43.291639
"""

EXPECT = [
    [-24.884208, -53.912765, -121.740258],
    [8.168795, -29.386459, 12.093634],
    [-72.789039, -74.097237, 33.234965],
    [68.593106, 106.293096, 173.113034],
    [11.344124, 20.429675, -65.916144],
    [150.444498, 8.030060, 43.291639],
]


def main():
    out = subprocess.run(["perl", SCRIPT, "MO", "Geometry", "DFT", "B3LYP"],
                         input=BLOCK, capture_output=True, text=True,
                         check=True).stdout
    lines = out.splitlines()
    start = lines.index("key: MO")
    size = [int(x) for x in lines[start + 2].split()]
    values = lines.index("values:", start) + 1
    rows = [[float(x) for x in lines[values + m].split()]
            for m in range(size[0])]
    if size != [6, 3]:
        print("FAIL: size %s, want [6, 3]" % size)
        return 1
    if rows != EXPECT:
        print("FAIL: coefficients\n got  %s\n want %s" % (rows, EXPECT))
        return 1
    print("PASS: run-together coefficients split into rows x columns values")
    return 0


if __name__ == "__main__":
    sys.exit(main())
