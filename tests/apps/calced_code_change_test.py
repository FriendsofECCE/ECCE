#!/usr/bin/env python3
"""
The Calculation Editor while the Code Change question is open (#308).

Changing the code asks "This calculation was set up for ECCE-QM.  Change it
to NWChem?".  The editor used to stay frozen (Freeze) while the question was
up, so the areas it uncovered were drawn as black boxes.  The script clicks
the NWChem button of a calculation that is not yet run; the question's test
hook (ECCE_TEST_DIALOG_CLOSE) says whether its parent is frozen and closes
it after a while.  Meanwhile the editor is photographed
(ECCE_CODE_CHANGE_PNG, else the state directory) and its black pixels
counted.

Same installed tree, isolation and services as run_tests.py.  Exit 77
(skip) without an install or Xvfb.
"""

import os
import re
import shutil
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402

QM_CALC = os.path.join(REPO, "tests", "ecceqm", "fixtures", "o2-roks")
PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")
QUESTION = "Electronic Structure Editor Code Change"


def geometry(display, wid):
    """(x, y, w, h) of a window on the screen, from xwininfo."""
    out = subprocess.run(["xwininfo", "-display", display.name, "-id", wid],
                         capture_output=True, text=True, timeout=20).stdout
    get = lambda k: int(re.search(k + r":\s+(-?\d+)", out).group(1))
    return (get("Absolute upper-left X"), get("Absolute upper-left Y"),
            get("Width"), get("Height"))


def blackPixels(png, cover=None):
    """Pixels of pure black in `png` (ImageMagick), or None.  `cover` is a
    rectangle (x0, y0, x1, y1) painted over first: the question itself,
    which a grab of the editor window shows as black."""
    draw = ["-fill", "white", "-draw", "rectangle %d,%d %d,%d" % cover] \
        if cover else []
    out = subprocess.run(["convert", png] + draw + ["-fill", "white",
                          "+opaque", "black", "-format", "%[fx:w*h*(1-mean)]",
                          "info:"], capture_output=True, text=True)
    try:
        return int(round(float(out.stdout.strip())))
    except ValueError:
        return None


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "codechange-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "qm")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        shutil.copytree(QM_CALC, os.path.join(project, "o2"), symlinks=True)
        # A finished calculation is read-only and cannot change code.
        meta = os.path.join(project, "o2", ".ecce-meta")
        text = open(meta).read().replace("ecce:state\t\tComplete",
                                         "ecce:state\t\tReady")
        open(meta, "w").write(text)
        os.environ["ECCE_LOCAL_DATA"] = folder
        png = os.environ.get("ECCE_CODE_CHANGE_PNG") or os.path.join(
            fixture.stateDir(), "calced-code-change.png")
        if os.path.exists(png):
            os.unlink(png)
        seen = {}

        def photographer():
            end = time.time() + 150
            while time.time() < end:
                wins = display.windows()
                if any(t == QUESTION for _, t in wins):
                    time.sleep(3)
                    editor = [w for w, t in wins if t.endswith("Editor")]
                    question = [w for w, t in wins if t == QUESTION]
                    seen["windows"] = [t for _, t in wins]
                    if editor:
                        ex, ey, _, _ = geometry(display, editor[0])
                        qx, qy, qw, qh = geometry(display, question[0])
                        seen["cover"] = (qx - ex, qy - ey,
                                         qx - ex + qw, qy - ey + qh)
                        subprocess.run(["import", "-display", display.name,
                                        "-window", editor[0], png],
                                       stdout=subprocess.DEVNULL,
                                       stderr=subprocess.DEVNULL)
                    return
                time.sleep(0.5)

        watcher = threading.Thread(target=photographer, daemon=True)
        watcher.start()
        restorePrefs = fixture.settleUpgradeNotices()
        # The question names itself on stderr and closes (No) after 15 s.
        os.environ["ECCE_TEST_DIALOG_CLOSE"] = "15"
        try:
            steps = ["wait 9000", "code NWChem", "wait 2000", "quit"]
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps,
                               "file://" + os.path.join(project, "o2") + "/")
            watcher.join(10)
            sym.check("[%s]" % QUESTION in log, "the question was asked",
                      run_tests._tail(log, 40))
            sym.check("ECCE_TEST_DIALOG_PARENT: not frozen" in log and
                      "ECCE_TEST_DIALOG_PARENT: frozen" not in log,
                      "the editor is not frozen while the question is open",
                      run_tests._tail(log, 40))
            black = blackPixels(png, seen.get("cover")) \
                if os.path.exists(png) else None
            print("black pixels in the editor while the question is open: "
                  "%s (%s); windows %s" % (black, png, seen.get("windows")))
            sym.check(black is not None and black < 2000,
                      "no black areas while the question is open (%s black "
                      "pixels; %s)" % (black, png))
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
            os.environ.pop("ECCE_TEST_DIALOG_CLOSE", None)
        if sym.failures:
            results.fail("calced-code-change", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
