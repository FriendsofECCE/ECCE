#!/usr/bin/env python3
"""
The wx-free rules behind allocation profiles (#541: the MachProfiles file
format, which profile opens, what is refused) and the allocation left (#613:
projinfo parsing on an anonymised real sample, the 15-minute cache).

    profiles_test.py

Compiles testAllocationProfiles.C, testAllocationLeft.C and
testLaunchInfoRows.C (#648), which include include/dsm/AllocationProfiles.H,
AllocationLeft.H and LaunchInfoRows.H directly (no wx or ecce
libraries), and runs them. SKIPs (77) without g++.
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DRIVERS = (("testAllocationProfiles", []),
           ("testAllocationLeft",
            [os.path.join(HERE, "fixtures", "projinfo-kebnekaise.txt")]),
           ("testLaunchInfoRows", []))


def main():
    if not shutil.which("g++"):
        print("SKIP  needs g++")
        return 77
    work = tempfile.mkdtemp(prefix="ecce-profiles-")
    try:
        for name, args in DRIVERS:
            exe = os.path.join(work, name)
            b = subprocess.run(["g++", "-std=c++17", "-O0", "-Wall", "-o", exe,
                                os.path.join(HERE, name + ".C")],
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True)
            if b.returncode != 0:
                print("FAIL  %s did not compile:\n%s" % (name, b.stdout))
                return 1
            r = subprocess.run([exe] + args, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True)
            print(r.stdout)
            if r.returncode != 0:
                return 1
        return 0
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
