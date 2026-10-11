#!/usr/bin/env python3
"""The Display Style dialog's sliders act when they are let go (#210).

    tests/apps/display_style_test.py              own Xvfb, services, local data
    tests/apps/display_style_test.py --shots DIR  also keep the images there

Opens a Builder on the water fixture and runs a scene script
(ECCE_VIEWER_SCENE, the Builder's "dsstyle" commands) that opens Custom
Style..., selects two atoms, and lets go of the Sphere Quality slider at 1,
then at 100, without ever pressing Update.  Each release must

  - change the style's definition (sphrq in "info"),
  - redraw by itself (the idle loop draws a frame, and the picture differs
    from the one before),
  - leave the selection and every atom's style as they were: the second half
    of Update, assigning the style to the selection, is not part of it.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
GOOD = os.path.join(HERE, "fixtures", "calc-orca-water-ccpvdz")
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")

SCENE = """\
viewall
zoom 3
dsstyle open
dsstyle info start
select 1 2
dsstyle info selected
snap before
dsstyle slider sphrq 1
idleframes idle-low
dsstyle info low
snap low
dsstyle slider sphrq 100
idleframes idle-high
dsstyle info high
snap high
dsstyle slider cylrq 1
dsstyle info last
"""

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-12:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def ppm(path):
    with open(path, "rb") as handle:
        data = handle.read()
    m = re.match(rb"P6\s+(\d+)\s+(\d+)\s+255\s", data)
    return data[m.end():] if m else b""


def different(a, b):
    """Pixels that differ between two equally sized images."""
    if not a or len(a) != len(b):
        return -1
    return sum(1 for i in range(0, len(a), 3) if a[i:i + 3] != b[i:i + 3])


def info(out, name):
    path = os.path.join(out, name + ".txt")
    if not os.path.exists(path):
        return None
    values = {}
    for line in open(path):
        key, _, rest = line.strip().partition(" ")
        values[key] = rest if key != "atoms" else (
            values.get("atoms", "") + rest + ";")
    return values


def run(display, data, work, keep):
    project = os.path.join(data, "users", "local", "style-test")
    os.makedirs(project, exist_ok=True)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, "water")
    shutil.copytree(GOOD, calc)
    out = os.path.join(work, "out")
    os.makedirs(out)
    script = os.path.join(out, "scene")
    with open(script, "w") as handle:
        handle.write(SCENE)
    result = apps.run(display, "builder", args=("-context", "file://%s/" % calc),
                      windowTimeout=120, settle=90,
                      env={"ECCE_VIEWER_SCENE": script,
                           "ECCE_VIEWER_SCENE_OUT": out,
                           "ECCE_TRANSPARENCY_FALLBACK_MS": "0"})
    log = result.log or ""
    what = "Sphere Quality released without Update"
    if result.crashed:
        return fail(what, "builder CRASHED (%s)" % result.signalName, log)
    failed = os.path.join(out, "FAILED")
    if os.path.exists(failed):
        return fail(what, "the script stopped: " + open(failed).read(), log)
    s0, s1 = info(out, "start"), info(out, "selected")
    lo, hi, last = info(out, "low"), info(out, "high"), info(out, "last")
    if not (s0 and s1 and lo and hi and last):
        return fail(what, "the scene never reported", log)
    problems = []
    if s0["sphrq"] != "50":
        problems.append("default sphrq %s, expected 50" % s0["sphrq"])
    if lo["sphrq"] != "1" or hi["sphrq"] != "100":
        problems.append("sphrq after release: %s, %s (want 1, 100)"
                        % (lo["sphrq"], hi["sphrq"]))
    if last["cylrq"] != "1":
        problems.append("cylrq after release %s, want 1" % last["cylrq"])
    for name, got in (("low", lo), ("high", hi), ("last", last)):
        if got["selected"] != "2":
            problems.append("%s: %s atoms selected, expected the 2 chosen"
                            % (name, got["selected"]))
        if got["atoms"] != s0["atoms"]:
            problems.append("%s: atoms' styles %s, were %s"
                            % (name, got["atoms"], s0["atoms"]))
    images = {n: ppm(os.path.join(out, n + ".ppm"))
              for n in ("before", "low", "high", "idle-low", "idle-high")
              if os.path.exists(os.path.join(out, n + ".ppm"))}
    for a, b in (("before", "low"), ("low", "high")):
        n = different(images.get(a, b""), images.get(b, b""))
        if n <= 0:
            problems.append("%s -> %s: picture unchanged (%d pixels)"
                            % (a, b, n))
    #  What the idle loop alone drew is what a forced paint shows.
    for idle, forced in (("idle-low", "low"), ("idle-high", "high")):
        count = open(os.path.join(out, idle + ".txt")).read() \
            if os.path.exists(os.path.join(out, idle + ".txt")) else ""
        m = re.search(r"idle frames (\d+)", count)
        if not m or int(m.group(1)) < 1:
            problems.append("%s: no frame drawn by itself after the release"
                            % forced)
        elif different(images.get(idle, b""), images.get(forced, b"")) != 0:
            problems.append("%s: the self-drawn frame differs from a forced "
                            "paint" % forced)
    if keep:
        os.makedirs(keep, exist_ok=True)
        for n in ("before", "low", "high"):
            src = os.path.join(out, n + ".ppm")
            if os.path.exists(src):
                subprocess.run(["convert", src,
                                os.path.join(keep, "style-%s.png" % n)])
    if problems:
        fail(what, "; ".join(problems), log)
    else:
        print("ok    %s: style changed, redrawn, selection and atom styles "
              "kept" % what)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shots", help="keep the pictures here")
    options = parser.parse_args()
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="style-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work, options.shots)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
