#!/usr/bin/env python3
"""GROMACS run settings: "Restrain chain <id>" boxes, in the real MD editor.

A two-chain system is prepared with MERGE=0 (the real md.gmxprep, gensub and
pdb2gmx).  Its topology is attached to an Equilibrate task and the Run
settings page is read and driven through the editor's test hook:

  * with no topology attached the page shows no chain boxes;
  * with the prepared topology there is one box per chain (A, B);
  * ticking only chain B writes -DPOSRES_B after what was typed in Defines,
    Save writes it on the .mdp define line, and grompp restrains exactly the
    atoms of chain B's posre file (counted from that file, not from ours);
  * unticking takes the flag out and leaves the typed flags alone.

    tests/launch/gromacs_chain_posres_test.py [--build build-cmake] [--png FILE]

Exit status 77 (CTest SKIP) without gmx, Xvfb or the built binaries.
"""
import argparse
import os
import re
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402
import gromacs_test as gt  # noqa: E402

# tests/apps has a run_tests of its own: load the gromacs_md modules apart.
sys.path.insert(0, os.path.join(REPO, "tests", "gromacs_md"))
_apps_rt = sys.modules.pop("run_tests", None)
import run_tests as rt  # noqa: E402
import prepare_options_test as po  # noqa: E402
if _apps_rt is not None:
    sys.modules["run_tests"] = _apps_rt

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def itp_atoms(path):
    n, on = 0, False
    for l in open(path):
        s = l.strip()
        if s.startswith("["):
            on = s.replace(" ", "") == "[position_restraints]"
        elif on and s and not s.startswith((";", "#")):
            n += 1
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    gmx = shutil.which("gmx")
    if not gmx:
        harness.skip("gmx is not installed")
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in gt.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    base = os.path.expanduser("~/.cache")
    os.makedirs(base, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix="gmx-chain-posres-", dir=base)
    s = harness.Session(build, "gmxchain", {"GROMACS": gmx}, local=True)
    ed = display = None
    try:
        pdb = os.path.join(tmp, "two.pdb")
        po.two_chain_pdb(pdb)
        d = rt.prepare(gmx, rt.Home(tmp, gmx), tmp, "two",
                       "SOURCE=protein\nFF=amber99sb-ildn\nWATER=tip3p\nBOX=cubic\n"
                       "DIST=1.0\nMERGE=0\nCONC=0\n", pdb)
        if not check(d is not None, "two chains prepared"):
            return 1
        atoms = {c: itp_atoms(os.path.join(d, "posre_Protein_chain_%s.itp" % c)) for c in "AB"}
        check(atoms["A"] > 0 and atoms["B"] > 0, "each chain has restraint atoms: %s" % atoms)

        for exe in gt.GUI:
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
        s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
        if not check(s.services(True), "services came up"):
            return 1
        display = xdisplay.Display().__enter__()
        cmddir = os.path.join(s.state, "mded")
        os.makedirs(cmddir)
        genv = s.env({"DISPLAY": display.name, "ECCE_TEST_MDED": cmddir,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        rc, out = s.driver("gromacsstudy", s.userUrl(), "chains")
        urls = [l for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and len(urls) == 4, "a GROMACS study was created"):
            return 1
        eq = urls[2].rstrip("/") + "/"

        ed = gt.Editor(s, display, genv, wrappers, cmddir, "mddynamics", "MDDynamics")
        if not ed.up():
            return 1
        check(ed.cmd("open " + eq) == "ok", "the Equilibrate task opens")
        check("chains=none" in ed.cmd("run"), "no topology: no chain boxes: %s" % ed.cmd("run"))

        out = ed.cmd("attach-gro " + os.path.join(d, "ion.gro"))
        check(out.startswith("ok"), "structure attached: %s" % out)
        out = ed.cmd("attach-top " + os.path.join(d, "topol.top"))
        check(out.startswith("ok"), "topology attached: %s" % out)
        out = ed.cmd("run")
        check("chains=A:0,B:0" in out, "one box per chain, none ticked: %s" % out)

        check(ed.cmd("run-set defines -DFLEXIBLE") == "ok", "defines typed")
        check(ed.cmd("run-set chain:B 1") == "ok", "chain B ticked")
        out = ed.cmd("run")
        check("chains=A:0,B:1" in out and "defines=-DFLEXIBLE -DPOSRES_B " in out,
              "only B, appended to what was typed: %s" % out)
        check(ed.cmd("run-set chain:C 1") == "no such chain", "an unknown chain is refused")
        check(ed.cmd("save") == "ok", "Save")
        mdp = open(os.path.join(eq.replace("file://", "").rstrip("/"), "Inputs",
                                "gromacs.mdp")).read()
        m = re.search(r"^define\s*=\s*(.*)$", mdp, re.M)
        define = m.group(1).strip() if m else ""
        check(define == "-DFLEXIBLE -DPOSRES_B", "the .mdp define line: %r" % define)
        got = po.posre_blocks(gmx, d, define)
        check(got == (1, atoms["B"]),
              "grompp restrains chain B's %d atoms only: %s" % (atoms["B"], got))

        if args.png:
            ed.cmd("size 900 760")
            ed.cmd("tab Run settings")
            time.sleep(1)
            check(ed.cmd("snap " + args.png) == "saved", "shot %s" % args.png)

        check(ed.cmd("run-set chain:B 0") == "ok" and ed.cmd("run-set chain:A 1") == "ok",
              "B unticked, A ticked")
        out = ed.cmd("run")
        check("chains=A:1,B:0" in out and "defines=-DFLEXIBLE -DPOSRES_A " in out,
              "the flag moved and -DFLEXIBLE stayed: %s" % out)
    finally:
        if ed:
            ed.close()
        if display:
            display.__exit__(None, None, None)
        s.stop()
        shutil.rmtree(tmp, ignore_errors=True)
    say("FAILED: %s" % failures if failures else "OK")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
