#!/usr/bin/env python3
"""Render a fixed scene set with one viewer build, or compare two renders.

    compare.py render <name> <build-dir> <install-home> <wrappers> <raw-dir>
    compare.py diff <raw-dir> <out-dir>

Driven by tools/coin/compare.sh.  `render` produces <raw-dir>/<name>/*.ppm:
the built-in systems through viewer-scenes (the Builder's scene code in a
bare SGViewer) and the two fixture calculations through the real Builder
(ECCE_VIEWER_SCENE), all on a private Xvfb and with no input events.
`diff` writes, per scene, vendored.png / coin.png / diff.png plus one
contact sheet of the pairs that differ beyond tolerance.
"""
import argparse
import json
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SCENES = os.path.join(HERE, "scenes")
SYSTEMS = ["water", "benzene", "crco6"]
# name, fixture directory, scene script
CALCS = [
    ("calc-water-vib", os.path.join(ROOT, "tests", "apps", "fixtures",
                                   "calc-water-vib"), "calc-water.scene"),
    ("orca-crco6", os.path.join(ROOT, "tests", "modiagram", "fixtures",
                                "orca-crco6"), "calc-crco6.scene"),
]


# ---------------------------------------------------------------- render

def renderStandalone(binary, outdir, display):
    env = dict(os.environ, DISPLAY=display, LIBGL_ALWAYS_SOFTWARE="1",
               ECCE_HOME=ROOT,
               FL_FONT_PATH=os.path.join(ROOT, "data", "client", "fonts") + "/")
    state = os.path.join(outdir, "state")
    os.makedirs(state, exist_ok=True)
    env["ECCE_REALUSERHOME"] = state
    for sysname in SYSTEMS:
        script = os.path.join(outdir, sysname + ".scene")
        text = open(os.path.join(SCENES, "styles.scene")).read()
        open(script, "w").write(text.replace("@SYS@", sysname))
        r = subprocess.run([binary, outdir, script, sysname], env=env,
                           capture_output=True, text=True, timeout=300)
        if r.returncode != 0:
            print("viewer-scenes %s failed: %s" % (sysname,
                  (r.stdout + r.stderr)[-800:]))
            open(os.path.join(outdir, sysname + ".FAILED"), "w").write(
                r.stdout + r.stderr)


def renderCalc(name, source, scene, outdir, timeout=900):
    sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
    import apps, fixture, isolate, xdisplay
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        if not all(apps.serviceState(display).values()):
            apps.stopServices(display)
            return "services did not come up (%s)" % log
        queues = os.path.join(state, ".ECCE", "Queues")
        os.makedirs(os.path.dirname(queues), exist_ok=True)
        if not os.path.exists(queues):
            open(queues, "w").close()
        fixture.ensureRealUserAccount()
        restore = fixture.settleUpgradeNotices()
        url, error = fixture.install(name, source_dir=source)
        if error:
            apps.stopServices(display)
            restore()
            return "fixture install failed: %s" % error
        env = display.env()
        env["ECCE_VIEWER_SCENE"] = os.path.join(SCENES, scene)
        env["ECCE_VIEWER_SCENE_OUT"] = outdir
        auth = os.path.join(state, "auth.pipe")
        fixture.authFile(auth, port=int(settings["ECCE_DATASERVER_PORT"]))
        proc = subprocess.Popen(
            [os.path.join(apps.WRAPPERS, "ecce-builder"), "-pipe", auth,
             "-context", url], env=env, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, start_new_session=True)
        err = None
        try:
            out = proc.communicate(timeout=timeout)[0]
        except subprocess.TimeoutExpired:
            os.killpg(os.getpgid(proc.pid), 9)
            out = proc.communicate()[0]
            err = "builder did not finish within %ds" % timeout
        apps.stopServices(display)
        restore()
        open(os.path.join(outdir, name + ".log"), "wb").write(out)
        if os.path.exists(os.path.join(outdir, "FAILED")):
            err = open(os.path.join(outdir, "FAILED")).read().strip()
        elif not os.path.exists(os.path.join(outdir, scene.replace(".scene", "") + "-base.ppm")) \
                and not err:
            err = "no snapshot written; see %s.log" % name
        return err


def cmdRender(a):
    outdir = os.path.join(a.raw_dir, a.name)
    os.makedirs(outdir, exist_ok=True)
    sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
    import xdisplay
    binary = os.path.join(a.build_dir, "viewer-scenes")
    os.environ["ECCE_TEST_HOME"] = a.install_home
    os.environ["ECCE_TEST_WRAPPERS"] = a.wrappers
    os.environ.setdefault("ECCE_TEST_XDISPLAYS", "170-179")
    os.environ["LIBGL_ALWAYS_SOFTWARE"] = "1"
    if a.only != "calc":
        with xdisplay.Display() as display:
            renderStandalone(binary, outdir, ":%d" % display.number)
    for name, source, scene in CALCS:
        if a.only == "builtin":
            break
        if a.calc and a.calc != name:
            continue
        err = renderCalc(name, source, scene, outdir)
        if err:
            print("%s: %s" % (name, err))
            open(os.path.join(outdir, name + ".FAILED"), "w").write(err)
    return 0


