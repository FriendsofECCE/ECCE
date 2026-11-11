#!/usr/bin/env python3
"""Vina docking made in the Calculation Editor, box and poses in the Builder (#455).

1IEP chain A with imatinib (STI) is stored as a Vina calculation.  The
Builder's Docking Box Around Selection sets the box from the selected ligand
and draws it; the Calculation Editor, driven through ECCE_TEST_CALCED, takes
the box, sets the ligand and the search and saves, so the run type and the
deck are the editor's own.  The run goes through Launch.  The Builder then
opens the Poses panel: Vina score label, no Boltzmann population, one
thumbnail per pose, the pocket view, the box drawn; the window is
photographed on a white background.

    tests/apps/vina_viewer_test.py [--build build-cmake] [--pdb 1iep.pdb]
                                   [--png DIR] [--keep]

Exit status 77 (skip) without vina, Xvfb, import, the PDB entry or the build.
"""
import argparse
import os
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
from harness import say  # noqa: E402

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def complexPdb(pdb, out):
    """Chain A protein then the STI ligand; returns the ligand's atom range."""
    rec, lig = [], []
    for line in open(pdb):
        if len(line) < 22 or line[21] != "A":
            continue
        if line.startswith("ATOM"):
            rec.append(line)
        elif line.startswith("HETATM") and line[17:20] == "STI":
            lig.append(line)
    with open(out, "w") as f:
        f.writelines(rec + lig + ["END\n"])
    return len(rec) + 1, len(rec) + len(lig)


def ask(cmdfile, logpath, proc, cmd, asked, seconds=180):
    with open(cmdfile, "a") as handle:
        handle.write(cmd + "\n")
    asked[cmd] = asked.get(cmd, 0) + 1
    want = "ECCE_TEST_CALCED: %s: " % cmd
    end = time.time() + seconds
    while time.time() < end and proc.poll() is None:
        seen = 0
        for line in open(logpath, errors="replace"):
            if line.startswith(want):
                seen += 1
                if seen == asked[cmd]:
                    return line[len(want):].strip()
        time.sleep(0.5)
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--pdb", default=os.path.join(REPO, "tests", "dock", "fixtures", "1iep.pdb"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("vina", "Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    if not os.path.exists(args.pdb):
        harness.skip("the PDB entry %s is not here" % args.pdb)
    pdbqt = os.environ.get("ECCE_PDBQT") or os.path.join(build, "ecce-pdbqt")
    if not os.access(pdbqt, os.X_OK):
        harness.skip("ecce-pdbqt is not built in %s" % build)
    harness.prerequisites(build, ())
    for exe in bo.GUI + ("calced",):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    os.environ["ECCE_PDBQT"] = pdbqt
    import xdisplay

    s = harness.Session(build, "vinaview", {"Vina": shutil.which("vina")},
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
    cplx = os.path.join(work, "1iep-a.pdb")
    first, last = complexPdb(args.pdb, cplx)
    ligand = "%d-%d" % (first, last)
    say("  ligand atoms %s" % ligand)

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        name = "imatinib-%d" % int(time.time())
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
                say("    runtype -> %s" % ask(cmdfile, logpath, proc,
                                              "runtype Docking (Vina)", asked))
                for key, val in (("ES.Runtype.Docking.LigandAtoms", ligand),
                                 ("ES.Runtype.Vina.BoxAuto", "1"),
                                 ("ES.Runtype.Vina.BoxMargin", "5"),
                                 ("ES.Runtype.Vina.Exhaustiveness", "4"),
                                 ("ES.Runtype.Vina.NumPoses", "4"),
                                 ("ES.Runtype.Vina.Seed", "7")):
                    ask(cmdfile, logpath, proc, "detail %s %s" % (key, val), asked)
                say("    save -> %s" % ask(cmdfile, logpath, proc, "save", asked, 300))
                ask(cmdfile, logpath, proc, "quit", asked, 60)
            finally:
                if proc.poll() is None:
                    try:
                        os.killpg(proc.pid, 15)
                        proc.wait(timeout=20)
                    except (OSError, subprocess.TimeoutExpired):
                        pass
        def stored(fname):
            for root, _dirs, files in os.walk(s.localData()):
                if fname in files and name in root:
                    return open(os.path.join(root, fname), errors="replace").read()
            return ""
        deck = stored("vina.vinain")
        check("center_x" in deck and "ligand" in deck,
              "the editor's Save wrote a real Vina config")
        check(re.search(r"^#\| RECEPTOR_ATOMS=1-\d+", deck, re.M) is not None and
              "#| LIGAND_ATOMS=%s\n" % ligand in deck,
              "receptor and ligand atoms are ranges in the deck")
        structure = stored("vina.vinafrag")
        check(re.search(r"^num_atoms:\s+%d$" % last, structure, re.M) is not None and
              structure.count("\n") > last, "the structure is stored as vina.vinafrag")

        rc, out = s.launch(url)
        check(rc == 0, "Launch ran to the end")
        # Launch copies the deck to its cache and stores that copy back.
        check(stored("vina.vinain") == deck, "Launch stored the deck back unchanged")
        state = s.waitState(url, seconds=2400)
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
             "poses select 1",
             "selectatoms " + ligand,
             "vinabox 5",
             "vinaboxinfo",
             "poses pocket off",
             "poses pocket on",
             "wait 3000",
             "poses info",
             "shot pocket",
             "poses pocket off",
             "quit"], png, "vina",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Poses"})
        for line in text.splitlines():
            if "poses info:" in line or "vinabox" in line or "selectatoms" in line:
                say("    " + line.strip()[:200])
        infos = re.findall(r"poses info: (.*)", text)
        check("no property panel named" not in text, "the Builder offers a Poses panel")
        check(bool(infos) and "population=0" in infos[0],
              "no Boltzmann population column for Vina")
        check(bool(infos) and "Vina score" in infos[0],
              "the score column is labelled Vina score")
        if len(infos) > 1:
            thumbs = int(re.search(r"thumbs=(\d+)", infos[-1]).group(1))
            check(thumbs > 0, "%d pose thumbnails rendered" % thumbs)
            pocket = re.search(r"ligand=(\d+) pocket=(\d+) hidden=(\d+)", infos[-1])
            check(bool(pocket) and int(pocket.group(2)) > 0 and int(pocket.group(3)) > 0,
                  "pocket view: %s" % (pocket.group(0) if pocket else "absent"))
            # The camera frames the pocket and the box, not the hidden receptor.
            fit = re.search(r"fit=([\d.]+)", infos[-1])
            check(bool(fit) and 0 < float(fit.group(1)) < 60,
                  "pocket view framed: %s A across" % (fit.group(1) if fit else "absent"))
        box = re.findall(r"vinaboxinfo: (.*)", text)
        check(bool(box) and box[0].startswith("drawn=1"),
              "the search box is drawn: %s" % (box[0] if box else "absent"))
        check("pocket" in shots, "the window was photographed")
        if "pocket" in shots:
            say("  screenshot: %s" % shots["pocket"])
        check("segmentation" not in text.lower(), "no crash in the Builder log")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
