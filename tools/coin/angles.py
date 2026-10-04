#!/usr/bin/env python3
"""Isosurface lobes from several camera angles in each transparency mode,
vendored vs Coin build (#166), to tell a draw-order tie from a real
disagreement.

    tools/coin/angles.py [outdir]     (default build-coin-compare/angles)

Renders scenes/isoangles.scene on water, benzene and crco6 with both builds
(viewer-scenes on private Xvfb :170-:179), then writes per-image metrics
(compare.py's, plus pixel counts and mean colour of red- and green-dominant
lobe pixels) to metrics.txt and a contact sheet, angles-sheet.png.
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
import compare, isoref

OUT = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                      else os.path.join(ROOT, "build-coin-compare", "angles"))
BUILDS = {"vendored": os.path.join(ROOT, "build-cmake"),
          "coin": os.path.join(ROOT, "build-coin")}
SYS = ["benzene", "crco6", "water"]
MODES = [("sb", "SORTED_OBJECT_BLEND"), ("sd", "SCREEN_DOOR"), ("da", "DELAYED_ADD"),
         ("sl", "SORTED_LAYERS_BLEND (coin; vendored = SORTED_OBJECT_BLEND)")]
ANGLES = ["000", "045", "090"]


def render():
    import subprocess, xdisplay
    os.environ["LIBGL_ALWAYS_SOFTWARE"] = "1"
    os.environ.setdefault("ECCE_TEST_XDISPLAYS", "170-179")
    for name, b in BUILDS.items():
        d = os.path.join(OUT, "raw", name)
        os.makedirs(d, exist_ok=True)
        env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", ECCE_HOME=ROOT,
                   FL_FONT_PATH=os.path.join(ROOT, "data", "client", "fonts") + "/",
                   ECCE_REALUSERHOME=os.path.join(d, "state"))
        os.makedirs(env["ECCE_REALUSERHOME"], exist_ok=True)
        with xdisplay.Display() as disp:
            env["DISPLAY"] = ":%d" % disp.number
            for s in SYS:
                script = os.path.join(d, s + ".scene")
                text = open(os.path.join(HERE, "scenes", "isoangles.scene")).read()
                open(script, "w").write(text.replace("@SYS@", s))
                r = subprocess.run([os.path.join(b, "viewer-scenes"), d, script, s],
                                   env=env, capture_output=True, text=True, timeout=300)
                if r.returncode != 0:
                    sys.exit("viewer-scenes %s %s failed: %s" % (name, s, (r.stdout + r.stderr)[-500:]))


def lobes(img):
    """Red- and green-dominant pixels (lobe colours blended with the
    background or stippled): counts and mean rgb of each."""
    import numpy as np
    i = img.astype(int)
    r, g, b = i[..., 0], i[..., 1], i[..., 2]
    red = (r - g > 40) & (r - b > 40)
    grn = (g - r > 20) & (g - b > 20)
    out = []
    for m in (red, grn):
        n = int(m.sum())
        out.append((n, [round(float(i[..., k][m].mean()), 1) for k in range(3)] if n else None))
    return out


def main():
    import numpy as np
    from PIL import Image, ImageDraw
    if "--nrender" not in sys.argv:
        render()
    lines, rows = [], []
    hdr = "%-26s %6s %6s %5s | %-5s %6s %-16s | %-5s %6s %-16s" % (
        "image", "corpx", "cormax", "xor", "build", "red_n", "red_meanRGB", "", "grn_n", "grn_meanRGB")
    lines.append(hdr)
    for s in SYS:
        for mk, mname in MODES:
            for a in ANGLES:
                n = "%s-%s-a%s" % (s, mk, a)
                v = compare.readPpm(os.path.join(OUT, "raw", "vendored", n + ".ppm"))
                c = compare.readPpm(os.path.join(OUT, "raw", "coin", n + ".ppm"))
                m, vc, cor, xor = compare.compare(v, c, 16)
                lv, lc = lobes(v), lobes(c)
                for nm, l in (("vend", lv), ("coin", lc)):
                    f = lambda t: "%6d %-16s" % (t[0], t[1])
                    lines.append("%-26s %6d %6d %5d | %-5s %s | %s" % (
                        n if nm == "vend" else "", m["corrected_diff_px"] if nm == "vend" else 0,
                        m["corrected_max_channel_diff"] if nm == "vend" else 0,
                        m["mask_xor_px"] if nm == "vend" else 0, nm, f(l[0]), f(l[1])))
                refimg = None
                if mk in ("sb", "sl", "da"):    # blended modes only; atoms are not in the reference
                    refimg = isoref.render(os.path.join(OUT, "raw", "coin", "%s-ref-a%s-iso.txt" % (s, a)),
                                           c[0, 0])[0]
                g = np.clip(cor.max(axis=2) * 4, 0, 255).astype(np.uint8)
                d = np.stack([g, g, g], axis=2)
                d[xor] = (255, 0, 0)
                rows.append((n, mname, a, m, lv, lc, v, c, d, refimg))
    open(os.path.join(OUT, "metrics.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))
    cell, lab = 200, 340
    sheet = Image.new("RGB", (lab + 4 * cell, len(rows) * (cell + 2)), (30, 30, 30))
    dr = ImageDraw.Draw(sheet)
    for i, (n, mname, a, m, lv, lc, v, c, d, refimg) in enumerate(rows):
        y = i * (cell + 2)
        for j, arr in enumerate((v, c, d)):
            sheet.paste(Image.fromarray(arr).resize((cell, cell), Image.NEAREST), (lab + j * cell, y))
        if refimg is not None:
            sheet.paste(Image.fromarray(refimg).resize((cell, cell), Image.NEAREST), (lab + 3 * cell, y))
        t = [n, "%s, camera %d deg" % (mname, int(a)),
             "vendored | coin | diff | reference (lobes only)", "corrected diff px %d, xor %d" % (m["corrected_diff_px"], m["mask_xor_px"]),
             "vend red %d %s" % (lv[0][0], lv[0][1]), "vend grn %d %s" % (lv[1][0], lv[1][1]),
             "coin red %d %s" % (lc[0][0], lc[0][1]), "coin grn %d %s" % (lc[1][0], lc[1][1])]
        for k, x in enumerate(t):
            dr.text((8, y + 8 + 16 * k), x, fill=(255, 255, 255))
    p = os.path.join(OUT, "angles-sheet.png")
    sheet.save(p)
    print("sheet:", p)


main()
