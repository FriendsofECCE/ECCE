#!/usr/bin/env python3
"""Thumbnail / Save As renders shade a potential-mapped surface like the canvas.

    tests/look/offscreen_shading.py BUILD/viewer-scenes [OUTDIR]

Draws a synthetic ESP surface over ball-and-stick benzene (scene-script
`esptest`), paints the canvas, then renders through VizRender::file
(`vizfile`, the thumbnail and Save As path) and the scene script's own
offscreen renderer, each the first render in a new offscreen context.
The surface's luminance mean and spread must match the canvas's, and its
red and blue must be there.  Exit 77 (skip) without the binary, Xvfb or PIL.
"""
import os
import statistics
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

SCENE = """style Ball And Stick
esptest
viewall
rotate 50
snap canvas
vizfile vizfile 400
offscreen offscreen 400
"""
# An unlit surface measured mean +45, sd -8 against the canvas.
MEAN_TOL = 8.0
SD_TOL = 5.0
MIN_RED = 0.05     # fraction of surface pixels that are clearly red
MIN_BLUE = 0.05


def skip(why):
    print("SKIP: " + why)
    sys.exit(77)


def measure(path):
    from PIL import Image
    im = Image.open(path).convert("RGB")
    px = list(im.getdata())
    bg = px[0]
    lum, red, blue = [], 0, 0
    for r, g, b in px:
        if abs(r - bg[0]) + abs(g - bg[1]) + abs(b - bg[2]) < 30:
            continue
        lum.append(0.299 * r + 0.587 * g + 0.114 * b)
        if r > 120 and r - max(g, b) > 50:
            red += 1
        elif b > 120 and b - max(r, g) > 25:
            blue += 1
    n = max(len(lum), 1)
    mean = statistics.mean(lum) if lum else 0.0
    sd = statistics.pstdev(lum) if len(lum) > 1 else 0.0
    return len(lum), mean, sd, red / n, blue / n


def main():
    if len(sys.argv) < 2 or not os.access(sys.argv[1], os.X_OK):
        skip("viewer-scenes not built (ninja viewer-scenes)")
    try:
        import PIL  # noqa: F401
    except ImportError:
        skip("PIL not installed")
    import xdisplay
    try:
        xdisplay.findXvfb()
    except xdisplay.DisplayUnavailable as e:
        skip(str(e))

    out = sys.argv[2] if len(sys.argv) > 2 else tempfile.mkdtemp(prefix="offshade")
    os.makedirs(out, exist_ok=True)
    scene = os.path.join(out, "esp.scene")
    open(scene, "w").write(SCENE)
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", ECCE_HOME=ROOT)
    env.pop("FL_FONT_PATH", None)
    env["ECCE_REALUSERHOME"] = tempfile.mkdtemp(prefix="offshade-home")
    os.environ.setdefault("ECCE_TEST_XDISPLAYS", "170-179")
    with xdisplay.Display() as display:
        env["DISPLAY"] = ":%d" % display.number
        r = subprocess.run([os.path.abspath(sys.argv[1]), out, scene, "benzene"],
                           env=env, capture_output=True, text=True, timeout=300)
    if r.returncode != 0:
        print("viewer-scenes failed:\n" + (r.stdout + r.stderr)[-1500:])
        return 1

    ref = measure(os.path.join(out, "canvas.ppm"))
    print("canvas    px %6d mean %6.1f sd %5.1f red %.2f blue %.2f" % ref)
    bad = 0
    for name in ("vizfile.png", "offscreen.ppm"):
        m = measure(os.path.join(out, name))
        print("%-9s px %6d mean %6.1f sd %5.1f red %.2f blue %.2f"
              % ((name.split(".")[0],) + m))
        if abs(m[1] - ref[1]) > MEAN_TOL or abs(m[2] - ref[2]) > SD_TOL:
            print("  FAIL: shading differs from the canvas"); bad += 1
        if m[3] < MIN_RED or m[4] < MIN_BLUE:
            print("  FAIL: surface colours missing"); bad += 1
    print("FAIL" if bad else "PASS", "(images in %s)" % out)
    return 1 if bad else 0


sys.exit(main())
