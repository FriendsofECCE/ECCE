#!/usr/bin/env python3
"""Screenshots of the ECCE-QM integration, headlessly, in local data mode.

    ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<install bin> \\
        tools/screenshots/qm_shots.py --out DIR [--only NAME ...] [--tmp DIR]

Names: calced (the Calculation Editor on a ready ECCE-QM calculation),
theory-details (the Theory Details dialog; made by tests/look/codereg_shot.py),
builder-mo (the Builder with the MOs panel and a singly occupied pi* orbital
of triplet O2).

The calculation is tests/ecceqm/fixtures/o2-roks, a real run of the bundled
engine.  Same method as help_shots.py: private Xvfb, light theme, nothing
clicked or typed.
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

QM_CALC = os.path.join(ROOT, "tests", "ecceqm", "fixtures", "o2-roks")
NAMES = ["calced", "theory-details", "builder-mo"]
#  The pi* pair of O2 are orbitals 8 and 9, occupied by one electron each.
MO = 8


class QmData(help_shots.Data):
    def calc(self, state, props=True, project="tutorial", name="o2-triplet",
             user=None):
        old = help_shots.FIXTURE
        help_shots.FIXTURE = QM_CALC
        try:
            return help_shots.Data.calc(self, state, props=props, setup=True,
                                        project=project, name=name, user=user)
        finally:
            help_shots.FIXTURE = old


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--only", action="append", choices=NAMES)
    parser.add_argument("--display", type=int, default=177)
    parser.add_argument("--tmp", default=os.path.join(ROOT, "build-cmake", "qm-shots"))
    options = parser.parse_args()
    want = options.only or NAMES
    os.makedirs(options.out, exist_ok=True)
    os.makedirs(options.tmp, exist_ok=True)

    if "theory-details" in want:
        subprocess.run([sys.executable, os.path.join(ROOT, "tests", "look", "codereg_shot.py"),
                        options.tmp, "--codes", "ecceqm", "--tag", "qm"], check=True)
        shutil.copy(os.path.join(options.tmp, "qm-ecceqm-theory-light.png"),
                    os.path.join(options.out, "theory-details.png"))
        want = [n for n in want if n != "theory-details"]
        if not want:
            return 0

    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    os.environ["ECCE_REALUSER"] = "student"
    xdisplay.SCREEN = "1700x1100x24"
    data = QmData("/tmp/ecce-qm-shots")
    os.environ["ECCE_LOCAL_DATA"] = data.folder
    env = {"ECCE_TRANSPARENCY_FALLBACK_MS": "0"}

    failed = 0
    with xdisplay.Display(number=options.display) as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            data.reset()
            apps.run(display, "organizer", windowTimeout=60, settle=20)
            for name in want:
                err = None
                data.reset()
                data.project()
                if name == "calced":
                    data.calc("Ready", props=False)
                    err = help_shots.shootWindow(
                        display, options.tmp, options.out, name, "ecce-calced",
                        ["-context", data.url("tutorial", "o2-triplet")],
                        env, settle=30)
                elif name == "builder-mo":
                    data.calc("Complete")
                    scene = "mo %d 0.04 50" % MO
                    err = help_shots.shootBuilder(
                        display, options.tmp, options.out, name,
                        data.url("tutorial", "o2-triplet"), "MOs",
                        size=(1500, 1000), turn=0, extra=scene)
                if err:
                    print("  %s: %s" % (name, err))
                    failed += 1
        finally:
            os.environ.pop("ECCE_NO_REAP", None)
            apps.stopServices(display)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
