#!/usr/bin/env python3
"""Results > Diffraction on the Packing in solids NaCl calculation (#587).

    tests/apps/powder_dialog_test.py [--build build-cmake] [--png DIR]

The example is imported through the Organizer and opened in the Builder; the
"calcdiffraction" script command is the Diffraction entry of its results
(Properties list and menu).  The dialog's CSV must equal `ecce-xrd powder` on
the structure file it was given, which carries the lattice (extended xyz), and
the NaCl lines must be where the CIF fixture puts them.  Exit status 77
(skip) without Xvfb, import or the built binaries.
"""
import argparse
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

    s = harness.Session(build, "powder-dialog", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in pc.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
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
        url = pc.importExample(s, env, wrappers,
                               os.path.join(pc.PK, "qe-nacl", "nacl.pwout"))
        if not bo.check(url is not None, "nacl imports: %s" % url):
            return 1
        out = os.path.join(s.state, "powder")
        os.makedirs(out, exist_ok=True)
        pngf, csvf, xyz = (os.path.join(out, n) for n in ("dialog.png", "dialog.csv", "dialog.xyz"))
        lines = ["wait 4000", "list Diffraction", "wait 1500",
                 "calcdiffraction %s %s %s" % (pngf, csvf, xyz), "quit"]
        text, _ = bo.runScript(s, display, env, wrappers, ["-context", url], lines, png,
                               "powder-nacl", {"ECCE_PANEL_MODE": "detail"})
        bo.check("BUILDER: calcdiffraction: done" in text and os.path.exists(csvf),
                 "the Diffraction entry calculated and saved", text[-1500:])
        if os.path.exists(csvf):
            bo.check(open(pngf, "rb").read(8) == b"\x89PNG\r\n\x1a\n", "the PNG is a PNG")
            head = open(xyz).read().splitlines()[1]
            bo.check(head.startswith('Lattice="5.64'), "the structure file carries the lattice: %s" % head)
            ref = os.path.join(out, "cli.csv")
            r = subprocess.run([sys.executable, CLI, "powder", xyz, "--out", ref],
                               capture_output=True, text=True)
            bo.check(r.returncode == 0, "the CLI ran on the same structure", r.stderr[-300:])
            bo.check(open(csvf).read() == open(ref).read(), "the dialog's CSV equals the CLI's")
            rows = [ln.split(",") for ln in open(csvf).read().splitlines()[1:]]
            peak = max(rows, key=lambda r: float(r[1]))
            bo.check(abs(float(peak[0]) - 31.70) < 0.02,
                     "strongest line (200) at 31.70, found %s" % peak[0])
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
