#!/usr/bin/env python3
"""
Does Machine Registration's/the Launcher's GB<->MB memory conversion round
trip correctly?

    ./run_tests.py

The "Max Memory" queue field is shown in GB (Andy's GB-everywhere UX
preference); the .Q file's memLimit and every other consumer of it
(MachinePreferences, TaskJob::maxmemory) stay MB, per README.Q and
siteconfig/example-slurm.Q. MemoryUnits.H (src/apps/machregister/ and its
identical copy in src/apps/launcher/) is the helper that converts at the
widget. This compiles testMemoryUnits.C, which includes the header
directly -- no wx or ecce libraries needed. Needs g++; SKIPs otherwise.
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))


def main():
    if not shutil.which("g++"):
        print("SKIP  needs g++")
        return 0

    work = tempfile.mkdtemp(prefix="ecce-machregister-")
    try:
        exe = os.path.join(work, "testMemoryUnits")
        build = subprocess.run(
            ["g++", "-O0", "-w", "-o", exe,
             os.path.join(HERE, "testMemoryUnits.C")],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if build.returncode != 0:
            print("FAIL  driver did not compile:\n%s"
                  % build.stdout.decode("utf-8", "replace"))
            return 1

        run = subprocess.run([exe], stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT)
        out = run.stdout.decode("utf-8", "replace")
        print(out)
        if run.returncode != 0:
            print("FAIL  testMemoryUnits exited %d" % run.returncode)
            return 1

        print("PASS")
        return 0
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
