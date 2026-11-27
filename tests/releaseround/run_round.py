#!/usr/bin/env python3
"""
The release workflow scripts W1-W6 (wiki Release-9.6.0, "Workflow scripts"),
headless, through ECCE's own test hooks.

    tests/releaseround/run_round.py [--only W1,W4] [--keep]
                                   [--allow-missing-codes]

Each workflow starts a fresh Organizer (one-program mode, Normal layout)
on an Xvfb display with the isolated HOME and data folder of tests/apps
(ECCE_TEST_HOME, ECCE_TEST_WRAPPERS), drives it with ECCE_TEST_ORGANIZER
commands, ECCE_BUILDER_SCRIPT for Builder and Viewer tabs, and quits it.
One line per step: `W1.3 PASS|FAIL|MANUAL|SKIP <evidence or reason>`,
then a summary; exit 0 only when every expected step was reported and
none failed. A step the runner never reached is a FAIL, and so is a round
that checked nothing (no install, no Xvfb, the Organizer did not start),
and so is a missing code: a release round that cannot launch Vina or
Quantum ESPRESSO has not exercised docking or the periodic path, and must
not report success for them. --allow-missing-codes gives a partial round.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
APPS = os.path.join(REPO, "tests", "apps")
sys.path.insert(0, APPS)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

STEPS = []          # (id, status, evidence)


def step(sid, status, evidence=""):
    evidence = " ".join(str(evidence).split())[:600]
    STEPS.append((sid, status, evidence))
    print("%s %s %s" % (sid, status, evidence), flush=True)


def passfail(sid, ok, evidence):
    step(sid, "PASS" if ok else "FAIL", evidence)
    return ok


def which(name):
    return shutil.which(name)


def tabPages(answer):
    """Tab captions of a `tabs` answer, in order (before menus=)."""
    return [c.rstrip("*") for c in re.findall(
        r"\d+:(.+?)(?=\s+\d+:|\s+menus=|\s*$)", answer.split("menus=")[0])]


def tabIndex(answer, part, nth=1):
    seen = 0
    for i, cap in enumerate(tabPages(answer)):
        if part in cap:
            seen += 1
            if seen == nth:
                return i
    return None


def readMeta(calcDir):
    try:
        text = open(os.path.join(calcDir, ".ecce-meta"), errors="replace").read()
    except OSError:
        return {}
    meta = {}
    for key in ("application", "contenttype", "state"):
        m = re.search(r"ecce:%s\t+(\S[^\n]*)" % key, text)
        if m:
            meta[key] = m.group(1).strip()
    return meta


def calcsUnder(top):
    """(path, application, state) of every calculation below top."""
    found = []
    for root, dirs, _files in os.walk(top):
        meta = readMeta(root)
        if meta.get("contenttype") == "ecceCalculation":
            found.append((root, meta.get("application", "?"),
                          meta.get("state", "?")))
            dirs[:] = []
    return sorted(found)


def urlPath(url, data=None):
    if url.startswith("file://"):
        return url[len("file://"):]
    if data and not os.path.isabs(url):
        # `examples` answers a path below the user's local data folder
        return os.path.join(data, "users", "local", url)
    return url


class Organizer(object):
    """One Organizer process, driven line by line through its hook files."""

    def __init__(self, display, work, data, extra=None):
        self.display, self.work, self.data = display, work, data
        self.extra = dict(extra or {})
        self.cmds = os.path.join(work, "organizer.cmd")
        self.answers = os.path.join(work, "organizer.answers")
        self.script = os.path.join(work, "builder.script")
        self.logpath = os.path.join(work, "organizer.log")
        self.proc = None
        self.seen = 0
        self.logMark = 0
        self.shots = 0

    def env(self):
        env = self.display.env()
        env.update({"ECCE_ONE_PROCESS": "1", "ECCE_TEST_ORGANIZER": self.cmds,
                    "ECCE_TEST_ORGANIZER_ANSWERS": self.answers,
                    "ECCE_BUILDER_SCRIPT": self.script, "ECCE_TIMING": "1",
                    # Without it only the first Builder/Viewer tab of the
                    # process runs the script (docs/AGENTS.md).
                    "ECCE_BUILDER_SCRIPT_EACH": "1",
                    "ECCE_LOCAL_DATA": self.data})
        env.update(self.extra)
        return env

    def start(self, timeout=90):
        for path in (self.cmds, self.answers, self.script):
            open(path, "w").close()
        self.seen = 0
        self.log = open(self.logpath, "a")
        self.log.write("\n==== start %s\n" % time.strftime("%H:%M:%S"))
        self.log.flush()
        self.logMark = os.path.getsize(self.logpath)
        self.proc = subprocess.Popen(
            [os.path.join(apps.WRAPPERS, "ecce-organizer")], env=self.env(),
            stdout=self.log, stderr=subprocess.STDOUT, start_new_session=True)
        t0 = time.time()
        ans = self.send("tabs", timeout)
        return ans if ans is not None else None, time.time() - t0

    def alive(self):
        return self.proc is not None and self.proc.poll() is None

    def send(self, line, timeout=120):
        """The hook's answer to line, or None (no answer, or it exited)."""
        if not self.alive():
            return None
        with open(self.cmds, "a") as h:
            h.write(line + "\n")
        prefix = line + ": "
        end = time.time() + timeout
        while time.time() < end:
            rows = open(self.answers, errors="replace").read().splitlines() \
                if os.path.exists(self.answers) else []
            if len(rows) > self.seen:
                row = rows[self.seen]
                self.seen += 1
                return row[len(prefix):] if row.startswith(prefix) else row
            if not self.alive():
                return None
            time.sleep(0.2)
        return None

    def logText(self, since=None):
        try:
            with open(self.logpath, errors="replace") as h:
                h.seek(self.logMark if since is None else since)
                return h.read()
        except OSError:
            return ""

    def builderScript(self, lines):
        """The script the next Builder or Viewer tab runs; ends with a
        barrier whose name is returned (waitScript)."""
        self.shots += 1
        name = "done%d" % self.shots
        with open(self.script, "w") as h:
            h.write("\n".join(list(lines) + ["shot " + name]) + "\n")
        return name

    def waitScript(self, name, timeout=180):
        ready = os.path.join(self.work, name + ".ready")
        end = time.time() + timeout
        while time.time() < end and self.alive():
            if os.path.exists(ready):
                open(os.path.join(self.work, name + ".go"), "w").close()
                open(self.script, "w").close()
                return True
            time.sleep(0.3)
        open(self.script, "w").close()
        return False

    def scriptOut(self, since):
        """(command outcomes, BUILDER:/SELOVERLAY: lines) since a log offset."""
        text = self.logText(since)
        outs = re.findall(r"ECCE_BUILDER_SCRIPT: (.*)", text)
        info = re.findall(r"^((?:BUILDER|SELOVERLAY): .*)$", text, re.M)
        return outs, info

    def mark(self):
        return os.path.getsize(self.logpath)

    def waitTool(self, timeout=90):
        end = time.time() + timeout
        ans = ""
        while time.time() < end:
            ans = self.send("toolstate", 20) or ""
            if "state=up" in ans or "timed out" in ans:
                break
            time.sleep(1)
        return ans

    def closeTab(self, part):
        tabs = self.send("tabs") or ""
        i = tabIndex(tabs, part)
        if i is None:
            return None, tabs
        self.send("tabclose %d" % i)
        time.sleep(3)
        after = self.send("tabs") or ""
        asked = False
        if len(tabPages(after)) >= len(tabPages(tabs)):
            asked = (self.send("answer no", 10) or "") != "no dialog"
            time.sleep(2)
            after = self.send("tabs") or ""
        return asked, after

    def quit(self, timeout=60):
        """File > Quit; seconds until the process ended, or None."""
        if not self.alive():
            return None
        t0 = time.time()
        with open(self.cmds, "a") as h:
            h.write("menu exit\n")
        answered = 0
        while time.time() - t0 < timeout:
            if not self.alive():
                return time.time() - t0
            if time.time() - t0 > 3 and answered < 2:
                with open(self.cmds, "a") as h:
                    h.write("answer ok\n")
                answered += 1
            time.sleep(0.3)
        return None

    def kill(self):
        if self.alive():
            try:
                os.killpg(self.proc.pid, 15)
                self.proc.wait(20)
            except (OSError, subprocess.TimeoutExpired):
                try:
                    os.killpg(self.proc.pid, 9)
                except OSError:
                    pass
        if getattr(self, "log", None):
            self.log.close()
            self.log = None