# ------------------------------------------------------------------ diff

def readPpm(path):
    import numpy as np
    if path.endswith(".jpg"):          # vizthumb: the stored thumbnail
        from PIL import Image
        return np.array(Image.open(path).convert("RGB"))
    with open(path, "rb") as f:
        data = f.read()
    parts = data.split(None, 4)
    w, h = int(parts[1]), int(parts[2])
    pix = np.frombuffer(data[len(data) - w * h * 3:], dtype=np.uint8)
    return pix.reshape(h, w, 3)


def compare(v, c, tol, covtol=2):
    import numpy as np
    v = v.astype(int)
    c = c.astype(int)
    # the background is whatever the corner pixel is, per image
    covV = (np.abs(v - v[0, 0]).max(axis=2) > covtol)
    covC = (np.abs(c - c[0, 0]).max(axis=2) > covtol)
    both = covV & covC
    raw = np.abs(v - c)
    # Cast (#83): the vendored build adds a constant to lit surfaces.  Take
    # it per channel as the commonest non-zero difference over pixels both
    # builds cover and the vendored one has not clipped (a median would be
    # 0 whenever most of the image is stippled or unlit).  A pixel counts as
    # matching if either the raw or the cast-removed difference is small, so
    # surfaces that carry no cast are not penalised.
    off = [0, 0, 0]
    for k in range(3):
        ok = both & (v[:, :, k] < 250)
        d = (v[:, :, k] - c[:, :, k])[ok]
        d = d[(d != 0) & (np.abs(d) <= 80)]
        if d.size > 0.02 * max(1, int(ok.sum())):
            off[k] = int(np.bincount(d + 80).argmax() - 80)
    vc = v.copy()
    cor = np.abs(v - c)
    for k in range(3):
        alt = np.abs(v[:, :, k] - off[k] - c[:, :, k])
        cor[:, :, k] = np.where(covV, np.minimum(cor[:, :, k], alt), cor[:, :, k])
        vc[:, :, k] = np.where(covV, np.clip(v[:, :, k] - off[k], 0, 255), v[:, :, k])
        cor[:, :, k][covV & (v[:, :, k] >= 250)] = 0   # clipped: not recoverable
    total = v.shape[0] * v.shape[1]
    m = {
        "total_px": total,
        "covered_vendored": int(covV.sum()),
        "covered_coin": int(covC.sum()),
        "mask_xor_px": int((covV ^ covC).sum()),
        "raw_diff_px": int((raw.max(axis=2) > tol).sum()),
        "raw_max_channel_diff": int(raw.max()),
        "cast_offset_rgb": off,
        "corrected_diff_px": int((cor.max(axis=2) > tol).sum()),
        "corrected_max_channel_diff": int(cor.max()),
        "corrected_mean_diff_covered": round(float(cor[both].mean()), 3)
                                       if both.any() else 0.0,
    }
    return m, vc, cor, covV ^ covC


def textCompare(vdir, cdir):
    """Scenes that write numbers or counts (pick, drag, redraws) instead of
    pixels: report each text pair as same, or the differing lines.  Numbers
    are compared to 1e-3."""
    import re
    out = []
    names = sorted({f for d in (vdir, cdir) for f in os.listdir(d)
                    if f.endswith(".txt")})
    num = re.compile(r"-?\d+\.\d+")
    for f in names:
        pv, pc = os.path.join(vdir, f), os.path.join(cdir, f)
        if not (os.path.exists(pv) and os.path.exists(pc)):
            out.append("TEXT %-30s only on %s" % (f, "vendored" if os.path.exists(pv) else "coin"))
            continue
        lv, lc = open(pv).read().splitlines(), open(pc).read().splitlines()
        bad = []
        for i in range(max(len(lv), len(lc))):
            x = lv[i] if i < len(lv) else ""
            y = lc[i] if i < len(lc) else ""
            nx, ny = num.findall(x), num.findall(y)
            same = (num.sub("#", x) == num.sub("#", y) and len(nx) == len(ny) and
                    all(abs(float(p) - float(q)) < 1e-3 for p, q in zip(nx, ny)))
            if not same:
                bad.append("   v: %s\n   c: %s" % (x, y))
        out.append("TEXT %-30s %s" % (f, "same" if not bad else "DIFFER"))
        out += bad
    return out


