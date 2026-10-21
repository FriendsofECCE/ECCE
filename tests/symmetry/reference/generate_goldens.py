#!/usr/bin/env python3
"""Runs the CURRENT Fortran programs over corpus/ and stores exit status and
stdout verbatim in expected/<program>/<case>.out.

    generate_goldens.py [--impl <build dir>] [--out <dir>] [--check-determinism]

--check-determinism runs the whole corpus twice into scratch directories and
fails unless the two are byte-identical (uninitialised Fortran variables would
show up here); it writes nothing to expected/.
"""
import argparse
import os
import shutil
import sys
import tempfile

import symref as S


def generate(impl, out):
    if os.path.isdir(out):
        shutil.rmtree(out)
    n = 0
    for prog, name, path in S.all_cases():
        rc, text = S.run_case(impl, S.parse_case(path))
        d = os.path.join(out, prog)
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, name + ".out"), "w") as f:
            f.write("rc %d\n%s" % (rc, text))
        n += 1
    return n


def tree(d):
    r = {}
    for root, _, files in os.walk(d):
        for f in files:
            p = os.path.join(root, f)
            r[os.path.relpath(p, d)] = open(p, "rb").read()
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--impl", default=S.default_impl())
    ap.add_argument("--out", default=S.EXPECTED)
    ap.add_argument("--check-determinism", action="store_true")
    a = ap.parse_args()
    if not a.check_determinism:
        print("wrote %d goldens to %s" % (generate(a.impl, a.out), a.out))
        return 0
    tmp = tempfile.mkdtemp(prefix="symref-det-")
    try:
        n = generate(a.impl, os.path.join(tmp, "a"))
        generate(a.impl, os.path.join(tmp, "b"))
        A, B = tree(os.path.join(tmp, "a")), tree(os.path.join(tmp, "b"))
        bad = sorted(k for k in set(A) | set(B) if A.get(k) != B.get(k))
        for k in bad:
            print("NOT DETERMINISTIC: " + k)
        print("determinism: %d cases, %d differ between two runs" % (n, len(bad)))
        return 1 if bad else 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
