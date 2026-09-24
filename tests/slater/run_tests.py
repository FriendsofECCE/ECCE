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
    return run.returncode


if __name__ == "__main__":
    sys.exit(main())