def sessionProcesses(data, roots=()):
    """(pid, comm) of ECCE processes started for this test's data folder.

    The environment marker alone also matches daemons that GTK or the
    desktop fork from an ECCE program (an input-method helper): they inherit
    the environment and outlive ECCE by design.  A process counts only when
    its executable or command line lies under one of `roots` (the install
    prefix and the isolated home), which is what makes it ECCE's.

    ECCE's own ssh connection master is a system `ssh` whose only ECCE path
    is its control socket under ECCE_SSH_CONTROL_DIR, so that directory
    counts as a root too.  It is read from the process's own environment,
    because the per-session name is chosen inside the wrapper; and with
    ControlPersist this master is exactly the kind of process that outlives
    a session.
    """
    if not os.path.isdir("/proc"):
        return None
    mark = ("ECCE_LOCAL_DATA=%s" % data).encode()
    roots = [os.path.realpath(r).encode() for r in roots if r]
    me = os.getpid()
    found = []
    for pid in os.listdir("/proc"):
        if not pid.isdigit() or int(pid) == me:
            continue
        try:
            env = open("/proc/%s/environ" % pid, "rb").read().split(b"\0")
            if mark not in env:
                continue
            own = [v.split(b"=", 1)[1] for v in env
                   if v.startswith(b"ECCE_SSH_CONTROL_DIR=") and b"=" in v]
            comm = open("/proc/%s/comm" % pid).read().strip()
            try:
                exe = os.path.realpath(os.readlink("/proc/%s/exe" % pid)).encode()
            except OSError:
                exe = b""
            cmd = open("/proc/%s/cmdline" % pid, "rb").read()
            if roots and not any(r in exe or r in cmd
                                 for r in roots + [o for o in own if o]):
                continue
            found.append((int(pid), comm))
        except OSError:
            pass
    return found


def waitState(org, url, want, timeout, seen=None):
    """Poll the `state` hook; the states seen in order, last one last."""
    seen = seen if seen is not None else []
    end = time.time() + timeout
    while time.time() < end:
        st = (org.send("state " + url, 30) or "?").strip()
        if not seen or seen[-1] != st:
            seen.append(st)
        if st.lower() in want:
            break
        time.sleep(1)
    return seen


DONE = ("complete", "completed", "failed", "unsuccessful", "system_failure", "killed")


def completed(state):
    return state.lower().startswith("complete")


