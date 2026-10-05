#!/usr/bin/env python3
"""ImageConverter against ImageMagick and PIL on real renderer output (#231).

    tests/look/imageconv.py BUILD/render-rgb [OUTDIR]

render-rgb runs VizRender::file on Xvfb, keeping the .rgb and the PNG/JPEG
that ImageConverter wrote.  Each .rgb is decoded independently by `convert`
and by PIL; the PNG must match both exactly, the JPEG within a tolerance.
Orientation is checked on an asymmetric molecule and blank renders fail.
Exit 77 (skip) without render-rgb, Xvfb, convert or PIL.
"""
import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "apps"))

JPEG_TOL = 2.0   # mean per-channel difference after JPEG compression


def skip(why):
    print("SKIP: " + why)
    sys.exit(77)


def convertVariants(binary, env, out):
    """SGI files written by ImageMagick (RLE, verbatim, with alpha) and a
    bare RGB file, all decoded by ImageConverter, against the source PNG."""
    from PIL import Image, ImageChops
    src = os.path.join(out, "ethanolamine.im.png")
    subprocess.run(["convert", os.path.join(out, "ethanolamine.rgb"), src],
                   check=True)
    ref = Image.open(src).convert("RGB")
    w, h = ref.size
    res = []
    jobs = [("sgi-rle", ["-compress", "RLE", "sgi:"]),
            ("sgi-verbatim", ["-compress", "None", "sgi:"]),
            ("sgi-alpha", ["-alpha", "set", "sgi:"])]
    for tag, args in jobs:
        f = os.path.join(out, tag + ".rgb")
        subprocess.run(["convert", src] + args[:-1] + [args[-1] + f], check=True)
        res.append((tag, f))
    # bare bottom-up rows, as the vendored viewer writes them
    f = os.path.join(out, "bare.rgb")
    open(f, "wb").write(ref.transpose(Image.FLIP_TOP_BOTTOM).tobytes())
    res.append(("bare", f))
    results = []
    for tag, f in res:
        png = f + ".png"
        r = subprocess.run([binary, f, png, str(w), str(h)], env=env,
                           capture_output=True, text=True, timeout=120)
        if r.returncode != 0:
            results.append((tag, r.stderr.strip())); continue
        d = ImageChops.difference(Image.open(png).convert("RGB"), ref).getbbox()
        results.append((tag, "differs from source" if d else None))
    # a size that disagrees with the header must not matter for SGI
    r = subprocess.run([binary, res[0][1], os.path.join(out, "wrongsize.png"),
                        "7", "9"], env=env, capture_output=True, text=True)
    d = ImageChops.difference(Image.open(os.path.join(out, "wrongsize.png"))
                              .convert("RGB"), ref).getbbox() if r.returncode == 0 else 1
    results.append(("sgi-header-size-wins", "failed" if d else None))
    return results


def main():
    if len(sys.argv) < 2 or not os.access(sys.argv[1], os.X_OK):
        skip("render-rgb not built (ninja render-rgb)")
    binary = os.path.abspath(sys.argv[1])
    if not shutil.which("convert"):
        skip("ImageMagick convert not installed")
    try:
        from PIL import Image, ImageChops, ImageStat
    except ImportError:
        skip("PIL not installed")
    import xdisplay
    try:
        xdisplay.findXvfb()
    except xdisplay.DisplayUnavailable as e:
        skip(str(e))

    out = sys.argv[2] if len(sys.argv) > 2 else tempfile.mkdtemp(prefix="imageconv")
    os.makedirs(out, exist_ok=True)
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1")
    env.setdefault("ECCE_HOME", os.environ.get("ECCE_HOME", "/opt/ecce"))
    state = tempfile.mkdtemp(prefix="imageconv-home")
    env["ECCE_REALUSERHOME"] = state
    os.environ.setdefault("ECCE_TEST_XDISPLAYS", "170-179")
    with xdisplay.Display() as display:
        env["DISPLAY"] = ":%d" % display.number
        r = subprocess.run([binary, out], env=env, capture_output=True,
                           text=True, timeout=240)
        if r.returncode == 0:
            variants = convertVariants(binary, env, out)
    shutil.rmtree(state, ignore_errors=True)
    if r.returncode != 0:
        print("render-rgb failed:\n" + (r.stdout + r.stderr)[-1500:])
        return 1

    bad = 0
    for tag, why in variants:
        if why:
            print("%s: FAIL %s" % (tag, why)); bad += 1
        else:
            print("%s: PASS" % tag)

    def maxdiff(a, b):
        if a.size != b.size:
            return 999
        return max(hi for _, hi in ImageChops.difference(a, b).getextrema())

    def meandiff(a, b):
        if a.size != b.size:
            return 999
        return max(ImageStat.Stat(ImageChops.difference(a, b)).mean)

    for name in ("ethanolamine", "benzene"):
        rgb = os.path.join(out, name + ".rgb")
        head = open(rgb, "rb").read(2)
        ref = os.path.join(out, name + ".im.png")
        subprocess.run(["convert", rgb, ref], check=True)
        im = Image.open(ref).convert("RGB")
        pil = Image.open(rgb).convert("RGB")
        png = Image.open(os.path.join(out, name + ".conv.png")).convert("RGB")
        jpg = Image.open(os.path.join(out, name + ".conv.jpg")).convert("RGB")
        px = png.load()
        bg = px[0, 0]
        nonbg = sum(1 for y in range(png.size[1]) for x in range(png.size[0])
                    if px[x, y] != bg)
        d = (maxdiff(png, im), maxdiff(png, pil), meandiff(jpg, im),
             meandiff(jpg, pil))
        print("%s: header=%s size=%s bg=%s nonbg=%d  png-vs-IM=%d png-vs-PIL=%d "
              "jpg-vs-IM=%.2f(mean) jpg-vs-PIL=%.2f(mean)" % ((name, head.hex(), png.size, bg,
                                              nonbg) + d))
        if d[0] or d[1]:
            print("  FAIL: PNG differs from reference"); bad += 1
        if d[2] > JPEG_TOL or d[3] > JPEG_TOL:
            print("  FAIL: JPEG differs from reference"); bad += 1
        if nonbg < 0.02 * png.size[0] * png.size[1]:
            print("  FAIL: render is blank"); bad += 1
        if name == "ethanolamine":
            # Red O is left of blue N in the scene, and the far H atom is the
            # top-most thing drawn, on the right.
            w, h = png.size
            def centroid(test):
                pts = [(x, y) for y in range(h) for x in range(w) if test(px[x, y])]
                return (sum(p[0] for p in pts) / len(pts) / w) if pts else None
            red = centroid(lambda c: c[0] > 150 and c[1] < 80 and c[2] < 80)
            blue = centroid(lambda c: c[2] > 150 and c[0] < 80)
            top = next(x for y in range(h) for x in range(w) if px[x, y] != bg)
            print("  red x=%s blue x=%s topmost pixel x=%.2f" % (red, blue, top / w))
            if red is None or blue is None:
                print("  FAIL: no red/blue atom pixels (channels scrambled?)"); bad += 1
            elif red > blue:
                print("  FAIL: O and N swapped (mirrored or channels swapped)"); bad += 1
            if top / w < 0.6:
                print("  FAIL: flipped vertically"); bad += 1
    print("FAIL" if bad else "PASS", "(images in %s)" % out)
    return 1 if bad else 0


sys.exit(main())
