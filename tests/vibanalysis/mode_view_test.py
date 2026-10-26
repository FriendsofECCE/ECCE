#!/usr/bin/env python3
"""Where the Vibrational Frequencies panel shows a mode (#579), in the real
Builder, headless: the Spectrum window's strip and hover box, the frequency
table's decimals and resizable Mode column, the Composition window's columns.

    tests/vibanalysis/mode_view_test.py [--build build-cmake] [--png DIR]

Runs on the ORCA alpha-D-galactopyranose example (its stored copy) and on
water from the ECCE-QM fixture.  The Builder is driven by its dump hooks and,
for the column drag, a real X mouse drag in Xvfb.  Exit 77 without Xvfb or
the built binaries.
"""
import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import importlib.util  # noqa: E402

_spec = importlib.util.spec_from_file_location(
    "vibanalysis_panel_test", os.path.join(HERE, "panel_test.py"))
vp = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(vp)
bo, harness, say, rt = vp.bo, vp.harness, vp.say, vp.rt
failed = []

GAL = os.path.join(REPO, "data", "client", "examples", "spectroscopy", "sugars",
                   "orca-alpha_d_galactopyranose", "stored", "alpha_d_galactopyranose")


def check(ok, text):
    if not vp.check(ok, text):
        failed.append(text)
    return ok


def read(path):
    if not os.path.exists(path):
        return []
    return [l.rstrip("\n") for l in open(path, encoding="utf-8")]


