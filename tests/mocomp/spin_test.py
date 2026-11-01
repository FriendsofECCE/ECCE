#!/usr/bin/env python3
"""
ecce-mocomp --spin (MoAoBasis::spinPopulations, #671): the one-pass Mulliken
spin populations against ORCA's own printed ones (1e-3) and against the
per-orbital sum the Builder's spin view used to compute (1e-8).

    spin_test.py [build-dir]     (also: ctest -R mocompspin)
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))

# fixture -> ORCA's "MULLIKEN ATOMIC CHARGES AND SPIN POPULATIONS" column
ORCA = {
    "cocl4uks": [2.691122, 0.077219, 0.077219, 0.077219, 0.077219],
}


def main():
    build = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "build-cmake")
    exe = os.path.join(build, "mocomp")
    if not os.path.exists(exe):
        print("SKIP: no mocomp in", build)
        return 77
    bad = 0
    for name, ref in ORCA.items():
        with tempfile.TemporaryDirectory() as home:
            env = dict(os.environ, ECCE_HOME=REPO, ECCE_REALUSER="mocomp",
                       ECCE_REALUSERHOME=home, HOME=home)
            env.pop("ECCE_LOCAL_DATA", None)
            p = subprocess.run([exe, os.path.join(HERE, "fixtures", name),
                                "--spin"], capture_output=True, text=True,
                               env=env)
        rows = [l.split("\t") for l in p.stdout.splitlines()]
        if p.returncode or len(rows) != len(ref):
            print("FAIL", name, p.returncode, p.stderr)
            bad += 1
            continue
        d_orca = max(abs(float(r[2]) - x) for r, x in zip(rows, ref))
        d_old = max(abs(float(r[2]) - float(r[3])) for r in rows)
        ok = d_orca < 1e-3 and d_old < 1e-8
        print("%s %s: vs ORCA %.1e, vs per-orbital sum %.1e  (%s)" %
              ("ok  " if ok else "FAIL", name, d_orca, d_old,
               p.stderr.strip()))
        bad += not ok
    return 1 if bad else 0


sys.exit(main())
