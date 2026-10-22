#!/usr/bin/env python3
"""The Vibrational Frequencies panel has no Start/Stop: Animation plays (#443).

    tests/apps/vib_autoplay_builder_test.py [--build build-cmake] [--png DIR]

Headless, with the tree's own binaries and the Builder's script hooks (no
synthetic input).  Choosing Animation animates the selected mode at once,
another mode animates that one, Space pauses and resumes, and Vector or
giving up the viewer stops it and puts the atoms back.  The same checks run
on water inside tests/apps/nmode_edit_test.py against an installed tree.
Exit status 77 (skip) without Xvfb, ImageMagick or the built binaries.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import nmode_edit_test as ne  # noqa: E402
import vib_arrows_builder_test as va  # noqa: E402
from harness import say  # noqa: E402

check = bo.check


def runScript(s, env, wrappers, url, tag, lines, outdir):
    path = os.path.join(outdir, tag + ".scene")
    with open(path, "w") as h:
        h.write("\n".join(lines + [""]))
    log = os.path.join(outdir, tag + ".log")
    benv = dict(env, ECCE_VIEWER_SCENE=path, ECCE_VIEWER_SCENE_OUT=outdir)
    with open(log, "w") as out:
        try:
            subprocess.run([os.path.join(wrappers, "ecce-builder"),
                            "-context", url], env=benv, stdout=out,
                           stderr=subprocess.STDOUT, timeout=180,
                           cwd=os.path.join(s.home, "bin"),
                           start_new_session=True)
        except subprocess.TimeoutExpired:
            pass
    return open(log, errors="replace").read()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "vibautoplay", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "nh3-vibration")
        va.makeCalc(calc)
        url = "file://" + calc + "/"

        #  One mode in this calculation: the second selection is the same row.
        script = [l.replace("vibmode 1", "vibmode 0") for l in ne.AUTOPLAY]
        log = runScript(s, env, wrappers, url, "autoplay", script, png)
        problems = ne.autoplayProblems(log)
        check(not problems, "animation autoplay, pause and restore: %s"
              % "; ".join(problems))
        if problems:
            say(log[-1500:])

        log = runScript(s, env, wrappers, url, "scale",
                        ["vibfocus", "vibanim", "vibscale 100", "vibvector",
                         "vibscale 100", "vibscale -3", "vibscale 2.5",
                         "vibstate"], png)
        got = [float(v) for v in re.findall(r"VIBSCALE: (\S+)", log)]
        check(got == [1.5, 5.0, 0.0, 2.5] and "VIBSTATE" in log,
              "typed amplitude clamped to 0-1.5, arrow length to 0-5, no crash: %s" % got)

        runScript(s, dict(env, ECCE_OPEN_PANEL="Vibrational Frequencies"),
                  wrappers, url, "shot",
                  ["vibfocus", "vibanim", "vibstep 3",
                   "xshot " + os.path.join(png, "vib-animation.png")], png)
        check(os.path.exists(os.path.join(png, "vib-animation.png")),
              "panel screenshot taken")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
