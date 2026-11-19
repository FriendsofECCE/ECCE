#!/usr/bin/env python3
"""One-program (#983) campaign scenarios A B C E F H I, headless.

  monolith_campaign_test.py [SCENARIO ...]      (default: A B C E F H I)

Each scenario starts the Organizer with ECCE_ONE_PROCESS=1 and drives it
through its ECCE_TEST_ORGANIZER hooks (and ECCE_VIEWER_SCENE for the
Viewer panels); see the shared scenario matrix of the #983 test campaign:
  A  first start, second start hands over, `ecce FILE` into a running copy
  B  every tool as a tab: open, switch, close each, all at once, closetools
  C  Viewer property panels on stored examples, the tab closed with them up
  E  Duplicate, the editor and Launcher on the copy (no job is launched)
  F  Quit paths: no tools, all tools, unsaved Builder, Ctrl-Q, the window
     manager's close, a second Quit while the question is up
  H  soak: 30 tool tabs opened and closed; RSS, threads, fds sampled
  I  log noise (Gtk-CRITICAL, assertions, "Debug:", wx log lines) in all
     of the above, as findings
Every Organizer runs with its own marker in the environment, so the check
for processes left behind sees only this test's.  Same install, isolation
and services as run_tests.py; exit 77 without an install or Xvfb.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

EX = os.path.join(REPO, "data", "client", "examples")
failures = []
LOGS = []          # every Organizer's output, for scenario I


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail.replace("\n", "\n      "))
                       if (detail and not ok) else ""), flush=True)
    if not ok:
        failures.append(what)
    return ok


def note(text):
    print("      " + text, flush=True)


def stored(rel):
    d = os.path.join(EX, rel, "stored")
    return os.path.join(d, [x for x in sorted(os.listdir(d))
                            if os.path.isdir(os.path.join(d, x))][0])


# One stored calculation per offered code where the examples have one.
CODES = {
    "ecceqm": "getting-started/ecceqm-water",
    "orca": "getting-started/orca-benzene",
    "g16": "workflows/sn2-cl-ch3br/g16-product",
    "nwchem": "getting-started/nwchem-methane",
    "mopac": "getting-started/mopac-ammonia",
    "xtb": "getting-started/xtb-water",
    "qe": "genchem/diffraction/qe-kcl",
}
# Viewer panels (scenario C): examples with many properties.
VIEWER = {
    "hf": "genchem/molecular-orbitals-of-diatomics/ecceqm-HF",
    "acala": "spectroscopy/amides-and-peptides/orca-ac_ala_nhme",
    "h2scan": "computational/h2-dissociation/orca-h2_rhf_scan",
    "methane": "getting-started/nwchem-methane",
}


def localData(names):
    folder = tempfile.mkdtemp(prefix="ecce-campaign-data-")
    calcs = {}
    for n in names:
        rel = CODES.get(n) or VIEWER.get(n)
        try:
            calcs[n] = stored(rel)
        except OSError:
            note("no stored example %s (%s)" % (n, rel))
    return folder, installLocal(folder, calcs)


class Org(object):
    """One Organizer in one-program mode, driven through its test hooks."""

    def __init__(self, display, data, env=None, args=()):
        self.work = tempfile.mkdtemp(prefix="ecce-campaign-")
        self.cmds = os.path.join(self.work, "commands")
        open(self.cmds, "w").close()
        self.tag = uuid.uuid4().hex
        self.data = data
        self.log = os.path.join(self.work, "organizer.log")
        self.env = display.env()
        self.env.update({"ECCE_ONE_PROCESS": "1",
                         "ECCE_TEST_ORGANIZER": self.cmds,
                         "ECCE_LOCAL_DATA": data,
                         "ECCE_CAMPAIGN_TAG": self.tag})
        self.env.update(env or {})
        self.proc = subprocess.Popen(
            [os.path.join(apps.WRAPPERS, "ecce-organizer")] + list(args),
            env=self.env, stdout=open(self.log, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)
        self.n = 0
        self.ready = self.ask("inprocess", 90) is not None

    def text(self):
        with open(self.log, errors="replace") as handle:
            return handle.read()

    def send(self, line, wait=0):
        with open(self.cmds, "a") as handle:
            handle.write(line + "\n")
        time.sleep(wait)

    def answers(self, command):
        return re.findall(r"ECCE_TEST_ORGANIZER: %s[^:\n]*: (.*)"
                          % re.escape(command), self.text())

    def ask(self, line, timeout=20):
        """Send a hook command, return its (newest) answer or None."""
        command = line.split()[0]
        before = len(self.answers(command))
        self.send(line)
        deadline = time.time() + timeout
        while time.time() < deadline:
            got = self.answers(command)
            if len(got) > before:
                return got[-1]
            if self.proc.poll() is not None:
                return None
            time.sleep(0.3)
        return None

    def pid(self):
        a = self.answers("inprocess")
        m = re.search(r"pid=(\d+)", a[-1]) if a else None
        return int(m.group(1)) if m else None

    def sample(self):
        """RSS (kB), threads and open fds of the Organizer process."""
        pid = self.pid()
        out = {"pid": pid}
        try:
            for line in open("/proc/%d/status" % pid):
                if line.startswith(("VmRSS:", "Threads:")):
                    k, v = line.split(":", 1)
                    out[k] = int(v.split()[0])
            out["fds"] = len(os.listdir("/proc/%d/fd" % pid))
        except (OSError, TypeError):
            pass
        return out

    def waitExit(self, timeout=60):
        try:
            return self.proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            return None

    def leftovers(self):
        """Processes still carrying this Organizer's marker."""
        mark = ("ECCE_CAMPAIGN_TAG=%s" % self.tag).encode()
        left = []
        for pid in os.listdir("/proc"):
            if not pid.isdigit() or int(pid) == os.getpid():
                continue
            try:
                if mark in open("/proc/%s/environ" % pid, "rb").read().split(b"\0"):
                    left.append("%s:%s" % (pid, open("/proc/%s/comm" % pid)
                                           .read().strip()))
            except OSError:
                pass
        return left

    def stop(self):
        text = self.text()
        LOGS.append(text)
        if self.proc.poll() is None:
            try:
                os.killpg(self.proc.pid, 15)
                self.proc.wait(timeout=20)
            except (OSError, subprocess.TimeoutExpired):
                try:
                    os.killpg(self.proc.pid, 9)
                except OSError:
                    pass
        crashed = self.proc.returncode is not None and self.proc.returncode < 0 \
            and self.proc.returncode != -15
        for pid in self.leftovers():
            try:
                os.kill(int(pid.split(":")[0]), 9)
            except OSError:
                pass
        shutil.rmtree(self.work, ignore_errors=True)
        return text, crashed