class Round(object):
    def __init__(self, display, root):
        self.display = display
        self.root = root
        self.data = os.path.join(root, "data")
        os.makedirs(self.data, exist_ok=True)
        self.project = None
        self.w1calc = None

    def org(self, tag, extra=None):
        work = os.path.join(self.root, tag)
        os.makedirs(work, exist_ok=True)
        return Organizer(self.display, work, self.data, extra)

    def home(self, org):
        if self.project:
            return "existing"
        ans = org.send("newproject releaseround") or ""
        if ans.startswith("ok "):
            self.project = ans.split()[1]
        elif self.project is None:
            # already there from an earlier workflow of this round
            self.project = "file://" + os.path.join(
                self.data, "users", "local", "releaseround")
        return ans

    def projName(self):
        return os.path.basename(urlPath(self.project).rstrip("/"))

    # ---------------------------------------------------------------- W1
    def w1(self):
        org = self.org("w1")
        try:
            ans, secs = org.start()
            if ans is None:
                return step("W1.1", "FAIL", "the Organizer did not answer (%s)"
                            % org.logText()[-400:])
            ex = org.send("examples", 180) or ""
            self.home(org)
            src = None
            if ex.startswith("ok "):
                for path, app, _st in calcsUnder(urlPath(ex.split()[1], self.data)):
                    if app == "ECCE-QM" and "water" in path:
                        src = path
                        break
            paste = org.send("server paste copy %s file://%s"
                             % (self.project, src), 120) if src else "no source"
            copies = [c for c in calcsUnder(urlPath(self.project))
                      if c[1] == "ECCE-QM"]
            # The pasted copy keeps the example's Loaded state (imported
            # results), whose Launch is off by design; New from This >
            # Duplicate makes the launchable copy, as the help says for a rerun.
            dup, fresh = "not reached", []
            if copies:
                dup = org.send("duplicate file://" + copies[0][0], 120) or ""
                fresh = [c for c in calcsUnder(urlPath(self.project))
                         if c[1] == "ECCE-QM" and c[0] != copies[0][0]]
            ok = bool(fresh)
            if ok:
                self.w1calc = "file://" + fresh[0][0]
            if not passfail("W1.1", ok, "examples: %s; paste: %s; copy: %s; "
                            "duplicate: %s -> %s" % (ex, paste, copies[:1],
                                                     dup[:80], fresh[:1])):
                return
            url = self.w1calc
            org.send("select " + url)

            b = org.builderScript(["wait 3000", "info"])
            m = org.mark()
            org.send("start Builder " + url)
            org.waitTool()
            org.waitScript(b)
            _o, info = org.scriptOut(m)
            atoms = [i for i in info if i.startswith("BUILDER: atoms=")]
            asked, tabs = org.closeTab("Builder")
            passfail("W1.2", atoms and "atoms=3 " in atoms[0] + " "
                     and asked is False and tabIndex(tabs, "Builder") is None,
                     "%s; closed=%s asked=%s tabs=%s" % (atoms[:1], asked is not None,
                                                        asked, tabs))

            org.send("start CalculationEditor " + url)
            org.waitTool()
            time.sleep(4)
            info = org.send("setup info") or ""
            save = org.send("setup save", 60) or ""
            time.sleep(3)
            st = org.send("state " + url) or ""
            theory = re.search(r"theorysum=(.*?)(?= runtypesum=|$)", info)
            runtype = re.search(r"runtypesum=(.*?)(?= \w+=|$)", info)
            passfail("W1.3", theory and runtype and "ready" in st.lower(),
                     "theory=[%s] runtype=[%s] save=%s state=%s"
                     % (theory and theory.group(1), runtype and runtype.group(1),
                        save, st))

            launch = org.send("setup launch", 60) or ""
            time.sleep(2)
            go = org.send("setup launch go", 120) or ""
            states = waitState(org, url, ("submitted", "running") + DONE, 60)
            local = re.search(r"machine=\S+", launch)
            passfail("W1.4", local and "enabled" in launch
                     and states[-1].lower() != "ready",
                     "launch=[%s] go=[%s] state=%s" % (launch, go, states[-1]))

            states = waitState(org, url, DONE, 900, states)
            passfail("W1.5", completed(states[-1]),
                     "states seen: %s" % " -> ".join(states))

            # The editor after a launch (#1290): Results box, output view, the
            # run state. Rerun is only cancelled and the Viewer button is
            # pressed once and closed, so W1.6 and W1.7 still see this run.
            res = org.send("setup results", 30) or ""
            outp = org.send("setup output", 30) or ""
            inp = org.send("setup input", 30) or ""
            outp2 = org.send("setup results output", 30) or ""
            rr = org.send("setup rerun cancel", 30) or ""
            st2 = (org.send("state " + url) or "").strip()
            # The Viewer before the duplicate: duplicating moves the editor
            # to the new copy, which has never run, so its Results box has no
            # Viewer to open and the check would fail on the wrong calculation.
            rv = org.send("setup results viewer", 30) or ""
            time.sleep(4)
            vtabs = org.send("tabs") or ""
            opened = tabIndex(vtabs, "Viewer") is not None
            if opened:
                org.closeTab("Viewer")
            before = len(calcsUnder(urlPath(self.project)))
            dup = org.send("setup duplicate", 60) or ""
            time.sleep(3)
            after = len(calcsUnder(urlPath(self.project)))
            passfail("W1.8", "results=shown" in res and "actions=duplicate,rerun" in res
                     and "editlaunch=disabled" in res and outp == "output"
                     and not inp.startswith(("unknown", "no ")) and "output" in outp2
                     and completed(st2) and after == before + 1 and opened,
                     "results=[%s] output=[%s] input=[%s] results-output=[%s] "
                     "rerun-cancel=[%s] state after=%s duplicate=[%s] calcs %d->%d "
                     "viewer=[%s] opened=%s" % (res, outp, inp[:60], outp2, rr, st2,
                                                dup[:60], before, after, rv, opened))

            self.viewerCheck(org, url, "W1.6",
                             (("geometry", r"geom|coord|atoms"),
                              ("energy", r"energ"),
                              ("orbitals", r"orbital|\bmo\b"),
                              ("vibrations", r"vib|frequenc|normal mode")))

            secs = org.quit()
            org.kill()
            ans, _ = org.start()
            st = (org.send("state " + url) or "").strip()
            sel = org.send("select " + url) or ""
            passfail("W1.7", secs is not None and completed(st)
                     and sel == "ok", "quit took %s s; after restart state=%s select=%s"
                     % (secs and round(secs, 1), st, sel))
            org.quit()
        finally:
            org.kill()

    VIEWER_PICKS = ("Calculation Summary", "Energies")

    def viewerCheck(self, org, url, sid, wants, atom=False):
        """One Viewer tab, one script: the Properties list has each wanted
        kind, the known entries VIEWER_PICKS present open, with atom atom 1
        is selected (#1236); then the tab is closed once and checked gone.
        Reopening a tab that did not close reuses it without the script."""
        lines = ["wait 4000", "plist"] + ["list " + p for p in self.VIEWER_PICKS]
        lines += ["wait 1500", "overlay"] + (["selectoverlay 1"] if atom else [])
        b = org.builderScript(lines)
        m = org.mark()
        org.send("start CalculationViewer " + url)
        org.waitTool()
        done = org.waitScript(b)
        outs, info = org.scriptOut(m)
        bi = org.send("builderinfo") or ""
        asked, tabs = org.closeTab("Viewer")
        closed = asked is not None and tabIndex(tabs, "Viewer") is None
        entries = [i[len("BUILDER: entry "):] for i in info
                   if i.startswith("BUILDER: entry ")]
        names = [e.split("/", 1)[-1] for e in entries]
        picks = [p for p in self.VIEWER_PICKS if p in names]
        absent = ["list %s:" % p for p in self.VIEWER_PICKS if p not in names]
        bad = [o for o in outs if ": ok" not in o and not o.startswith("shot")
               and not any(o.startswith(a) for a in absent)]
        sel = [i for i in info if i.startswith("SELOVERLAY:")]
        missing = [k for k, rx in wants
                   if not any(re.search(rx, e, re.I) for e in entries)
                   and not (k == "geometry" and re.search(r"atoms=[1-9]", bi))
                   and not (k == "cell" and re.search(r"lattice=1", bi))]
        if not closed:
            bad.append("Viewer tab still open after close: %s" % tabs)
        ok = done and entries and not missing and not bad
        if wants:
            passfail(sid, ok, "%s; entries=%s; clicked=%s; missing=%s; errors=%s"
                     % (bi, entries, picks, missing, bad))
        return {"entries": entries, "picks": picks, "bad": bad, "sel": sel,
                "done": done, "info": bi, "closed": closed}

    # ---------------------------------------------------------------- W2
    def w2(self):
        pdb = os.path.join(REPO, "tests", "fragreaders", "data", "181L.pdb")
        org = self.org("w2")
        try:
            ans, _ = org.start()
            if ans is None:
                return step("W2.1", "FAIL", "the Organizer did not answer")
            self.home(org)
            edit = ["wait 5000", "info", "select_hetero", "wait 1000", "delkey",
                    "wait 1500", "info", "undo", "wait 1500", "info", "redo",
                    "wait 1500", "info", "menu Build|Add Hydrogen", "wait 4000",
                    "info", "ligands", "ligand 1", "wait 1000"]
            b = org.builderScript(edit)
            m = org.mark()
            env = org.env()
            t0 = time.time()
            hand = subprocess.run(
                [os.path.join(apps.INSTALL, "libexec", "ecce-one-instance"), pdb],
                env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True, timeout=60)
            done = org.waitScript(b, 240)
            outs, info = org.scriptOut(m)
            atoms = [int(x) for x in re.findall(r"BUILDER: atoms=(\d+)", "\n".join(info))]
            tabs = org.send("tabs") or ""
            passfail("W2.1", hand.returncode == 0 and tabIndex(tabs, "Builder") is not None
                     and atoms and atoms[0] > 1000,
                     "ecce-one-instance rc=%d (%.1f s) %s; tabs=%s; atoms=%s"
                     % (hand.returncode, time.time() - t0, hand.stdout.strip()[-120:],
                        tabs, atoms[:1]))
            if len(atoms) >= 5:
                a0, aDel, aUndo, aRedo, aH = atoms[:5]
                passfail("W2.2", aDel < a0 and aUndo == a0 and aRedo == aDel,
                         "atoms %d -> delete %d -> undo %d -> redo %d" % (a0, aDel, aUndo, aRedo))
                passfail("W2.3", aH > aRedo,
                         "Add Hydrogen: %d -> %d atoms; %s" % (aRedo, aH, [
                             o for o in outs if "Add Hydrogen" in o]))
            else:
                step("W2.2", "FAIL", "script did not finish: done=%s outs=%s" % (done, outs[-4:]))
                step("W2.3", "FAIL", "not reached")
            ligs = [o for o in outs if o.startswith("ligands") or o.startswith("ligand 1")]
            lig_lines = [i for i in info if "ligand" in i.lower() or "SELOVERLAY" in i]
            text = org.logText(m)
            cands = re.findall(r"^.*BNZ.*$", text, re.M)
            passfail("W2.4", any("BNZ" in c for c in cands) and
                     all(": ok" in o or "BNZ" in o for o in ligs),
                     "ligands=%s; %s; %s" % (cands[:2], ligs, lig_lines[-1:]))
            asked, tabs = org.closeTab("Builder")
            # The edits above are unsaved, so closing MUST ask (#1307, #1308);
            # asked is None when there was no Builder tab, which is not a pass.
            step("W2.4c", "PASS" if asked is True and tabIndex(tabs, "Builder") is None
                 else "FAIL", "closing the edited, unsaved Builder tab must ask and "
                 "then close: asked=%s tabs=%s" % (asked, tabs))

            # Save As's calculation dialog has no hook: the same structure
            # goes into a Vina calculation made with File > New, whose
            # Builder imports and edits it as above and saves.
            ans = org.send("newcalc %s dock181l Vina" % self.projName()) or ""
            url = ans.split()[1] if ans.startswith("ok ") else None
            if not url:
                step("W2.5", "FAIL", "newcalc Vina: %s" % ans)
                for n in ("W2.6", "W2.7", "W2.8"):
                    step(n, "SKIP", "no calculation")
                return
            b = org.builderScript(["wait 4000", "import " + pdb, "wait 4000",
                                   "select_hetero", "wait 1000", "delkey", "wait 1500",
                                   "menu Build|Add Hydrogen", "wait 4000", "info",
                                   "save", "wait 5000"])
            m = org.mark()
            org.send("start Builder " + url)
            org.waitTool()
            org.waitScript(b, 240)
            outs, info = org.scriptOut(m)
            org.closeTab("Builder")
            org.send("start CalculationEditor " + url)
            org.waitTool()
            time.sleep(5)
            org.send("setup runtype", 60)
            info = ""
            for _ in range(15):
                info = org.send("setup info") or ""
                if "ligandpick=" in info and "ligandpick=0" not in info:
                    break
                time.sleep(2)
            lig = re.search(r"ligandpick=(\d+)", info)
            save = org.send("setup save", 60) or ""
            stored = []
            for root, _d, files in os.walk(urlPath(url)):
                for f in files:
                    try:
                        stored += [ln.strip() for ln in open(os.path.join(root, f),
                                                             errors="replace")
                                   if "LigandAtoms" in ln]
                    except OSError:
                        pass
            passfail("W2.5", lig and int(lig.group(1)) > 0 and stored,
                     "(Save As dialog replaced by New Vina + import; no hook) "
                     "builder=%s; ligandpick=%s; save=%s; stored=%s"
                     % ([o for o in outs if o.startswith(("save", "import"))],
                        lig and lig.group(1), save, stored[:1]))
            if not which("vina"):
                for n in ("W2.6", "W2.7", "W2.8"):
                    step(n, "SKIP", "vina is not installed on this host; W2 "
                         "stopped at the saved deck")
                return
            launch = org.send("setup launch", 60) or ""
            go = org.send("setup launch go", 120) or ""
            # The warning is inline in the editor (no dialog to answer), so a
            # warned Launch just stays Ready with the button "Launch anyway".
            # The hook cannot read the warning's text, only that it is pending.
            warned = "label=Launch anyway" in (
                org.send("setup launch get info", 30) or "")
            passfail("W2.7", not warned, "the box is automatic around the receptor's "
                     "own ligand, so the first Launch must not warn (#1296): "
                     "launch=[%s] go=[%s]" % (launch, go))
            if warned:
                org.send("setup launch go", 120)   # warn, never block
            states = waitState(org, url, DONE, 1800)
            self.dockWarning(org, url)
            if completed(states[-1]):
                self.viewerCheck(org, url, "W2.6", (("poses", r"pose|dock"),))
            else:
                step("W2.6", "FAIL", "launch=[%s] go=[%s] states=%s"
                     % (launch, go, " -> ".join(states)))
        finally:
            org.quit()
            org.kill()

    def dockWarning(self, org, url):
        """W2.8: a hand-set box far from the receptor warns on Launch, and
        Launch anyway stays on offer (warn, never block). The copy is never
        launched. Presence is read from the button label; the text is not
        readable from a script."""
        dup = org.send("duplicate " + url, 120) or ""
        copies = [c[0] for c in calcsUnder(urlPath(self.project))
                  if c[1] == "Vina" and "file://" + c[0] != url]
        if not copies:
            return step("W2.8", "FAIL", "no copy to set a box on: %s" % dup[:80])
        cu = "file://" + copies[-1]
        org.send("start CalculationEditor " + cu)
        org.waitTool()
        time.sleep(5)
        org.send("setup runtype", 60)
        sets = [org.send("setup set %s %s" % kv, 30) or "" for kv in (
            ("ES.Runtype.Vina.BoxAuto", "0"), ("ES.Runtype.Vina.BoxCenterX", "9999"),
            ("ES.Runtype.Vina.BoxCenterY", "9999"), ("ES.Runtype.Vina.BoxCenterZ", "9999"))]
        org.send("setup save", 60)
        time.sleep(3)
        org.send("setup launch", 60)
        go = org.send("setup launch go", 120) or ""
        info = org.send("setup launch get info", 30) or ""
        st = (org.send("state " + cu) or "").strip().lower()
        passfail("W2.8", "label=Launch anyway" in info and st == "ready",
                 "box set by hand at 9999: go=[%s] info=[%s] state=%s fields=%s"
                 % (go, info[:120], st, [x[:30] for x in sets]))

    # ---------------------------------------------------------------- W3
    def w3(self):
        org = self.org("w3")
        try:
            ans, _ = org.start()
            if ans is None:
                return step("W3.1", "FAIL", "the Organizer did not answer")
            self.home(org)
            ans = org.send("newcalc %s diamond QuantumESPRESSO" % self.projName()) or ""
            url = ans.split()[1] if ans.startswith("ok ") else None
            if not url:
                return step("W3.1", "FAIL", "newcalc QuantumESPRESSO: %s" % ans)
            b = org.builderScript([
                "wait 4000", "add C Lone 0 0 0", "wait 1500",
                "panel Periodic Builder", "wait 2000", "pbc create", "wait 1000",
                "pbc type Lattice", "pbc cell 3.567 3.567 3.567 90 90 90",
                "wait 1000", "pbc spacegroup Fd-3m", "pbc generate", "wait 2000",
                "info", "pbc replicate 2 2 2", "wait 2500", "info",
                "pbc replicate 1 1 1", "wait 2500", "info", "save", "wait 5000"])
            m = org.mark()
            org.send("start Builder " + url)
            org.waitTool()
            done = org.waitScript(b, 240)
            outs, info = org.scriptOut(m)
            text = "\n".join(info)
            atoms = [int(x) for x in re.findall(r"BUILDER: atoms=(\d+)", text)]
            bounds = re.findall(r"BUILDER: atom_bounds=(.*)", text)
            drawn = re.findall(r"BUILDER: lattice_drawn=(.*)", text)
            bad = [o for o in outs if ": ok" not in o and not o.startswith("shot")]
            passfail("W3.1", done and atoms and atoms[0] == 8 and not bad,
                     "atoms=%s errors=%s" % (atoms, bad))
            inside = None
            if len(bounds) >= 2 and len(drawn) >= 2:
                a = [float(x) for x in bounds[1].split()]
                d = [float(x) for x in drawn[1].split()]
                inside = all(a[k] >= d[k] - 0.05 and a[k + 3] <= d[k + 3] + 0.05
                             for k in range(3))
            passfail("W3.2", len(atoms) >= 3 and atoms[1] == 64 and atoms[2] == 8
                     and inside, "atoms 1x1x1/2x2x2/back=%s; 2x2x2 atom bounds %s "
                     "inside drawn cell %s: %s" % (atoms, bounds[1:2], drawn[1:2], inside))
            step("W3.2c", "MANUAL", "the Periodic Builder's Centre lattice box has no "
                 "script hook; tick it before Replicate by hand")
            org.closeTab("Builder")
            org.send("start CalculationEditor " + url)
            org.waitTool()
            time.sleep(5)
            info = org.send("setup info") or ""
            save = org.send("setup save", 60) or ""
            time.sleep(3)
            st = (org.send("state " + url) or "").strip()
            ok = passfail("W3.3", "ready" in st.lower(),
                          "info=[%s] save=%s state=%s" % (info[:300], save, st))
            if not which("pw.x"):
                step("W3.3L", "SKIP", "pw.x is not installed on this host; stopped after Save")
                step("W3.4", "SKIP", "no run")
                return
            if not ok:
                step("W3.4", "SKIP", "not saved")
                return
            launch = org.send("setup launch", 60) or ""
            go = org.send("setup launch go", 120) or ""
            states = waitState(org, url, DONE, 1800)
            passfail("W3.3L", completed(states[-1]),
                     "launch=[%s] go=[%s] states=%s" % (launch, go, " -> ".join(states)))
            if completed(states[-1]):
                r = self.viewerCheck(org, url, "W3.4", (("energy", r"energ"),
                                                        ("cell", r"cell|lattice")))
                bands = [e for e in r["entries"] if re.search(r"band|dos", e, re.I)]
                step("W3.4b", "PASS" if bands else "SKIP",
                     "band/DOS entries: %s" % (bands or "none for an scf run"))
            else:
                step("W3.4", "SKIP", "the run did not complete")
        finally:
            org.quit()
            org.kill()

    # ---------------------------------------------------------------- W4
    def w4(self):
        org = self.org("w4")
        try:
            ans, _ = org.start()
            if ans is None:
                return step("W4.1", "FAIL", "the Organizer did not answer")
            made = []
            for s in ("getting-started", "genchem", "spectroscopy", "organic"):
                a = org.send("examples " + s, 300) or ""
                if a.startswith("ok "):
                    made.append(urlPath(a.split()[1], self.data))
            calcs = []
            for top in made:
                calcs += [c for c in calcsUnder(top) if c[2].lower() in
                          ("loaded", "completed", "imported")]
            byCode, rest = {}, []
            for c in calcs:
                (rest if c[1] in byCode else byCode.setdefault(c[1], [])).append(c)
            pick = [v[0] for v in byCode.values()] + rest
            pick = pick[:10]
            rows, slow, nodesk = [], [], []
            for path, app, _st in pick:
                url = "file://" + path
                t0 = time.time()
                sel = org.send("select " + url, 60) or ""
                desk = org.send("desk") or ""
                wall = time.time() - t0
                time.sleep(1)     # the thumbnail follows from the event loop
                rt = org.send("refreshtimes") or ""
                total = re.search(r"total=(\d+)", rt)
                thumb = re.search(r"thumb=(\d+)", rt)
                ms = int(total.group(1)) if total else None
                rows.append("%s(%s) %sms thumb=%s" % (os.path.basename(path), app, ms,
                                                       thumb and thumb.group(1)))
                if sel != "ok" or ms is None or ms > 1000 or wall > 5:
                    slow.append("%s sel=%s total=%s wall=%.1f" % (os.path.basename(path),
                                                                   sel, ms, wall))
                if "shown=1" not in desk:
                    nodesk.append("%s: %s" % (os.path.basename(path), desk[:80]))
            codes = sorted(set(c[1] for c in pick))
            passfail("W4.1", len(pick) >= 10 and len(codes) >= 4,
                     "%d calculations, codes %s" % (len(pick), codes))
            passfail("W4.2", not slow and not nodesk and pick,
                     "%s; over 1 s: %s; desk not shown: %s"
                     % ("; ".join(rows), slow, nodesk))
            step("W4.2t", "MANUAL", "the structure picture on the Builder button is "
                 "timed (thumb=) but not compared; look at it")
            sels, panels = [], []
            for path, _app, _st in pick[:3]:
                r = self.viewerCheck(org, "file://" + path, None, (), atom=True)
                panels.append("%s: clicked %s errors %s" % (os.path.basename(path),
                                                           r["picks"], r["bad"]))
                sels.append("%s: %s" % (os.path.basename(path), r["sel"][:1]))
                okp = r["done"] and r["picks"] and not r["bad"]
                if not okp:
                    panels[-1] += " FAILED"
            passfail("W4.3", len(panels) == 3 and not any("FAILED" in p for p in panels),
                     "; ".join(panels))
            passfail("W4.4", len(sels) == 3 and all(re.search(r"SELOVERLAY: 1 \[\S+", s)
                                                    for s in sels), "; ".join(sels))
            tabs = org.send("tabs") or ""
            desk = org.send("desk") or ""
            passfail("W4.5", tabIndex(tabs, "Viewer") is None and "shown=1" in desk
                     and "(hidden)" not in desk, "tabs=%s; %s" % (tabs, desk))
        finally:
            org.quit()
            org.kill()

    # ---------------------------------------------------------------- W5
    def w5(self):
        org = self.org("w5")
        try:
            ans, _ = org.start()
            if ans is None:
                return step("W5.1", "FAIL", "the Organizer did not answer")
            a = org.send("examples", 300) or ""
            calcs = calcsUnder(urlPath(a.split()[1], self.data)) if a.startswith("ok ") else []
            if len(calcs) < 2:
                return step("W5.1", "FAIL", "examples: %s" % a)
            u1, u2 = "file://" + calcs[0][0], "file://" + calcs[1][0]
            # A run calculation's Builder opens as its Viewer (#1254), so the
            # Builder and the Editor go on a Duplicate, which has not run.
            self.home(org)
            before = set(c[0] for c in calcsUnder(urlPath(self.project)))
            paste = org.send("server paste copy %s %s" % (self.project, u1), 120) or ""
            pasted = [c[0] for c in calcsUnder(urlPath(self.project)) if c[0] not in before]
            dup = org.send("duplicate file://" + pasted[0], 120) or "" if pasted else "no copy"
            fresh = [c[0] for c in calcsUnder(urlPath(self.project))
                     if c[0] not in before and c[0] not in pasted]
            if not fresh:
                return step("W5.1", "FAIL", "no unrun copy for the Builder: paste=%s "
                            "pasted=%s duplicate=%s" % (paste, pasted, dup[:120]))
            u0 = "file://" + fresh[0]
            open(org.script, "w").close()
            for line in ("start Builder " + u0, "start CalculationEditor " + u0,
                         "start CalculationViewer " + u1,
                         "start CalculationViewer " + u2):
                org.send(line)
                org.waitTool()
                time.sleep(2)
            tabs = org.send("tabs") or ""
            pages = tabPages(tabs)
            nview = sum("Viewer" in p for p in pages)
            passfail("W5.1", len(pages) == 5 and nview == 2
                     and any("Builder" in p for p in pages)
                     and any("Editor" in p for p in pages), "tabs=%s" % tabs)
            i = tabIndex(tabs, "Builder")
            if i is None:
                return step("W5.2", "FAIL", "no Builder tab to move: tabs=%s" % tabs)
            out = org.send("towindow %d" % i) or ""
            time.sleep(3)
            t1 = org.send("tabs") or ""
            back = org.send("totabs Builder") or ""
            time.sleep(3)
            t2 = org.send("tabs") or ""
            passfail("W5.2", len(tabPages(t1)) == 4 and len(tabPages(t2)) == 5,
                     "(hook, not a drag) towindow=%s -> %s; totabs=%s -> %s"
                     % (out, t1, back, t2))
            step("W5.2d", "MANUAL", "dragging a tab out of the strip needs a real mouse")
            items = org.send("windowitems") or ""
            picks = []
            for n in range(len(tabPages(t2))):
                picks.append(org.send("windowmenu %d" % n) or "")
                time.sleep(1)
            alive = org.alive()
            # all([]) is True: an empty tab list must not pass.
            passfail("W5.3", alive and items and picks
                     and len(picks) == len(tabPages(t2)) and all(p for p in picks),
                     "windowitems=%s; windowmenu each=%s" % (items, picks))
            step("W5.3h", "MANUAL", "moving the pointer over the Window menu's items "
                 "(the #1239 highlight path) has no hook")
            closed, asked = [], []
            for _ in range(4):
                tabs = org.send("tabs") or ""
                pages = tabPages(tabs)
                if len(pages) <= 1:
                    break
                n = len(pages) - 1
                org.send("tabclose %d" % n)
                time.sleep(3)
                after = org.send("tabs") or ""
                if len(tabPages(after)) < len(pages):
                    closed.append(pages[n])
                else:
                    asked.append("%s: %s" % (pages[n], org.send("answer no", 10)))
                    time.sleep(2)
            tabs = org.send("tabs") or ""
            # This Builder was only opened, never edited: closing asks nothing.
            # Nothing closed means W5.1 left one tab and there was nothing to test.
            passfail("W5.4", len(tabPages(tabs)) == 1 and closed and not asked,
                     "closed %s; needed an answer: %s; left %s" % (closed, asked, tabs))
            secs = org.quit(30)
            time.sleep(3)
            left = sessionProcesses(
                self.data, (apps.INSTALL,
                            self.display.env().get("ECCE_REALUSERHOME")))
            if left is None:
                step("W5.5", "SKIP", "no /proc: list processes by hand (pgrep -a ecce)")
            else:
                passfail("W5.5", secs is not None and secs < 10 and not left,
                         "quit took %s s; processes of this session left: %s"
                         % (secs and round(secs, 1), left))
        finally:
            org.kill()

    # ---------------------------------------------------------------- W6
    def w6(self):
        import getpass
        remote = os.path.join(self.root, "w6-remote")
        os.makedirs(remote, exist_ok=True)
        address = "sftp://%s@localhost:2299%s" % (getpass.getuser(), remote)
        prefs = os.path.join(self.display.env()["ECCE_REALUSERHOME"], ".ECCE")
        os.makedirs(prefs, exist_ok=True)
        added = os.path.join(prefs, "AddedServers")
        saved = open(added).read() if os.path.exists(added) else None
        with open(added, "w") as h:
            h.write("ssh\tCluster\t%s\t0\n" % address)
        control = os.path.join(self.root, "w6-cm")
        os.makedirs(control, mode=0o700, exist_ok=True)
        org = self.org("w6", {
            "PATH": os.path.join(APPS, "fakessh") + os.pathsep + self.display.env()["PATH"],
            "FAKE_SSH_LOG": os.path.join(self.root, "w6-ssh.log"),
            "ECCE_SSH_CONTROL_DIR": control, "ECCE_ASKPASS": "/bin/false"})
        try:
            ans, _ = org.start()
            if ans is None:
                return step("W6.1", "FAIL", "the Organizer did not answer")
            t0 = time.time()
            con = org.send("server connect Cluster", 120) or ""
            roots = org.send("server roots") or ""
            passfail("W6.1", con == "connected" and "Cluster" in roots,
                     "(fake ssh, no real cluster) connect=%s in %.1f s; roots=%s"
                     % (con, time.time() - t0, roots))
            step("W6.2", "MANUAL", "a Slurm submit needs a real cluster; the run "
                 "below is a copy of a finished calculation, not a job")
            src = self.w1calc
            if not src:
                a = org.send("examples", 300) or ""
                cs = calcsUnder(urlPath(a.split()[1], self.data)) if a.startswith("ok ") else []
                src = "file://" + cs[0][0] if cs else None
            proj = org.send("server newproject Cluster runs") or ""
            purl = proj.split()[1] if proj.startswith("ok ") else None
            paste = org.send("server paste copy %s %s" % (purl, src), 180) \
                if purl and src else "no project or source"
            calc = purl.rstrip("/") + "/" + os.path.basename(urlPath(src).rstrip("/")) \
                if purl and src else None
            t0 = time.time()
            sel = org.send("select %s/" % address, 60) or ""
            desk = org.send("desk") or ""
            selc = org.send("select " + calc, 60) if calc else ""
            wall = time.time() - t0
            st0 = (org.send("state " + calc) or "").strip() if calc else ""
            passfail("W6.3", sel == "ok" and selc == "ok" and wall < 10
                     and st0 not in ("", "no such calculation"),
                     "paste=%s; root select=%s calc select=%s in %.1f s; state=%s; %s"
                     % (paste, sel, selc, wall, st0, desk[:100]))
            d = org.send("server rootchoose Cluster Disconnect", 60) or ""
            r1 = org.send("server roots") or ""
            c = org.send("server rootchoose Cluster Connect", 120) or ""
            r2 = org.send("server roots") or ""
            st1 = (org.send("state " + calc) or "").strip() if calc else ""
            passfail("W6.4", d == "ok" and "(not connected)" in r1 and c == "ok"
                     and "Cluster (SSH) (not connected)" not in r2 and st1 == st0,
                     "disconnect=%s roots=%s; connect=%s roots=%s; state %s -> %s"
                     % (d, r1, c, r2, st0, st1))
            if calc:
                r = self.viewerCheck(org, calc, None, ())
                passfail("W6.5", r["done"] and re.search(r"atoms=[1-9]", r["info"])
                         and r["closed"],
                         "(no job to fetch: W6.2 MANUAL) Viewer on the SSH folder's "
                         "copy: %s; entries=%s" % (r["info"], r["entries"][:6]))
            else:
                step("W6.5", "FAIL", "no calculation in the SSH folder")
        finally:
            org.quit()
            org.kill()
            if saved is None:
                os.remove(added)
            else:
                open(added, "w").write(saved)


