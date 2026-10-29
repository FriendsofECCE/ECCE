#!/usr/bin/env python3
"""trace(P.S) = N for xtb's orbitals through ECCE's own C++ (testXtbTracePS.C).

Parses the real xtb --molden fixtures with scripts/parsers/xtb.mo, builds the
basis with ICalcUtils::gtoBasisConfig, flattens it with xTB.edml's MOOrdering
and takes the overlap from EspField.  Needs a CMake build tree rebuilt with
this change (ECCE_TEST_BUILD, default build-cmake); skips without one.
ECCE_TEST_EXTRA_OBJ may name extra object files put before the libraries.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
BUILD = os.environ.get("ECCE_TEST_BUILD", os.path.join(ROOT, "build-cmake"))
LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "ecceutil", "eccecomm", "eccecipc", "eccercmd"]
FIX = os.path.join(ROOT, "tests", "parsers", "fixtures", "xtb")
CASES = [("h2o", 8), ("ch4", 8), ("so2", 18), ("ch3_uhf", 7)]


def main():
    if not os.path.isdir(BUILD):
        print("  skipped: no build tree at %s (set ECCE_TEST_BUILD)" % BUILD)
        return 0
    tmp = tempfile.mkdtemp(prefix="xtbps-")
    exe = os.path.join(tmp, "testXtbTracePS")
    extra = os.environ.get("ECCE_TEST_EXTRA_OBJ", "").split()
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"), "-o", exe,
            os.path.join(HERE, "testXtbTracePS.C")] + extra +
           ["-L" + BUILD] + ["-l" + l for l in LIBS] * 3 +
           ["-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto",
            "-Wl,-rpath," + BUILD])
    b = subprocess.run(cmd, capture_output=True, text=True)
    if b.returncode != 0:
        print(b.stderr)
        return 2
    env = dict(os.environ, ECCE_HOME=ROOT)
    env.setdefault("ECCE_REALUSERHOME", os.path.expanduser("~"))
    rc = 0
    for name, nel in CASES:
        molden = os.path.join(FIX, name + "_gfn2.molden")
        parsed = subprocess.run(
            ["perl", os.path.join(ROOT, "scripts", "parsers", "xtb.mo"), ".",
             "Energy", "SE", "GFN2-xTB"],
            input="ECCE-XTB-MOLDEN\n" + open(molden).read(),
            capture_output=True, text=True, check=True).stdout
        out = os.path.join(tmp, name + ".txt")
        open(out, "w").write(parsed)
        r = subprocess.run([exe, molden, out, str(nel)], capture_output=True,
                           text=True, env=env)
        print("%-8s %s" % (name, (r.stdout + r.stderr).strip()))
        rc |= r.returncode
    return rc


if __name__ == "__main__":
    sys.exit(main())
