#!/usr/bin/env python3
"""Regenerate the pre-parsed copies of the shipped examples.

    tools/examples/preparse.py          (needs the installed tree of this
                                         checkout, as tests/apps/run_tests.py:
                                         ECCE_TEST_HOME=<DESTDIR>/usr/local)

Runs the Organizer's first-start import of the `general` example set with
ECCE_NO_PREPARSED=1, i.e. through the real parsers, in a fresh isolated state,
and stores each resulting calculation as
data/client/examples/general/<example>/stored/<calc>/ with a `stamp` naming
the output it came from (StoredCalcCopy::stampOf).  Run it when an example
or a parser changes; tests/apps/examples_preparsed_test.py fails when a stored
copy no longer agrees with a parse.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
GENERAL = os.path.join(ROOT, "data", "client", "examples", "general")
OUTPUTS = {  # example folder -> output file, as CalcMgr's exampleSets()
    "ecceqm-water": "water.qmout", "orca-benzene": "benzene.out",
    "ecceqm-ethanol": "ethanol.qmout", "mopac-ammonia": "ammonia.out",
    "nwchem-methane": "methane.eprint"}
FNV = 1469598103934665603


def stamp(path):
    with open(path, "rb") as f:
        data = f.read()
    h = FNV
    for b in data:
        h = ((h ^ b) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return "1 %x %d" % (h, len(data))


def main():
    out = tempfile.mkdtemp(prefix="preparse-",
                           dir=os.path.expanduser("~/.cache"))
    env = dict(os.environ, ECCE_NO_PREPARSED="1")
    arm = os.path.join(ROOT, "tests", "apps", "examples_arm.py")
    rc = subprocess.run([sys.executable, arm, out], env=env).returncode
    if rc != 0:
        return rc
    project = os.path.join(out, "Examples")
    for example, output in OUTPUTS.items():
        calc = example.split("-", 1)[1]
        dest = os.path.join(GENERAL, example, "stored")
        shutil.rmtree(dest, ignore_errors=True)
        shutil.copytree(os.path.join(project, calc),
                        os.path.join(dest, calc), symlinks=True,
                        ignore=shutil.ignore_patterns("Thumbnail*", "*.lock"))
        with open(os.path.join(dest, "stamp"), "w") as f:
            f.write(stamp(os.path.join(GENERAL, example, output)) + "\n")
        print("stored", example)
    shutil.rmtree(out, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
