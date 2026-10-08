#!/usr/bin/env python3
"""
The Quantum ESPRESSO tutorial (help/src/qe-first-calculation.md), headless.

Replays every step of the two tutorial calculations in the real apps, each
one through the handler a click or Enter in the same control reaches
(ECCE_TEST_ORGANIZER, ECCE_BUILDER_SCRIPT, ECCE_CALCED_SCRIPT,
ECCE_LAUNCHER_SCRIPT -- no synthetic input), runs pw.x for real through the
Launcher and the job monitor, and checks the results the Viewer holds
against a pw.x run of a deck written here by hand:

  si-scf     bulk silicon, Fd-3m with one Si atom, a = 5.43 A, PBE, scf
  h2o-relax  a water molecule in a 10 A box, relax; geometry trace

    tests/apps/qe_walkthrough_test.py [--case si-scf|h2o-relax] [--local]

QE_TUTORIAL_PNGS=<dir> keeps a PNG of every step there (those are the
screenshots of the help page).  Same installed tree, isolation and services
as run_tests.py (ECCE_TEST_HOME, ECCE_TEST_WRAPPERS).  Exit 77 (skip)
without an install, Xvfb, pw.x or the SSSP pseudopotentials.
"""

import argparse
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402

PSEUDO = "/usr/share/espresso/pseudo"
RY_EV = 13.605693122994
BOHR = 0.529177210903

failures = []


def say(text):
    print(text, flush=True)


def fail(what):
    failures.append(what)
    say("  FAIL %s" % what)


def ok(what):
    say("  ok   %s" % what)


def xdo(display, *argv):
    return subprocess.run(["xdotool"] + list(argv), env=display.env(),
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT).stdout.decode().strip()


def windowSize(display, wid):
    out = subprocess.run(["xwininfo", "-display", display.name, "-id", wid],
                         stdout=subprocess.PIPE).stdout.decode()
    w = h = 0
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Width:"):
            w = int(line.split()[1])
        elif line.startswith("Height:"):
            h = int(line.split()[1])
    return w, h


class Shots(object):
    """Photographs the windows of one running app when its script asks."""

    def __init__(self, display, outdir, tmp):
        self.display, self.outdir, self.tmp = display, outdir, tmp

    def windowsOf(self, known):
        found = []
        for wid, name in self.display.windows():
            if wid in known or not name:
                continue
            w, h = windowSize(self.display, wid)
            if w > 60 and h > 60:
                found.append((wid, name, w, h))
        return found

    def take(self, wid, dest):
        full = os.path.join(self.tmp, os.path.basename(dest))
        subprocess.run(["import", "-display", self.display.name, "-window",
                        wid, full], check=True)
        subprocess.run(["convert", full, "-strip", dest], check=True)
        if shutil.which("optipng"):
            subprocess.run(["optipng", "-quiet", "-o3", dest], check=False)


