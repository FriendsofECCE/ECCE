#!/usr/bin/env python3
"""The Poses panel on a protein-sized docking result (#966) and its cell (#968).

The whole of 1IEP (both chains, the crystal waters and the CRYST1 line, about
4700 atoms) with imatinib as the ligand: the small fixtures of
`vina_viewer_test.py` (one chain, no waters) hid how the panel scales.  The
Builder opens the Poses panel with ECCE_PANEL_TIMING=1, and the per-part
timings printed on stderr are collected here, so the cost of the thumbnails,
the pocket view and the camera fit is measured, not guessed.

The same run answers #968: a crystallographic file carries CRYST1, and the
structure of a docking calculation must not become periodic from it, so
`fragdump` must report no lattice and no cell drawn, and the screenshot must
show the pocket, not a cell with a speck in it.

    tests/apps/vina_poses_large_test.py [--build build-cmake] [--pdb 1iep.pdb]
                                        [--png DIR] [--budget 10000] [--keep]

Exit status 77 (skip) without vina, Xvfb, import, the build or the PDB entry
(fetched from RCSB once into tests/dock/fixtures and kept).
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import vina_viewer_test as vv  # noqa: E402
from harness import say  # noqa: E402

FIXTURE = os.path.join(REPO, "tests", "dock", "fixtures", "1iep.pdb")
URL = "https://files.rcsb.org/download/1IEP.pdb"

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def fixture(path):
    """The PDB entry, fetched once and kept; None when it cannot be had."""
    if os.path.exists(path):
        return path
    os.makedirs(os.path.dirname(path), exist_ok=True)
    try:
        with urllib.request.urlopen(URL, timeout=60) as src:
            text = src.read().decode("latin-1")
    except Exception as exc:                       # offline, proxy, RCSB down
        say("  cannot fetch %s: %s" % (URL, exc))
        return None
    open(path, "w").write(text)
    return path


def wholeComplex(pdb, out):
    """Every chain, the waters and CRYST1; chain A's imatinib last.

    Returns (atom count, first, last) of the ligand's 1-based range.
    """
    cryst, rec, lig = "", [], []
    for line in open(pdb, errors="replace"):
        if line.startswith("CRYST1"):
            cryst = line
        elif line.startswith("ATOM"):
            rec.append(line)
        elif line.startswith("HETATM"):
            if line[17:20] == "STI" and line[21] == "A":
                lig.append(line)
            elif line[17:20] != "STI":           # waters, ions
                rec.append(line)
    with open(out, "w") as f:
        f.writelines(([cryst] if cryst else []) + rec + lig + ["END\n"])
    return len(rec) + len(lig), len(rec) + 1, len(rec) + len(lig)


def timings(text):
    """{part: [ms, ...]} from the ECCE_PANEL_TIMING lines."""
    out = {}
    for part, ms in re.findall(r"\[panel-timing\] (\S+?)(?:\(\d+\))? ([\d.]+) ms", text):
        out.setdefault(part, []).append(float(ms))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--pdb", default=FIXTURE)
    ap.add_argument("--png")
    ap.add_argument("--budget", type=float, default=10000.0,
                    help="ms the panel may take to open (default 10000)")
    ap.add_argument("--energy-range", type=float, default=1000.0,
                    help="keep every mode Vina finds, so the panel gets a full "
                         "pose set at a low search effort")
    ap.add_argument("--exhaustiveness", type=int, default=2,
                    help="Vina search effort; the panel, not the docking, is measured")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("vina", "Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    pdb = fixture(args.pdb)
    if not pdb:
        harness.skip("the PDB entry %s is not here and cannot be fetched" % args.pdb)
    pdbqt = os.environ.get("ECCE_PDBQT") or os.path.join(build, "ecce-pdbqt")
    if not os.access(pdbqt, os.X_OK):
        harness.skip("ecce-pdbqt is not built in %s" % build)
    harness.prerequisites(build, ())
    for exe in bo.GUI + ("calced",):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    os.environ["ECCE_PDBQT"] = pdbqt
    import xdisplay

    s = harness.Session(build, "vinalarge", {"Vina": shutil.which("vina")},
                        local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
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
    cplx = os.path.join(work, "1iep-all.pdb")
    natoms, first, last = wholeComplex(pdb, cplx)
    ligand = "%d-%d" % (first, last)
    say("  %d atoms, ligand %s" % (natoms, ligand))
    check(natoms > 4000, "the receptor is protein-sized (%d atoms)" % natoms)

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        name = "imatinib-large-%d" % int(time.time())
        stub = os.path.join(work, name + ".vinain")
        open(stub, "w").write("# placeholder; the editor writes the real deck\n")
        url, out = s.create(name, "vina_es", stub, "vina.vinain", rundir,
                            extra=("procs=2", "code=Vina", "pdb=" + cplx))
        if not check(url is not None, "calculation created"):
            say(out)
            return 1

        cmdfile = os.path.join(s.state, "calced.cmd")
        open(cmdfile, "w").close()
        logpath = os.path.join(s.state, "calced.log")
        asked = {}
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "ECCE_TEST_CALCED": cmdfile,
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
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
                                 ("ES.Runtype.Vina.BoxMargin", "5"),
                                 ("ES.Runtype.Vina.Exhaustiveness", str(args.exhaustiveness)),
                                 ("ES.Runtype.Vina.NumPoses", "9"),
                                 ("ES.Runtype.Vina.EnergyRange",
                                  str(args.energy_range)),
                                 ("ES.Runtype.Vina.Seed", "7")):
                    vv.ask(cmdfile, logpath, proc, "detail %s %s" % (key, val), asked)
                say("    save -> %s" % vv.ask(cmdfile, logpath, proc, "save", asked, 600))
                vv.ask(cmdfile, logpath, proc, "quit", asked, 60)
            finally:
                if proc.poll() is None:
                    try:
                        os.killpg(proc.pid, 15)
                        proc.wait(timeout=20)
                    except (OSError, subprocess.TimeoutExpired):
                        pass

        rc, out = s.launch(url)
        check(rc == 0, "Launch ran to the end")
        state = s.waitState(url, seconds=5400)
        if not check(state == "completed", "reached completed (last: %s)" % state):
            say(s.driver("reason", url)[1])
            return 1
        time.sleep(3)
        props = s.props(url)
        check("POSESCORE" in props and "GEOMTRACE" in props,
              "poses and frames stored: " + " ".join(props))

        benv = dict(env)
        benv.pop("ECCE_TEST_CALCED", None)
        text, shots = bo.runScript(
            s, display, benv, wrappers, ["-context", url],
            ["wait 8000",
             "background 1 1 1",
             "poses info",
             # The thumbnails are made two per timer tick: let them finish, so
             # the measurement covers every per-pose part of the open.
             "wait 20000",
             "poses info",
             "poses pocket on",
             "wait 4000",
             "poses info",
             "shot poses",
             # Stepping the poses is what the hand test did: each step
             # restyles the pocket and steps the trace.
             ] + sum([["poses select %d" % k, "wait 1500"] for k in range(1, 10)], []) + [
             "poses info",
             "shot pose2",
             "quit"], png, "vinalarge",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Poses",
             "ECCE_PANEL_TIMING": "1"})

        open(os.path.join(png, "builder.txt"), "w").write(text)
        parts = timings(text)
        say("  panel timings (ms, protein-sized system):")
        for part in sorted(parts, key=lambda p: -sum(parts[p])):
            vals = parts[part]
            say("    %-28s n=%-3d total=%8.1f  max=%8.1f" %
                (part, len(vals), sum(vals), max(vals)))
        check(bool(parts), "the panel printed its timings")
        # Opening the panel is `initialize` plus the first selection; the
        # later ones are the cost of stepping a pose, reported separately.
        opened = sum(parts.get("initialize", [0])) + max(parts.get("updateSelection", [0]))
        steps = parts.get("updateSelection", [])
        check(opened < args.budget,
              "the panel opens in %.0f ms (budget %.0f)" % (opened, args.budget))
        check(not steps or max(steps) < args.budget / 2,
              "stepping a pose costs at most %.0f ms" % (max(steps) if steps else 0))

        # #968: a crystallographic file must not make a docking structure
        # periodic, or the cell is drawn and frames the camera.
        # The Builder has no cell hook, so the stored structure is the check:
        # a lattice there is drawn and frames the camera (#968).
        frag = ""
        for root, _dirs, files in os.walk(s.localData()):
            if "vina.vinafrag" in files and name in root:
                frag = open(os.path.join(root, "vina.vinafrag"), errors="replace").read()
        check(frag and "lattice_vectors" not in frag,
              "the docking structure is not periodic (no cell from CRYST1)")

        infos = re.findall(r"poses info: (.*)", text)
        fit = re.search(r"fit=([\d.]+)", infos[-1]) if infos else None
        check(bool(fit) and 0 < float(fit.group(1)) < 60,
              "pocket framed on the atoms, not the cell: %s" %
              (fit.group(1) if fit else "absent"))
        for shot in ("poses", "pose2"):
            if shot in shots:
                say("  screenshot: %s" % shots[shot])
        check("poses" in shots, "the window was photographed")
        check("segmentation" not in text.lower(), "no crash in the Builder log")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