WORKFLOWS = ("W1", "W2", "W3", "W4", "W5", "W6")
# The numbered steps of each workflow (wiki Release-9.6.0). Every one must be
# reported as PASS, FAIL, MANUAL or SKIP; silence is a FAIL, so a workflow
# that returns early cannot pass by leaving its later steps out. Lettered
# sub-steps (W3.2c, W5.2d, ...) are extras and not required.
EXPECTED = {"W1": 8, "W2": 8, "W3": 4, "W4": 5, "W5": 5, "W6": 5}

# Codes the round launches, with the workflow that needs each. A step whose
# code is absent reports SKIP, which satisfies EXPECTED just as PASS does, so
# without this a round on a host that had quietly lost vina would pass having
# docked nothing. ECCE-QM is not here: W1 launches it unconditionally.
CODES = {"vina": "W2", "pw.x": "W3"}


def underTest():
    """Machine, platform, commit and install the round ran on, for the record."""
    import platform
    import socket
    try:
        commit = subprocess.run(["git", "-C", REPO, "describe", "--always", "--dirty", "--abbrev=12"],
                                capture_output=True, text=True, timeout=20).stdout.strip()
    except Exception:
        commit = ""
    return "host=%s platform=%s commit=%s install=%s" % (
        socket.gethostname(), platform.platform(), commit or "unknown",
        os.environ.get("ECCE_TEST_HOME", "(unset)"))


