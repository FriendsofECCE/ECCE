#!/usr/bin/env python3
"""Parse-script output with long lines arrives whole in the property store.

    tests/parsers/propinterp_test.py            run
    tests/parsers/propinterp_test.py --props DIR [--nbf N]
                                                also write the documents
                                                (a local calculation's Props)

ecceqm.parse writes a whole vector (ORBENG, ORBOCC) on one line and one
MO's coefficients per line; orca.mo, orca.mulliken and mopac.charges write
vectors the same way.  PropertyInterpreter read value lines into a 2048-byte
buffer: a longer line was cut there (ECCE-QM above ~120 functions, ORCA
above ~245), the stream was left failed, and every property after it in the
same batch was lost.  The MO panel then got a 156x156 table with 152 values
and crashed.

Runs a synthetic ECCE-QM orbital block (energies, occupations and a square
coefficient table of --nbf functions, default 160) through the real
ecceqm.parse and the real PropertyInterpreter (propinterp.C, linked against
the build tree's static libraries) and checks every count.  It also stores the three MDMSD records of the
NaCl-dissolving example (fixtures/gromacs, ~1250 characters each, #544) and
checks the document holds each one whole.

ECCE_TEST_BUILD names the build tree (default build-cmake).  Exit 77 (skip)
without built libraries.
"""
import argparse
import os
import random
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
BUILD = os.environ.get("ECCE_TEST_BUILD", os.path.join(REPO, "build-cmake"))
LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "eccecipc", "ecceutil", "eccecomm", "eccercmd"]
SKIP = 77


def qmBlocks(nbf):
    """ecce-qm's orbital output (docs/qm/README.md) for a closed shell."""
    rnd = random.Random(156)
    nocc = nbf // 3
    energies = sorted(rnd.uniform(-20.0, 3.0) for _ in range(nbf))
    orbitals = ["begin orbitals restricted  # index energy(hartree) occupation"]
    for i, e in enumerate(energies):
        orbitals.append("%d %.12f %.1f" % (i + 1, e, 2.0 if i < nocc else 0.0))
    orbitals.append("end orbitals")
    coeffs = ["begin mo_coefficients restricted  # one line per MO"]
    for i in range(nbf):
        coeffs.append("%d " % (i + 1) + " ".join(
            "%.10f" % rnd.uniform(-1.0, 1.0) for _ in range(nbf)))
    coeffs.append("end mo_coefficients")
    return ["\n".join(orbitals) + "\n", "\n".join(coeffs) + "\n"]


def parse(block):
    proc = subprocess.run(["perl", os.path.join(REPO, "scripts", "parsers",
                                                "ecceqm.parse")],
                          input=block, capture_output=True, text=True,
                          check=True)
    return proc.stdout


def buildDriver(out):
    if not all(os.path.exists(os.path.join(BUILD, "lib%s.a" % l))
               for l in LIBS):
        return False
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(REPO, "include"), "-o",
            out, os.path.join(HERE, "propinterp.C"), "-L" + BUILD]
           + ["-l" + l for l in LIBS] * 3
           + ["-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto"])
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        sys.exit("could not build propinterp:\n" + proc.stderr[-3000:])
    return True


def mdmsdCheck(driver, env, work):
    """Several long string records (#544, NaCl in water: NA, CL, SOL) stay
    one MDMSD vector with every record whole."""
    fixture = os.path.join(HERE, "fixtures", "gromacs",
                           "nacl_dissolving_ecce-parse-md.txt")
    with open(fixture) as fh:
        text = fh.read()
    records = re.findall(r"^ECCE-MDMSD\s+(\S+)", text, re.M)
    script = os.path.join(REPO, "scripts", "parsers", "gromacs.md")
    stream = subprocess.run(["perl", script], input=text, capture_output=True,
                            text=True, check=True).stdout
    out = os.path.join(work, "msd-props")
    os.makedirs(out)
    proc = subprocess.run([driver, out], input=stream, env=env,
                          capture_output=True, text=True)
    want = "MDMSD rows %d columns  values %d" % (len(records), len(records))
    if want not in proc.stdout.splitlines():
        return ["MDMSD: got %r, want %r" % (proc.stdout.strip(), want)]
    with open(os.path.join(out, "MDMSD")) as fh:
        doc = fh.read()
    cut = [r.split(";")[0] for r in records if r not in doc]
    return ["MDMSD record %s not stored whole" % c for c in cut]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--props", help="write the documents here")
    parser.add_argument("--nbf", type=int, default=160)
    options = parser.parse_args()
    nbf = options.nbf

    with tempfile.TemporaryDirectory(prefix="propinterp-") as work:
        driver = os.path.join(work, "propinterp")
        if not buildDriver(driver):
            print("SKIP: no built static libraries in %s (ECCE_TEST_BUILD)"
                  % BUILD)
            return SKIP
        stream = "".join(parse(b) for b in qmBlocks(nbf))
        longest = max(len(l) for l in stream.splitlines())
        #  The interpreter reads only ECCE_HOME's property table; keep any
        #  ~/.ECCE lookups inside the scratch directory.
        env = dict(os.environ, ECCE_HOME=REPO, ECCE_REALUSERHOME=work,
                   HOME=work)
        argv = [driver] + ([options.props] if options.props else [])
        if options.props:
            os.makedirs(options.props, exist_ok=True)
        proc = subprocess.run(argv, input=stream, env=env,
                              capture_output=True, text=True)
        msdFailures = mdmsdCheck(driver, env, work)

    got = {}
    for line in proc.stdout.splitlines():
        m = re.match(r"(\S+) rows (\S*) columns (\S*) values (\d+)$", line)
        if m:
            got[m.group(1)] = (m.group(2), m.group(3), int(m.group(4)))
    want = {"ORBENG": (str(nbf), "", nbf), "ORBOCC": (str(nbf), "", nbf),
            "MO": (str(nbf), str(nbf), nbf * nbf),
            "MOAOORDER": (None, "", 1)}
    failures = []
    for key, (rows, cols, n) in sorted(want.items()):
        if key not in got:
            failures.append("%s missing" % key)
        elif got[key][2] != n or (rows and got[key][0] != rows):
            failures.append("%s: %s rows, %d values; want %s rows, %d values"
                            % (key, got[key][0], got[key][2], rows, n))
    failures += msdFailures
    print("%d functions, longest parser line %d characters" % (nbf, longest))
    for f in failures:
        print("FAIL  " + f)
    if proc.returncode != 0 or failures:
        print(proc.stdout + proc.stderr[-2000:])
        print("FAILED")
        return 1
    print("ok    ORBENG, ORBOCC, MO (%dx%d), MOAOORDER all whole" % (nbf, nbf))
    print("ok    MDMSD: three records of ~1250 characters, all whole")
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
