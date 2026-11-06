#!/usr/bin/env python3
"""Playing a geometry trace keeps the user's camera, headless.

    tests/apps/trace_camera_test.py [--build build-cmake] [--keep]

The Ar-liquid GROMACS example (1000 atoms in a periodic box) is imported
with the Organizer hook `gmximport`; the stored ORCA optimisation of methyl
bromide is copied in as a data-server calculation.  For each, the Builder
opens the Geometry Trace panel, the scene script zooms in so that atoms
leave the view, steps several frames and plays the trace: the camera must
be the zoomed one afterwards (it was fitted back to the whole box on every
frame).  Exit status 77 (skip) without Xvfb or the built binaries.
"""
import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
sys.path.insert(0, os.path.join(REPO, "tests", "parsers"))

import builder_overlay_test as bo  # noqa: E402
import energies_groups_test as eg  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
EXAMPLES = os.path.join(REPO, "data", "client", "examples")
GMX = os.path.join(EXAMPLES, "genchem", "gases-liquids-solids", "gromacs-ar-liquid")
ORCA = os.path.join(EXAMPLES, "organic", "reactions", "orca-methyl_bromide", "stored")
EXES = ("builder", "organizer")


def cameras(text):
    """label -> the numbers of a "SCENE camera" line."""
    out = {}
    for label, nums in re.findall(r"SCENE camera (\S+): (.*)", text):
        out[label] = [float(v) for v in re.findall(r"-?[\d.]+(?:e[-+]?\d+)?", nums)]
    return out


def same(a, b, tol=1e-4):
    return a is not None and b is not None and len(a) == len(b) and \
        all(abs(x - y) <= tol * max(1.0, abs(x)) for x, y in zip(a, b))


def importGromacs(s, env, wrappers, proj, run):
    """The GROMACS run as a study in proj; the dynamics task's directory."""
    cmds = os.path.join(s.state, "organizer-commands")
    open(cmds, "w").close()
    olog = os.path.join(s.state, "organizer.log")
    org = subprocess.Popen(
        [os.path.join(wrappers, "ecce-organizer")],
        env=dict(env, ECCE_TEST_ORGANIZER=cmds),
        cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
        stderr=subprocess.STDOUT, start_new_session=True)
    try:
        with open(cmds, "a") as h:
            h.write("gmximport %s %s\n" % (proj, run))
        end = time.time() + 240
        answer = traces = None
        while time.time() < end and org.poll() is None:
            text = open(olog, errors="replace").read()
            want = "ECCE_TEST_ORGANIZER: gmximport %s %s: " % (proj, run)
            for line in text.splitlines():
                if line.startswith(want):
                    answer = line[len(want):]
            traces = glob.glob(os.path.join(proj[len("file://"):], "*", "*",
                                            "Props", "GEOMTRACE"))
            if answer and traces:
                time.sleep(5)    # the monitor stores the rest after it
                break
            time.sleep(1)
        check(bool(answer) and answer.startswith("ok"), "gmximport: %s" % answer)
        check(bool(traces), "the imported dynamics has a GEOMTRACE")
        return os.path.dirname(os.path.dirname(traces[0])) if traces else None
    finally:
        bo.stop(org)


def run(s, display, env, wrappers, name, url, frames):
    scene = os.path.join(s.state, name + ".scene")
    with open(scene, "w") as f:
        f.write("\n".join(
            ["gtfocus", "gtframe 0", "camera fitted", "zoom 4", "camera zoomed"] +
            ["gtframe %d" % i for i in frames] +
            ["gtplay 1500 20", "gtstop", "camera played", "gtinview", "hold 1", ""]))
    text, _ = bo.runScript(
        s, display, env, wrappers, ["-context", url],
        ["wait 60000", "quit"], s.state, name,
        {"ECCE_OPEN_PANEL": "Geometry Trace", "ECCE_VIEWER_SCENE": scene,
         "ECCE_VIEWER_SCENE_HOLD": "1",
         "ECCE_VIEWER_SCENE_OUT": os.path.join(s.state, name)})
    for line in re.findall(r"SCENE camera .*|GTSTRESS: .*"
                           r"|ECCE_VIEWER_SCENE.*fail.*", text):
        say("%s: %s" % (name, line))
    cam = cameras(text)
    steps = len(re.findall(r"GTSTRESS: step", text))
    check(steps == len(frames) + 1, "%s: %d frames stepped (%d)" % (name, len(frames) + 1, steps))
    check(not same(cam.get("fitted"), cam.get("zoomed")), "%s: the zoom moved the camera" % name)
    check("inview outside=1" in text, "%s: the zoomed view cuts atoms off" % name)
    check(same(cam.get("zoomed"), cam.get("played")),
          "%s: stepping and Play keep the zoomed camera" % name)
    return text


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb", path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    for exe in EXES:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "tracecam", {"GROMACS": "/bin/true"},
                        local=True, keep=args.keep)
    for exe in EXES:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    gw = os.path.join(REPO, "packaging", "gateway")
    for entry in os.listdir(gw):
        if entry.startswith("ecce-"):
            harness.link(os.path.join(gw, entry), os.path.join(s.home, "bin", entry))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    gmxrun = os.path.join(s.state, "gmxrun")
    shutil.copytree(GMX, gmxrun)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        orca = eg.install(s, "methyl_bromide", ORCA)
        proj = orca.rstrip("/").rsplit("/", 1)[0] + "/"
        dyn = importGromacs(s, env, wrappers, proj, gmxrun)
        if dyn:
            text = run(s, display, env, wrappers, "gromacs", "file://" + dyn + "/",
                       [1, 5, 20, 50])
            check("atoms=1000 lattice=yes" in text,
                  "gromacs: 1000 atoms in a periodic box")
        run(s, display, env, wrappers, "orca", orca, [1, 2, 3])
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
