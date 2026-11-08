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

Also runs testCrOMoAoOrder: the real scripts/parsers/orca.mo against a
real ORCA CrO/def2-SVP MO block, then MoAoOrder + BasisFlatten, checking
c^T S c == 1 -- the regression test for fault A (ORCA's own def2-SVP
shell order interleaving a polarization shell among the d shells) and
the f(+-3) spherical sign fix in ORCA.edml, together.
"""

import os
import subprocess
import sys

# eccercmd holds the libssh transport when the build found libssh
_SSH = (["-lssh"] if __import__("subprocess").run(
    ["pkg-config", "--exists", "libssh"], capture_output=True).returncode == 0 else [])

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

BUILD = os.environ.get("ECCE_TEST_BUILD", os.path.join(ROOT, "build-cmake"))
LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "ecceutil", "eccecomm", "eccecipc", "eccercmd"]


def main():
    if not os.path.isdir(BUILD):
        print("  skipped: no build tree at %s (set ECCE_TEST_BUILD)" % BUILD)
        return 0

    out = os.path.join(HERE, "testBasisFlatten")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out,
            os.path.join(HERE, "testBasisFlatten.C"),
            "-L" + BUILD]
           + ["-l" + l for l in LIBS]*3 + ["-lxerces-c", *__import__("subprocess").run(["pkg-config", "--libs", "hdf5"], capture_output=True, text=True).stdout.split(), "-lmosquitto", "-lssl", "-lcrypto", *_SSH])
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
    rc = run.returncode

    rc2 = run_cro_moaoorder(env)
    # HF/cc-pVQZ has a g shell on F (#861): ORCA's spherical table must
    # reach lval=4 or BasisFlatten leaves empty functions.
    rc3 = run_cro_moaoorder(env, ["fixtures/hf_qz.nb", "fixtures/hf_qz.xyz", "10"],
                            "fixtures/hf_qz_mo_block.txt")
    # HF/cc-pV5Z, H off-axis so every m mixes: F has g AND h shells (#863).
    # Each code's own parser output, its own MOOrdering table.
    fx = ["fixtures/hf_5z.nb", "fixtures/hf_5z.xyz", "10"]
    rc4 = run_cro_moaoorder(env, fx + ["ORCA"], "fixtures/hf_5z_orca_mo_block.txt")
    rc5 = run_cro_moaoorder(env, fx + ["NWChem", "5e-3"],
                            "fixtures/hf_5z_nwchem_eprint.txt", ["nwchem.mo"])
    # Gaussian re-contracts general contractions that share primitives, so
    # its fort.7 refers to the basis it echoes with GFInput (hf_5z_g16.nb),
    # not to the cc-pV5Z it was given.
    rc6 = run_cro_moaoorder(env, ["fixtures/hf_5z_g16.nb", "fixtures/hf_5z.xyz",
                                  "10", "Gaussian-16"],
                            "fixtures/hf_5z_g16_fort7.txt", ["gaussian-16.mo"])
    return rc or rc2 or rc3 or rc4 or rc5 or rc6


def run_cro_moaoorder(env, extra=(), block="fixtures/cro_mo_block.txt",
                      parser=("orca.mo", "MO", "Energy", "SCF", "RHF")):
    """orca.mo's canonical-order fix (fault A) + MoAoOrder's un-permute,
    against ORCA's real CrO/def2-SVP MO block -- see testCrOMoAoOrder.C.
    """
    perl = subprocess.run(
        ["perl", os.path.join(ROOT, "scripts/parsers", parser[0]), *parser[1:]],
        stdin=open(os.path.join(HERE, block)),
        capture_output=True, text=True, env=env)
    if perl.returncode != 0:
        print("%s failed on %s:" % (parser[0], block))
        print(perl.stderr)
        return 2
    parsed = os.path.join(HERE, "cro_mo_parsed.txt")
    with open(parsed, "w") as f:
        f.write(perl.stdout)

    out = os.path.join(HERE, "testCrOMoAoOrder")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out, os.path.join(HERE, "testCrOMoAoOrder.C"),
            "-L" + BUILD]
           + ["-l" + l for l in LIBS]*3 + ["-lxerces-c", *__import__("subprocess").run(["pkg-config", "--libs", "hdf5"], capture_output=True, text=True).stdout.split(), "-lmosquitto", "-lssl", "-lcrypto", *_SSH])
    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode != 0:
        print("could not build testCrOMoAoOrder:")
        print(build.stderr)
        os.unlink(parsed)
        return 2

    run = subprocess.run([out, parsed, *extra], capture_output=True, text=True,
                          env=env, cwd=HERE)
    print(run.stdout, end="")
    if run.stderr:
        print(run.stderr, end="")
    os.unlink(out)
    os.unlink(parsed)
    return run.returncode


if __name__ == "__main__":
    sys.exit(main())
