#!/usr/bin/env python3
"""ECCE-QM's printed symmetry number reaches THERMOSIGMA (#1043).

ecce-thermo takes sigma from THERMOSIGMA when the parser stored one, as for
ORCA and Gaussian.  This checks that ecceqm.desc routes the `symmetry_number`
line to ecceqm.parse and that the parse emits the key, on the line as an
example calculation's output has it.
"""
import glob
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PARSERS = os.path.join(REPO, "scripts", "parsers")


def main():
    fails = 0

    def check(ok, what):
        nonlocal fails
        print("%s: %s" % ("ok  " if ok else "FAIL", what))
        fails += 0 if ok else 1

    desc = open(os.path.join(PARSERS, "ecceqm.desc")).read()
    block = re.search(r"\[THERMOSIGMA\]\n(.*?)\[END\]", desc, re.S)
    check(block is not None and "Begin=^symmetry_number\\s" in block.group(1)
          and "Script=ecceqm.parse" in block.group(1), "ecceqm.desc has a THERMOSIGMA entry")
    outs = glob.glob(os.path.join(REPO, "data", "client", "examples", "genchem", "*",
                                  "ecceqm-N2", "N2.qmout"))
    line = "symmetry_number 2"
    if outs:
        found = [l.strip() for l in open(outs[0]) if l.startswith("symmetry_number")]
        check(found == ["symmetry_number 2"], "the N2 example's output prints sigma 2")
        line = found[-1] if found else line
    out = subprocess.run(["perl", os.path.join(PARSERS, "ecceqm.parse")], input=line + "\n",
                         capture_output=True, text=True).stdout
    check(re.search(r"key: THERMOSIGMA\nsize:\n1\nvalues:\n2\n", out) is not None,
          "ecceqm.parse emits THERMOSIGMA 2")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