def cmdDiff(a):
    import numpy as np
    from PIL import Image, ImageDraw
    vdir = os.path.join(a.raw_dir, "vendored")
    cdir = os.path.join(a.raw_dir, "coin")
    names = sorted({f.rsplit(".", 1)[0] for d in (vdir, cdir)
                    for f in os.listdir(d)
                    if f.endswith((".ppm", ".jpg", ".UNAVAILABLE"))})
    os.makedirs(a.out_dir, exist_ok=True)
    rows, results, missing = [], {}, []
    for n in names:
        ext = ".jpg" if n.endswith("vizthumb") else ".ppm"
        pv, pc = os.path.join(vdir, n + ext), os.path.join(cdir, n + ext)
        if not (os.path.exists(pv) and os.path.exists(pc)):
            who = "vendored" if not os.path.exists(pv) else "coin"
            why = ("renderer unavailable (offscreen)"
                   if os.path.exists(os.path.join(a.raw_dir, who, n + ".UNAVAILABLE"))
                   else "not produced")
            missing.append("%s: %s on %s" % (n, why, who))
            continue
        v, c = readPpm(pv), readPpm(pc)
        m, vc, cor, xor = compare(v, c, a.tol, 24 if ext == ".jpg" else 2)  # JPEG noise
        d = os.path.join(a.out_dir, n)
        os.makedirs(d, exist_ok=True)
        Image.fromarray(v).save(os.path.join(d, "vendored.png"))
        Image.fromarray(c).save(os.path.join(d, "coin.png"))
        # diff: corrected difference x4 as grey, coverage-mask differences red
        g = np.clip(cor.max(axis=2) * 4, 0, 255).astype(np.uint8)
        dimg = np.stack([g, g, g], axis=2)
        dimg[xor] = (255, 0, 0)
        Image.fromarray(dimg).save(os.path.join(d, "diff.png"))
        json.dump(m, open(os.path.join(d, "metrics.json"), "w"), indent=1)
        results[n] = m
        flagged = (m["corrected_diff_px"] > a.frac * m["total_px"] or
                   m["mask_xor_px"] > a.mask_frac * m["total_px"])
        m["flagged"] = bool(flagged)
        if flagged:
            rows.append((n, m, v, c, dimg))

    lines = ["%-34s %7s %7s %6s %6s %7s %6s %6s" % (
        "scene", "rawpx", "rawmax", "cast", "corpx", "cormax", "xor", "flag")]
    for n, m in results.items():
        lines.append("%-34s %7d %7d %6d %6d %7d %6d %6s" % (
            n, m["raw_diff_px"], m["raw_max_channel_diff"], max(m["cast_offset_rgb"]),
            m["corrected_diff_px"], m["corrected_max_channel_diff"],
            m["mask_xor_px"], "DIFF" if m["flagged"] else ""))
    for n in missing:
        lines.append("MISSING: " + n)
    lines += textCompare(vdir, cdir)
    open(os.path.join(a.out_dir, "summary.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))

    # contact sheet: one row per flagged pair
    cell, lab = 240, 330
    foot = 20 * len(missing)
    sheet = Image.new("RGB", (lab + 3 * cell, max(1, len(rows)) * (cell + 4) + foot),
                      (30, 30, 30))
    dr = ImageDraw.Draw(sheet)
    if not rows:
        dr.text((10, 10), "no pair differs beyond tolerance", fill=(255, 255, 255))
    for i, (n, m, v, c, dimg) in enumerate(rows):
        y = i * (cell + 4)
        for j, arr in enumerate((v, c, dimg)):
            im = Image.fromarray(arr).resize((cell, cell), Image.NEAREST)
            sheet.paste(im, (lab + j * cell, y))
        txt = ["%s" % n,
               "vendored | coin | diff (x4, mask diff red)",
               "raw: %d px > %d, max %d" % (m["raw_diff_px"], a.tol,
                                              m["raw_max_channel_diff"]),
               "cast offset removed (rgb): %s" % m["cast_offset_rgb"],
               "corrected: %d px > %d, max %d" % (
                   m["corrected_diff_px"], a.tol, m["corrected_max_channel_diff"]),
               "coverage xor: %d px (v %d, c %d)" % (
                   m["mask_xor_px"], m["covered_vendored"], m["covered_coin"])]
        for k, t in enumerate(txt):
            dr.text((8, y + 8 + 16 * k), t, fill=(255, 255, 255))
    for i, t in enumerate(missing):
        dr.text((8, sheet.size[1] - foot + 4 + 20 * i), "no pair: " + t,
                fill=(255, 200, 80))
    sheet.save(os.path.join(a.out_dir, "contact-sheet.png"))
    return 0


def main():
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("render")
    for x in ("name", "build_dir", "install_home", "wrappers", "raw_dir"):
        r.add_argument(x)
    r.add_argument("--only", choices=["builtin", "calc"])
    r.add_argument("--calc", help="run just this fixture (default: all)")
    d = sub.add_parser("diff")
    d.add_argument("raw_dir")
    d.add_argument("out_dir")
    d.add_argument("--tol", type=int, default=16, help="channel difference")
    d.add_argument("--frac", type=float, default=0.002,
                   help="flag if corrected differing pixels exceed this "
                        "fraction of the image")
    d.add_argument("--mask-frac", type=float, default=0.001)
    a = p.parse_args()
    return cmdRender(a) if a.cmd == "render" else cmdDiff(a)


if __name__ == "__main__":
    sys.exit(main())
