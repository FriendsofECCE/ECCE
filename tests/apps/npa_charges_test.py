#!/usr/bin/env python3
"""
The Viewer's "Natural (NPA) Charges, computed by ECCE" row (#1054): an
ORCA water calculation that ran NBO lists both NPA rows; ECCE's row shows
the charges ecce-npa computes from the same results.h5 (Assign Charges puts
them on the atoms), and the first selection stores them in results.h5
(charges/npa_ecce).  Fixture: an ORCA B3LYP/def2-SVP water calculation made
by tests/npa/make_fixtures.py, local data.

Same installed tree and isolation as shot_hooks_test.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without them.
"""

import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

CALCS = {"water": os.path.join(HERE, "fixtures", "calc-water-orca-nbo")}
ECCE_ROW = "Natural (NPA) Charges, computed by ECCE"
NBO_ROW = "Natural (NPA) Charges"
SCENE = """windowsize 1200 900
assigncharges %s
assigncharges %s
""" % (ECCE_ROW, NBO_ROW)


def npaCli(h5):
    exe = os.path.join(os.environ.get("ECCE_TEST_WRAPPERS", ""), "ecce-npa")
    out = subprocess.run([exe, h5, "--json"], capture_output=True, text=True, check=True).stdout
    return [a["charge"] for a in json.loads(out)["atoms"]]


def charges(log, row):
    m = re.search(r"SHOTHOOK: assigncharges %s atoms=\d+ .*? q=\[([^\]]*)\]" % re.escape(row), log)
    return [float(x) for x in m.group(1).split()] if m else None


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "npa-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        calc = urls["water"][len("file://"):]
        h5 = os.path.join(calc, "Outputs", "results.h5")
        if not os.path.exists(h5):
            h5 = os.path.join(calc, "results.h5")
        try:
            want = npaCli(h5)
            scene = os.path.join(folder, "scene")
            with open(scene, "w") as h:
                h.write(SCENE)
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            result = apps.run(display, "builder", args=("-context", urls["water"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_VIEWER_SCENE": scene, "ECCE_VIEWER_SCENE_OUT": out,
                                   "ECCE_VIEWER_SCENE_HOLD": "5"})
            log = result.log or ""
            p = []
            got = charges(log, ECCE_ROW)
            if got is None:
                p.append("no '%s' panel (assigncharges)" % ECCE_ROW)
            elif len(got) != len(want) or max(abs(a - b) for a, b in zip(got, want)) > 1e-4:
                p.append("viewer charges %s, ecce-npa %s" % (got, want))
            if charges(log, NBO_ROW) is None:
                p.append("the NBO row '%s' is missing beside ECCE's" % NBO_ROW)
            #  HDF5 keeps group names as plain bytes
            if b"npa_ecce" not in open(h5, "rb").read():
                p.append("results.h5 has no charges/npa_ecce after the first selection")
            print("%s  NPA rows%s" % ("ok  " if not p else "FAIL",
                                      ": viewer %s = ecce-npa" % got if not p else ": " + "; ".join(p)))
            if p:
                print("\n".join(l for l in log.splitlines() if "SHOTHOOK" in l or "FAIL" in l or "SCENE" in l)[-2500:])
                results.fail("npa_charges", "; ".join(p))
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