def runApp(display, shots, wrapper, args, env, script, size, names,
           timeout=240, title=None, sizeDelay=0):
    """Run `wrapper` with a script; photograph at each `shot NAME` line.

    names maps a shot name to the PNG file name, or None for no PNG.  Returns
    (exit status or None if killed, log text)."""
    scriptDir = tempfile.mkdtemp(prefix="qe-script-", dir=shots.tmp)
    scriptFile = os.path.join(scriptDir, "script")
    with open(scriptFile, "w") as handle:
        handle.write("\n".join(script) + "\n")
    environment = display.env()
    environment.update(env)
    environment[env["_HOOK"]] = scriptFile
    del environment["_HOOK"]
    if environment.get("ECCE_VIEWER_SCENE_OUT") == "@":
        environment["ECCE_VIEWER_SCENE_OUT"] = scriptDir
    environment["ECCE_TRANSPARENCY_FALLBACK_MS"] = "0"
    environment["OMP_NUM_THREADS"] = "2"
    known = set(w for w, _ in display.windows())
    log = open(os.path.join(scriptDir, "log"), "w")
    command = os.path.join(apps.WRAPPERS, wrapper)
    if os.environ.get("QE_GDB") and wrapper == "ecce-builder":
        #  A copy of the wrapper that starts the binary under gdb.
        patched = os.path.join(scriptDir, "builder-gdb")
        text = open(command).read().replace(
            '{ "$ECCE_HOME/bin/builder"',
            '{ gdb -batch -ex "set disable-randomization off" -ex run -ex "bt 30" --args "$ECCE_HOME/bin/builder"')
        open(patched, "w").write(text)
        os.chmod(patched, 0o755)
        command = patched
    proc = subprocess.Popen([command] + list(args),
                            env=environment, stdout=log,
                            stderr=subprocess.STDOUT, start_new_session=True)
    sized = None
    end = time.time() + timeout
    status = None
    finished = False
    while time.time() < end and not finished:
        status = proc.poll()
        if status is not None:
            break
        if sized is None:
            main = [w for w in shots.windowsOf(known)
                    if title is None or w[1].startswith(title)]
            if main:
                sized = max(main, key=lambda w: w[2] * w[3])[0]
                time.sleep(sizeDelay)
                xdo(display, "windowmove", sized, "0", "0")
                xdo(display, "windowsize", sized, str(size[0]), str(size[1]))
        for name in list(names):
            marker = name if name.endswith(".ppm") else name + ".ready"
            if os.path.exists(os.path.join(scriptDir, marker)):
                if sized and name.endswith(".ppm"):
                    #  A resize back and forth makes the window repaint all
                    #  of itself; one resize leaves a stale block of it.
                    xdo(display, "windowsize", sized, str(size[0] + 2),
                        str(size[1] + 2))
                    time.sleep(3)
                    xdo(display, "windowsize", sized, str(size[0]),
                        str(size[1]))
                if sized:
                    xdo(display, "windowraise", sized)   # no window manager
                time.sleep(6 if name.endswith(".ppm") else 4)
                if name.endswith(".ppm"):
                    tree = subprocess.run(
                        ["xwininfo", "-display", display.name, "-root",
                         "-tree"], stdout=subprocess.PIPE).stdout.decode()
                    open(os.path.join(shots.tmp, "xtree.txt"), "w").write(tree)
                target = names.pop(name)
                if target and sized:
                    wid = sized
                    if isinstance(target, tuple):   # another window
                        want, target = target
                        others = [w for w in shots.windowsOf(known)
                                  if want in w[1] and w[0] != sized]
                        wid = others[0][0] if others else None
                        if wid is None:
                            fail("no window %r for shot %s" % (want, name))
                    if wid:
                        shots.take(wid, os.path.join(shots.outdir, target))
                if name.endswith(".ppm"):
                    finished = True          # one picture, then done
                else:
                    open(os.path.join(scriptDir, name + ".go"), "w").close()
        time.sleep(0.5)
    if status is None:
        if not finished:
            #  Timed out: keep a picture of the harness's own display.
            subprocess.run(["import", "-display", display.name, "-window",
                            "root", os.path.join(shots.tmp,
                                                 "timeout-%s.png" % wrapper)])
        for sig in (15, 9):
            try:
                os.killpg(proc.pid, sig)
                proc.wait(timeout=10)
                break
            except Exception:
                pass
    log.close()
    text = open(os.path.join(scriptDir, "log"), errors="replace").read()
    return status, text


# ---------------------------------------------------------------------------
#  pw.x on a deck of our own: the oracle
# ---------------------------------------------------------------------------

def pwx(deck, workdir, name):
    with open(os.path.join(workdir, name + ".in"), "w") as handle:
        handle.write(deck)
    env = dict(os.environ, OMP_NUM_THREADS="2")
    out = subprocess.run(["pw.x", "-in", name + ".in"], cwd=workdir, env=env,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         timeout=1800).stdout.decode()
    with open(os.path.join(workdir, name + ".out"), "w") as handle:
        handle.write(out)
    return out


def finalEnergyRy(text):
    found = re.findall(r"^!\s+total energy\s+=\s+(\S+) Ry", text, re.M)
    return float(found[-1]) if found else None


