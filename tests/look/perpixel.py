#!/usr/bin/env python3
"""Per-pixel lighting of atoms and bonds keeps everything but the shading (#210).

    tests/look/perpixel.py BUILD/viewer-scenes [OUTDIR]

Renders benzene in every display style, with a selected atom, labels,
per-atom and overall colour schemes, the thumbnail / Save As render
(VizRender::file), an offscreen (thumbnail) render and a synthetic MO under Coin's sorted-layers
transparency, once with per-pixel lighting and once with ECCE_GL_PERPIXEL=0
(fixed-function, as before).  Both the canvas and the offscreen context must
report the program in use; the covered pixels must be the same in every
picture (geometry, selection and picking unchanged), wireframe byte for
byte; the shading may move but not the overall brightness; and the lobes
must still be blended over the atoms, which needs the program to peel the
layers as Coin's own fragment program does.  Exit 77 without the binary,
Xvfb or PIL, in the vendored build, or where GLSL is unavailable.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

SCENE = """style Ball And Stick
viewall
rotate 25
zoom 2
snap bs
perpixel canvas
offscreen off 400
perpixel offscreen
vizfile viz 400
perpixel vizfile
style CPK
snap cpk
style Stick
snap stick
style Wireframe
snap wire
style Ball And Stick
select 1
snap sel
deselect
labels element
snap labels
labels none
stylecs Position from CM/Ball And Stick
snap cs
stylecs Block Color/Ball And Stick
snap block
style Ball And Stick
zoom 0.5
isotest
transparency SORTED_LAYERS_BLEND
snap peel
"""
SHOTS = ("bs", "off", "viz", "cpk", "stick", "wire", "sel", "labels", "cs", "block", "peel")
LUM_TOL = 4.0        # mean of each channel over covered pixels, 0..255
MASK_TOL = 0.002     # fraction of covered pixels that may differ in coverage
PEEL_TOL = 0.005     # fraction of all pixels differing by more than 40


def skip(why):
    print("SKIP: " + why)
    sys.exit(77)


PROBE = """style Ball And Stick
viewall
zoom 2
snap probe
perpixel canvas
offscreen probeoff 400
perpixel offscreen
"""


def run(binary, out, scene, extra, display):
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", ECCE_HOME=ROOT, **extra)
    env.pop("FL_FONT_PATH", None)
    env["ECCE_REALUSERHOME"] = os.path.join(out, "home")
    env["DISPLAY"] = ":%d" % display.number
    os.makedirs(env["ECCE_REALUSERHOME"], exist_ok=True)
    r = subprocess.run([binary, out, scene, "benzene"], env=env,
                       capture_output=True, text=True, timeout=300)
    if r.returncode != 0:
        print("viewer-scenes failed:\n" + (r.stdout + r.stderr)[-1500:])
        sys.exit(1)
    status = {k: open(os.path.join(out, k + ".txt")).read().strip()
              for k in ("canvas", "offscreen", "vizfile")
              if os.path.exists(os.path.join(out, k + ".txt"))}
    return status


def compare(a, b, name):
    from PIL import Image
    ext = ".png" if name == "viz" else ".ppm"
    A = Image.open(os.path.join(a, name + ext)).convert("RGB")
    B = Image.open(os.path.join(b, name + ext)).convert("RGB")
    bg = A.getpixel((0, 0))
    pa, pb = list(A.getdata()), list(B.getdata())
    ma = [p != bg for p in pa]
    mb = [p != bg for p in pb]
    covered = max(sum(ma), 1)
    maskdiff = sum(x != y for x, y in zip(ma, mb)) / covered

    def mean(px, mask, k):
        v = [p[k] for p, m in zip(px, mask) if m]
        return sum(v) / max(len(v), 1)
    #  Largest shift of a channel's mean over the covered pixels: keeps
    #  element and property colours, not just the overall brightness.
    dlum = max((mean(pb, mb, k) - mean(pa, ma, k) for k in range(3)), key=abs)
    big = sum(max(abs(p[0] - q[0]), abs(p[1] - q[1]), abs(p[2] - q[2])) > 40
              for p, q in zip(pa, pb)) / len(pa)
    changed = sum(p != q for p, q in zip(pa, pb)) / len(pa)
    return maskdiff, dlum, big, changed


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

    keep = len(sys.argv) > 2
    top = sys.argv[2] if keep else tempfile.mkdtemp(prefix="perpixel")
    try:
        on, off = os.path.join(top, "on"), os.path.join(top, "off")
        for d in (on, off):
            os.makedirs(d, exist_ok=True)
        scene = os.path.join(top, "perpixel.scene")
        open(scene, "w").write(SCENE)
        binary = os.path.abspath(sys.argv[1])
        os.environ.setdefault("ECCE_TEST_XDISPLAYS", "170-179")
        probe = os.path.join(top, "probe")
        os.makedirs(probe, exist_ok=True)
        open(os.path.join(top, "probe.scene"), "w").write(PROBE)
        with xdisplay.Display() as display:
            s_probe = run(binary, probe, os.path.join(top, "probe.scene"), {}, display)
            if s_probe["canvas"] == "off":
                skip("no per-pixel lighting in this build (vendored Inventor)")
            if s_probe["canvas"] == "unavailable":
                skip("GLSL unavailable on this display")
            s_on = run(binary, on, scene, {}, display)
            s_off = run(binary, off, scene, {"ECCE_GL_PERPIXEL": "0"}, display)
        print("status on:", s_on, "off:", s_off)
        ok = set(s_on.values()) == {"on"} and set(s_off.values()) == {"off"} \
            and len(s_on) == 3
        for name in SHOTS:
            maskdiff, dlum, big, changed = compare(off, on, name)
            line = "%-7s coverage %.4f  colour %+.2f  >40 %.4f  changed %.4f" % (
                name, maskdiff, dlum, big, changed)
            bad = maskdiff > MASK_TOL or abs(dlum) > LUM_TOL
            if name == "wire" and changed != 0:
                bad = True       # lines are not lit by the program
            if name == "bs" and changed < 0.001:
                bad = True       # the program must actually shade the atoms
            if name == "peel" and big > PEEL_TOL:
                bad = True       # atoms drawn over the lobes: peeling lost
            print(line + ("  FAIL" if bad else ""))
            ok = ok and not bad
        print("PASS" if ok else "FAIL")
        return 0 if ok else 1
    finally:
        if not keep:
            shutil.rmtree(top, ignore_errors=True)


sys.exit(main())
