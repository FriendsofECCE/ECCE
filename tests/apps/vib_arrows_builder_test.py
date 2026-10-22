#!/usr/bin/env python3
"""The Vector view of a normal mode shows the arrowheads that end inside a
neighbouring atom (#430), headless, with the tree's own binaries.

    tests/apps/vib_arrows_builder_test.py [--build build-cmake] [--png DIR]

Ammonia's symmetric N-H stretch: each hydrogen's arrow points along its bond
into the nitrogen, so the head lies inside the nitrogen's ball.  The test
counts arrow-tinted pixels inside the nitrogen ball, where nothing else
is tinted: only the see-through pass puts the heads there.  No synthetic
input: the Builder is driven by its own script hooks.  Exit status 77 (skip)
without Xvfb, import, python3-pil/numpy or the built binaries.
"""
import argparse
import math
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

SOURCE = os.path.join(REPO, "tests", "ecceqm", "fixtures", "h2o-vibration")
check = bo.check
NB = 1.012
ANG = math.radians(106.7)


def geometry():
    """N on the axis above three hydrogens: [(symbol, x, y, z)]."""
    cosb = math.cos(ANG)
    # cos(HNH) = 1 - 1.5 sin^2(a), a the angle of N-H to the axis.
    sin_a = math.sqrt((1 - cosb) / 1.5)
    cos_a = math.sqrt(1 - sin_a ** 2)
    atoms = [("N", 0.0, 0.0, 0.0)]
    for k in range(3):
        t = 2 * math.pi * k / 3
        atoms.append(("H", NB * sin_a * math.cos(t), NB * sin_a * math.sin(t),
                      -NB * cos_a))
    return atoms


def mode(atoms):
    """Symmetric stretch.  The panel draws the negative of the mode by
    default, so this is every H away from N in the file and toward N on
    screen; N stays."""
    out = [0.0, 0.0, 0.0]
    for _, x, y, z in atoms[1:]:
        r = math.sqrt(x * x + y * y + z * z)
        out += [0.5 * x / r, 0.5 * y / r, 0.5 * z / r]
    return out


def makeCalc(dest):
    shutil.rmtree(dest, ignore_errors=True)
    shutil.copytree(SOURCE, dest, symlinks=True)
    props = os.path.join(dest, "Props")
    for name in ("GEOMTRACE", "VIBIR", "VIBRAM", "MOAOORDER", "MO"):
        for p in (os.path.join(props, name), os.path.join(props, ".DAV", name)):
            if os.path.exists(p):
                os.remove(p)
    atoms = geometry()
    vals = "\n".join("%.8f" % v for v in mode(atoms))
    with open(os.path.join(props, "VIB"), "w") as h:
        h.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                '<vectable columnLabel="Atom" columnLabels="X Y Z" columns="3" '
                'name="VIB" rowLabel="Normal Mode" rowLabels="1" rows="4" '
                'units="NA" vectorLabel="Coordinate" vectorLabels="%s" '
                'vectors="12">\n\n  <step number="0"/>\n\n'
                '  <matrix number="1">%s</matrix>\n\n</vectable>\n'
                % (" ".join("NA" for _ in range(12)), vals))
    with open(os.path.join(props, "VIBFREQ"), "w") as h:
        h.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                '<vector name="VIBFREQ" rowLabel="Normal Mode" rowLabels="1" '
                'rows="1" units="cm-1">3400.0</vector>\n')
    with open(os.path.join(dest, "Parameters", "chemsys.mvm"), "w") as h:
        h.write("title: nh3\ntype: molecule\nnum_atoms: 4\n"
                "atom_info: symbol cart\natom_list: \n%s"
                "attr_list:\n%s"
                "atom_type_list:\n%s"
                "num_bonds: 3\nbond_list: \n1 2 1.00000 1\n1 3 1.00000 1\n"
                "1 4 1.00000 1\n"
                % ("".join("%s %.6f %.6f %.6f\n" % a for a in atoms),
                   " 0.000000   1   0   0   0\n" * 4,
                   "        7   1\n        1   1\n        1   1\n        1   1\n"))


def renderScene(s, env, wrappers, url, tag, style, outdir):
    """Builder scene script (ECCE_VIEWER_SCENE): the style, the mode's
    vectors, one offscreen picture; the Builder exits afterwards."""
    script = os.path.join(outdir, tag + ".scene")
    with open(script, "w") as h:
        h.write("\n".join(["style " + style, "vibfocus", "vibmode 0",
                           "vibvector", "offscreen " + tag + " 500", ""]))
    log = os.path.join(outdir, tag + ".log")
    benv = dict(env, ECCE_VIEWER_SCENE=script, ECCE_VIEWER_SCENE_OUT=outdir)
    with open(log, "w") as out:
        try:
            subprocess.run([os.path.join(wrappers, "ecce-builder"),
                            "-context", url], env=benv, stdout=out,
                           stderr=subprocess.STDOUT, timeout=180,
                           cwd=os.path.join(s.home, "bin"),
                           start_new_session=True)
        except subprocess.TimeoutExpired:
            pass
    path = os.path.join(outdir, tag + ".ppm")
    return path if os.path.exists(path) else None, log


def viewerPixels(path):
    from PIL import Image
    import numpy
    return numpy.asarray(Image.open(path).convert("RGB")).astype(int)


def analyse(px):
    """(nitrogen disc pixels, arrow-tinted pixels inside the disc).

    The disc is fitted to the nitrogen's blue pixels (the arrows only
    remove a few of them).  Nitrogen does not move in the mode, so inside
    its disc only the hydrogens' arrowheads can show: blended 40 % over the
    blue they are neither saturated blue nor white."""
    import numpy
    r, g, b = px[..., 0], px[..., 1], px[..., 2]
    blue = (b > 100) & (b > r + 60) & (b > g + 30)
    ys, xs = numpy.nonzero(blue)
    cy, cx = ys.mean(), xs.mean()
    rad = math.sqrt(blue.sum() / math.pi)
    yy, xx = numpy.indices(blue.shape)
    disc = (yy - cy) ** 2 + (xx - cx) ** 2 < (0.95 * rad) ** 2
    tint = (r > 90) & (g > 100) & (b < 190) & (b < g + 40) & (r > 0.6 * b)
    return int(disc.sum()), int((tint & disc).sum())


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--min-changed", type=int, default=150)
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    try:
        import numpy  # noqa: F401
        from PIL import Image  # noqa: F401
    except ImportError:
        harness.skip("python3-pil and python3-numpy are needed")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "vibarrows", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binaries: %s" % os.path.realpath(os.path.join(s.home, "bin", "builder")))

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        data = s.localData()
        project = os.path.join(data, "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "nh3-vibration")
        makeCalc(calc)
        for style in ("Ball And Stick", "CPK"):
            tag = style.split()[0].lower()
            path, log = renderScene(s, env, wrappers, "file://" + calc + "/",
                                    "nh3-" + tag, style, png)
            if not check(path is not None, "%s: rendered" % style):
                say(open(log, errors="replace").read()[-1500:])
                continue
            disc, tinted = analyse(viewerPixels(path))
            say("%s: N disc %d px, arrow-tinted pixels in it %d"
                % (style, disc, tinted))
            check(disc > 500, "%s: the nitrogen ball is found" % style)
            check(tinted >= args.min_changed,
                  "%s: arrowheads show through the nitrogen ball (%d px)"
                  % (style, tinted))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