def firstEnergyRy(text):
    found = re.findall(r"^!\s+total energy\s+=\s+(\S+) Ry", text, re.M)
    return float(found[0]) if found else None


def si_reference(workdir):
    """The conventional cubic cell, written by hand: 8 atoms, 4x4x4 mesh."""
    a = 5.43
    pos = []
    for fcc in ((0, 0, 0), (0, .5, .5), (.5, 0, .5), (.5, .5, 0)):
        for shift in ((0, 0, 0), (.25, .25, .25)):
            pos.append(tuple((fcc[i] + shift[i]) * a for i in range(3)))
    deck = """&control
 calculation='scf', prefix='ref', outdir='./scratch', pseudo_dir='%s',
 tstress=.true., tprnfor=.true.
/
&system
 ibrav=1, celldm(1)=%.10f, nat=8, ntyp=1, ecutwfc=30, ecutrho=240
/
&electrons
 conv_thr=1e-10
/
ATOMIC_SPECIES
 Si 28.0855 Si.pbe-n-rrkjus_psl.1.0.0.UPF
ATOMIC_POSITIONS angstrom
%sK_POINTS automatic
 4 4 4 0 0 0
""" % (PSEUDO, a / BOHR,
       "".join(" Si %.8f %.8f %.8f\n" % p for p in pos))
    return pwx(deck, workdir, "si")


def parseXyzFromPwin(text):
    """(cell vectors in angstrom, [(symbol, x, y, z)]) of a pw.x input."""
    cell = []
    m = re.search(r"CELL_PARAMETERS\s*\{?\s*angstrom\}?\s*\n((?:.*\n){3})",
                  text, re.I)
    for line in m.group(1).splitlines():
        cell.append([float(v) for v in line.split()])
    atoms = []
    m = re.search(r"ATOMIC_POSITIONS\s*\{?\s*(\w+)\}?\s*\n((?:\s*[A-Za-z]+\s+"
                  r"[-0-9.eE+]+\s+[-0-9.eE+]+\s+[-0-9.eE+]+.*\n)+)", text, re.I)
    for line in m.group(2).splitlines():
        w = line.split()
        atoms.append((w[0], float(w[1]), float(w[2]), float(w[3])))
    return cell, atoms


def h2o_reference(workdir, cell, atoms):
    """The same start geometry, the same settings, relaxed by pw.x directly."""
    a = cell[0][0]
    deck = """&control
 calculation='relax', prefix='ref', outdir='./scratch', pseudo_dir='%s',
 tprnfor=.true., forc_conv_thr=1e-3, etot_conv_thr=1e-5
/
&system
 ibrav=1, celldm(1)=%.10f, nat=3, ntyp=2, ecutwfc=60, ecutrho=480
/
&electrons
 conv_thr=1e-8
/
&ions
/
ATOMIC_SPECIES
 H 1.00794 H.pbe-rrkjus_psl.1.0.0.UPF
 O 15.9994 O.pbe-n-kjpaw_psl.0.1.UPF
ATOMIC_POSITIONS angstrom
%sK_POINTS gamma
""" % (PSEUDO, a / BOHR,
       "".join(" %s %.8f %.8f %.8f\n" % at for at in atoms))
    return pwx(deck, workdir, "h2o")


def lastPositions(text):
    """Final ATOMIC_POSITIONS of a pw.x relax run."""
    blocks = re.findall(r"ATOMIC_POSITIONS \(angstrom\)\n((?:\w+\s+[-0-9.]+"
                        r"\s+[-0-9.]+\s+[-0-9.]+.*\n)+)", text)
    return [[float(v) for v in l.split()[1:4]] for l in
            blocks[-1].splitlines()], [l.split()[0] for l in
                                       blocks[-1].splitlines()]


def dist(p, q):
    return math.sqrt(sum((p[i] - q[i]) ** 2 for i in range(3)))


