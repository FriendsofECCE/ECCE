#!/usr/bin/env python3
"""Transparent atoms: the Display Style 'Atom opacity' setting (#879).

    tests/apps/atom_opacity_test.py              own Xvfb, services, local data
    tests/apps/atom_opacity_test.py --shots DIR  also keep the pictures there

Two atoms lie on the view axis, N behind O and then O behind N (an .xyz file
each), seen end on, so the back sphere sits exactly behind the front one:

  - 100 % draws the picture of no opacity setting at all;
  - at 30 % the centre pixel is the far-before-near blend
    0.3 front + 0.21 back + 0.49 background, with the opaque colours taken
    from the other file's 100 % picture; it is told apart from the near-first
    blend (weights swapped) and from a front sphere over nothing;
  - "opacity 30 selection" on the front atom (atom 2 in both files) gives
    0.3 front + 0.7 back: the other atom stays opaque;
  - the setting is still there after a redraw (viewall) and in the PNG export.

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
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77

SCENE = """\
background 1 1 1
style Ball And Stick
viewall
snap base
opacity 100
snap full
vizfile export100 480
opacity 30
snap alpha
dsstyle info alpha
vizfile export30 480
viewall
snap redraw
opacity 100
select 2
opacity 30 selection
deselect
snap sel
"""

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-int(os.environ.get("OPACITY_LOG_LINES", "12")):])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def ppm(path):
    with open(path, "rb") as handle:
        data = handle.read()
    m = re.match(rb"P6\s+(\d+)\s+(\d+)\s+255\s", data)
    if not m:
        raise AssertionError("not a PPM: " + path)
    return int(m.group(1)), int(m.group(2)), data[m.end():]


def centre(image):
    w, h, data = image
    i = ((h // 2) * w + w // 2) * 3
    return data[i], data[i + 1], data[i + 2]


def run(display, name, xyz, work, keep):
    out = os.path.join(work, name)
    os.makedirs(out)
    script = os.path.join(out, "scene")
    with open(script, "w") as handle:
        handle.write(SCENE)
    result = apps.run(display, "builder", args=("-context", "file://" + xyz),
                      windowTimeout=120, settle=90,
                      env={"ECCE_VIEWER_SCENE": script,
                           "ECCE_VIEWER_SCENE_OUT": out,
                           "ECCE_TRANSPARENCY_FALLBACK_MS": "0",
                           "ECCE_TEST_XYZ_UNITS": "angstrom",
                           "LP_NUM_THREADS": "2", "LIBGL_ALWAYS_SOFTWARE": "1"})
    log = result.log or ""
    what = "Atom opacity (%s)" % name
    if result.crashed:
        return fail(what, "builder CRASHED (%s)" % result.signalName, log)
    failed = os.path.join(out, "FAILED")
    if os.path.exists(failed):
        return fail(what, "the script stopped: " + open(failed).read(), log)
    shots = {}
    for n in ("base", "full", "alpha", "redraw", "sel"):
        path = os.path.join(out, n + ".ppm")
        if not os.path.exists(path):
            return fail(what, "no picture " + n, log)
        shots[n] = ppm(path)
    for n in ("export100", "export30"):
        if not os.path.exists(os.path.join(out, n + ".png")):
            return fail(what, "no export " + n, log)
    if keep:
        os.makedirs(keep, exist_ok=True)
        for n in list(shots) + []:
            subprocess.run(["convert", os.path.join(out, n + ".ppm"),
                            os.path.join(keep, "opacity-%s-%s.png" % (name, n))])
        for n in ("export100", "export30"):
            shutil.copy(os.path.join(out, n + ".png"),
                        os.path.join(keep, "opacity-%s-%s.png" % (name, n)))
    return what, out, shots, log


def check(what, out, shots, log):
    problems = []
    c100, c30 = centre(shots["full"]), centre(shots["alpha"])
    print("      %s centre pixel: opaque %s, 30%% %s, selection %s" %
          (what, c100, c30, centre(shots["sel"])))
    if shots["full"][2] != shots["base"][2]:
        problems.append("100% differs from the picture without the setting")
    if shots["alpha"][2] == shots["full"][2]:
        problems.append("30% draws the opaque picture")
    if shots["redraw"][2] != shots["alpha"][2]:
        problems.append("the picture changed after a redraw")
    sign = 1 if c100[0] > c100[2] else -1        # red on top, or blue on top
    info = open(os.path.join(out, "alpha.txt")).read()
    if "atom_opacity 30" not in info:
        problems.append("the style still reports another opacity: " + info[:60])
    if shots["sel"][2] in (shots["full"][2], shots["alpha"][2]):
        problems.append("opacity on the selection only is not a picture of its own")
    from PIL import Image, ImageChops
    a = Image.open(os.path.join(out, "export100.png")).convert("RGB")
    b = Image.open(os.path.join(out, "export30.png")).convert("RGB")
    if ImageChops.difference(a, b).getbbox() is None:
        problems.append("the PNG export does not show the opacity")
    if problems:
        fail(what, "; ".join(problems), log)
    else:
        print("ok    %s: 100%% unchanged, 30%% shows through in the right order" % what)
    return sign, c100, c30, centre(shots["sel"])


def blend(*terms):
    return tuple(sum(w * c[i] for w, c in terms) for i in range(3))


def nearest(what, got, candidates):
    """The candidate blend closest to `got`; a failure unless it is `want`."""
    dist = {k: max(abs(g - e) for g, e in zip(got, v)) for k, v in candidates.items()}
    return min(dist, key=dist.get), dist


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shots", help="keep the pictures here")
    parser.add_argument("--gdb", action="store_true",
                        help="run the builder under gdb; its backtrace lands in the log")
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
    if options.gdb:
        from geomtrace_stress import _wrapBuilder
        _wrapBuilder(settings["ECCE_HOME"],
                     "gdb -q -batch -ex run -ex bt -ex quit --args")
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="opacity-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    files = {}
    for name, atoms in (("front-o", "N 0 0 -1.2\nO 0 0 1.2\n"),
                        ("front-n", "O 0 0 -1.2\nN 0 0 1.2\n")):
        files[name] = os.path.join(work, name + ".xyz")
        with open(files[name], "w") as handle:
            handle.write("2\nopacity test\n" + atoms)
    xdisplay.SCREEN = "1366x768x24"
    try:
        with xdisplay.Display() as display:
            gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
            subprocess.run([gateway], env=display.env(), timeout=180)
            try:
                got = {}
                for name in ("front-o", "front-n"):
                    shot = run(display, name, files[name], work, options.shots)
                    if shot:
                        got[name] = check(*shot)
                signs = [g[0] for g in got.values()]
                # The back atom of one file is the front atom of the other.
                white = (255, 255, 255)
                for name, other in (("front-o", "front-n"), ("front-n", "front-o")):
                    if name not in got or other not in got:
                        continue
                    front, back = got[name][1], got[other][1]
                    for label, seen, want, cands in (
                            ("30 %", got[name][2], "far-first", {
                                "far-first": blend((.3, front), (.21, back), (.49, white)),
                                "near-first": blend((.21, front), (.3, back), (.49, white)),
                                "back hidden": blend((.3, front), (.7, white))}),
                            ("selection", got[name][3], "front only", {
                                "front only": blend((.3, front), (.7, back)),
                                "opaque": front,
                                "all 30 %": blend((.3, front), (.21, back), (.49, white))})):
                        best, dist = nearest(name, seen, cands)
                        if best != want or dist[want] > 12:
                            fail("Atom opacity (%s)" % name, "%s centre %s is nearest the "
                                 "%s blend %s (off by %s)" % (label, seen, best,
                                 tuple(round(v) for v in cands[best]), dist))
                        else:
                            print("ok    Atom opacity (%s): %s centre %s = %s blend"
                                  % (name, label, seen, want))
                if len(signs) == 2 and signs[0] == signs[1]:
                    fail("Atom opacity", "swapping the atoms did not swap the "
                         "colour on top at 100%")
            finally:
                stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
                subprocess.run([stop], env=display.env(), timeout=120)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
