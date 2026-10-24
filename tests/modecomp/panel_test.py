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
    high = os.path.join(s.state, "high.dump")
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
        text, _shots = bo.runScript(
            s, display, env, wrappers, ["-context", url],
            ["wait 12000", "quit"], png, "nma",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Vibrational Frequencies",
             "ECCE_NMODE_COMPOSITION_DUMP": comp,
             "ECCE_NMODE_COMPOSITION_COORDS": "2-4-9",
             "ECCE_NMODE_HIGHLIGHT": high, "ECCE_NMODE_HIGHLIGHT_ATOMS": "2 3"})
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()

    check(os.path.exists(comp), "the composition window dumped its table")
    check(os.path.exists(high), "the spectrum highlight dumped")
    if os.path.exists(comp):
        lines = [l.rstrip("\n").split("\t") for l in open(comp, encoding="utf-8")]
        names = lines[0][1:]
        check(any(n == "C2–N4–H9 bend" for n in names) and
              names.count("C2–N4–H9 bend") == 2 or
              "C2–N4–H9 bend" in names,
              "a typed coordinate is in the set")
        amide1 = [l for l in lines[1:] if 1700 < float(l[1]) < 2000][0]
        co = names.index("C2–O3 stretch")
        check(float(amide1[2 + co]) > 60, "window: amide I is mostly C=O (%s %%)" % amide1[2 + co])
        shutil.copy(comp + ".png", os.path.join(png, "composition.png"))
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
