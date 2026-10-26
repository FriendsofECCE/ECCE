#!/usr/bin/env python3
"""
One atom, one number (#582): atom 2 of the stored ECCE-QM HF calculation
must read 2 in the viewer's selection status line, in an atom label, in a
Mulliken Charges table row and in the Density Profile lists, all counting
from 1.  Headless Builder scene commands (atomnumbers, dpcentre, dpatoms).

Same installed tree and isolation as densityprofile_pick_test.py
(ECCE_TEST_HOME, ECCE_TEST_WRAPPERS).  Exit 77 (skip) without them.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402
from densityprofile_pick_test import stored   # noqa: E402

CALCS = {"hf": stored("genchem/molecular-orbitals-of-diatomics", "ecceqm-HF")}
SCENE = """dpopen
dpatoms 2 1
dpstate
dpmode atom
dpcentre 2
dpstate
atomnumbers 2 Mulliken Charges
hold 1
"""


def check(log):
    p = []
    m = re.search(r"ATOMNUMBERS: 2 status=\[(\S+)[^\]]*\] label=\[(\S+)\]", log)
    if not m:
        return ["no atomnumbers line"]
    sym = re.match(r"[A-Za-z]+", m.group(2))
    sym = sym.group(0) if sym else "?"
    want = sym + "2"
    for what, got in (("status line", m.group(1)), ("label", m.group(2))):
        if got != want:
            p.append("%s says %s, atom 2 is %s" % (what, got, want))
    row = re.search(r"ATOMNUMBERS: table .* row \[2\] \[(\w+)\]", log)
    if not row:
        p.append("no row 2 in the Mulliken Charges table")
    elif row.group(1) != sym:
        p.append("charge table row 2 is %s, not %s" % (row.group(1), sym))
    cl = re.findall(r"^centrelabel (\S+)", log, re.M)
    al = re.findall(r"^atomalabel (\S+)", log, re.M)
    if not cl or cl[-1] != want:
        p.append("Density Profile centre list says %s, not %s" % (cl[-1:] or None, want))
    if len(al) < 2 or al[1] != want:
        p.append("Density Profile 'from' list says %s, not %s" % (al[1:2] or None, want))
    return p


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "atomnumbers-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        try:
            scene = os.path.join(folder, "scene")
            with open(scene, "w") as h:
                h.write(SCENE)
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            result = apps.run(display, "builder", args=("-context", urls["hf"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_PANEL_FULLSCREEN": "1",
                                   "ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out})
            problems = check(result.log or "")
            print("%s  atom numbers%s" % ("ok  " if not problems else "FAIL",
                                          "" if not problems else ": " + "; ".join(problems)))
            if problems:
                print("\n".join(l for l in (result.log or "").splitlines()
                                if "ATOMNUMBERS" in l or "FAIL" in l or "label " in l)[-1500:])
                results.fail("atom_numbers", "; ".join(problems))
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
