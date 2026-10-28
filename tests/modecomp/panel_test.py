#!/usr/bin/env python3
"""Mode composition window and spectrum highlight (#486) in the real Builder,
headless, on ORCA's N-methylacetamide frequencies.

    tests/modecomp/panel_test.py [--build build-cmake] [--png DIR]

The Builder's hooks open the Composition window and select atoms as a viewer
selection would (ECCE_NMODE_COMPOSITION_DUMP, ECCE_NMODE_HIGHLIGHT); what the
window and the canvas hold is compared with the library's numbers.
Exit 77 without Xvfb or the built binaries.
"""
import argparse
import importlib.util
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "vibanalysis"))
spec = importlib.util.spec_from_file_location(
    "vibanalysis_panel_test", os.path.join(REPO, "tests", "vibanalysis", "panel_test.py"))
vp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(vp)
bo, harness, say = vp.bo, vp.harness, vp.say
failed = []


def check(ok, text):
    if not vp.check(ok, text):
        failed.append(text)
    return ok

FIXTURE = os.path.join(HERE, "nma_trans_freq.out")
ELEMENTS = list("CCONC") + list("HHHHHHH")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
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

    s = harness.Session(build, "modecomp", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI + vp.SYMMETRY:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    comp = os.path.join(s.state, "comp.dump")
    comp1 = os.path.join(s.state, "comp1.dump")
    copy = os.path.join(s.state, "copy.txt")
    high = os.path.join(s.state, "high.dump")
    grid = os.path.join(s.state, "grid.dump")
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
        vp.makeCalc(project, "nma", FIXTURE, "orca.desc", "Vibration", ELEMENTS, "NMA")
        url = "file://" + os.path.join(project, "nma") + "/"
        base = {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Vibrational Frequencies"}
        #  Same calculation twice: C-H bends off, then on.
        for dump, ch, extra in ((comp, "0", {"ECCE_NMODE_COMPOSITION_GRID": grid,
                                             "ECCE_NMODE_COMPOSITION_COPY": copy,
                                             "ECCE_NMODE_HIGHLIGHT": high,
                                             "ECCE_NMODE_HIGHLIGHT_ATOMS": "2 3"}),
                                (comp1, "1", {})):
            env2 = dict(base, ECCE_NMODE_COMPOSITION_DUMP=dump,
                        ECCE_NMODE_COMPOSITION_CH=ch, **extra)
            bo.runScript(s, display, env, wrappers, ["-context", url],
                         ["wait 12000", "quit"], png, "nma", env2)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()

    check(os.path.exists(comp) and os.path.exists(comp1), "the composition window dumped its tables")
    check(os.path.exists(high), "the spectrum highlight dumped")
    names = {}
    for tag, path in (("off", comp), ("on", comp1)):
        if not os.path.exists(path):
            continue
        lines = [l.rstrip("\n").split("\t") for l in open(path, encoding="utf-8")]
        names[tag] = lines[0][1:]
        sums = [sum(float(v) for v in l[2:]) for l in lines[1:]]
        check(all(abs(t - 100) < 0.5 or t < 1e-6 for t in sums),
              "C-H bends %s: shares sum to 100 %%" % tag)
        if tag == "off":
            amide1 = [l for l in lines[1:] if 1700 < float(l[1]) < 2000][0]
            co = names[tag].index("C2–O3 stretch")
            check(float(amide1[2 + co]) > 60,
                  "window: amide I is mostly C=O (%s %%)" % amide1[2 + co])
            shutil.copy(path + ".png", os.path.join(png, "composition.png"))
    if len(names) == 2:
        extra = len(names["on"]) - len(names["off"])
        check(extra >= 6, "C-H bends add coordinates (%d more)" % extra)
        check(any(n.startswith("H") and "C" in n and "bend" in n for n in names["on"]) and
              not any(n.startswith("H") and "C" in n and "bend" in n for n in names["off"]),
              "H-C-X bends appear only with the checkbox")
    check(os.path.exists(grid), "the grid's cells dumped")
    if os.path.exists(grid):
        glines = [l.rstrip("\n").split("\t") for l in open(grid, encoding="utf-8")]
        rows = [l for l in glines if l[0] == "row"]
        check(rows and all(l[4].strip() for l in rows),
              "no empty 'Largest contributions' cell (%d rows)" % len(rows))
        check(all(len(l[4].split("; ")) >= 3 for l in rows),
              "every mode lists three contributions")
        check(not any("added:" in l[4] for l in rows), "no 'added:' text")
        check([l for l in glines if l[0] == "ch"] == [["ch", "0"]], "checkbox state is dumped")
    check(os.path.exists(copy), "the Copy text dumped")
    if os.path.exists(copy):
        clines = [l.split("\t") for l in open(copy, encoding="utf-8").read().splitlines()]
        check(clines[0] == ["Mode", "cm-1", "Mode type", "Largest contributions"],
              "Copy header: %s" % clines[0])
        check(all(len(l) == 4 for l in clines) and len(clines) == len(rows) + 1,
              "Copy: tab-separated, one line per row")
        csv = open(copy + ".csv", encoding="utf-8").read().splitlines()
        check(csv[0] == 'Mode,cm-1,Mode type,Largest contributions' or
              csv[0] == '"Mode","cm-1","Mode type","Largest contributions"', "csv header")
    if os.path.exists(high):
        lines = [l.rstrip("\n").split("\t") for l in open(high, encoding="utf-8")]
        check(lines[0] == ["key C2–O3 stretch"], "key names the bond: %s" % lines[0])
        hot = [i for i, l in enumerate(lines[1:]) if float(l[1]) >= 10]
        check(len(hot) == 1, "one mode highlighted: %s" % hot)
        shutil.copy(high + ".png", os.path.join(png, "highlight.png"))
    print("%d failed" % len(failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
