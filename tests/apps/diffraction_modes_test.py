#!/usr/bin/env python3
"""Every Diffraction view draws without Update, also for a slow structure (#1222).

A molecule (HCl: Fibre, Debye) and a periodic structure (graphite: Powder,
Single crystal, Detector frame, Fibre, Debye), plus the Pattern window opened
on an out-of-date pattern. ecce-xrd is delayed by 3 s so the pane is in its
"press Update" mode; each view must still calculate, and the canvas as shown
on screen must carry more ink after the run than just after the switch.
"""
import argparse
import os
import re
import shutil
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "launch"))
import builder_overlay_test as bo
import harness
import packing_cells_test as pc

EXAMPLES = os.path.join(ROOT, "data", "client", "examples", "genchem")
NAMES = {0: "fibre", 1: "powder", 2: "single", 3: "frame", 4: "debye"}


def ink(path):
    """Pixels that differ from the most common colour (8-bit RGB/RGBA PNG)."""
    data = open(path, "rb").read()
    pos, idat, w = 8, b"", 0
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            w, h, depth, ctype = struct.unpack(">IIBB", body[:10])
            bpp = {2: 3, 6: 4}[ctype]
        elif kind == b"IDAT":
            idat += body
        pos += 12 + n
    raw, stride, prev, rows = zlib.decompress(idat), w * bpp, None, []
    for y in range(h):
        f, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        up = prev or bytearray(stride)
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            c = up[i - bpp] if i >= bpp else 0
            if f == 1: line[i] = (line[i] + a) & 255
            elif f == 2: line[i] = (line[i] + up[i]) & 255
            elif f == 3: line[i] = (line[i] + (a + up[i]) // 2) & 255
            elif f == 4:
                p = a + up[i] - c
                pa, pb, pc_ = abs(p - a), abs(p - up[i]), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc_ else up[i] if pb <= pc_ else c)) & 255
        rows.append(line)
        prev = line
    px = [tuple(r[i:i + 3]) for r in rows for i in range(0, stride, bpp)]
    bg = max(set(px[::97]), key=px[::97].count)
    return sum(1 for p in px if max(abs(p[k] - bg[k]) for k in range(3)) > 8)


def run(s, display, env, wrappers, png, output, prefix, views):
    url = pc.importExample(s, env, wrappers, output)
    if not bo.check(bool(url), prefix + " example imports"):
        return
    path = lambda n: os.path.join(png, "%s-%s.png" % (prefix, n))
    lines = ["wait 3000", "background 1 1 1", "xrd size 1366 768", "list Diffraction", "wait 1500",
             "xrd wait", "xrd state", "xrd canvas-shot " + path("open")]
    for v in views:
        lines += ["xrd set view %d" % v, "xrd canvas-shot " + path(NAMES[v] + "-empty"),
                  "xrd wait", "xrd state", "xrd canvas-shot " + path(NAMES[v])]
    # A settings edit in Update mode leaves the pattern out of date; Pattern must recalculate it.
    lines += ["xrd set B 5", "wait 500", "xrd state", "xrd breakout 1", "wait 500",
              "xrd wait", "xrd state", "xrd canvas-shot " + path("pattern"), "xrd breakout 0", "quit"]
    text, _ = bo.runScript(s, display, env, wrappers, ["-context", url], lines, png, prefix,
                           {"ECCE_PANEL_MODE": "detail"})
    open(os.path.join(png, prefix + ".log"), "w").write(text)
    states = re.findall(r"BUILDER: xrd: view=(.*)", text)
    bo.check(len(states) == len(views) + 3 and "manual=1" in states[0],
             "%s: first run is slow, the pane is in Update mode" % prefix)
    for v, st in zip(views, states[1:]):
        name = NAMES[v]
        ok = st.startswith("%d " % v) and "dirty=0" in st and "running=0" in st
        empty, full = path(name + "-empty"), path(name)
        drawn = os.path.exists(empty) and os.path.exists(full) and ink(full) > max(400, 1.3 * ink(empty))
        bo.check(ok and drawn, "%s: %s calculates and draws without Update (%s)" % (prefix, name, st))
    if len(states) == len(views) + 3:
        stale, opened = states[-2], states[-1]
        bo.check("dirty=1" in stale and "dirty=0" in opened and "docked=0" in opened and
                 os.path.exists(path("pattern")) and ink(path("pattern")) > 400,
                 "%s: Pattern window calculates an out-of-date pattern on open" % prefix)
    bo.check("Update required" not in text and "refused:" not in text and "Unhandled" not in text,
             "%s: hooks finish without Update or exceptions" % prefix)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--build", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--png")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ())
    import xdisplay
    s = harness.Session(build, "diffraction-modes", {}, local=True)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in pc.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    tool = os.path.join(s.home, "bin", "ecce-xrd")
    shutil.copy(os.path.join(ROOT, "scripts", "ecce_xrd.py"), tool + "-real")
    with open(tool, "w") as f:
        f.write("#!" + sys.executable + "\nimport runpy, time\ntime.sleep(3)\n" +
                "runpy.run_path(" + repr(tool + "-real") + ", run_name='__main__')\n")
    os.chmod(tool, 0o755)
    wrappers = os.path.join(s.state, "wrappers")
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            bo.check(False, "isolated services start")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        run(s, display, env, wrappers, png,
            os.path.join(EXAMPLES, "molecular-and-covalent-solids", "qe-graphite", "graphite.pwout"),
            "graphite", [2, 3, 0, 4, 1])
        run(s, display, env, wrappers, png,
            os.path.join(EXAMPLES, "intermolecular-interactions", "ecceqm-hcl_covalent", "hcl_covalent.qmout"),
            "hcl", [4, 0, 4])
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    harness.say("FAILED: " + "; ".join(bo.failures) if bo.failures else "PASSED")
    return 1 if bo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
