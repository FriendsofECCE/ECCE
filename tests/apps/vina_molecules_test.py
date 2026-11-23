#!/usr/bin/env python3
"""Vina Runtype Details lists the structure's ligand candidates (#1244).

A structure with a ligand, a smaller ligand-like residue, an additive and an
ion is stored as a Vina calculation.  The Calculation Editor writes the
dialog's <out>.molecules side file (label TAB atom range, likely ligand
first) and the dialog fills Ligand atoms with the first one.  Before any
Save the preview shows Vina's config, and pressing Runtype Details changes
the editor's controls and note once each (#1261).

    tests/apps/vina_molecules_test.py [--build build-cmake] [--keep]

Exit status 77 (skip) without Xvfb, vina or the build.
"""
import argparse
import os
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
from vina_viewer_test import ask  # noqa: E402

PDB = os.path.join(HERE, "fixtures", "select-protein-ligand.pdb")
WANT = ["LIG 101 (6 atoms)\t10-15", "LG2 102 (3 atoms)\t16-18",
        "GOL 103 (6 atoms)\t19-24", "NA 301 (1 atom)\t27"]
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def ask_org(cmds, logpath, proc, cmd, count, seconds=60):
    """One Organizer test-hook line; count = how often cmd was sent so far."""
    with open(cmds, "a") as handle:
        handle.write(cmd + "\n")
    want = "ECCE_TEST_ORGANIZER: %s: " % cmd
    end = time.time() + seconds
    while time.time() < end and proc.poll() is None:
        seen = [ln for ln in open(logpath, errors="replace") if ln.startswith(want)]
        if len(seen) >= count:
            return seen[count - 1][len(want):].strip()
        time.sleep(0.5)
    return None


def check_preview(s, url, display, tmpdir, wrappers):
    """The unsaved Vina calculation's preview is its config (#1261).  The
    Set-up area exists only in the one-process editor tab, so this drives
    the Organizer (one process, tabs) rather than the standalone calced."""
    home = os.path.join(s.state, "orghome")
    os.makedirs(os.path.join(home, ".ECCE"), exist_ok=True)
    with open(os.path.join(home, ".ECCE", "EcceGlobal"), "w") as prefs:
        prefs.write("WindowsAndTabs.Layout:\tnormal\n")
    cmds = os.path.join(s.state, "organizer.cmd")
    open(cmds, "w").close()
    logpath = os.path.join(s.state, "organizer.log")
    env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita", "ECCE_ONE_PROCESS": "1",
                 "ECCE_TEST_ORGANIZER": cmds, "ECCE_TMPDIR": tmpdir,
                 "ECCE_REALUSERHOME": home,
                 "PATH": wrappers + os.pathsep + s.env()["PATH"]})
    with open(logpath, "w") as log:
        proc = subprocess.Popen([os.path.join(wrappers, "ecce-organizer")], env=env,
                                cwd=os.path.join(s.home, "bin"), stdout=log,
                                stderr=subprocess.STDOUT, start_new_session=True)
        try:
            time.sleep(25)
            say("  launchbutton: %s" % ask_org(cmds, logpath, proc, "launchbutton " + url, 1, 60))
            time.sleep(20)
            say("  tabs: %s" % ask_org(cmds, logpath, proc, "tabs", 1, 30))
            say("  setup info: %s" % ask_org(cmds, logpath, proc, "setup info", 1, 30))
            ask_org(cmds, logpath, proc, "setup input", 1, 30)
            text = ""
            count = 0
            end = time.time() + 90
            while time.time() < end and "center_x" not in text:
                count += 1
                text = ask_org(cmds, logpath, proc, "setup text", count, 30) or ""
                time.sleep(2)
            check("no theory" not in text, "the preview does not say 'no theory': %r" % text[:300])
            check("receptor = receptor.pdbqt" in text and "center_x" in text,
                  "the preview shows Vina's config: %r" % text[:300])
        finally:
            if proc.poll() is None:
                try:
                    os.killpg(proc.pid, 15)
                    proc.wait(timeout=20)
                except (OSError, subprocess.TimeoutExpired):
                    pass


