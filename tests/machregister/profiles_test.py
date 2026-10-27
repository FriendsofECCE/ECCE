#!/usr/bin/env python3
"""
The rules for allocation profiles (#541): the MachProfiles file format, which
profile opens, what is refused.

    profiles_test.py

Compiles testAllocationProfiles.C, which includes include/dsm/AllocationProfiles.H
directly (no wx or ecce libraries), and runs it. SKIPs (77) without g++.
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    if not shutil.which("g++"):
        print("SKIP  needs g++")
        return 77
    work = tempfile.mkdtemp(prefix="ecce-profiles-")
    try:
        exe = os.path.join(work, "testAllocationProfiles")
        b = subprocess.run(["g++", "-std=c++17", "-O0", "-Wall", "-o", exe,
                            os.path.join(HERE, "testAllocationProfiles.C")],
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if b.returncode != 0:
            print("FAIL  driver did not compile:\n" + b.stdout)
            return 1
        r = subprocess.run([exe], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True)
        print(r.stdout)
        return 0 if r.returncode == 0 else 1
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