def run(s, display, env, wrappers, png, calc, prefix, extra):
    name = calc
    url = "file://" + os.path.join(s.project, name) + "/"
    base = {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Vibrational Frequencies"}
    base.update(extra)
    text, _ = bo.runScript(s, display, env, wrappers, ["-context", url],
                           ["wait 15000", "quit"], png, prefix, base)
    check("segmentation" not in text.lower(), "%s: no crash in the Builder log" % name)
    return text


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--only")
    ap.add_argument("--shots", action="store_true", help="also photograph the docked panel")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "vibview", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI + vp.SYMMETRY:
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
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        s.project = project

        shutil.rmtree(os.path.join(project, "gal"), ignore_errors=True)
        shutil.copytree(GAL, os.path.join(project, "gal"), symlinks=True)
        name, fixture, elements, key = vp.MOLECULES[0]
        vp.makeCalc(project, "h2o", fixture, "ecceqm.desc", "GeoVib", list(elements), key)

        out = s.state
        for calc, click in [c for c in (("gal", 39), ("h2o", 8)) if not args.only or c[0] == args.only]:
            say("== %s" % calc)
            tab = os.path.join(out, "%s-table.dump" % calc)
            run(s, display, env, wrappers, png, calc, calc + "-table",
                {"ECCE_NMODE_TABLE_DUMP": tab, "ECCE_NMODE_TABLE_DRAG": "90"})
            d = vp.readDump(tab)
            cells = d["cells"]
            rows = sorted({r for r, _c in cells})
            check(len(rows) > 0, "%s: table dumped (%d rows)" % (calc, len(rows)))
            freq = [cells[(r, 0)][2] for r in rows]
            check(all(re.fullmatch(r"-?\d+\.\d", f) for f in freq),
                  "%s: frequencies have one decimal: %s" % (calc, freq[:4]))
            ircol = [c for c, (_w, lab) in d["cols"].items() if lab.startswith("Infrared")]
            if ircol:
                ir = [cells[(r, ircol[0])][2] for r in rows]
                check(all(re.fullmatch(r"-?\d+\.\d\d", v) for v in ir),
                      "%s: intensities have two decimals: %s" % (calc, ir[:4]))
            drag = read(tab + ".drag")
            before = d["cols"][2][0]
            after = [int(l.split()[2]) for l in drag if l.startswith("col 2 ")]
            check(bool(after) and after[0] >= before + 60,
                  "%s: dragging the Mode column's edge 90 px widens it (%d -> %s)"
                  % (calc, before, after))
            tips = [l for l in read(tab) if l.startswith("tip ")]
            check(any(len(t.split(None, 2)) > 2 and "%" in t for t in tips),
                  "%s: hovering a Mode cell gives the label and the shares" % calc)
            if calc == "gal":
                label7 = d["modes"][7]["label"]
                check(label7 != "-" and "τ" in label7,
                      "gal: mode 7 is described as a torsion: %s | %s"
                      % (label7, d["modes"][7]["detail"]))
                say("  mode 7 cell after drag: %s" % [l for l in drag if l.startswith("cell 7 ")])

            if args.shots:
                bo.runScript(s, display, env, wrappers,
                             ["-context", "file://" + os.path.join(project, calc) + "/"],
                             ["wait 9000", "shot panel", "quit"], png, "%s-panel" % calc,
                             {"ECCE_PANEL_MODE": "detail",
                              "ECCE_OPEN_PANEL": "Vibrational Frequencies"})

            sp = os.path.join(out, "%s-spectrum.dump" % calc)
            run(s, display, env, wrappers, png, calc, calc + "-spectrum",
                {"ECCE_SPECTRUM_DUMP": sp, "ECCE_SPECTRUM_WINDOW": "1",
                 "ECCE_SPECTRUM_CLICK": str(click), "ECCE_SPECTRUM_HOVER": str(click),
                 "ECCE_EXIT_AFTER_DUMP": "1"})
            lines = read(sp)
            hover = [l.split(" ", 2)[2] for l in lines if l.startswith("hover ")]
            strip = [l.split(" ", 2)[2] for l in lines if l.startswith("strip ") and l[6].isdigit()]
            say("  hover: %s" % hover)
            say("  strip: %s" % strip)
            check(bool(hover) and re.fullmatch(r"Mode %d — -?\d+\.\d cm.+" % click, hover[0]),
                  "%s: hover line 1 is 'Mode N — freq': %s" % (calc, hover[:1]))
            check(len(hover) >= 3 and hover[1] == d["modes"][click]["label"] and
                  "intensity" in hover[2].lower() + "intensity",
                  "%s: hover line 2 is the short label (%s), then the intensities: %s"
                  % (calc, d["modes"][click]["label"], hover[1:]))
            check(not any("%" in h for h in hover), "%s: no contributions in the hover box" % calc)
            check(bool(strip) and strip[0].startswith("Mode %d" % click) and
                  d["modes"][click]["label"] in strip[0],
                  "%s: the strip's bold line names the mode and its label: %s" % (calc, strip[:1]))
            check(len(strip) >= 2 and all("%" in x for x in strip[1:]),
                  "%s: the strip lists the contributions (%d)" % (calc, len(strip) - 1))
            if os.path.exists(sp + ".window.png"):
                shutil.copy(sp + ".window.png", os.path.join(png, "%s-spectrum-window.png" % calc))

            cp = os.path.join(out, "%s-comp.dump" % calc)
            cg = cp + ".grid"
            run(s, display, env, wrappers, png, calc, calc + "-comp",
                {"ECCE_NMODE_COMPOSITION_DUMP": cp, "ECCE_NMODE_COMPOSITION_GRID": cg,
                 "ECCE_NMODE_COMPOSITION_RESIZE": "1200"})
            g = read(cg)
            hdr = [l.split() for l in g if l.startswith(("before", "after"))]
            check(len(hdr) == 2, "%s: composition grid dumped" % calc)
            if len(hdr) == 2:
                lastB, lastA = int(hdr[0][-1]), int(hdr[1][-1])
                check(lastA >= lastB + 200,
                      "%s: the last column grows with the window (%d -> %d)" % (calc, lastB, lastA))
                total = sum(int(x) for x in hdr[1][hdr[1].index("cols") + 1:])
                check(abs(total - int(hdr[1][6])) <= 25,
                      "%s: the columns fill the grid (%d of %s px)" % (calc, total, hdr[1][4]))
            rowsC = [l.split("\t") for l in g if l.startswith("row")]
            if calc == "gal" and rowsC:
                r7 = [r for r in rowsC if r[1] == "7"][0]
                check(r7[3] == d["modes"][7]["label"],
                      "gal: composition describes mode 7 as the table does: %s / %s"
                      % (r7[3], d["modes"][7]["label"]))
                check("torsion" in r7[4], "gal: mode 7's contributions include torsions: %s" % r7[4])
                say("  composition mode 7: %s" % r7[1:])
            if os.path.exists(cp + ".png"):
                shutil.copy(cp + ".png", os.path.join(png, "%s-composition.png" % calc))
            for suffix in ("-table", "-spectrum", "-comp"):
                pass
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    print("%d failed" % len(failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
