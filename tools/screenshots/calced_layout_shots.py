#!/usr/bin/env python3
"""The Calculation Editor on a ready NWChem and a ready ORCA calculation,
headlessly in local data mode (Geometry Constraints is in the Settings box).

    ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<install bin> \\
        tools/screenshots/calced_layout_shots.py --out DIR [--tmp DIR]
"""

import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

import help_shots  # noqa: E402
import apps        # noqa: E402
import isolate     # noqa: E402
import xdisplay    # noqa: E402

FIXTURES = os.path.join(ROOT, "tests", "apps", "fixtures")
CASES = [("nwchem", "calc-water-opt"), ("orca", "calc-orca-water-ccpvdz")]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--tmp", required=True)
    options = parser.parse_args()
    os.makedirs(options.out, exist_ok=True)
    os.makedirs(options.tmp, exist_ok=True)
    settings = isolate.apply(apps.INSTALL)
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    os.environ["ECCE_REALUSER"] = "student"
    xdisplay.SCREEN = "1700x1100x24"
    data = help_shots.Data(os.path.join(options.tmp, "data"))
    os.environ["ECCE_LOCAL_DATA"] = data.folder
    env = {"ECCE_TRANSPARENCY_FALLBACK_MS": "0"}
    failed = 0
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            data.reset()
            apps.run(display, "organizer", windowTimeout=60, settle=20)
            for name, fixture in CASES:
                data.reset()
                data.project()
                help_shots.FIXTURE = os.path.join(FIXTURES, fixture)
                data.calc("Ready", props=False, name=name)
                err = help_shots.shootWindow(
                    display, options.tmp, options.out, "calced-" + name,
                    "ecce-calced", ["-context", data.url("tutorial", name)],
                    env, settle=30)
                if err:
                    print("  %s: %s" % (name, err))
                    failed += 1
        finally:
            os.environ.pop("ECCE_NO_REAP", None)
            apps.stopServices(display)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
