#!/usr/bin/env python3
"""Builder File > Add Structure from File and Open use the platform's file
chooser (#342); ECCE_TEST_LOCAL_FILE stands in for the pick, since a native
dialog cannot be driven.

    tests/apps/local_file_test.py            own Xvfb and services

Headless, through ECCE_BUILDER_SCRIPT on a calculation (pbc_edit_test's
harness): the menu item is chosen and the water written here must arrive.
"""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                       # noqa: E402
import fixture                    # noqa: E402
import pbc_edit_test as pe        # noqa: E402

WATER = """3
water
O   0.000  0.000  0.117
H   0.000  0.757 -0.467
H   0.000 -0.757 -0.467
"""

CASES = (
    ("add", ["wait 2000", "cmd clear", "menu File|Add Structure from File...",
             "wait 2000", "info"], "BuilderAddStructure"),
    ("open", ["wait 2000", "menu File|Open...", "wait 3000", "info"],
     "BuilderOpen"),
)


def runCase(display, results, case, url, args, verbose, timeout):
    name, script, key = case
    work = os.path.join(fixture.stateHome(), "local-file", name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    xyz = os.path.join(work, "water.xyz")
    with open(xyz, "w") as handle:
        handle.write(WATER)
    path = os.path.join(work, "local.script")
    with open(path, "w") as handle:
        handle.write("\n".join(script + ["quit"]) + "\n")
    result = apps.run(display, "builder", args=args, windowTimeout=120,
                      settle=timeout,
                      env={"ECCE_BUILDER_SCRIPT": path,
                           "ECCE_TEST_LOCAL_FILE": xyz,
                           "ECCE_TEST_XYZ_UNITS": "angstrom",
                           "ECCE_REALUSER": fixture.USER})
    log = result.log or ""
    tail = "\n".join("      | " + l for l in log.splitlines()[-20:])
    if result.crashed:
        results.fail("local file " + name, "builder CRASHED\n" + tail)
        return
    found = []
    if "LOCALFILE: %s -> %s" % (key, xyz) not in log:
        found.append("the menu item did not ask the local file chooser (%s)" % key)
    n = [(int(a), int(b)) for a, b in
         re.findall(r"BUILDER: atoms=(\d+) lattice=\w+ nubs=(\d+)", log)]
    if not n or n[-1][0] - n[-1][1] != 3:
        found.append("the water did not arrive: atoms, nubs %s" % (n[-1:],))
    for p in found:
        results.fail("local file " + name, p + "\n" + tail)
    if not found:
        print("ok    local file %s" % name)


def main():
    pe.CASES = CASES
    pe.runCase = runCase
    pe.FIXTURE = "local-file-water"
    return pe.main()


if __name__ == "__main__":
    sys.exit(main())