def angle(o, a, b):
    u = [a[i] - o[i] for i in range(3)]
    v = [b[i] - o[i] for i in range(3)]
    c = sum(u[i] * v[i] for i in range(3)) / (
        math.sqrt(sum(x * x for x in u)) * math.sqrt(sum(x * x for x in v)))
    return math.degrees(math.acos(max(-1.0, min(1.0, c))))


# ---------------------------------------------------------------------------
#  The tutorial's two calculations
# ---------------------------------------------------------------------------

class Organizer(object):
    """One Organizer kept up for the run, fed commands through its hook."""

    def __init__(self, display, tmp, args):
        self.cmds = os.path.join(tmp, "organizer-commands")
        open(self.cmds, "w").close()
        self.logPath = os.path.join(tmp, "organizer.log")
        env = display.env()
        env.update(ECCE_TEST_ORGANIZER=self.cmds,
                   ECCE_TRANSPARENCY_FALLBACK_MS="0")
        self.proc = subprocess.Popen(
            [os.path.join(apps.WRAPPERS, "ecce-organizer")] + list(args),
            env=env, stdout=open(self.logPath, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)
        self.display = display

    def send(self, line, timeout=120):
        with open(self.cmds, "a") as handle:
            handle.write(line + "\n")
        end = time.time() + timeout
        want = "ECCE_TEST_ORGANIZER: %s: " % line
        while time.time() < end:
            for text in open(self.logPath, errors="replace").read().splitlines():
                if text.startswith(want):
                    return text[len(want):]
            time.sleep(0.5)
        return "no answer"

    def stop(self):
        for sig in (15, 9):
            try:
                os.killpg(self.proc.pid, sig)
                self.proc.wait(timeout=10)
                return
            except Exception:
                pass


def findDeck(project, calc):
    root = calcRoot(project, calc)
    for dirpath, _, files in os.walk(root):
        for name in files:
            if name.endswith(".pwin"):
                return os.path.join(dirpath, name)
    return None


def userUrl():
    if LOCAL:
        return "file://%s/users/local" % LOCAL[0]
    return "http://localhost:%d/Ecce/users/%s" % (
        fixture.dataserverPort(), fixture.realUser())


def auth(tmp):
    if LOCAL:
        return ()
    return ("-pipe", fixture.authFile(
        os.path.join(tmp, "auth.%d.pipe" % time.time_ns()),
        user=fixture.realUser()))


LOCAL = []     # [data folder] with --local: no data server, projects in a folder


def calcRoot(project, calc):
    if LOCAL:
        return os.path.join(LOCAL[0], "users", "local", project, calc)
    return os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users",
                        fixture.realUser(), project, calc)


def propFile(project, calc, name):
    path = os.path.join(calcRoot(project, calc), "Props", name)
    return open(path, errors="replace").read() if os.path.exists(path) else ""


def dumpTree(project, calc):
    root = calcRoot(project, calc)
    for dirpath, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if d != ".DAV"]
        for f in sorted(files):
            if not f.startswith("."):
                say("   file %s" % os.path.relpath(os.path.join(dirpath, f),
                                                   root))


CASES = {
    "si-scf": dict(
        project="qe-tutorial", prefix="si",
        builder=[
            "wait 4000", "shot builder-start",
            "add Si Lone 0 0 0", "wait 1500",
            "panel Periodic Builder", "wait 2000", "shot builder-atom",
            "pbc create", "wait 1000", "pbc type Lattice",
            "pbc cell 5.43 5.43 5.43 90 90 90", "wait 1000",
            "pbc spacegroup Fd-3m", "pbc generate", "wait 2000",
            "info", "expect atoms 8", "shot builder-generated",
            "save", "wait 4000", "quit"],
        calced=[
            "wait 3000", "ready", "shot calced-start", "info",
            "details theory", "wait 4000", "shot calced-theory",
            "close-details", "wait 1000",
            "button save", "wait 5000", "info", "shot calced-final", "quit"],
        panels=("Energies",)),
    "h2o-relax": dict(
        project="qe-tutorial", prefix="h2o",
        builder=[
            "wait 4000", "shot builder-start",
            "add O Bent 0 0 0", "wait 1000", "cmd addh", "wait 1500",
            "panel Periodic Builder", "wait 2000",
            "pbc create", "wait 1000", "pbc type Lattice",
            "pbc cell 10 10 10 90 90 90", "wait 1500",
            "info", "expect atoms 3", "shot builder-generated",
            "save", "wait 4000", "quit"],
        calced=[
            "wait 3000", "ready", "runtype Geometry", "wait 2000",
            "gui ES.Theory.PW.EcutWfc 60",
            "gui ES.Theory.PW.KPointScheme Gamma point only",
            "wait 1000", "shot calced-start", "info",
            "details theory", "wait 4000", "shot calced-theory",
            "close-details", "wait 1000",
            "button save", "wait 5000", "info", "shot calced-final", "quit"],
        panels=("Energies", "Geometry Trace")),
}


