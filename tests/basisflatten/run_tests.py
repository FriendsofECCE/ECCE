#!/usr/bin/env python3
"""Did moving ComputeMoCmd's basis flattening into tdat/BasisFlatten
change a single number?

    tests/basisflatten/run_tests.py

BasisFlatten::normalize()/getoddNormalize()/flatten() are ComputeMoCmd's
own NORMP/NORMF/angular-monomial code, moved verbatim so the MO symmetry
projection (SymmetryAnalysis, #147/#151) and the ESP field evaluator
share ONE implementation.  This links against the real build (JCode,
CodeFactory, TGBSConfig, XML) rather than reimplementing them, because
the whole point is whether ECCE's own object graph still produces the
same numbers through the moved code -- and checks the result against an
independent closed-form oracle (see testBasisFlatten.C), not against the
moved code's own intermediate steps.

Needs a configured CMake build tree; exit 0 and "skipped" if there isn't
one (this is the register-with-ECCE's-own-XML/JCode test, not the
no-build-tree-needed kind tests/symmetry and tests/slater otherwise use).
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

BUILD = os.environ.get("ECCE_TEST_BUILD", os.path.join(ROOT, "build-cmake"))
LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "ecceutil", "eccecomm", "eccecipc", "ecceexp", "eccercmd"]


def main():
    if not os.path.isdir(BUILD):
        print("  skipped: no build tree at %s (set ECCE_TEST_BUILD)" % BUILD)
        return 0

    out = os.path.join(HERE, "testBasisFlatten")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out,
            os.path.join(HERE, "testBasisFlatten.C"),
            "-L" + BUILD]
           + ["-l" + l for l in LIBS]*3 + ["-lxerces-c"])
    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode != 0:
        print("could not build testBasisFlatten:")
        print(build.stderr)
        return 2

    env = dict(os.environ)
    env["ECCE_HOME"] = ROOT
    env.setdefault("ECCE_REALUSERHOME", os.path.expanduser("~"))
    run = subprocess.run([out], capture_output=True, text=True, env=env)
    print(run.stdout, end="")
    if run.stderr:
        print(run.stderr, end="")
    os.unlink(out)
    return run.returncode


if __name__ == "__main__":
    sys.exit(main())
