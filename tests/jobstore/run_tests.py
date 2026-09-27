#!/usr/bin/env python3
"""
Unit test for JobFailureReason::extractReason (src/comm/commxt's job
monitor cleanup(), #-- Andy 2026-09-27).

Compiles tests/jobstore/testJobFailureReason.C with plain g++ against
include/ only (JobFailureReason.H has no ECCE/X11 dependencies) and runs
it against real and synthetic fixtures under fixtures/.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
FIXTURES = os.path.join(HERE, "fixtures")


def main():
    out = os.path.join(HERE, "testJobFailureReason.bin")
    cmd = ["g++", "-O2", "-w", "-std=c++14",
           "-I", os.path.join(ROOT, "include"),
           os.path.join(HERE, "testJobFailureReason.C"),
           "-o", out]
    r = subprocess.run(cmd)
    if r.returncode != 0:
        print("BUILD FAILED", file=sys.stderr)
        return 1

    r = subprocess.run([out, FIXTURES])
    try:
        os.remove(out)
    except OSError:
        pass
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