options_cases = []


def walk(display, tmp, outdir, org, authArgs, name):
    case = CASES[name]
    project, prefix = case["project"], case["prefix"]
    url = "%s/%s/%s" % (userUrl(), project, name)
    if name == options_cases[0]:
        say(org.send("newproject " + project))
    say(org.send("newcalc %s %s QuantumESPRESSO" % (project, name)))
    shots = Shots(display, outdir, tmp)

    def app(wrapper, hook, script, size, names, env=None, title="ECCE"):
        environment = {"_HOOK": hook}
        environment.update(env or {})
        status, log = runApp(display, shots, wrapper,
                             authArgs + ("-context", url), environment,
                             script, size, names, title=title,
                             sizeDelay=20 if hook == "ECCE_VIEWER_SCENE" else 0)
        say("%s exit %s" % (wrapper, status))
        if status not in (0, None):
            say("\n".join(log.splitlines()[-40:]))
        for line in log.splitlines():
            if re.search(r"SCRIPT: .*(FAIL|unknown|no |exception)", line):
                fail("%s: %s" % (wrapper, line.strip()))
            if "BUILDER:" in line or "CALCED: theory" in line or \
                    "LAUNCHER]" in line:
                say("   " + line.strip())
        return status, log

    app("ecce-builder", "ECCE_BUILDER_SCRIPT", case["builder"], (1500, 1380),
        {"builder-start": "%s-1-builder.png" % prefix,
         "builder-atom": "%s-2-periodic-builder.png" % prefix,
         "builder-generated": "%s-3-cell.png" % prefix})
    app("ecce-calced", "ECCE_CALCED_SCRIPT", case["calced"], (1100, 900),
        {"calced-start": "%s-4-calced.png" % prefix,
         "calced-theory": ("Theory", "%s-5-theory.png" % prefix),
         "calced-final": "%s-6-calced-ready.png" % prefix})
    deck = findDeck(project, name)
    deckText = open(deck).read() if deck else ""
    say("deck:\n" + deckText)

    run = os.path.join(fixture.stateHome(), "qe-runs", name)
    launcher = ["wait 3000", "machine localhost", "rundir " + run,
                "wait 1500", "shot %s/%s-7-launcher.png" % (outdir, prefix),
                "launch", "wait 4000",
                "shot %s/%s-8-launched.png" % (outdir, prefix),
                "wait %s" % os.environ.get("QE_LAUNCHER_WAIT", "50000"), "quit"]
    app("ecce-launcher", "ECCE_LAUNCHER_SCRIPT", launcher, (700, 800), {},
        title=None)
    last = ""
    logf = os.path.join(calcRoot(project, name), "Outputs",
                        "eccejobstorelog.ecce_run_log")
    for _ in range(int(os.environ.get("QE_POLL", "200"))):
        last = org.send("state " + url)
        #  The job store's own log is the first word on the final state;
        #  the Organizer's answer follows it.
        if os.path.exists(logf):
            states = re.findall(r'name="Calculation State Change"[^>]*>'
                                r'([A-Za-z ]+?)\s*</event>',
                                open(logf, errors="replace").read())
            if states and states[-1] in ("Complete", "Failed", "Unsuccessful",
                                         "Killed"):
                last = states[-1]
                break
        time.sleep(3)
    say("run state: %s" % last)
    if last.lower() not in ("complete", "completed"):
        fail("%s ended in state %s" % (name, last))
    time.sleep(5)
    if os.environ.get("QE_LAUNCHER_WAIT"):
        say(subprocess.run("ps -eo pid,etime,cmd | grep -E 'eccejob|pw.x' | "
                           "grep -v grep | cut -c1-120", shell=True,
                           stdout=subprocess.PIPE).stdout.decode())
    dumpTree(project, name)
    for prop in ("TE", "GEOMTRACE"):
        text = propFile(project, name, prop)
        say("--- Props/%s (%d bytes)\n%s" % (prop, len(text), text[:1200]))
    summary = org.send("summary " + url)
    say("summary: " + summary)
    for panel in case["panels"]:
        tag = panel.lower().replace(" ", "-")
        scene = ["style Ball And Stick", "hold 25", "viewall",
                 "snap results-ready", "hold 120"]
        app("ecce-builder", "ECCE_VIEWER_SCENE", scene, (1500, 1380),
            {"results-ready.ppm": "%s-9-%s.png" % (prefix, tag)},
            env={"ECCE_OPEN_PANEL": panel, "ECCE_VIEWER_SCENE_OUT": "@",
                 "ECCE_VIEWER_SCENE_SIZE": "640x480",
                 "ECCE_VIEWER_SCENE_HOLD": "130"})
    snap = os.path.join(outdir, "%s-10-organizer.png" % prefix)
    say(org.send("snap " + snap))
    check(name, project, deckText, summary, tmp)
    return deckText