def closeAll(o, answers=4):
    o.send("closetools", 3)
    for _ in range(answers):
        o.send("answer no", 1.5)


def quitAndCheck(o, label, steps, expectExit=True):
    for line, wait in steps:
        o.send(line, wait)
    rc = o.waitExit(60 if expectExit else 10)
    for _ in range(30):
        time.sleep(1)
        left = o.leftovers()
        if not left:
            break
    if expectExit:
        check(rc == 0, "F %s: the Organizer exits with status 0" % label,
              "status %s; tail: %s" % (rc, o.text()[-1500:]))
        check(not left, "F %s: no ECCE process left" % label, " ".join(left))
    else:
        check(rc is None, "F %s: still running" % label, "status %s" % rc)
    return rc, left


# ---------------------------------------------------------------- scenarios

def scenarioA(display):
    data, urls = localData(["ecceqm"])
    o = Org(display, data)
    try:
        if not check(o.ready, "A first start: the Organizer answers its hooks",
                     o.text()[-1500:]):
            return
        env = dict(o.env)
        env.pop("ECCE_TEST_ORGANIZER")
        env["ECCE_CAMPAIGN_TAG"] = o.tag + "-second"
        t0 = time.time()
        r = subprocess.run([os.path.join(apps.WRAPPERS, "ecce-organizer")],
                           env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, text=True, timeout=120)
        check(r.returncode == 0 and "already running" in r.stdout,
              "A second plain start hands over and exits (%.1f s)"
              % (time.time() - t0), "status %d: %s" % (r.returncode, r.stdout[-800:]))
        xyz = os.path.join(data, "co2.xyz")
        with open(xyz, "w") as h:
            h.write("3\nco2\nC 0 0 0\nO 0 0 1.16\nO 0 0 -1.16\n")
        r = subprocess.run([os.path.join(apps.WRAPPERS, "ecce-organizer"), xyz],
                           env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, text=True, timeout=120)
        time.sleep(10)
        o.send("answer", 6)         # an .xyz asks for its units first
        tabs = o.ask("tabs") or ""
        info = o.ask("builderinfo") or ""
        check(r.returncode == 0 and "Builder=1" in (o.ask("inprocess") or ""),
              "A `ecce FILE.xyz` into the running copy opens a Builder tab",
              "status %d: %s | tabs %s" % (r.returncode, r.stdout[-400:], tabs))
        check("atoms=3" in info, "A the handed-over file shows its 3 atoms",
              info + " | " + " | ".join(re.findall(r"ECCE_TEST_ORGANIZER: startApp.*",
                                                   o.text())))
        closeAll(o)
        o.stop()
        o = Org(display, data, args=[xyz])
        time.sleep(10)
        o.send("answer", 6)
        info = o.ask("builderinfo") or ""
        check("atoms=3" in info, "A first start with FILE.xyz shows its 3 atoms",
              info + " | " + (o.ask("tabs") or ""))
        closeAll(o)
    finally:
        o.stop()
        shutil.rmtree(data, ignore_errors=True)


