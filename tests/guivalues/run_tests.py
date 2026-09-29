#!/usr/bin/env python3
"""
Is a memory size saved in a pre-GB unit converted before anything reads it?

    ./run_tests.py

Every theory dialog's memory field is entered in GB and every generator
reads the bare number as GB, because GUIValues::dumpKeyVals() does not pass
the unit on.  A calculation saved earlier still stores Megawords (Gaussian,
NWChem, GAMESS-UK, MetaDyn) or Megabytes (ORCA), and read as GB that is a
100-1000x memory request: Calculation-9-2's 1800 Megawords became
%Mem=1800GB.  GUIValues::load() converts; this compiles it with a small
driver (testLegacyMemory.C) and checks what a generator and a dialog would
each be handed.  Needs g++ and build-cmake/libecceutil.a; SKIPs otherwise.
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
UTIL = os.path.join(ROOT, "build-cmake", "libecceutil.a")

EXPECT = [
    # 1800 MW = 14.4 GB at 8 bytes a word: nearest whole GB.
    "ES.Theory.SCF.MemorySize|14|Gigabytes|1|1|integer_input",
    "ES.Theory.SCF.MemorySize: 14",
    # 40 MW = 0.32 GB: a nonzero setting must not become 0.
    "ES.Theory.MemorySize|1|Gigabytes|1|1|integer_input",
    # Disk is still entered in Megawords: left alone.
    "ES.Theory.SCF.DiskSize|64|Megawords|1|1|integer_input",
    "converted 1",
    # ORCA's per-core qualifier survives the conversion.
    "--orca converted 1",
    "ES.Theory.SCF.MemorySize|1|Gigabytes / core|1|1|integer_input",
    # A current GB value is untouched and does not mark the calc stale.
    "--current converted 0",
    "ES.Theory.SCF.MemorySize|6|Gigabytes|1|1|integer_input",
    "--copy converted 1",
]


def main():
    if not shutil.which("g++") or not os.path.exists(UTIL):
        print("SKIP  needs g++ and %s" % UTIL)
        return 0
    work = tempfile.mkdtemp(prefix="ecce-guivalues-")
    try:
        exe = os.path.join(work, "testLegacyMemory")
        build = subprocess.run(
            ["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"),
             "-o", exe, os.path.join(HERE, "testLegacyMemory.C"),
             os.path.join(ROOT, "src", "tdat", "calcorg", "GUIValues.C"),
             UTIL], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if build.returncode != 0:
            print("FAIL  driver did not compile:\n%s"
                  % build.stdout.decode("utf-8", "replace"))
            return 1
        out = subprocess.run([exe], stdout=subprocess.PIPE, check=True)
        lines = out.stdout.decode().splitlines()
    finally:
        shutil.rmtree(work, ignore_errors=True)
    missing = [line for line in EXPECT if line not in lines]
    for line in missing:
        print("FAIL  expected %r" % line)
    if missing:
        print("---- driver output ----\n" + "\n".join(lines))
        return 1
    print("PASS  %d checks" % len(EXPECT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
