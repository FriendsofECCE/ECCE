#!/usr/bin/env python3
"""Every shipped stored GEOMTRACE fits its structure (#920).

    tests/apps/examples_geomtrace_test.py

Each frame of data/client/examples/**/stored/<calc>/Props/GEOMTRACE must have
as many rows as Parameters/chemsys.mvm has atoms (or as it has non-dummy
atoms: MOPAC and Gaussian leave X out).  The one legitimate difference is a
CREST protomer or solvent cluster: more rows than atoms, and the row labels
("n-El") start with the structure's own elements, the extra atoms after them
(what ScanPoint::adoptTraceAtoms rebuilds from).  Data only: no install.
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..", "..", "data", "client", "examples")


def structure(mvm):
    text = open(mvm).read()
    n = int(re.search(r"num_atoms:\s*(\d+)", text).group(1))
    lines = text.split("atom_list:", 1)[1].splitlines()[1:n + 1]
    return [ln.split()[0] for ln in lines]


def main():
    failures, checked = [], 0
    for trace in sorted(glob.glob(os.path.join(ROOT, "**", "stored", "*",
                                               "Props", "GEOMTRACE"),
                                  recursive=True)):
        calc = os.path.dirname(os.path.dirname(trace))
        rel = os.path.relpath(calc, ROOT)
        mvm = os.path.join(calc, "Parameters", "chemsys.mvm")
        if not os.path.exists(mvm):
            failures.append("%s: no chemsys.mvm" % rel)
            continue
        atoms = structure(mvm)
        text = open(trace).read()
        labels = re.search(r'rowLabels="([^"]*)"', text)
        labels = [l.split("-", 1)[-1] for l in labels.group(1).split()] \
            if labels else []
        real = [a for a in atoms if a != "X"]
        extended = len(labels) > len(atoms) and labels[:len(atoms)] == atoms
        for k, step in enumerate(re.findall(r"<step[^>]*>(.*?)</step>",
                                            text, re.S)):
            n = len(step.split())
            if n % 3 or n // 3 not in (len(atoms), len(real)) and not (
                    extended and n // 3 == len(labels)):
                failures.append("%s: step %d holds %d values for %d atoms"
                                % (rel, k, n, len(atoms)))
                break
        checked += 1
    for f in failures:
        print("FAIL  " + f)
    print("%d stored traces checked" % checked)
    print("FAILED" if failures or not checked else "PASSED")
    return 1 if failures or not checked else 0


if __name__ == "__main__":
    sys.exit(main())