TOOLS = [("Builder", ""), ("PeriodicTable", ""), ("MachineBrowser", ""),
         ("MachineRegister", "")]


def scenarioB(display):
    data, urls = localData(list(CODES) + ["hf"])
    o = Org(display, data)
    try:
        if not check(o.ready, "B the Organizer answers its hooks", o.text()[-1500:]):
            return
        plan = [("Builder", "")] + [("Builder", urls["hf"])]
        plan += [("CalculationEditor", urls[c]) for c in CODES if c in urls]
        plan += [("Basistool", urls["ecceqm"]), ("Launcher", urls["ecceqm"]),
                 ("PeriodicTable", ""), ("MachineBrowser", ""),
                 ("MachineRegister", ""), ("MDEnergy", ""), ("MDPrepare", "")]
        # Each alone: open, check, close.
        for tool, url in plan:
            label = "%s %s" % (tool, os.path.basename(url.rstrip("/")) or "")
            o.send("start %s %s" % (tool, url), 12)
            state = o.ask("toolstate") or ""
            inproc = o.ask("inprocess") or ""
            tabs = o.ask("tabs") or ""
            ok = o.proc.poll() is None
            check(ok and "timed out" not in state,
                  "B %s opens (%s)" % (label, state.split(" status")[0]),
                  "inprocess %s | tabs %s | state %s" % (inproc, tabs, state))
            if not ok:
                return
            closeAll(o, 2)
            after = o.ask("inprocess") or ""
            check(not re.search(r"=[1-9]", after.split(" ", 1)[-1]),
                  "B %s closes" % label, after)
        o.send("menu preferences", 4)
        check(o.proc.poll() is None, "B Preferences opens")
        o.send("answer cancel", 2)
        # All at once, switch through the tabs, then closetools.
        for tool, url in plan:
            o.send("start %s %s" % (tool, url), 6)
        time.sleep(8)
        tabs = o.ask("tabs") or ""
        n = len(re.findall(r"\d+:", tabs.split(" windows")[0]))
        note("all open: " + tabs[:400])
        for k in range(n):
            o.send("tab %d" % k, 1)
        check(o.proc.poll() is None, "B switching through %d tabs" % n)
        closeAll(o, 8)
        after = o.ask("inprocess") or ""
        check(not re.search(r"=[1-9]", after.split(" ", 1)[-1]),
              "B closetools closes them all", after)
        check(o.proc.poll() is None, "B the Organizer still runs")
    finally:
        text, crashed = o.stop()
        check(not crashed, "B no crash", "status %s" % o.proc.returncode)
        shutil.rmtree(data, ignore_errors=True)


def scenarioC(display):
    data, urls = localData(list(VIEWER))
    try:
        for name, url in urls.items():
            out = tempfile.mkdtemp(prefix="ecce-campaign-scene-")
            scene = os.path.join(out, "scene")
            with open(scene, "w") as h:
                h.write("hold 4\nptree before\n")
            o = Org(display, data, env={"ECCE_VIEWER_SCENE": scene,
                                        "ECCE_VIEWER_SCENE_OUT": out,
                                        "ECCE_VIEWER_SCENE_HOLD": "600"})
            o.send("start Builder " + url, 25)
            entries = []
            try:
                with open(os.path.join(out, "before.txt")) as h:
                    entries = [l[6:].strip() for l in h if l.startswith("entry ")]
            except OSError:
                pass
            o.stop()
            if not check(entries, "C %s: the Viewer lists its properties" % name):
                continue
            note("%s: %s" % (name, ", ".join(entries)))
            # Every entry in turn, then the tab is closed with the last up.
            with open(scene, "w") as h:
                h.write("hold 4\n")
                for e in entries:
                    h.write("list %s\nhold 3\n" % e.split("/", 1)[-1])
                h.write("ptree after\nhold 600\n")
            o = Org(display, data, env={"ECCE_VIEWER_SCENE": scene,
                                        "ECCE_VIEWER_SCENE_OUT": out,
                                        "ECCE_VIEWER_SCENE_HOLD": "600"})
            o.send("start Builder " + url, 20 + 4 * len(entries))
            for _ in range(60):
                if os.path.exists(os.path.join(out, "after.txt")):
                    break
                time.sleep(3)
            fails = re.findall(r"ECCE_VIEWER_SCENE: (.*)", o.text())
            check(os.path.exists(os.path.join(out, "after.txt")) and
                  o.proc.poll() is None,
                  "C %s: every property panel opened" % name,
                  "\n".join(fails[-8:]) + o.text()[-800:])
            closeAll(o, 3)
            after = o.ask("inprocess") or ""
            check(o.proc.poll() is None and "Builder=0" in after,
                  "C %s: tab closed with its panels up" % name,
                  "status %s %s" % (o.proc.returncode, after))
            text, crashed = o.stop()
            check(not crashed, "C %s: no crash" % name)
            shutil.rmtree(out, ignore_errors=True)
    finally:
        shutil.rmtree(data, ignore_errors=True)


