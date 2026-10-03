#!/usr/bin/env python3
"""
FileEDSI, the data-storage backend that needs no Apache (#216).

    ./run_tests.py         build the driver and run it
    ./run_tests.py -v      echo the driver's own output

filedsiTest.C goes through EDSIFactory with a file:// URL against a
throwaway directory and checks metadata (the per-directory .ecce-meta
sidecar), append and ranged reads, and that copy, move and remove carry
the metadata with the resource.  No server of any kind is started.

Links the static libraries of build-cmake (ECCE_TEST_BUILD overrides) and
SKIPs when there is no build tree.  Exit status is 0 only if every check
passed (or the suite was skipped).
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
BUILD = os.environ.get("ECCE_TEST_BUILD", os.path.join(REPO, "build-cmake"))
LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "eccecipc", "ecceutil", "eccecomm", "eccercmd"]


def main():
    verbose = "-v" in sys.argv[1:]
    if not all(os.path.exists(os.path.join(BUILD, "lib%s.a" % l)) for l in LIBS):
        print("SKIP: no built static libraries in %s (set ECCE_TEST_BUILD)"
              % BUILD)
        return 0
    state = tempfile.mkdtemp(prefix="filedsi.")
    try:
        env = dict(os.environ)
        env.setdefault("ECCE_HOME", REPO)
        home = os.path.join(state, "home")
        os.mkdir(home)
        env["ECCE_REALUSERHOME"] = home
        env["HOME"] = home
        env["ECCE_REALUSER"] = "tester"
        rc = 0
        # resourceTest runs twice: create, then re-open in a new process.
        for name, args in (("filedsiTest", []), ("resourceTest", ["create"]),
                           ("resourceTest", ["reopen"]), ("lockTest", [])):
            driver = os.path.join(state, name)
            if not os.path.exists(driver):
                cmd = (["g++", "-O0", "-w", "-I", os.path.join(REPO, "include"),
                        "-o", driver, os.path.join(HERE, name + ".C"),
                        "-L" + BUILD] + ["-l" + l for l in LIBS] * 3
                       + ["-lxerces-c"])
                proc = subprocess.run(cmd, capture_output=True, text=True)
                if proc.returncode != 0:
                    print("could not build %s:\n%s" % (name, proc.stderr[-3000:]))
                    rc = 1
                    continue
            scratch = os.path.join(state, name + ".store")
            if not os.path.isdir(scratch):
                os.mkdir(scratch)
            proc = subprocess.run([driver] + args + [scratch],
                                  capture_output=True, text=True, env=env,
                                  timeout=120)
            lines = proc.stdout.splitlines()
            bad = [l for l in lines if l.startswith("FAIL ")]
            if verbose or bad or proc.returncode != 0:
                print(proc.stdout + proc.stderr[-2000:])
            print("%s %s: %d checks, %d failed" %
                  (name, " ".join(args), sum(1 for l in lines
                             if l.startswith(("PASS ", "FAIL "))), len(bad)))
            if proc.returncode != 0 or bad:
                rc = 1
        return rc
    finally:
        shutil.rmtree(state, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
