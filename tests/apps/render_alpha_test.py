"""
Builder Add Structure from File on a large PDB: every frame is opaque (#999).

The viewer's GL canvas has an alpha channel and Coin clears it to 0, so a
compositing X server showed whatever lay beneath the window -- black while
the load kept GTK from painting -- instead of the white Background
preference. With ECCE_TEST_RENDER_CORNER=1 the render area reports a corner
pixel of each frame; every one must be the preference colour, fully opaque.

Uses the ECCE_TEST_IMPORT hook (import_test.py) with the same installed
tree and isolation as run_tests.py. Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

PDB = os.path.abspath(os.path.join(HERE, "..", "dock", "fixtures", "1iep.pdb"))
WHITE = (255, 255, 255, 255)


def setWhiteBackground():
    """Background=#FFFFFF in the (isolated) wxbuilder.ini."""
    path = os.path.join(fixture.stateHome(), ".ECCE", "wxbuilder.ini")
    lines = []
    if os.path.exists(path):
        with open(path) as handle:
            lines = [l for l in handle.read().splitlines()
                     if not l.startswith(("Background=", "Foreground="))]
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as handle:
        handle.write("\n".join(lines + ["Background=#FFFFFF"]) + "\n")


def main():
    def checkApp(display, name, results, verbose=False):
        setWhiteBackground()
        os.environ["ECCE_TEST_IMPORT"] = PDB
        os.environ["ECCE_TEST_RENDER_CORNER"] = "1"
        os.environ["ECCE_TEST_IMPORT_SETTLE"] = "3000"
        authPath = fixture.authFile(
            os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
            user=fixture.realUser())
        result = apps.run(display, "builder", args=("-pipe", authPath),
                          windowTimeout=60, settle=60)
        log = result.log or ""
        #  Only the frames drawn after the import started: the load itself.
        log = log[max(log.find("ECCE_TEST_IMPORT: start"), 0):]
        frames = [tuple(int(v) for v in m.groups()) for m in re.finditer(
            r"RENDER_CORNER before (\d+) (\d+) (\d+) (\d+) "
            r"after (\d+) (\d+) (\d+) (\d+)", log)]
        done = re.search(r"ECCE_TEST_IMPORT: done, (\d+) atoms", log)
        bad = [f for f in frames if f[4:] != WHITE]
        if result.crashed:
            problem = "CRASHED (%s)" % result.signalName
        elif not done:
            problem = "never finished the import"
        elif not frames:
            problem = "no RENDER_CORNER lines (no frame drawn?)"
        elif bad:
            problem = "%d of %d frames not opaque white, first %s" % (
                len(bad), len(frames), bad[0][4:])
        else:
            print("ok    %d frames opaque white, %s atoms; corner alpha "
                  "before the fix-up: %s" % (len(frames), done.group(1),
                  sorted(set(f[3] for f in frames))))
            return
        results.fail("render_alpha", "%s\n%s" % (problem,
                                                 run_tests._tail(log, 20)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
