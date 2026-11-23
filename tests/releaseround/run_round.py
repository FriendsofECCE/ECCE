#!/usr/bin/env python3
"""
The release workflow scripts W1-W6 (wiki Release-9.6.0, "Workflow scripts"),
headless, through ECCE's own test hooks.

    tests/releaseround/run_round.py [--only W1,W4] [--keep-going]

Each workflow starts a fresh Organizer (one-program mode, Normal layout)
on an Xvfb display with the isolated HOME and data folder of tests/apps
(ECCE_TEST_HOME, ECCE_TEST_WRAPPERS), drives it with ECCE_TEST_ORGANIZER
commands, ECCE_BUILDER_SCRIPT for Builder and Viewer tabs, and quits it.
One line per step: `W1.3 PASS|FAIL|MANUAL|SKIP <evidence or reason>`,
then a summary; exit 1 on any FAIL, 77 without an install or Xvfb.
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


def sessionProcesses(data):
    """(pid, comm) of processes started for this test's data folder."""
    if not os.path.isdir("/proc"):
        return None
    mark = ("ECCE_LOCAL_DATA=%s" % data).encode()
    me = os.getpid()
    found = []
    for pid in os.listdir("/proc"):
        if not pid.isdigit() or int(pid) == me:
            continue
        try:
            if mark in open("/proc/%s/environ" % pid, "rb").read().split(b"\0"):
                found.append((int(pid), open("/proc/%s/comm" % pid).read().strip()))
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
            ok = bool(copies)
            if ok:
                self.w1calc = "file://" + copies[0][0]
            if not passfail("W1.1", ok, "examples: %s; paste: %s; copy: %s"
                            % (ex, paste, copies[:1])):
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

    def viewerCheck(self, org, url, sid, wants, panels=2, atom=False):
        """Viewer tab: the Properties list has each wanted kind; clicks the
        first `panels` entries; with atom, selects atom 1 (#1236)."""
        b = org.builderScript(["wait 4000", "plist"])
        m = org.mark()
        org.send("start CalculationViewer " + url)
        org.waitTool()
        org.waitScript(b)
        _o, info = org.scriptOut(m)
        entries = [i[len("BUILDER: entry "):] for i in info
                   if i.startswith("BUILDER: entry ")]
        bi = org.send("builderinfo") or ""
        org.closeTab("Viewer")
        missing = [k for k, rx in wants
                   if not any(re.search(rx, e, re.I) for e in entries)
                   and not (k == "geometry" and re.search(r"atoms=[1-9]", bi))]
        lines = ["wait 4000"]
        picks = [e.split("/", 1)[-1] for e in entries[:panels]]
        lines += ["list " + p for p in picks] + ["wait 1500", "overlay"]
        if atom:
            lines += ["selectoverlay 1"]
        b = org.builderScript(lines)
        m = org.mark()
        org.send("start CalculationViewer " + url)
        org.waitTool()
        done = org.waitScript(b)
        outs, info = org.scriptOut(m)
        bad = [o for o in outs if ": ok" not in o and not o.startswith("shot")]
        sel = [i for i in info if i.startswith("SELOVERLAY:")]
        ok = done and entries and not missing and not bad
        if wants:
            passfail(sid, ok, "%s; entries=%s; clicked=%s; missing=%s; errors=%s"
                     % (bi, entries, picks, missing, bad))
        return {"entries": entries, "picks": picks, "bad": bad, "sel": sel,
                "done": done, "info": bi}

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
            step("W2.4c", "PASS" if tabIndex(tabs, "Builder") is None else "FAIL",
                 "closing the edited Builder tab: asked=%s tabs=%s" % (asked, tabs))

            # Save As's calculation dialog has no hook: the same structure
            # goes into a Vina calculation made with File > New, whose
            # Builder imports and edits it as above and saves.
            ans = org.send("newcalc %s dock181l Vina" % self.projName()) or ""
            url = ans.split()[1] if ans.startswith("ok ") else None
            if not url:
                step("W2.5", "FAIL", "newcalc Vina: %s" % ans)
                step("W2.6", "SKIP", "no calculation")
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
                     % ([o for o in outs if o.startswith("save")],
                        lig and lig.group(1), save, stored[:1]))
            if not which("vina"):
                step("W2.6", "SKIP", "vina is not installed on this host; W2 "
                     "stopped at the saved deck")
                return
            launch = org.send("setup launch", 60) or ""
            go = org.send("setup launch go", 120) or ""
            states = waitState(org, url, DONE, 1800)
            if completed(states[-1]):
                self.viewerCheck(org, url, "W2.6", (("poses", r"pose|dock"),))
            else:
                step("W2.6", "FAIL", "launch=[%s] go=[%s] states=%s"
                     % (launch, go, " -> ".join(states)))
        finally:
            org.quit()
            org.kill()

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
                                                        ("cell", r"cell|lattice|geom")))
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
                okp = r["done"] and len(r["picks"]) == 2 and not r["bad"]
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
            open(org.script, "w").close()
            for line in ("start Builder " + u1, "start CalculationEditor " + u1,
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
            passfail("W5.3", alive and items and all(p for p in picks),
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
            passfail("W5.4", len(tabPages(tabs)) == 1 and not asked,
                     "closed %s; needed an answer: %s; left %s" % (closed, asked, tabs))
            secs = org.quit(30)
            time.sleep(3)
            left = sessionProcesses(self.data)
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
                passfail("W6.5", r["done"] and re.search(r"atoms=[1-9]", r["info"]),
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


def main():
    only = list(WORKFLOWS)
    if "--only" in sys.argv:
        i = sys.argv.index("--only")
        only = [w.strip().upper() for w in sys.argv[i + 1].split(",") if w.strip()]
        del sys.argv[i:i + 2]
    keep = "--keep" in sys.argv
    if keep:
        sys.argv.remove("--keep")

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
        fails = [s for s in STEPS if s[1] == "FAIL"]
        if fails:
            results.fail("releaseround", "; ".join(s[0] for s in fails))

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer", "--budget", "7200"] + sys.argv[1:]
    rc = run_tests.main()
    print("\n==== release round summary")
    for s in STEPS:
        print("%-6s %-6s %s" % (s[0], s[1], s[2][:200]))
    count = {k: sum(1 for s in STEPS if s[1] == k)
             for k in ("PASS", "FAIL", "MANUAL", "SKIP")}
    print("PASS %(PASS)d  FAIL %(FAIL)d  MANUAL %(MANUAL)d  SKIP %(SKIP)d" % count)
    if count["FAIL"]:
        return 1
    return rc if not STEPS else 0


if __name__ == "__main__":
    sys.exit(main())
