#!/usr/bin/env python3
"""The classic ECCE NWChem MD workflow, headless: Prepare, Optimize, Energy, Dynamics.

Cytidine (the Structure Library's RNA monomer) goes into an NWChem MD study.
Prepare is edited through MDPrep's test hook (amber, SPC/E water, Cl- ions,
5 nm box, periodic solvation), the other tasks through MDEdBase's; each is
launched and run by nwchem.  Optimize and Dynamics use fewer steps than the
classic guides to keep the run short.

    tests/launch/nwchem_md_walk_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without nwchem, Xvfb or the built binaries.
"""

import argparse
import glob
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402
from gromacs_test import Editor, check, failures  # noqa: E402

CYTIDINE = os.path.join(REPO, "data", "client", "StructureLibrary", "RNA_Bases",
                        "Monomer", "Cytidine.mvm")
NWDATA = "/usr/share/nwchem"
STEPS = {"optimize": "200", "dynamics": "500"}


def where(u):
    return u[len("file://"):].rstrip("/")


def tail_files(task):
    out = []
    for sub in ("Inputs", "Outputs"):
        out += sorted(os.listdir(os.path.join(where(task), sub))) \
            if os.path.isdir(os.path.join(where(task), sub)) else []
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    nwchem = shutil.which("nwchem")
    if not nwchem or not os.path.isdir(os.path.join(NWDATA, "amber_s")):
        harness.skip("nwchem and its force-field data are not installed")
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    exes = ("mdprepare", "mdoptimize", "mdenergy", "mddynamics")
    for exe in exes:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "nwmdwalk", {"NWChem": nwchem}, local=True,
                        keep=args.keep)
    for exe in exes:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    s.registerMachine("localhost", "Shell", {"NWChem": nwchem})
    rundir = os.path.join(s.state, "run")

    # The Builder's force-field configuration: amber parameters and segments
    with open(os.path.join(s.state, ".ECCE", "Builder"), "w") as h:
        h.write("FFTYPE: NWChem\n")
        h.write("NWChemFF: \\,%s/amber_s/amber.par,%s/amber_x/amber.par\n" % (NWDATA, NWDATA))
        h.write("NWChemSEG: \\,%s/amber_s,%s/amber_x\n" % (NWDATA, NWDATA))

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        display = xdisplay.Display().__enter__()
        cmddir = os.path.join(s.state, "mded")
        os.makedirs(cmddir)
        genv = s.env({"DISPLAY": display.name, "ECCE_TEST_MDED": cmddir,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})

        rc, out = s.driver("nwchemmdstudy", s.userUrl(), "cytidine",
                           "frag=" + CYTIDINE, "all")
        urls = [l.rstrip("/") + "/" for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and len(urls) == 5, "an NWChem MD study with four tasks: %s"
                     % out.strip().replace("\n", " | ")[:300]):
            return 1
        tasks = dict(zip(("study", "prepare", "optimize", "energy", "dynamics"), urls))
        for kind in ("prepare", "optimize", "energy", "dynamics"):
            rc, out = s.driver("mdsetup", tasks[kind], "localhost", rundir, s.user())
            check(rc == 0, "launch settings for %s" % kind)

        # ---- Prepare ---------------------------------------------------
        say("Prepare")
        ed = Editor(s, display, genv, wrappers, cmddir, "mdprepare", "MDPrepare")
        try:
            if not ed.up():
                return 1
            check(ed.cmd("open " + tasks["prepare"]) == "ok", "the Prepare opens")
            say("  title: %s" % ed.cmd("title"))
            for line in ("ci Cl 1 1.0 24 0.2", "solvent water", "center", "orient",
                         "box 5.0 5.0 5.0", "solvate", "periodic"):
                check(ed.cmd(line) == "ok", line)
            cmds = ed.cmd("cmds")
            say("  commands: %s" % cmds)
            check("box 5.000000 5.000000 5.000000" in cmds, "the box is 5 nm: %s" % cmds)
            check(ed.cmd("save") == "ok", "Save")
            deck = glob.glob(os.path.join(where(tasks["prepare"]), "Inputs", "*.nw*"))
            text = open(deck[0]).read() if deck else ""
            say("  deck: %s" % text.replace("\n", " | "))
            for want in ("solvent HOH model spce", "solvate", "periodic", "write chemsys.pdb"):
                check(want in text, "the deck has: %s" % want)
        finally:
            ed.close()
        rc, out = s.launch(tasks["prepare"])
        check(rc == 0, "Launch accepted the Prepare: %s" % out.strip()[-120:])
        state = s.waitState(tasks["prepare"], 900)
        check(state in ("completed", "loaded"), "Prepare ran to %s" % state)
        say("  files: %s" % tail_files(tasks["prepare"]))
        pdbs = glob.glob(os.path.join(where(tasks["prepare"]), "Outputs", "*.pdb"))
        natoms = 0
        if pdbs:
            natoms = sum(1 for l in open(pdbs[0], errors="replace")
                         if l.startswith(("ATOM", "HETATM")))
        say("  atoms after solvation: %d" % natoms)
        check(natoms > 1000, "the solvated system holds water: %d atoms" % natoms)

        # ---- Optimize, Energy, Dynamics --------------------------------
        for kind, binary, app in (("optimize", "mdoptimize", "MDOptimize"),
                                  ("energy", "mdenergy", "MDEnergy"),
                                  ("dynamics", "mddynamics", "MDDynamics")):
            say(kind)
            ed = Editor(s, display, genv, wrappers, cmddir, binary, app)
            try:
                if not ed.up():
                    return 1
                check(ed.cmd("open " + tasks[kind]) == "ok", "the %s opens" % kind)
                say("  title: %s | pages: %s" % (ed.cmd("title"), ed.cmd("tabs")))
                if kind in STEPS:
                    check(ed.cmd("set-steps " + STEPS[kind]) == "ok", "%s steps" % STEPS[kind])
                check(ed.cmd("save") == "ok", "Save")
                check(ed.cmd("launch") == "enabled", "Launch is enabled")
            finally:
                ed.close()
            deck = glob.glob(os.path.join(where(tasks[kind]), "Inputs", "*.nw*"))
            say("  deck: %s" % (open(deck[0]).read().replace("\n", " | ") if deck else None))
            rc, out = s.launch(tasks[kind])
            check(rc == 0, "Launch accepted the %s: %s" % (kind, out.strip()[-120:]))
            state = s.waitState(tasks[kind], 1500)
            check(state in ("completed", "loaded"), "%s ran to %s" % (kind, state))
            say("  properties: %s" % s.props(tasks[kind]))
            say("  files: %s" % tail_files(tasks[kind]))
    finally:
        if display is not None:
            display.__exit__(None, None, None)
        leftovers = s.stop()
        if leftovers:
            say("left running: %s" % leftovers)
    say("%d failure(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
