#!/usr/bin/env python3
"""PNGs of the MO Diagram of General Chemistry diatomics, from the stored
example calculations, for before/after comparisons of the diagram's look.

    ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<install bin> \\
        tools/screenshots/modiagram_examples.py [OUTDIR [NAME ...]]

NAME is an example folder under genchem/molecular-orbitals-of-diatomics
without its "ecceqm-" prefix (default: H2 HF LiH LiF CO N2).  Runs on a
private Xvfb with local data, through the same Builder hooks as the tests.
OUTDIR defaults to /tmp/ecce-modiag-<commit>, so an offload `apptest` run
(which passes no arguments) leaves one folder per commit on the host.
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

import apps                   # noqa: E402
import isolate                # noqa: E402
import orca_mo_test as base   # noqa: E402
import xdisplay               # noqa: E402

TOPIC = os.path.join(ROOT, "data", "client", "examples", "genchem", "molecular-orbitals-of-diatomics")


def main():
    rev = subprocess.run(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"],
                         capture_output=True, text=True).stdout.strip() or "tree"
    out = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else "/tmp/ecce-modiag-" + rev
    names = sys.argv[2:] or ["H2", "HF", "LiH", "LiF", "CO", "N2"]
    os.makedirs(out, exist_ok=True)
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="modiag-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1700x1000x24"
    bad = 0
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            for n in names:
                stored = os.path.join(TOPIC, "ecceqm-" + n, "stored")
                calc = [d for d in os.listdir(stored) if os.path.isdir(os.path.join(stored, d))][0]
                url = base.makeCalc(data, n.lower(), source=os.path.join(stored, calc))
                d = os.path.join(work, n)
                png = os.path.join(d, n + ".png")
                res = base.runBuilder(display, url, d, "hold 2\n", shot=png, panel="MO Diagram")
                if res.crashed or not os.path.exists(png):
                    print("FAIL", n)
                    bad += 1
                    continue
                shutil.copy(png, os.path.join(out, "modiagram-%s.png" % n))
                print("ok", n)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("PNGs in", out)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
