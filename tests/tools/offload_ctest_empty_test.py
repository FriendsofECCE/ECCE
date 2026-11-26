#!/usr/bin/env python3
"""#1301: an offload ctest run whose -R matches nothing must fail.

Plain `ctest -R typo` exits 0 after running no tests.  Check that (a) ctest
here fails an empty selection under --no-tests=error, and (b) every ctest
invocation in remote-run.sh passes that flag.
"""
import os, re, subprocess, sys, tempfile

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
src = open(os.path.join(root, "tools/offload/remote-run.sh")).read()

bad = [l.strip() for l in src.splitlines()
       if re.search(r"&& ctest |\bctest -j", l) and "--no-tests=error" not in l]
if bad:
    print("FAIL: ctest invoked without --no-tests=error:\n  " + "\n  ".join(bad))
    sys.exit(1)

with tempfile.TemporaryDirectory() as d:
    open(d + "/CMakeLists.txt", "w").write(
        "cmake_minimum_required(VERSION 3.19)\nproject(t NONE)\nenable_testing()\n"
        "add_test(NAME real COMMAND ${CMAKE_COMMAND} -E true)\n")
    subprocess.run(["cmake", "-S", d, "-B", d + "/b"], check=True, capture_output=True)
    def rc(*a):
        return subprocess.run(["ctest", "--test-dir", d + "/b", *a],
                              capture_output=True).returncode
    if rc("-R", "real", "--no-tests=error") != 0:
        print("FAIL: matching pattern should pass"); sys.exit(1)
    if rc("-R", "nomatch_typo", "--no-tests=error") == 0:
        print("FAIL: empty selection still exits 0"); sys.exit(1)
print("PASS")
