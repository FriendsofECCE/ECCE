#!/usr/bin/env python3
"""Debye dialog exports equal the CLI for a molecule and periodic cell (#1009).

Each run calculates the initial view before switching to Debye, so old powder
reflections must disappear. Exit 77 without Xvfb or built binaries.
"""
import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "launch"))
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

CLI = os.path.join(ROOT, "scripts", "ecce_xrd.py")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--png")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built" % exe)
    import xdisplay

    s = harness.Session(build, "debye-dialog", {}, local=True)
    pngdir = args.png or os.path.join(s.state, "png")
    os.makedirs(pngdir, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    shutil.copy(CLI, os.path.join(s.home, "bin", "ecce-xrd"))
    os.chmod(os.path.join(s.home, "bin", "ecce-xrd"), 0o755)
    wrappers = os.path.join(s.state, "wrappers")
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
        for tag, periodic, source, doublet in (
                ("molecule", False, 0, 1),
                ("cell", True, 1, 0)):
            structure = os.path.join(s.state, tag + ".mvm")
            with open(structure, "w") as f:
                # MVM carries units and the optional cell without an import prompt.
                f.write("title: NaCl pair\ntype: molecule\n")
                if periodic:
                    f.write("lattice_vectors:\n 5.64 0 0\n 0 5.64 0\n 0 0 5.64\n")
                f.write("num_atoms: 2\natom_info: symbol cart\natom_list:\n"
                        "Na 0 0 0\nCl 2.82 0 0\nattr_list:\n"
                        " 0 1 0 0 0\n 0 1 0 0 0\natom_type_list:\n"
                        " 0 1\n 0 1\nnum_bonds: 0\n")
            pf, cf, xyz, ref = (os.path.join(s.state, tag + suffix)
                                for suffix in (".png", ".csv", "-dialog.xyz", "-cli.csv"))
            text, _ = bo.runScript(s, display, env, wrappers, [structure],
                                   ["wait 3000", "debye %s %s %s %d %d" %
                                    (pf, cf, xyz, source, doublet), "quit"],
                                   pngdir, "debye-" + tag)
            if not bo.check("BUILDER: debye: done" in text and os.path.exists(cf),
                            tag + ": Debye calculated and exported"):
                say(text[-2000:])
                continue
            bo.check("BUILDER: debye state: debye=1 powder=0 reflection=\n" in text,
                     tag + ": switch clears Bragg reflections")
            bo.check(('Lattice="' in open(xyz).read().splitlines()[1]) == periodic,
                     tag + ": exported structure preserves cell presence")
            bo.check("BUILDER: debye cursor: " in text and "Q = " in text and "I = " in text,
                     tag + ": cursor reports curve intensity and Q")
            bo.check(open(pf, "rb").read(8) == b"\x89PNG\r\n\x1a\n",
                     tag + ": PNG exported")
            cmd = [sys.executable, CLI, "debye", xyz, "--out", ref]
            if doublet:
                cmd += ["--source", "cu"]
            else:
                cmd += ["--wavelength", "0.7093"]
            r = subprocess.run(cmd, capture_output=True, text=True)
            bo.check(r.returncode == 0, tag + ": CLI runs on exported structure")
            bo.check(r.returncode == 0 and open(cf).read() == open(ref).read(),
                     tag + ": GUI CSV equals CLI CSV")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(bo.failures) if bo.failures else "PASSED")
    return 1 if bo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
