#!/usr/bin/env python3
"""Window shots of the Vibrational Frequencies panel, graph and table view
(#367), headless, for a look at the layout.

    tests/vibanalysis/shots.py --png DIR [--build build-cmake] [mol ...]

Uses panel_test's calculations and the Builder's script hooks only.
"""
import argparse
import importlib.util
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
spec = importlib.util.spec_from_file_location("vib_panel_test", os.path.join(HERE, "panel_test.py"))
pt = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pt)
bo, harness, say = pt.bo, pt.harness, pt.say


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(pt.REPO, "build-cmake"))
    ap.add_argument("--png", required=True)
    ap.add_argument("mols", nargs="*", default=["h2o", "ch4"])
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ())
    import xdisplay

    s = harness.Session(build, "vibshots", {}, local=True)
    os.makedirs(args.png, exist_ok=True)
    for exe in bo.GUI + pt.SYMMETRY:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        for name, fixture, elements, key in pt.MOLECULES:
            if name not in args.mols:
                continue
            pt.makeCalc(project, name, fixture, "ecceqm.desc", "GeoVib", list(elements), key)
            url = "file://" + os.path.join(project, name) + "/"
            for view in ("graph", "table"):
                extra = {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Vibrational Frequencies"}
                if view == "table":
                    extra["ECCE_NMODE_TABLE_DUMP"] = os.path.join(s.state, name + ".dump")
                else:
                    extra["ECCE_SPECTRUM_DUMP"] = os.path.join(s.state, name + ".spec")
                    extra["ECCE_SPECTRUM_CLICK"] = "8"
                text, shots = bo.runScript(s, display, env, wrappers, ["-context", url],
                                           ["wait 7000", "shot " + view, "quit"],
                                           args.png, name, extra)
                say("%s %s: %s" % (name, view, shots))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()


if __name__ == "__main__":
    sys.exit(main())
