#!/usr/bin/env python3
"""Fragment-orbital compositions from Multiwfn's charge decomposition analysis.

    multiwfn_reference.py MULTIWFN COMPLEX.molden FRAG1.molden FRAG2.molden MO... > reference.tsv

Drives Multiwfn (2026.9.20, main function 16, then option 2 "fragment orbital
contributions to a complex orbital") through stdin with its Mulliken method
(settings.ini iCDAcomp=1) and compthresCDA=0.01 (print every FO with at least
0.01 %).  ICDACOMP=2 in the environment gives SCPA.  Prints "mo fragment fo percent" per line.  The moldens are the
ORCA 6.1.1 outputs (orca_2mkl -molden) of the calculations in inputs/, the same
files that were converted to the fixtures' results.h5.  Fragments must be
consecutive blocks of the complex's atoms, in order (Multiwfn requires it).
Multiwfn is not part of the repository.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile


def main():
    mw, cx, f1, f2 = [os.path.abspath(a) for a in sys.argv[1:5]]
    mos = [int(a) for a in sys.argv[5:]]
    work = tempfile.mkdtemp(prefix="fragorb-mw-")
    ini = open(os.path.join(os.path.dirname(mw), "settings.ini")).read()
    ini = re.sub(r"compthresCDA=\s*[0-9.]+", "compthresCDA= 0.01", ini)
    ini = re.sub(r"iCDAcomp=\s*\d", "iCDAcomp= " + os.environ.get("ICDACOMP", "1"), ini)
    open(os.path.join(work, "settings.ini"), "w").write(ini)
    script = "\n".join([cx, "16", "2", f1, f2, "2"] + [str(m) for m in mos] + ["0", "-1", "q", ""])
    env = dict(os.environ, Multiwfnpath=work, OMP_STACKSIZE="200M")
    out = subprocess.run([mw], input=script, cwd=work, env=env, text=True,
                         capture_output=True).stdout
    shutil.rmtree(work, ignore_errors=True)
    mo = None
    print("# mo fragment fo percent (Multiwfn 2026.9.20 CDA, Mulliken)")
    for line in out.splitlines():
        m = re.search(r"Occupation number of orbital\s+(\d+) of the complex", line)
        if m:
            mo = int(m.group(1))
        m = re.match(r"\s*Orbital\s+(\d+) of fragment\s+(\d+), Occ:.*Contribution:\s*(-?[0-9.]+) %", line)
        if m and mo:
            print(mo, m.group(2), m.group(1), m.group(3))


if __name__ == "__main__":
    main()
