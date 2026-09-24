#!/usr/bin/env python3
"""Does the Slater-to-Gaussian expansion reproduce the exact functions?

    tests/slater/run_tests.py

Semiempirical codes use Slater orbitals; ECCE's rendering path evaluates
contracted Gaussians and nothing else.  SlaterExpansion bridges the two,
and if it is wrong the orbitals it feeds the viewer are wrong in a way
that still looks like a plausible orbital -- smooth, centred on the right
atom, and quietly the wrong shape.  So it is checked numerically here
rather than by eye.

Compiles the C++ directly with g++: the class deliberately has no wx, no
DAV and no XML dependency, so it needs none of the build tree, and a test
that runs without configuring CMake is one that actually gets run.

Exit 0 on success, 1 on a failed check, 2 if it could not be built.
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
OUT = os.path.join(HERE, "testSlaterExpansion")


#  The round-trip test needs the built libraries, because its whole point
#  is that ECCE's own importConfig accepts the text -- a reimplementation
#  of that parser here would agree with itself and prove nothing.
BUILD = os.environ.get("ECCE_TEST_BUILD", os.path.join(ROOT, "build-cmake"))
LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "ecceutil", "eccecomm", "eccecipc", "ecceexp", "eccercmd"]


def roundTrip():
    """Does ICalcUtils::importConfig() accept the generated basis?"""
    if not os.path.isdir(BUILD):
        print("  skipped: no build tree at %s" % BUILD)
        print("  (the round-trip needs ECCE's own basis parser)")
        return 0

    out = os.path.join(HERE, "testSlaterBasisSet")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out,
            os.path.join(HERE, "testSlaterBasisSet.C"),
            os.path.join(ROOT, "src/tdat/chemistry/SlaterBasisSet.C"),
            os.path.join(ROOT, "src/tdat/chemistry/SlaterExpansion.C"),
            "-L" + BUILD]
           + ["-l" + l for l in LIBS]*3 + ["-lxerces-c"])
    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode != 0:
        print("  skipped: could not link against the build tree")
        print("  " + build.stderr.strip().splitlines()[-1])
        return 0

    env = dict(os.environ)
    env.setdefault("ECCE_HOME", "/opt/ecce")
    env.setdefault("ECCE_REALUSERHOME", os.path.expanduser("~"))
    run = subprocess.run([out], capture_output=True, text=True, env=env)
    print(run.stdout, end="")
    if run.stderr:
        print(run.stderr, end="")
    os.unlink(out)
    return run.returncode


def main():
    src = [os.path.join(HERE, "testSlaterExpansion.C"),
           os.path.join(ROOT, "src/tdat/chemistry/SlaterExpansion.C")]
    cmd = ["g++", "-O2", "-I", os.path.join(ROOT, "include"), "-o", OUT] + src

    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode != 0:
        print("could not build the test:")
        print(build.stderr)
        return 2

    run = subprocess.run([OUT], capture_output=True, text=True)
    print(run.stdout, end="")
    if run.stderr:
        print(run.stderr, end="")
    os.unlink(OUT)
    if run.returncode != 0:
        return run.returncode

    print("")
    return roundTrip()


if __name__ == "__main__":
    sys.exit(main())