def tevalue(project, calc):
    m = re.search(r">\s*(-?[0-9.]+)\s*</value>", propFile(project, calc, "TE"))
    return float(m.group(1)) if m else None


def check(name, project, deckText, summary, tmp):
    """Compare what ECCE produced with pw.x run on a deck written here."""
    work = os.path.join(tmp, "ref-" + name)
    os.makedirs(work, exist_ok=True)
    te = tevalue(project, name)
    if te is None:
        fail("%s: no total energy stored" % name)
        return
    if name == "si-scf":
        for want in ("calculation = 'scf'", "ecutwfc = 30.0", "ecutrho = 240",
                     "4 4 4 0 0 0", "Si.pbe-n-rrkjus_psl.1.0.0.UPF"):
            if want not in deckText:
                fail("si-scf deck lacks %r" % want)
        if len(re.findall(r"^\s+Si\s+[-0-9.]+\s+[-0-9.]+\s+[-0-9.]+\s*$",
                          deckText, re.M)) != 8:
            fail("si-scf deck does not hold 8 atoms")
        ref = finalEnergyRy(si_reference(work)) / 2
        say("si-scf  ECCE %.8f Ha, hand-written pw.x deck %.8f Ha, "
            "difference %.1e Ha" % (te, ref, te - ref))
        if abs(te - ref) > 2e-6:
            fail("si-scf energy differs from pw.x by %.1e Ha" % (te - ref))
        for field, value in (("Formula", "Si8"), ("Atoms", "8"),
                             ("Electrons", "112"), ("Symmetry", "Fd-3m"),
                             ("Charge", "0"), ("Runtype", "Energy")):
            if not re.search(r"\b%s=%s\b" % (field, re.escape(value)),
                             summary):
                fail("si-scf summary: %s is not %s: %s" % (field, value,
                                                          summary))
    else:
        cell, atoms = parseXyzFromPwin(deckText)
        for want in ("calculation = 'relax'", "ecutwfc = 60",
                     "ecutrho = 480", "K_POINTS gamma", "&IONS"):
            if want not in deckText:
                fail("h2o-relax deck lacks %r" % want)
        text = h2o_reference(work, cell, atoms)
        ref = finalEnergyRy(text) / 2
        say("h2o-relax  ECCE %.8f Ha, hand-written pw.x deck %.8f Ha, "
            "difference %.1e Ha" % (te, ref, te - ref))
        if abs(te - ref) > 5e-5:
            fail("h2o-relax energy differs from pw.x by %.1e Ha" % (te - ref))
        pos, sym = lastPositions(text)
        o = pos[sym.index("O")]
        h = [pos[i] for i in range(len(sym)) if sym[i] == "H"]
        say("h2o-relax  pw.x: O-H %.4f %.4f A, H-O-H %.2f deg"
            % (dist(o, h[0]), dist(o, h[1]), angle(o, h[0], h[1])))
        steps = re.findall(r'<step number="\d+">([^<]*)</step>',
                           propFile(project, name, "GEOMTRACE"))
        bfgs = re.search(r"bfgs converged in\s+\d+ scf cycles and\s+(\d+) "
                         r"bfgs steps", text)
        if not steps or not bfgs:
            fail("h2o-relax: no geometry trace or no converged optimisation")
        else:
            last = [float(v) for v in steps[-1].split()]
            eo, h1, h2 = last[0:3], last[3:6], last[6:9]
            say("h2o-relax  ECCE trace: %d frames (pw.x: %s bfgs steps), "
                "O-H %.4f %.4f A, H-O-H %.2f deg"
                % (len(steps), bfgs.group(1), dist(eo, h1), dist(eo, h2),
                   angle(eo, h1, h2)))
            if len(steps) != int(bfgs.group(1)) + 1:
                fail("h2o-relax: %d trace frames for %s bfgs steps"
                     % (len(steps), bfgs.group(1)))
            if abs(dist(eo, h1) - dist(o, h[0])) > 2e-3 or \
                    abs(angle(eo, h1, h2) - angle(o, h[0], h[1])) > 0.2:
                fail("h2o-relax: the stored final geometry differs from "
                     "the pw.x one")
        for field, value in (("Formula", "H2O"), ("Atoms", "3"),
                             ("Charge", "0"), ("Runtype", "Geometry")):
            if not re.search(r"\b%s=%s\b" % (field, re.escape(value)),
                             summary):
                fail("h2o-relax summary: %s is not %s: %s" % (field, value,
                                                             summary))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--case", action="append", choices=sorted(CASES))
    parser.add_argument("--local", action="store_true",
                        help="local data mode: a folder instead of a data "
                             "server")
    options, rest = parser.parse_known_args()
    if not (shutil.which("pw.x") and os.path.isdir(PSEUDO)):
        say("SKIP: pw.x or the SSSP pseudopotentials are not installed")
        return 77
    options_cases.extend(options.case or ["si-scf", "h2o-relax"])
    if options.local:
        LOCAL.append(tempfile.mkdtemp(prefix="qe-localdata-"))
        os.environ["ECCE_LOCAL_DATA"] = LOCAL[0]

    def checkApp(display, name, results, verbose=False):
        if not (shutil.which("pw.x") and os.path.isdir(PSEUDO)):
            say("SKIP: pw.x or the SSSP pseudopotentials are not installed")
            return
        outdir = os.environ.get("QE_TUTORIAL_PNGS") or tempfile.mkdtemp()
        os.makedirs(outdir, exist_ok=True)
        tmp = tempfile.mkdtemp(prefix="qe-walk-")
        #  What `ecce` does at a first start (packaging/ecce.in): the
        #  template names every code by its plain command name.
        config = os.path.join(fixture.stateHome(), ".ECCE", "CONFIG.localhost")
        shutil.copy(os.path.join(apps.INSTALL, "siteconfig", "CONFIG-Examples",
                                 "CONFIG.localhost"), config)
        org = Organizer(display, tmp, auth(tmp))
        try:
            time.sleep(25)
            for name in options_cases:
                walk(display, tmp, outdir, org, auth(tmp), name)
        finally:
            org.stop()
        if failures:
            results.fail("qe-walkthrough", "; ".join(failures))
    #  Tall enough for the Builder's whole right-hand column.
    xdisplay.SCREEN = "1700x1450x24"
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + rest
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