def scenarioE(display):
    data, urls = localData(["ecceqm"])
    o = Org(display, data)
    try:
        if not o.ready:
            check(False, "E the Organizer answers its hooks")
            return
        project = os.path.join(data, "users", "local", "qm")
        before = set(os.listdir(project))
        dup = o.ask("duplicate " + urls["ecceqm"], 60) or ""
        note("duplicate: " + dup[:300])
        new = sorted(set(os.listdir(project)) - before)
        copy = "file://" + os.path.join(project, new[0]) + "/" if new else ""
        check(bool(copy), "E Duplicate makes a copy", dup)
        if copy:
            o.send("start CalculationEditor " + copy, 15)
            o.send("start Launcher " + copy, 12)
            machines = o.ask("launcher get machines") or ""
            note("launcher machines: " + machines)
            inproc = o.ask("inprocess") or ""
            check("CalculationEditor=1" in inproc and "Launcher=1" in inproc,
                  "E the editor and the Launcher open on the copy", inproc)
        closeAll(o, 3)
        check(o.proc.poll() is None, "E the Organizer still runs")
    finally:
        text, crashed = o.stop()
        check(not crashed, "E no crash")
        shutil.rmtree(data, ignore_errors=True)


def scenarioF(display):
    data, urls = localData(["ecceqm", "hf"])
    every = [("Builder", ""), ("Builder", urls["hf"]),
             ("CalculationEditor", urls["ecceqm"]), ("Launcher", urls["ecceqm"]),
             ("MDEnergy", ""), ("PeriodicTable", "")]
    try:
        o = Org(display, data)
        quitAndCheck(o, "no tools", [("close", 3), ("answer", 3)])
        o.stop()

        o = Org(display, data)
        for t, u in every:
            o.send("start %s %s" % (t, u), 8)
        quitAndCheck(o, "every tool open", [("close", 3), ("answer", 3)] +
                     [("answer no", 2)] * 6)
        o.stop()

        script = os.path.join(tempfile.mkdtemp(prefix="ecce-campaign-"), "s")
        with open(script, "w") as h:
            h.write("wait 3000\nadd C Tetrahedral 8 0 0\nsavestate\n")
        import progress_box_test as pb
        fresh = "file://" + pb.makeCalc(os.path.join(data, "users", "local", "qm"),
                                        "water-new", "created", None)
        o = Org(display, data, env={"ECCE_BUILDER_SCRIPT": script})
        o.send("start Builder " + fresh, 25)
        quitAndCheck(o, "unsaved Builder, Cancel", [("close", 3), ("answer", 3),
                                                    ("answer cancel", 3)],
                     expectExit=False)
        quitAndCheck(o, "unsaved Builder, then Don't save",
                     [("close", 3), ("answer", 3), ("answer no", 3)])
        o.stop()

        o = Org(display, data)
        o.send("start Builder " + urls["hf"], 15)
        quitAndCheck(o, "Ctrl-Q (menu exit) from a tool tab",
                     [("menu exit", 3), ("answer", 3), ("answer no", 3)])
        o.stop()

        o = Org(display, data)
        o.send("start CalculationEditor " + urls["ecceqm"], 15)
        quitAndCheck(o, "window manager close", [("hostclose", 3), ("answer", 3),
                                                ("answer no", 3)])
        o.stop()

        o = Org(display, data)
        o.send("start Builder " + urls["hf"], 15)
        quitAndCheck(o, "second Quit while the question is up",
                     [("close", 2), ("close", 2), ("menu exit", 2),
                      ("answer", 3), ("answer", 3), ("answer no", 3),
                      ("answer no", 3)])
        o.stop()
    finally:
        shutil.rmtree(data, ignore_errors=True)


