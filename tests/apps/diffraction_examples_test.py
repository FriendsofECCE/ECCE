#!/usr/bin/env python3
"""The Diffraction topic and two Molecular and covalent solids: each example
imports and opens its pattern (#972).

    tests/apps/diffraction_examples_test.py [--build build-cmake] [--png DIR]

Each example of the topic is imported through the Organizer and opened in the
Builder; the "calcdiffraction" script command is the Diffraction entry of its
results and reports the pattern it opens: powder for the periodic calculations,
fibre for the xTB molecules.  The periodic ones also give a layer (NaCl hk0
only h,k both even; the all-odd hk1 spots weak for NaCl, nearly absent for KCl; quartz 00l only l = 3n).  Exit
status 77 (skip) without Xvfb, import or the built binaries.
"""
import argparse
import math
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import packing_cells_test as pc  # noqa: E402
from harness import say  # noqa: E402

CLI = os.path.join(REPO, "scripts", "ecce_xrd.py")
DX = os.path.join(REPO, "data", "client", "examples", "genchem")
# folder, calculation, output extension, pattern the entry opens
CASES = (("diffraction/qe-nacl_powder", "nacl_powder", ".pwout", "powder"),
         ("diffraction/qe-kcl", "kcl", ".pwout", "powder"),
         ("diffraction/qe-nacl_bragg", "nacl_bragg", ".pwout", "powder"),
         ("diffraction/qe-quartz", "quartz", ".pwout", "powder"),
         ("diffraction/xtb-b_dna", "bdna", ".out", "fibre"),
         ("diffraction/xtb-alpha_helix", "helix", ".out", "fibre"),
         # Molecular and covalent solids: a layered and a molecular crystal
         ("molecular-and-covalent-solids/qe-graphite", "graphite", ".pwout", "powder"),
         ("molecular-and-covalent-solids/qe-urea", "urea", ".pwout", "powder"))


