#!/usr/bin/env python3
"""Save of a protein-sized Vina calculation, timed per step (#964).

A generated 4710-atom structure (a 700-residue chain, waters and a 24-atom
ligand) is stored as a Vina calculation; the Calculation Editor, run with
ECCE_SAVE_TIMING=1, saves it.  The per-step times it prints are shown and the
whole Save must stay under LIMIT seconds; the deck and vina.vinafrag must be
stored.

    tests/apps/vina_save_timing_test.py [--build build-cmake] [--limit 30] [--keep]

Exit status 77 (skip) without Xvfb or the build.
"""
import argparse
import os
import random
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import vina_viewer_test as vv  # noqa: E402
from harness import say  # noqa: E402

NRES, NWAT = 700, 400
ATOMS = ("N", "CA", "C", "O", "CB")


def bigPdb(out):
    """Chain A (NRES residues of 5 atoms), HOH waters, then ligand LIG.
    Returns (total atoms, ligand range)."""
    rnd = random.Random(7)
    lines, n = [], 0

    def add(rec, name, res, resn, x, y, z, el):
        nonlocal n
        n += 1
        lines.append("%-6s%5d %-4s %3s A%4d    %8.3f%8.3f%8.3f  1.00  0.00"
                     "          %2s\n" % (rec, n, name, res, resn, x, y, z, el))

    for i in range(NRES):
        for k, a in enumerate(ATOMS):
            add("ATOM", a, "ALA", i + 1, 3.8 * (i % 40) + 1.2 * k,
                4.5 * (i // 40) + 0.3 * k, 1.1 * k, a[0])
    for w in range(NWAT):
        x, y, z = 60 + 3.1 * (w % 10), 3.1 * ((w // 10) % 10), 3.1 * (w // 100)
        add("HETATM", "O", "HOH", NRES + 1 + w, x, y, z, "O")
        add("HETATM", "H1", "HOH", NRES + 1 + w, x + .96, y, z, "H")
        add("HETATM", "H2", "HOH", NRES + 1 + w, x - .24, y + .93, z, "H")
    first = n + 1
    for k in range(24):
        add("HETATM", "C%d" % (k + 1), "LIG", NRES + NWAT + 1,
            20 + 1.4 * (k % 6), 10 + 1.4 * (k // 6), 3.0, "C")
    lines.append("END\n")
    open(out, "w").write("".join(lines))
    return n, "%d-%d" % (first, n)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--limit", type=float, default=30.0)
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb", path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    for exe in bo.GUI + ("calced",):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "vinasave", {"Vina": shutil.which("vina") or "/usr/bin/vina"},
                        local=True, keep=args.keep)
    for exe in bo.GUI + ("calced",):
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    gw = os.path.join(REPO, "packaging", "gateway")
    for entry in os.listdir(gw):
        if entry.startswith("ecce-"):
            harness.link(os.path.join(gw, entry), os.path.join(s.home, "bin", entry))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    pdb = os.path.join(work, "big.pdb")
    total, ligand = bigPdb(pdb)
    say("  %d atoms, ligand %s" % (total, ligand))
    display = None
    try:
        if not s.services(True):
            vv.check(False, "services came up")
            return 1
        name = "bigdock-%d" % int(time.time())
        stub = os.path.join(work, name + ".vinain")
        open(stub, "w").write("# placeholder\n")
        url, out = s.create(name, "vina_es", stub, "vina.vinain", rundir,
                            extra=("procs=2", "code=Vina", "pdb=" + pdb))
        if not vv.check(url is not None, "calculation created"):
            say(out)
            return 1
        cmdfile = os.path.join(s.state, "calced.cmd")
        open(cmdfile, "w").close()
        logpath = os.path.join(s.state, "calced.log")
        asked = {}
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "ECCE_TEST_CALCED": cmdfile, "ECCE_SAVE_TIMING": "1",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        t0 = None
        with open(logpath, "w") as log:
            proc = subprocess.Popen([os.path.join(wrappers, "ecce-calced"),
                                     "-context", url], env=env,
                                    cwd=os.path.join(s.home, "bin"),
                                    stdout=log, stderr=subprocess.STDOUT,
                                    start_new_session=True)
            try:
                time.sleep(12)
                vv.ask(cmdfile, logpath, proc, "runtype Docking (Vina)", asked)
                for key, val in (("ES.Runtype.Docking.LigandAtoms", ligand),
                                 ("ES.Runtype.Vina.BoxAuto", "1"),
                                 ("ES.Runtype.Vina.Seed", "7")):
                    vv.ask(cmdfile, logpath, proc, "detail %s %s" % (key, val), asked)
                t0 = time.time()
                say("    save -> %s" % vv.ask(cmdfile, logpath, proc, "save", asked, 900))
                elapsed = time.time() - t0
                vv.ask(cmdfile, logpath, proc, "quit", asked, 60)
            finally:
                if proc.poll() is None:
                    try:
                        os.killpg(proc.pid, 15)
                        proc.wait(timeout=20)
                    except (OSError, subprocess.TimeoutExpired):
                        pass
        for line in open(logpath, errors="replace"):
            if line.startswith("ECCE_SAVE_TIMING"):
                say("    " + line.strip())
        say("  Save took %.1f s" % elapsed)
        vv.check(elapsed < args.limit, "Save under %.0f s" % args.limit)

        def stored(fname):
            for root, _dirs, files in os.walk(s.localData()):
                if fname in files and name in root:
                    return open(os.path.join(root, fname), errors="replace").read()
            return ""
        vv.check("center_x" in stored("vina.vinain"), "the deck was stored")
        vv.check(re.search(r"^num_atoms:\s+%d$" % total, stored("vina.vinafrag"),
                           re.M) is not None, "vina.vinafrag was stored")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    return 1 if vv.failures else 0


if __name__ == "__main__":
    sys.exit(main())