def main():
    only = list(WORKFLOWS)
    if "--only" in sys.argv:
        i = sys.argv.index("--only")
        only = [w.strip().upper() for w in sys.argv[i + 1].split(",") if w.strip()]
        del sys.argv[i:i + 2]
    keep = "--keep" in sys.argv
    if keep:
        sys.argv.remove("--keep")
    allowMissing = "--allow-missing-codes" in sys.argv
    if allowMissing:
        sys.argv.remove("--allow-missing-codes")

    print("under test: %s" % underTest(), flush=True)

    def checkApp(display, name, results, verbose=False):
        import one_process_test as op
        op.setLayout(display, "normal")
        root = tempfile.mkdtemp(prefix="ecce-releaseround-")
        print("work folder: %s" % root, flush=True)
        r = Round(display, root)
        try:
            for w in only:
                print("---- %s" % w, flush=True)
                try:
                    getattr(r, w.lower())()
                except Exception as e:   # one broken workflow must not hide the rest
                    import traceback
                    traceback.print_exc()
                    step(w + ".x", "FAIL", "runner error: %r" % e)
        finally:
            if not keep:
                shutil.rmtree(root, ignore_errors=True)
        reported = set(s[0] for s in STEPS)
        # EXPECTED is hand-kept: a numbered step in the source beyond it
        # would never be required to report.
        src = open(__file__).read()
        for w in only:
            top = max([int(n) for n in re.findall(r'"%s\.(\d+)"' % w, src)] or [0])
            if top > EXPECTED.get(w, 0):
                step(w + ".x", "FAIL", "the source has %s.%d but EXPECTED says %d: "
                     "update EXPECTED" % (w, top, EXPECTED.get(w, 0)))
        for w in only:
            for n in range(1, EXPECTED.get(w, 0) + 1):
                sid = "%s.%d" % (w, n)
                if sid not in reported:
                    step(sid, "FAIL", "not reached: the runner reported nothing for this step")
        fails = [s for s in STEPS if s[1] == "FAIL"]
        if fails:
            results.fail("releaseround", "; ".join(s[0] for s in fails))

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer", "--budget", "7200"] + sys.argv[1:]
    rc = run_tests.main()
    if not STEPS:
        step("round", "FAIL", "nothing was checked (harness exit %s): no install, no "
             "Xvfb, or the Organizer did not start" % rc)
    elif rc != 0 and not any(s[1] == "FAIL" for s in STEPS):
        step("round", "FAIL", "the test harness exited %s with no step failed: "
             "see its output above" % rc)
    missing = sorted(c for c, w in CODES.items() if w in only and not which(c))
    if missing and not allowMissing:
        step("codes", "FAIL", "not on PATH: %s. The steps that need them reported "
             "SKIP, so %s launched nothing. Install the codes, or pass "
             "--allow-missing-codes for a partial round."
             % (", ".join(missing),
                ", ".join(sorted(set(CODES[c] for c in missing)))))
    elif missing:
        step("codes", "SKIP", "not on PATH: %s; partial round by "
             "--allow-missing-codes" % ", ".join(missing))
    print("\n==== release round summary")
    print("under test: %s" % underTest())
    for s in STEPS:
        print("%-6s %-6s %s" % (s[0], s[1], s[2][:200]))
    count = {k: sum(1 for s in STEPS if s[1] == k)
             for k in ("PASS", "FAIL", "MANUAL", "SKIP")}
    print("PASS %(PASS)d  FAIL %(FAIL)d  MANUAL %(MANUAL)d  SKIP %(SKIP)d" % count)
    if count["FAIL"] or rc != 0:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