def check_details_open(cmdfile, logpath, proc, asked):
    """One press of Runtype Details changes the editor once (#1261)."""
    ask(cmdfile, logpath, proc, "watch start", asked, 30)
    ask(cmdfile, logpath, proc, "details runtype", asked, 30)
    time.sleep(10)
    got = ask(cmdfile, logpath, proc, "watch", asked, 30) or ""
    say("  watch: " + got[:600])
    for name in ("theory", "runtype", "theory-details", "runtype-details"):
        part = [p for p in got.split() if p.startswith(name + "=")]
        if not check(part != [], "watch reports %s" % name):
            continue
        off, on = part[0].split("=")[1].split("/")
        check(int(off[3:]) <= 1 and int(on[2:]) <= 1,
              "%s goes off and on at most once each: %s" % (name, part[0]))
    notes = [n.strip() for n in got.split(" |")[1:]]
    back = [n for i, n in enumerate(notes) if n in notes[:i]]
    check(not back, "the preview note does not swing back: %r" % notes)
    ask(cmdfile, logpath, proc, "close-details", asked, 30)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("vina", "Xvfb"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI + ("calced",):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "vinamol", {"Vina": shutil.which("vina")},
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
    tmpdir = os.path.join(s.state, "tmp")
    for d in (work, rundir, tmpdir):
        os.makedirs(d, exist_ok=True)

    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        name = "mol-%d" % int(time.time())
        stub = os.path.join(work, name + ".vinain")
        open(stub, "w").write("# placeholder\n")
        url, out = s.create(name, "vina_es", stub, "vina.vinain", rundir,
                            extra=("procs=2", "code=Vina", "pdb=" + PDB))
        if not check(url is not None, "calculation created"):
            say(out)
            return 1
        cmdfile = os.path.join(s.state, "calced.cmd")
        open(cmdfile, "w").close()
        logpath = os.path.join(s.state, "calced.log")
        asked = {}
        with xdisplay.Display() as display:
            env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita", "ECCE_VINA_TRACE": "1",
                         "ECCE_TEST_CALCED": cmdfile, "ECCE_TMPDIR": tmpdir,
                         "PATH": wrappers + os.pathsep + s.env()["PATH"]})
            with open(logpath, "w") as log:
                proc = subprocess.Popen([os.path.join(wrappers, "ecce-calced"),
                                         "-context", url], env=env,
                                        cwd=os.path.join(s.home, "bin"),
                                        stdout=log, stderr=subprocess.STDOUT,
                                        start_new_session=True)
                try:
                    time.sleep(12)
                    ask(cmdfile, logpath, proc, "runtype Docking (Vina)", asked)
                    mol = None
                    end = time.time() + 60
                    while time.time() < end and mol is None:
                        for root, _d, files in os.walk(tmpdir):
                            for f in files:
                                if f.endswith(".molecules"):
                                    mol = os.path.join(root, f)
                        time.sleep(1)
                    # The editor flushes line by line, so the file can be
                    # seen empty or half written; wait for the full list.
                    lines = []
                    end = time.time() + 30
                    while mol and time.time() < end and lines != WANT:
                        try:
                            lines = open(mol, encoding="utf-8").read().splitlines()
                        except OSError:
                            lines = []
                        time.sleep(0.5)
                    check(lines == WANT, "side file lists the candidates: %r" % lines)
                    # The dialog fills the key from the first line (it
                    # reports back over the editor's socket).
                    val = ""
                    end = time.time() + 40
                    while time.time() < end and "10-15" not in val:
                        val = ask(cmdfile, logpath, proc,
                                  "details ES.Runtype.Docking.LigandAtoms",
                                  asked, 20) or ""
                        time.sleep(2)
                    if not check("LigandAtoms=10-15" in val,
                                 "the first candidate fills Ligand atoms: %r" % val):
                        # The dialog's own trace of the default (stderr is
                        # the editor's, so it lands in calced.log).
                        for line in open(logpath, errors="replace"):
                            if "VINADLG" in line or "Traceback" in line or "Error" in line:
                                say("    " + line.rstrip())
                    check_details_open(cmdfile, logpath, proc, asked)
                    ask(cmdfile, logpath, proc, "quit", asked, 60)
                    time.sleep(5)
                    check_preview(s, url, display, tmpdir, wrappers)
                finally:
                    if proc.poll() is None:
                        try:
                            os.killpg(proc.pid, 15)
                            proc.wait(timeout=20)
                        except (OSError, subprocess.TimeoutExpired):
                            pass
    finally:
        s.stop()
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