def same_csv_new_png(out, a, b, what):
    """Exposure and colour scheme change the picture, never the data."""
    pa, pb = (os.path.join(out, n) for n in (a + ".png", b + ".png"))
    ca, cb = (os.path.join(out, n) for n in (a + ".csv", b + ".csv"))
    ok = all(os.path.exists(f) for f in (pa, pb, ca, cb))
    bo.check(ok and open(pa, "rb").read() != open(pb, "rb").read(), "%s: the PNG changes" % what)
    bo.check(ok and open(ca).read() == open(cb).read(), "%s: the CSV does not" % what)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in pc.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "diffraction-examples", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in pc.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    # The dialog runs $ECCE_HOME/bin/ecce-xrd; take the one of this tree.
    shutil.copy(CLI, os.path.join(s.home, "bin", "ecce-xrd"))
    os.chmod(os.path.join(s.home, "bin", "ecce-xrd"), 0o755)
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            bo.check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        for folder, calc, ext, kind in CASES:
            url = pc.importExample(s, env, wrappers, os.path.join(DX, folder, calc + ext))
            if not bo.check(url is not None, "%s imports: %s" % (calc, url)):
                continue
            out = os.path.join(s.state, calc)
            os.makedirs(out, exist_ok=True)
            pngf, csvf = os.path.join(out, "d.png"), os.path.join(out, "d.csv")
            lines = ["wait 4000", "list Diffraction", "wait 1500",
                     "calcdiffraction %s %s" % (pngf, csvf)]
            if kind != "fibre":
                lines.append("calclayer %s %s %s 0 0.8 2,0,0" % (
                    os.path.join(out, "l.png"), os.path.join(out, "l.csv"),
                    "k" if calc == "quartz" else "l"))
            if calc in ("kcl", "nacl_powder"):
                # hk1 holds only all-odd spots: F = 4(f_cation - f_Cl)
                lines.append("calclayer %s %s l 1 0.8 2,0,0" % (
                    os.path.join(out, "l1.png"), os.path.join(out, "l1.csv")))
            if calc in ("kcl", "nacl_powder"):
                # precession photographs of hk1 at the default and a high exposure
                for tag, ex in (("p_def", 0), ("p_hi", 20)):
                    lines.append("calcmeasure layer1 %s %s %d 0" % (
                        os.path.join(out, tag + ".png"), os.path.join(out, tag + ".csv"), ex))
            if calc == "nacl_powder":
                m = lambda n: os.path.join(out, n)
                lines += ["calcmeasure layer0 %s %s 0 0 2,0,0 4,0,0" % (m("m0.png"), m("m0.csv")),
                          "calcmeasure frame %s %s 0 0 strongest" % (m("fr_film.png"), m("fr_film.csv")),
                          "calcmeasure frame %s %s 0 1" % (m("fr_dark.png"), m("fr_dark.csv")),
                          "calcmeasure powder %s %s 0 0 31.7" % (m("pw.png"), m("pw.csv"))]
            if calc == "nacl_powder":
                for view_kind in ("layer0", "frame"):
                    lines.append("calcmeasure %s %s %s 0 0 viewtest" % (
                        view_kind, os.path.join(out, view_kind + "-view.png"), os.path.join(out, view_kind + "-view.csv")))
            if calc == "bdna":
                lines.append("calcdiffraction %s %s %s 0 1" % (os.path.join(out, "dark.png"),
                                                              os.path.join(out, "dark.csv"),
                                                              os.path.join(out, "dark.xyz")))
                lines.append("calcdiffraction %s %s %s 0 0 viewtest" % (
                    os.path.join(out, "fibre-view.png"), os.path.join(out, "fibre-view.csv"),
                    os.path.join(out, "view.xyz")))
            if calc == "nacl_powder":
                # the window's own drawings: a frame with labels and rings, and
                # NaCl compared with KCl as a "measured" pattern (stacked panels)
                lines.append("calcframe %s %s 0 30 1 1" % (os.path.join(out, "f.png"), os.path.join(out, "f.csv")))
                kcl = os.path.join(out, "kcl.csv")
                subprocess.run([sys.executable, CLI, "powder", os.path.join(REPO, "tests", "fixtures", "cif", "kcl.cif"),
                                "--out", kcl], capture_output=True)
                xy = os.path.join(out, "kcl.xy")
                open(xy, "w").write("".join(ln.split(",")[0] + " " + ln.split(",")[1] + "\n"
                                            for ln in open(kcl).read().splitlines()[1:]))
                lines.append("calcpowder %s %s %s 0 1 %s" % (os.path.join(out, "cmp.png"), os.path.join(out, "cmp.csv"),
                                                              os.path.join(out, "cmp.xyz"), xy))
            text, _ = bo.runScript(s, display, env, wrappers, ["-context", url], lines + ["quit"],
                                   png, "diffraction-" + calc, {"ECCE_PANEL_MODE": "detail"})
            for name in os.listdir(out):
                if name.endswith(".png"):
                    shutil.copy(os.path.join(out, name), os.path.join(png, "%s-%s" % (calc, name)))
            bo.check("BUILDER: calcdiffraction pattern: %s" % kind in text and
                     "BUILDER: calcdiffraction: done" in text,
                     "%s: the Diffraction entry opens the %s pattern" % (calc, kind))
            if calc in ("bdna", "nacl_powder"):
                count = 1 if calc == "bdna" else 2
                bo.check(text.count("BUILDER: image view: PASS") == count,
                         "%s: wheel anchor, pan, zoomed measure click, rectangle, Home and double click" % calc)
                for tag in (("fibre",) if calc == "bdna" else ("layer0", "frame")):
                    prefix = os.path.join(out, tag + "-view.png")
                    def data(suffix):
                        path = prefix + suffix + ".png"
                        return open(path, "rb").read() if os.path.exists(path) else b""
                    full, reset, zoom, pan, rectangle = (data(k) for k in (".full", ".reset", ".zoom", ".pan", ".rectangle"))
                    bo.check(full and full == reset, "%s %s: reset restores full PNG" % (calc, tag))
                    bo.check(zoom and pan and rectangle and full != zoom and zoom != pan and full != rectangle,
                             "%s %s: navigation changes exported current view" % (calc, tag))
                if calc == "nacl_powder":
                    bo.check(len(re.findall(r"DIFFRACTION: zoomed click \(-?\d+ -?\d+ -?\d+\)", text)) == 2,
                             "frame and precession measure clicks identify hkl after zoom and pan")
            if calc == "bdna":
                same_csv_new_png(out, "d", "dark", "bdna: Dark scheme")
            if calc in ("kcl", "nacl_powder"):
                same_csv_new_png(out, "p_def", "p_hi", "%s hk1: high exposure" % calc)
            if calc == "nacl_powder":
                same_csv_new_png(out, "fr_film", "fr_dark", "frame: Dark scheme")
                bo.check("BUILDER: calcmeasure: done" in text, "the measure hooks ran")
                rd = re.findall(r"BUILDER: calcmeasure readout: (.*)", text)
                bo.check(len(rd) == 3, "three readouts: %d" % len(rd))
                if len(rd) == 3:
                    # layer: (2 0 0) to (4 0 0) is 1/d(200) = 1/2.82 A, one spot between, cell 5.64 A
                    bo.check("0.3546 Å⁻¹" in rd[0] and "1 spots between" in rd[0] and "cell edge 5.640 Å" in rd[0],
                             "layer measure: %s" % rd[0])
                    fm = re.match(r"\((-?\d+) (-?\d+) (-?\d+)\) to \((-?\d+) (-?\d+) (-?\d+)\): ([\d.]+) mm.*, ([\d.]+) ?(?:°|deg)", rd[1])
                    if bo.check(fm is not None, "frame measure: %s" % rd[1]):
                        g = [int(v) for v in fm.groups()[:6]]
                        fr = [ln.split(",") for ln in open(os.path.join(out, "fr_film.csv")).read().splitlines()[1:]]
                        pt = {}
                        for r in fr:
                            pt.setdefault(tuple(int(v) for v in r[:3]), (float(r[3]), float(r[4])))
                        (x1, y1), (x2, y2) = pt[tuple(g[:3])], pt[tuple(g[3:])]
                        ang = math.degrees(math.acos((x1 * x2 + y1 * y2) / math.hypot(x1, y1) / math.hypot(x2, y2)))
                        bo.check(abs(math.hypot(x2 - x1, y2 - y1) - float(fm.group(7))) < 0.01 and
                                 abs(ang - float(fm.group(8))) < 0.11,
                                 "frame distance and angle match the CSV (%.2f mm, %.1f deg)" % (
                                     math.hypot(x2 - x1, y2 - y1), ang))
                    bo.check("d = 2.8" in rd[2] and "Q = 2.2" in rd[2] and "2θ = 31.7" in rd[2] and "I = " in rd[2],
                             "powder cursor: %s" % rd[2])
            if kind == "fibre":
                bo.check("BUILDER: calclayer" not in text, "%s: no single-crystal layer for a molecule" % calc)
                continue
            bo.check("BUILDER: calclayer: done" in text, "%s: the layer is calculated" % calc)
            rows = [ln.split(",") for ln in open(os.path.join(out, "l.csv")).read().splitlines()[1:]]
            present = [(int(r[0]), int(r[1]), int(r[2])) for r in rows if r[9] == "0"]
            if calc in ("nacl_bragg", "nacl_powder"):
                bo.check(present and all(h % 2 == 0 and k % 2 == 0 for h, k, l in present),
                         "%s hk0: only h,k both even" % calc)
            elif calc == "quartz":
                col = [l for h, k, l in present if h == 0 and k == 0]
                bo.check(col and all(l % 3 == 0 for l in col), "quartz 00l: only l = 3n (%s)" % col)
            if calc in ("kcl", "nacl_powder"):
                # Rock salt's hk0 layer is the same for any two ions (h,k both
                # even); the ions show in hk1, the all-odd spots, which are
                # weak for Na+/Cl- and nearly gone for K+/Cl- (18 electrons each).
                def imax(name):
                    rs = [ln.split(",") for ln in open(os.path.join(out, name)).read().splitlines()[1:]]
                    return max((float(r[7]) for r in rs), default=0.0)
                ratio = imax("l1.csv") / max(imax("l.csv"), 1e-30)
                if calc == "kcl":
                    bo.check(ratio < 0.01, "kcl: all-odd hk1 spots nearly absent (%.4f of hk0)" % ratio)
                else:
                    bo.check(ratio > 0.01, "nacl: all-odd hk1 spots visible (%.4f of hk0)" % ratio)
        if bo.failures:
            say(text[-2000:])
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(bo.failures) if bo.failures else "PASSED")
    return 1 if bo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
