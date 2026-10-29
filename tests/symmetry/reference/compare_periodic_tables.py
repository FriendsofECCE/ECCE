#!/usr/bin/env python3
"""Compare C++ and Fortran genmollat across all legacy group names."""
import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
SPNAMES = os.path.join(ROOT, "src/apps/symmetry/spnames.f90")


def table_names(source, table):
    text = open(source).read()
    found = []
    pos = 0
    needle = "data (" + table
    while True:
        start = text.lower().find(needle.lower(), pos)
        if start < 0:
            return found
        slash = text.find("/", start)
        end = slash + 1
        quoted = False
        while end < len(text):
            if text[end] == "'":
                quoted = not quoted
            elif text[end] == "/" and not quoted:
                break
            end += 1
        found.extend(value.strip() for value in
                     re.findall(r"'([^']*)'", text[slash + 1:end]))
        pos = end + 1


def run(executable, group):
    input_text = ("1\n0.001\n%-10s\n5 6 7 90 100 90\n"
                  "C\n6 0.123 0.234 0.345\n") % group
    result = subprocess.run([executable], input=input_text, text=True,
                            capture_output=True, timeout=30)
    if result.returncode:
        return result.returncode, result.stderr.strip(), None
    lines = result.stdout.splitlines()
    count = int(lines[0])
    atoms = []
    for line in lines[2:2 + count]:
        atoms.append((line[:16].strip(),
                      [float(value) for value in line[16:].split()]))
    return 0, "", atoms


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fortran", required=True,
                        help="path to the reference genmollat executable")
    parser.add_argument("--cpp", required=True,
                        help="path to the genmollat_cpp executable")
    args = parser.parse_args()
    cases = sum(([(kind, name) for name in table_names(SPNAMES, table)]
                 for kind, table in (("space", "spgnam"), ("layer", "lyrnam"),
                                     ("line", "linenam"))), [])
    failures = 0
    for kind, group in cases:
        expected_rc, expected_err, expected = run(args.fortran, group)
        actual_rc, actual_err, actual = run(args.cpp, group)
        problem = None
        if expected_rc != actual_rc:
            problem = "exit status %d != %d" % (actual_rc, expected_rc)
        elif expected is not None and actual is not None:
            if len(expected) != len(actual):
                problem = "atom count %d != %d" % (len(actual), len(expected))
            else:
                for index, (want, got) in enumerate(zip(expected, actual), 1):
                    if want[0] != got[0] or max(
                            (abs(a - b) for a, b in zip(want[1], got[1])),
                            default=0.0) > 1e-6:
                        problem = "atom %d differs" % index
                        break
        if problem:
            failures += 1
            print("FAIL %s %s: %s" % (kind, group, problem))
    print("%d periodic group names, %d failed" % (len(cases), failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