def scenarioH(display, rounds=30):
    data, urls = localData(["ecceqm", "orca", "hf", "acala", "methane", "mopac"])
    o = Org(display, data)
    samples = []
    try:
        if not check(o.ready, "H the Organizer answers its hooks"):
            return
        plan = [("Builder", urls["ecceqm"]), ("Builder", urls["hf"]),
                ("CalculationEditor", urls["orca"]), ("Builder", urls["acala"]),
                ("CalculationEditor", urls["mopac"]), ("Builder", urls["methane"])]
        o.ask("inprocess")
        samples.append((0, o.sample()))
        for k in range(rounds):
            tool, url = plan[k % len(plan)]
            o.send("start %s %s" % (tool, url), 10)
            closeAll(o, 2)
            if o.proc.poll() is not None:
                break
            if k + 1 in (1, rounds // 2, rounds):
                o.ask("inprocess")
                samples.append((k + 1, o.sample()))
        for k, s in samples:
            note("H after %2d tabs: RSS %s kB, threads %s, fds %s"
                 % (k, s.get("VmRSS"), s.get("Threads"), s.get("fds")))
        check(o.proc.poll() is None, "H the Organizer survives %d tabs" % rounds,
              "status %s %s" % (o.proc.returncode, o.text()[-1500:]))
        if len(samples) >= 3:
            first, last = samples[1][1], samples[-1][1]
            mid = samples[-2][1]
            grow = (last.get("VmRSS", 0) - first.get("VmRSS", 0)) / 1024.0
            check(grow < 150, "H RSS growth from tab 1 to %d under 150 MB "
                  "(%.0f MB; middle-to-end %.0f MB)" % (
                      rounds, grow,
                      (last.get("VmRSS", 0) - mid.get("VmRSS", 0)) / 1024.0))
            check(last.get("Threads", 0) - first.get("Threads", 0) <= 4,
                  "H thread count steady (%s -> %s)" % (first.get("Threads"),
                                                       last.get("Threads")))
            check(last.get("fds", 0) - first.get("fds", 0) <= 10,
                  "H fd count steady (%s -> %s)" % (first.get("fds"),
                                                   last.get("fds")))
        quitAndCheck(o, "after the soak", [("close", 3), ("answer", 3),
                                           ("answer no", 2), ("answer no", 2)])
    finally:
        o.stop()
        shutil.rmtree(data, ignore_errors=True)


NOISE = re.compile(r"Gtk-CRITICAL|Gtk-WARNING|GLib-\w+-CRITICAL|"
                   r"assert|Debug:|wxWidgets|ERROR: AddressSanitizer|"
                   r"runtime error:|LeakSanitizer|Segmentation|Aborted|"
                   r"Traceback|\bError\b")


def scenarioI(_display):
    seen = {}
    for text in LOGS:
        for line in text.splitlines():
            if NOISE.search(line) and "ECCE_TEST_ORGANIZER" not in line:
                key = re.sub(r"0x[0-9a-f]+|\d+", "N", line.strip())[:200]
                seen[key] = seen.get(key, 0) + 1
    for key, n in sorted(seen.items(), key=lambda kv: -kv[1]):
        note("I %4dx %s" % (n, key))
    check(not seen, "I no Gtk-CRITICAL, assertion or error lines in %d "
          "Organizer logs (%d distinct)" % (len(LOGS), len(seen)))


SCENARIOS = {"A": scenarioA, "B": scenarioB, "C": scenarioC, "E": scenarioE,
             "F": scenarioF, "H": scenarioH, "I": scenarioI}


def main():
    wanted = [a.upper() for a in sys.argv[1:] if a.upper() in SCENARIOS] or \
        list("ABCEFHI")
    sys.argv = [a for a in sys.argv if a.upper() not in SCENARIOS]
    if "I" in wanted:           # last: it reads the others' logs
        wanted = [w for w in wanted if w != "I"] + ["I"]

    def checkApp(display, name, results, verbose=False):
        for s in wanted:
            print("== scenario %s" % s, flush=True)
            started = time.time()
            try:
                SCENARIOS[s](display)
            except Exception as e:      # one scenario's error is a finding
                import traceback
                traceback.print_exc()
                check(False, "%s raised %s" % (s, e))
            print("== scenario %s: %.0f s" % (s, time.time() - started), flush=True)
        if failures:
            results.fail("monolith_campaign", "; ".join(failures))

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer", "--budget", "9000"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
