#!/usr/bin/env python3
"""Geometry Step Plots is listed only without a Geometry Trace (#1008).

Two calculations in the Builder's Properties list (detail mode):
calc-water-opt (NWChem optimisation, GEOMTRACE stored) must list Geometry
Trace and not Geometry Step Plots; the same calculation with its Props
replaced by those the GROMACS parse scripts give for the Trp-cage Dynamics
log (per-step vectors, no GEOMTRACE) must list Geometry Step Plots.  The
Builder script's "list <entry>" clicks the entry and says so when the list
has no such entry.

Exit status 77 (skip) without Xvfb or the built binaries.
"""
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import md_summary_test as md  # noqa: E402  (sets the tests/launch path)
import builder_overlay_test as bo  # noqa: E402
import energies_groups_test as eg  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

failures = []


def check(ok, what):
    say(("ok   " if ok else "FAIL ") + what)
    if not ok:
        failures.append(what)
    return ok


def listed(out, entry):
    """True/False from the script's report on "list <entry>", None if absent."""
    for line in out.splitlines():
        if "ECCE_BUILDER_SCRIPT: list %s:" % entry in line:
            return line.rstrip().endswith(": ok")
    return None


def main():
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(md.REPO, "build-cmake"))
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    work = tempfile.mkdtemp(prefix="stepplots-")
    fixture = os.path.join(HERE, "fixtures", "calc-water-opt")
    shutil.copytree(fixture, os.path.join(work, "calc-opt"), symlinks=True)
    shutil.copytree(fixture, os.path.join(work, "calc-md"), symlinks=True)
    md.writeProps(os.path.join(work, "calc-md"), md.parsedProps(md.LOG))

    s = harness.Session(build, "stepplots", {}, local=True)
    png = os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        for name, want in (("calc-opt", {"Geometry Trace": True,
                                         "Geometry Step Plots": False}),
                           ("calc-md", {"Geometry Step Plots": True})):
            url = eg.install(s, name, work)
            lines = ["wait 4000"] + ["list " + e for e in want] + ["quit"]
            out, _shots = bo.runScript(s, display, env, wrappers,
                                       ["-context", url], lines, png, name,
                                       {"ECCE_PANEL_MODE": "detail"})
            for entry, expected in want.items():
                got = listed(out, entry)
                if not check(got is expected, "%s: %s %s" % (
                        name, entry, "listed" if expected else "not listed")):
                    say("  report: %r" % got)
                    say(out[-2000:])
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
        shutil.rmtree(work, ignore_errors=True)
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
