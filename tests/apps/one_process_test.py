#!/usr/bin/env python3
"""
Builder and Calculation Editor as windows of the Organizer's process
(#983, ECCE_ONE_PROCESS=1), headless.

The Organizer's test hook starts tools as its buttons do; with the switch
on they must open in the Organizer's process (same pid in the message
trace), the Builder's save notice must reach the Organizer, a second
Builder (another calculation) must open beside the first, the editor must
get its invoke, and closing the tool windows must leave the Organizer
running and answering.

Same installed tree, isolation and services as run_tests.py.  Exit 77
(skip) without an install or Xvfb.
"""

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
import run_tests  # noqa: E402
import progress_box_test as pb  # noqa: E402

failures = []
# --gdb: run the Organizer under gdb, for a backtrace on a crash.
GDB = "--gdb" in sys.argv
if GDB:
    sys.argv.remove("--gdb")
# --shots DIR: screenshots of the tabbed window (runTabs).
if "--shots" in sys.argv:
    i = sys.argv.index("--shots")
    os.environ["ECCE_TABS_SHOTS"] = sys.argv[i + 1]
    del sys.argv[i:i + 2]
    import xdisplay  # noqa: E402
    xdisplay.SCREEN = "1366x768x24"
# --tabs-only: only the tabbed run.
if "--tabs-only" in sys.argv:
    sys.argv.remove("--tabs-only")
    os.environ["ECCE_TABS_ONLY"] = "1"


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url1 = "file://" + pb.makeCalc(project, "water-a", "created", None)
        url2 = "file://" + pb.makeCalc(project, "water-b", "created", None)
        cmds = os.path.join(work, "commands")
        script = os.path.join(work, "builder-script")
        calced = os.path.join(work, "calced-script")
        open(cmds, "w").close()
        with open(calced, "w") as handle:
            # The editor's Basis Set Tool button, from the in-process editor.
            handle.write("wait 5000\ntool Basistool\n")
        with open(script, "w") as handle:
            handle.write("save\n")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start Builder " + url1, 25)
            append("start Builder " + url2, 15)
            append("start CalculationEditor " + url1, 15)
            append("start MDEnergy", 15)
            append("start MDPrepare", 15)
            time.sleep(10)                   # the editor's script, above
            append("inprocess", 2)
            append("closetools", 5)
            append("answer no", 3)    # questions the tools ask, if any
            append("answer no", 3)
            append("inprocess", 3)

        extra = {"ECCE_TEST_GDB": "1"} if GDB else {}
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**extra, "ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_TEST_MESSAGES": "1",
                               "ECCE_CALCED_SCRIPT": calced,
                               "ECCE_BUILDER_SCRIPT": script,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        check(not result.crashed and "Aborted" not in log,
              "the Organizer ran to the end without a crash",
              result.signalName or log[-1500:])

        inproc = pb.answers(log, "inprocess")
        pid = None
        if check(len(inproc) == 2, "both window counts answered", log[-2000:]):
            m = re.match(r"pid=(\d+) Builder=(\d+) CalculationEditor=(\d+)",
                         inproc[0])
            pid = m.group(1) if m else None
            check(m and m.group(2) == "2" and m.group(3) == "1"
                  and "MDEnergy=1 BasisTool=1 MDPrepare=1" in inproc[0],
                  "two Builders, one editor, MD Energy, MD Prepare and the "
                  "Basis Set Tool (from the editor's button) open in the "
                  "Organizer",
                  inproc[0])
            check("Builder=0 CalculationEditor=0 MDEnergy=0 BasisTool=0 "
                  "MDPrepare=0" in inproc[1],
                  "closing the tool windows leaves the Organizer running",
                  inproc[1])
        check(pb.answers(log, "closetools") == ["queued", "closed"],
              "the tool windows closed", str(pb.answers(log, "closetools")))

        trace = re.findall(r"ECCE_TEST_MESSAGES: (\S+) (\S+) pid=(\d+) "
                           r"(\S+) from (\S*) (\S*)", log)
        builders = set(t[1] for t in trace
                       if t[0] == "Builder" and t[3] == "ecce_invoke")
        check(len(builders) == 2 and all(t[2] == pid for t in trace),
              "each Builder got its invoke, in the Organizer's process",
              str(sorted(builders)) + " pid " + str(pid))
        check(any(t[0] == "CalculationEditor" and t[3] == "ecce_invoke"
                  and t[2] == pid for t in trace),
              "the editor got its invoke in the Organizer's process")
        check(any(t[0] == "Organizer" and t[3].startswith("ecce_url_")
                  and t[4] == "Builder" for t in trace),
              "the Builder's save notice reached the Organizer",
              "\n".join(" ".join(t) for t in trace if t[0] == "Organizer"))
        for t in trace:
            print("      trace: " + " ".join(t))
        if GDB:
            print(log[-20000:])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runQuit(display, gateway=False):
    """Quit asks the in-process Builder about its unsaved edit: Cancel there
    cancels the Quit, No lets the process end.  Under the MainFrame (small
    layout): the Organizer's frame goes before it, and no window is left."""
    setLayout(display, "small")
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-q", "created", None)
        cmds = os.path.join(work, "commands")
        script = os.path.join(work, "builder-script")
        open(cmds, "w").close()
        with open(script, "w") as handle:
            handle.write("wait 3000\nadd C Tetrahedral 8 0 0\nsavestate\n")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start Builder " + url, 25)
            if not gateway:
                append("start MDEnergy", 15)   # Quit with these open
                append("start Basistool " + url, 15)
            append("inprocess", 2)
            if gateway:
                # The session's ecce_quit: forced, the Builder still asks
                # about its edit (Yes/No).
                append("gatewayquit", 3)
                append("answer no", 3)
                append("answer no", 25)
                return
            append("close", 3)          # File > Quit
            append("answer", 3)         # "Quit"
            append("answer cancel", 3)  # the Builder's save question
            append("inprocess", 2)
            append("close", 3)
            append("answer", 3)
            append("answer no", 3)         # the Builder
            # The Basis Set Tool's own save question for the calculation it
            # was opened on; the process
            # ends after the broker's 10 s linger (OnExit).
            append("answer no", 25)
            if GDB:
                # Where a process that did not end is: attach, all threads.
                pids = [p for p in os.listdir("/proc") if p.isdigit() and
                        open("/proc/%s/comm" % p).read().strip() == "organizer"]
                for p in pids:
                    print(subprocess.run(
                        ["gdb", "-batch", "-p", p, "-ex",
                         "thread apply all bt 40"],
                        capture_output=True, text=True).stdout[-15000:])

        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_BUILDER_SCRIPT": script,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        inproc = pb.answers(log, "inprocess")
        check(inproc and "Builder=1 CalculationEditor=0" in inproc[0],
              "the Builder opened in the Organizer", str(inproc) + log[-1500:])
        check("BUILDER: dirty=1" in log, "the Builder has unsaved changes")
        if gateway:
            check(pb.answers(log, "gatewayquit") == ["sent"] and
                  pb.answers(log, "answer no")[:1] ==
                  ["answered Save Builder Changes?"],
                  "ecce_quit: the Builder asked about its changes once",
                  str(pb.answers(log, "answer no")))
            check(result.returncode == 0 and "Aborted" not in log,
                  "ecce_quit ends the process cleanly",
                  "returncode %s\n%s" % (result.returncode, log[-1500:]))
        else:
            check("MDEnergy=1 BasisTool=1" in inproc[0],
                  "an MD editor and the Basis Set Tool opened in the Organizer",
                  inproc[0])
            check(len(inproc) == 2 and
                  "Builder=1 CalculationEditor=0" in inproc[1],
                  "Cancel in the Builder's question cancels the Quit",
                  str(inproc))
            # Which questions were answered (titles), for the record.
            print("      answers: " + str(pb.answers(log, "answer")))
            check(pb.answers(log, "answer cancel") ==
                  ["answered Save Builder Changes?"] and
                  pb.answers(log, "answer no") ==
                  ["answered Save Builder Changes?",
                   "answered Save Basis Set Tool Changes?"],
                  "the Builder asked about its changes at each Quit, then "
                  "the Basis Set Tool about its own",
                  str(pb.answers(log, "answer")))
            check(result.returncode == 0 and "Aborted" not in log,
                  "No in the Builder's question ends the process cleanly",
                  "returncode %s\n%s" % (result.returncode, log[-1500:]))
            # The hook only reports while the main loop is still running,
            # i.e. when a window outlived the Organizer.
            left = re.search(r"left after quit:([^\n]*)", log)
            check(left is None, "no top-level window is left after Quit",
                  left.group(1) if left else "")
        if GDB:
            print(log[-20000:])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runQuitHost(display):
    """The window manager's close of the MainFrame (hook hostclose) is a
    Quit: the Quit question, then the dirty Builder tab's, then a clean exit."""
    setLayout(display, "small")
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-h", "created", None)
        cmds = os.path.join(work, "commands")
        script = os.path.join(work, "builder-script")
        open(cmds, "w").close()
        with open(script, "w") as handle:
            handle.write("wait 3000\nadd C Tetrahedral 8 0 0\nsavestate\n")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start Builder " + url, 25)
            append("quitlabel", 2)         # the Builder tab's menu bar
            append("hostclose", 3)
            append("answer cancel", 3)     # Quit ECCE: Cancel, still running
            append("inprocess", 2)
            append("hostclose", 3)
            append("answer", 3)            # Quit
            append("answer no", 25)        # the Builder's question
            # The listener went with the window: a start now is a new
            # Organizer, not "already running".
            environment = display.env()
            environment.update({"ECCE_ONE_PROCESS": "1",
                                "ECCE_LOCAL_DATA": data})
            proc = subprocess.Popen(
                [os.path.join(apps.WRAPPERS, "ecce-organizer")],
                env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True)
            time.sleep(25)
            seen["after"] = proc.poll() is None
            proc.terminate()
            try:
                seen["after_out"] = proc.communicate(timeout=20)[0]
            except subprocess.TimeoutExpired:
                proc.kill()
                seen["after_out"] = proc.communicate()[0]

        seen = {}
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_BUILDER_SCRIPT": script,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        check(pb.answers(log, "quitlabel") == ["Quit ECCE"],
              "a tool tab's File > Quit reads Quit ECCE",
              str(pb.answers(log, "quitlabel")))
        check(pb.answers(log, "answer cancel") == ["answered Quit ECCE"] and
              len(pb.answers(log, "inprocess")) == 1 and
              "Builder=1" in pb.answers(log, "inprocess")[0],
              "closing the host asks Quit ECCE; Cancel keeps everything",
              str(pb.answers(log, "answer cancel")) + log[-1500:])
        check(pb.answers(log, "answer") == ["answered Quit ECCE"] * 2 +
              ["answered Save Builder Changes?"],
              "closing the host again asks Quit, then the Builder's question",
              str(pb.answers(log, "answer")))
        check(seen.get("after") is True and
              "already running" not in seen.get("after_out", ""),
              "a start after Quit opens a window, not 'already running'",
              str(seen))
        left = re.search(r"left after quit:([^\n]*)", log)
        check(result.returncode == 0 and "Aborted" not in log and left is None,
              "closing the host ends the process cleanly, nothing left",
              "returncode %s\n%s" % (result.returncode, log[-1500:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runSingleInstance(display):
    """A second start of the same user and data folder brings the running
    window to the front instead of opening another Organizer (#1069); file
    names in it open in a Builder tab of the first."""
    setLayout(display, "small")
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        os.makedirs(os.path.join(data, "users", "local"))
        xyz = os.path.join(work, "wa ter#1%.xyz")
        with open(xyz, "w") as handle:
            handle.write("3\nwater\nO 0 0 0.117\nH 0 0.757 -0.469\n"
                         "H 0 -0.757 -0.469\n")
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()
        script = os.path.join(work, "builder-script")
        with open(script, "w") as handle:
            handle.write("wait 3000\nexpect atoms 3\n")
        env = {"ECCE_ONE_PROCESS": "1", "ECCE_LOCAL_DATA": data}
        seen = {}

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def organizers():
            out = subprocess.run(["pgrep", "-fc", "bin/organizer"],
                                 stdout=subprocess.PIPE, text=True).stdout
            return int(out.strip() or 0)

        def second(args):
            environment = display.env()
            environment.update(env)
            started = time.time()
            done = subprocess.run(
                [os.path.join(apps.WRAPPERS, "ecce-organizer")] + args,
                env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True, timeout=90)
            return done.returncode, time.time() - started, done.stdout

        def inspect(_display):
            append("raised", 2)
            seen["before"] = organizers()
            seen["plain"] = second([])
            seen["file"] = second([xyz])
            time.sleep(20)                  # the Builder tab opens
            append("raised", 2)
            append("inprocess", 3)
            seen["after"] = organizers()

        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**env, "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_BUILDER_SCRIPT": script},
                          inspect=inspect)
        log = result.log or ""
        for what in ("plain", "file"):
            rc, secs, text = seen.get(what, (None, 0, ""))
            check(rc == 0 and secs < 30 and "already running" in text,
                  "a second start (%s) exits at once with a message" % what,
                  "rc=%s %.0f s\n%s" % (rc, secs, text[-800:]))
        check(pb.answers(log, "raised") == ["0", "2"],
              "the running window was asked to come to the front twice",
              str(pb.answers(log, "raised")) + log[-1500:])
        check(seen.get("before") is not None and
              seen.get("before") == seen.get("after"),
              "still the same number of organizer processes",
              "%s -> %s" % (seen.get("before"), seen.get("after")))
        inproc = pb.answers(log, "inprocess")
        check(len(inproc) == 1 and "Builder=1" in inproc[0],
              "the .xyz named by the second start opened in a Builder of the first",
              str(inproc) + log[-1500:])
        check("FAIL" not in "\n".join(l for l in log.splitlines()
                                      if "ECCE_BUILDER_SCRIPT" in l),
              "the Builder holds the 3 atoms of the file (name with a space, "
              "'#' and '%')",
              "\n".join(l for l in log.splitlines()
                        if "ECCE_BUILDER_SCRIPT" in l or "atoms" in l)[-800:])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def organizerOutput(display, env, seconds=30):
    """Start the Organizer by itself, let it run, return what it printed."""
    environment = display.env()
    environment.update(env)
    proc = subprocess.Popen([os.path.join(apps.WRAPPERS, "ecce-organizer")],
                            env=environment, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            start_new_session=True)
    time.sleep(seconds)
    alive = proc.poll() is None
    apps._terminate(proc)
    return alive, apps._drain(proc)


def runUntrustedDir(display):
    """A run folder that is not private to the user (mode 0777, a symlink
    in it) is not used: single-instance checking is off, the file the link
    points to is untouched, and the Organizer still starts (#1065 review)."""
    setLayout(display, "small")
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        os.makedirs(os.path.join(data, "users", "local"))
        run = os.path.join(work, "run")
        folder = os.path.join(run, "ecce-one")
        os.makedirs(folder)
        os.chmod(run, 0o700)
        os.chmod(folder, 0o777)
        victim = os.path.join(work, "victim")
        with open(victim, "w") as handle:
            handle.write("keep")
        # The token name is not predictable; the folder check comes first.
        link = os.path.join(folder, "x.tok")
        os.symlink(victim, link)
        env = {"ECCE_ONE_PROCESS": "1", "ECCE_LOCAL_DATA": data,
               "XDG_RUNTIME_DIR": run}
        alive, log = organizerOutput(display, env)
        check("not a private folder" in log,
              "an untrusted run folder is reported and single-instance is off",
              log[-1500:])
        check(open(victim).read() == "keep",
              "the file a symlink in the folder points to is untouched")
        check(sorted(os.listdir(folder)) == ["x.tok"],
              "nothing is created in the untrusted folder",
              str(os.listdir(folder)))
        check(alive, "the Organizer started anyway", log[-800:])
        # A private one is used, with a token only the user can read.
        run2 = os.path.join(work, "run2")
        os.makedirs(run2)
        os.chmod(run2, 0o700)
        env["XDG_RUNTIME_DIR"] = run2
        organizerOutput(display, env)
        toks = [f for f in os.listdir(os.path.join(run2, "ecce-one"))
                if f.endswith(".tok")] if os.path.isdir(
                    os.path.join(run2, "ecce-one")) else []
        check(len(toks) == 1 and oct(os.stat(os.path.join(
            run2, "ecce-one", toks[0])).st_mode & 0o777) == "0o600",
            "a private run folder holds a token readable only by the user",
            str(toks))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runGeometry(display):
    """The host saves its rectangle under the Organizer's prefix: a size set
    in one run is the window's size in the next."""
    setLayout(display, "small")
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        cmds = os.path.join(work, "commands")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def start(inspect):
            open(cmds, "w").close()
            result = apps.run(display, "organizer", windowTimeout=60,
                              settle=20,
                              env={"ECCE_ONE_PROCESS": "1",
                                   "ECCE_TEST_ORGANIZER": cmds,
                                   "ECCE_LOCAL_DATA": data},
                              inspect=inspect)
            return result, result.log or ""

        def first(_display):
            append("size 1150 850", 3)
            append("close", 3)
            append("answer", 20)

        def second(_display):
            append("size", 3)
            append("close", 3)
            append("answer", 20)

        result, log = start(first)
        sized = pb.answers(log, "size")
        check(sized == ["ok 1150x850"], "the host takes a size", str(sized))
        result, log = start(second)
        sized = pb.answers(log, "size")
        check(sized == ["ok 1150x850"] and result.returncode == 0,
              "the next start restores the host's size", str(sized) + log[-800:])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def setLayout(display, preset, rows=None):
    """Preferences > Windows and tabs before the Organizer starts: the tab
    strip is made at its start.  rows: {tool key: "tab"|"window"}."""
    home = display.env().get("ECCE_REALUSERHOME") or os.environ["ECCE_REALUSERHOME"]
    prefs = os.path.join(home, ".ECCE", "EcceGlobal")
    os.makedirs(os.path.dirname(prefs), exist_ok=True)
    lines = []
    if os.path.exists(prefs):
        lines = [ln for ln in open(prefs)
                 if not ln.startswith("WindowsAndTabs.")]
    lines.append("WindowsAndTabs.Layout:\t%s\n" % preset)
    for key, value in (rows or {}).items():
        lines.append("WindowsAndTabs.%s:\t%s\n" % (key, value))
    with open(prefs, "w") as out:
        out.writelines(lines)


def toolShown(inproc, cls):
    """'tab', 'window' or None for the tool frame class in an inprocess line:
    a tabbed tool's frame stays hidden."""
    m = re.search(r" tlw=%s(\(hidden\))?/" % cls, inproc)
    return None if m is None else ("tab" if m.group(1) else "window")


def runLayouts(display):
    """Preferences > Windows and tabs (#983 stage 2): Wide keeps windows and
    no tab strip; Normal (for now as Small) opens tabs; a Customize row set
    to Window opens that tool in a window; the tab menu has Open in Window
    and Always open in a window, which makes the layout Custom."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    shots = os.environ.get("ECCE_TABS_SHOTS")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-l", "ready", None)
        cmds = os.path.join(work, "commands")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def organizer(preset, inspect):
            open(cmds, "w").close()
            setLayout(display, preset)
            result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                              env={"ECCE_ONE_PROCESS": "1",
                                   "ECCE_TEST_ORGANIZER": cmds,
                                   "ECCE_LOCAL_DATA": data},
                              inspect=inspect)
            log = result.log or ""
            choices = [a for a in pb.answers(log, "prefs")
                       if a.startswith("choice=")]
            return result, log, choices

        def wide(_display):
            append("prefs windows show", 3)
            append("mainwindow", 2)
            append("tabs", 2)
            append("start Builder " + url, 25)
            append("inprocess", 2)
            append("close", 3)
            append("answer", 3)
            append("answer no", 20)

        result, log, pw = organizer("wide", wide)
        check(len(pw) >= 1 and pw[0].startswith("choice=wide preset=wide")
              and "builder:window" in pw[0],
              "Wide: the dropdown shows Wide, every row Window", str(pw))
        tabs = pb.answers(log, "tabs")
        check(tabs == ["no tabs"], "Wide: no tab strip", str(tabs))
        mw = pb.answers(log, "mainwindow")
        check(mw == ["host=none topwindow=CalcMgr"],
              "Wide: no MainFrame, the Organizer's own frame is the main "
              "window", str(mw))
        inproc = pb.answers(log, "inprocess")
        check(len(inproc) == 1 and toolShown(inproc[0], "Builder") == "window",
              "Wide: the Builder opens in a window", str(inproc))
        check(not result.crashed and result.returncode == 0,
              "Wide: the Organizer ends cleanly", log[-1500:])

        def normal(_display):
            if shots:
                os.makedirs(shots, exist_ok=True)
            append("prefs windows show", 3)
            append("prefs windows row calced window" +
                   (" " + os.path.join(shots, "prefs-windows-table.png")
                    if shots else ""), 4)
            if shots:
                append("prefs windows snap " +
                       os.path.join(shots, "prefs-windows.png"), 3)
            append("start Builder " + url, 25)
            append("start CalculationEditor " + url, 25)
            append("inprocess", 2)
            append("tabs", 2)
            append("tabmenu 1", 2)
            append("alwayswindow 1", 8)
            append("inprocess", 2)
            append("prefs windows show", 3)
            append("prefs windows preset small", 3)
            append("layout show", 2)
            append("layout normal", 2)
            append("close", 3)
            append("answer", 3)
            append("answer no", 3)
            append("answer no", 20)

        result, log, pw = organizer("normal", normal)
        print("      prefs: " + "\n             ".join(pw))
        check(len(pw) >= 1 and pw[0].startswith("choice=normal preset=normal")
              and "builder:tab" in pw[0] and "calced:tab" in pw[0],
              "Normal: the dropdown shows Normal, the rows are tabs (as Small "
              "until the Set-up tab exists)", str(pw))
        check(len(pw) >= 2 and pw[1].startswith("choice=custom preset=custom")
              and "calced:window" in pw[1] and "builder:tab" in pw[1],
              "Customize: the Calculation Editor row set to Window makes the "
              "layout Custom", str(pw))
        inproc = pb.answers(log, "inprocess")
        print("      inprocess: " + "\n                 ".join(inproc))
        check(len(inproc) == 2 and toolShown(inproc[0], "Builder") == "tab"
              and toolShown(inproc[0], "CalcEd") == "window",
              "Custom: the Builder opens as a tab, the editor in a window",
              str(inproc) + log[-1500:])
        tm = pb.answers(log, "tabmenu")
        check(tm == ["Open in Window|Always open in a window"],
              "the tab's right-click menu has Open in Window and Always open "
              "in a window", str(tm))
        aw = pb.answers(log, "alwayswindow")
        check(len(aw) == 1 and "window:" in aw[0] and "builder:window" in aw[0]
              and "preset=custom" in aw[0],
              "Always open in a window: the Builder moves to a window and its "
              "row becomes Window", str(aw))
        check(len(inproc) == 2 and toolShown(inproc[1], "Builder") == "window",
              "after Always open in a window the Builder's frame is shown",
              str(inproc))
        check(len(pw) >= 3 and pw[2].startswith("choice=custom")
              and "builder:window" in pw[2],
              "Preferences shows the row the tab menu changed", str(pw))
        check(len(pw) >= 4 and pw[3].startswith("choice=small preset=small")
              and "window" not in pw[3].split("rows=")[-1],
              "choosing Small fills every row with Tab", str(pw))
        lay = pb.answers(log, "layout")
        check(len(lay) == 2 and lay[0].startswith("preset=small")
              and lay[1].startswith("preset=normal"),
              "the layout hook reads and sets the preset", str(lay))
        check(not result.crashed and result.returncode == 0,
              "Normal/Custom: the Organizer ends cleanly", log[-1500:])
    finally:
        setLayout(display, "wide")
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runTabs(display):
    """Small layout (#983 stage 2): the Builder opens as a tab, the menu
    bar follows the active tab, Open in Window and Move to Tabs keep the
    Builder, Quit asks about the dirty Builder tab.  ECCE_TABS_SHOTS=<dir>
    saves the screen with the Organizer tab and the Builder tab."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    shots = os.environ.get("ECCE_TABS_SHOTS")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-t", "created", None)
        cmds = os.path.join(work, "commands")
        script = os.path.join(work, "builder-script")
        open(cmds, "w").close()
        with open(script, "w") as handle:
            handle.write("background 1 1 1\nwait 3000\n"
                         "add C Tetrahedral 8 0 0\nsavestate\n")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def shot(name):
            if shots:
                os.makedirs(shots, exist_ok=True)
                subprocess.run(["import", "-display", ":%s" % display.number,
                                "-window", "root", os.path.join(shots, name)],
                               check=False)

        def inspect(_display):
            append("mainwindow", 2)
            append("tabs", 2)
            append("start Builder " + url, 25)
            append("tabs", 2)                  # Builder tab active
            append("tabinfo", 2)
            append("dialog", 1)
            shot("tabs-builder-dialog.png")
            time.sleep(2)                      # it closes itself after 1.5 s
            append("dialoginfo", 2)
            append("menu exit", 3)             # app-level: Organizer answers
            append("answer cancel", 3)         # "Quit ECCE", Builder active
            shot("tabs-builder.png")
            append("charhook 27", 2)           # Esc, as GTK delivers it
            append("tab 0", 3)
            append("tabs", 2)                  # Organizer's menus back
            shot("tabs-organizer.png")
            append("charhook 27", 2)           # the Organizer tab is active
            for what in ("prefs", "find", "props " + url):
                append("orgwindow " + what, 3)
                append("orgwindowinfo", 1)
            append("windowmenu", 2)
            append("windowmenu 1", 3)          # Builder through the menu
            append("mainwindow", 2)
            append("windowmenu 0", 3)
            append("mainwindow", 2)
            append("menu exit", 3)             # File > Quit reaches CalcMgr
            append("answer cancel", 3)         # the "Quit ECCE" question
            append("tabs", 2)                  # still running
            append("towindow 1", 8)            # Open in Window
            append("inprocess", 2)
            append("sbinfo Builder", 2)
            append("towindow 0", 2)            # the Organizer is not a tool
            shot("tabs-builder-window.png")
            append("totabs Builder", 8)        # Window > Move to Tabs
            append("inprocess", 2)
            append("close", 3)                 # File > Quit
            append("answer", 3)                # "Quit"
            append("answer cancel", 3)         # the Builder's save question
            append("tabs", 2)
            append("close", 3)
            append("answer", 3)
            append("answer no", 25)

        setLayout(display, "small")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_BUILDER_SCRIPT": script,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        if GDB:
            print(log[-6000:])
        tabs = pb.answers(log, "tabs")
        print("      tabs: " + "\n            ".join(tabs))
        mw = pb.answers(log, "mainwindow")
        check(len(mw) == 3 and mw[0].startswith(
                  "host=MainFrame organizer=hidden mapped=0 tabs=1 title=") and
              "Organizer" in mw[0] and mw[0].endswith("topwindow=MainFrame"),
              "the main window is a MainFrame; the Organizer's own frame is "
              "hidden and unmapped", str(mw))
        wm = pb.answers(log, "windowmenu")
        check(len(wm) == 3 and wm[0].startswith("0:Organizer* 1:") and
              re.match(r"0:Organizer 1:\S.*\* $", wm[1]) is not None and
              wm[2].startswith("0:Organizer* 1:"),
              "every bar has a Window menu listing the tabs; choosing an "
              "entry switches the tab", str(wm))
        check(len(mw) == 3 and "Organizer" in mw[1] and " \u2013 " in mw[1]
              and " \u2013 " not in mw[0] and " \u2013 " not in mw[2],
              "the window title names the active tab and keeps "
              "'Organizer'", str(mw))
        orgMenus = tabs[0].split("menus=")[-1] if tabs else ""
        check(len(tabs) >= 4 and tabs[0].startswith("0:Organizer*"),
              "the Organizer is the first tab", str(tabs) + log[-1500:])
        if len(tabs) >= 4:
            check(re.search(r"1:.*Builder.*\* ", tabs[1]) is not None
                  and "Render" in tabs[1].split("menus=")[-1]
                  and "Render" not in orgMenus,
                  "the Builder opened as the active tab with its menus",
                  tabs[1])
            check(tabs[2].startswith("0:Organizer*") and
                  tabs[2].split("menus=")[-1] == orgMenus,
                  "selecting the Organizer tab brings its menu bar back",
                  tabs[2])
        ow = pb.answers(log, "orgwindowinfo")
        check(len(ow) == 3 and all(
                  o == "transient=host inside=1 mapped=1" for o in ow),
              "the Organizer's preferences, find and properties windows are "
              "transient for the main window, inside it and mapped",
              str(ow) + str(pb.answers(log, "orgwindow")))
        info = pb.answers(log, "tabinfo")
        print("      tabinfo: " + str(info))
        m = re.search(r"page=(\d+)x(\d+) onscreen=1 children=\d+ shown=(\d+) "
                      r"statusbar=page frame=hidden", info[0] if info else "")
        check(m is not None and int(m.group(1)) > 400 and
              int(m.group(2)) > 300 and int(m.group(3)) >= 3,
              "the Builder tab shows its windows and status bar, its own "
              "frame stays unmapped", str(info))
        dlg = pb.answers(log, "dialoginfo")
        check(dlg and "transient=1 mapped=1 inside=1" in dlg[0]
              and " focus=" in dlg[0],
              "a dialog opened with the Builder's hidden frame as parent is "
              "transient for the Organizer, shown and inside its window",
              str(dlg))
        check("BUILDER: charhook key=27" in log,
              "a key hook on the main window reaches the Builder tab",
              str(pb.answers(log, "charhook")))
        tw = pb.answers(log, "towindow")
        check(len(tw) == 2 and tw[1] == "not a tool tab",
              "the Organizer tab cannot be moved to a window", str(tw))
        check(pb.answers(log, "menu") == ["queued", "queued"] and
              len(pb.answers(log, "charhook")) == 2,
              "File > Quit and a key hook on the Organizer tab are answered",
              str(pb.answers(log, "menu")))
        check(tw and "window:" in tw[0] and "(hidden)" not in tw[0]
              and "(no menus)" not in tw[0] and " 1:" not in tw[0],
              "Open in Window moves the Builder into a shown window with its menus", str(tw))
        sbi = pb.answers(log, "sbinfo")
        m = re.search(r"parent=frame bottom=(\d+) client=(\d+)",
                      sbi[0] if sbi else "")
        check(m is not None and abs(int(m.group(1)) - int(m.group(2))) <= 4,
              "after Open in Window the status bar is at the frame's bottom",
              str(sbi))
        tt = pb.answers(log, "totabs")
        check(tt and " 1:" in tt[0] and "window:" not in tt[0],
              "Move to Tabs puts it back as a tab", str(tt))
        inproc = pb.answers(log, "inprocess")
        check(len(inproc) == 2 and all("Builder=1" in i for i in inproc),
              "the same Builder throughout", str(inproc))
        check("BUILDER: dirty=1" in log, "the Builder has unsaved changes")
        check(pb.answers(log, "answer cancel") ==
              ["answered Quit ECCE", "answered Quit ECCE",
               "answered Save Builder Changes?"] and
              pb.answers(log, "answer no") ==
              ["answered Save Builder Changes?"],
              "the Organizer's File > Quit asks Quit ECCE (Cancel keeps "
              "running); Quit asks about the Builder tab: Cancel keeps it, "
              "No quits",
              str(pb.answers(log, "answer")))
        check(len(tabs) >= 4 and " 1:" in tabs[-1],
              "the Builder tab is still there after Cancel", str(tabs))
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log,
              "the tabbed Organizer ends cleanly",
              "returncode %s\n%s" % (result.returncode, log[-2000:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runTabsEditor(display):
    """#983 stage 2: the Calculation Editor as a tab, beside a Builder tab.
    Menus follow the active tab, Open in Window / Move to Tabs, Quit.
    The editor has no status bar (sbinfo says none).  ECCE_TABS_SHOTS=<dir>
    saves the editor tab and its window."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    shots = os.environ.get("ECCE_TABS_SHOTS")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-e", "created", None)
        cmds = os.path.join(work, "commands")
        script = os.path.join(work, "builder-script")
        open(cmds, "w").close()
        with open(script, "w") as handle:
            handle.write("background 1 1 1\nwait 3000\n"
                         "add C Tetrahedral 8 0 0\nsavestate\n")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def shot(name):
            if shots:
                os.makedirs(shots, exist_ok=True)
                subprocess.run(["import", "-display", ":%s" % display.number,
                                "-window", "root", os.path.join(shots, name)],
                               check=False)

        def inspect(_display):
            append("start CalculationEditor " + url, 25)
            append("tabs", 2)                  # 1: the editor, active
            append("tabinfo Launch...,Save,Verify,dump,gtk", 2)
            append("setup info", 2)            # Small: no Set-up tab
            append("dialog", 1)
            shot("tabs-editor-dialog.png")
            time.sleep(2)                      # it closes itself after 1.5 s
            append("dialoginfo", 2)
            shot("tabs-editor.png")
            append("start Builder " + url, 25)
            append("tabs", 2)                  # 2: the Builder, active
            shot("tabs-editor-and-builder.png")
            append("tab 1", 3)
            append("tabs", 2)                  # the editor's menus again
            shot("tabs-editor-back.png")
            append("towindow 1", 8)
            append("sbinfo Editor", 2)
            shot("tabs-editor-window.png")
            append("totabs Editor", 8)
            append("inprocess", 2)
            append("close", 3)                 # File > Quit
            append("answer", 3)                # "Quit"
            append("answer no", 3)             # the editor's question
            append("answer no", 25)            # the Builder's question

        setLayout(display, "small")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_BUILDER_SCRIPT": script,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        if GDB:
            print(log[-6000:])
        tabs = pb.answers(log, "tabs")
        print("      tabs: " + "\n            ".join(tabs))
        check(len(tabs) >= 3 and re.search(r"1:.*Editor.*\* ", tabs[0])
              is not None and "Save" not in tabs[0].split("menus=")[0]
              and "File" in tabs[0].split("menus=")[-1]
              and "Render" not in tabs[0].split("menus=")[-1],
              "the editor opened as the active tab with its menus",
              str(tabs) + log[-1500:])
        if len(tabs) >= 3:
            check(re.search(r"2:.*Builder.*\* ", tabs[1]) is not None
                  and "Render" in tabs[1].split("menus=")[-1],
                  "with both open, the Builder tab has the Builder's menus",
                  tabs[1])
            check(re.search(r"1:.*Editor.*\* ", tabs[2]) is not None
                  and tabs[2].split("menus=")[-1] ==
                  tabs[0].split("menus=")[-1],
                  "switching back to the editor swaps its menu bar back",
                  tabs[2])
        info = pb.answers(log, "tabinfo")
        m = re.search(r"page=(\d+)x(\d+) onscreen=1 children=\d+ shown=(\d+) "
                      r"statusbar=none frame=hidden", info[0] if info else "")
        check(m is not None and int(m.group(1)) > 300 and
              int(m.group(2)) > 300 and int(m.group(3)) >= 1,
              "the editor tab shows its windows, its own frame stays unmapped",
              str(info))
        su = pb.answers(log, "setup")
        check(su == ["no set-up area"],
              "Small: the editor tab has no detail area", str(su))
        dlg = pb.answers(log, "dialoginfo")
        check(dlg and "transient=1 mapped=1 inside=1" in dlg[0]
              and " focus=" in dlg[0],
              "a dialog opened from the editor tab is transient for the "
              "Organizer, shown and inside its window", str(dlg))
        # Every control reachable (seen, or by scrolling), and the page is
        # not one flat colour as the X server shows it.
        check(info and all(("%s=reachable" % b) in info[0]
                           for b in ("Launch...", "Save", "Verify"))
              and "clipped=0" in info[0] and "STALE" not in info[0],
              "the editor's buttons and controls are reachable in the tab",
              str(info))
        m = re.search(r"colours=(\d+)", info[0] if info else "")
        check(m is not None and int(m.group(1)) > 20,
              "the editor tab is painted, not one flat colour", str(info))
        tw = pb.answers(log, "towindow")
        check(tw and "window:" in tw[0] and "(hidden)" not in tw[0]
              and "(no menus)" not in tw[0] and "Editor" in tw[0],
              "Open in Window gives the editor a shown window with its menus",
              str(tw))
        check(pb.answers(log, "sbinfo") == ["none"],
              "the editor has no status bar", str(pb.answers(log, "sbinfo")))
        tt = pb.answers(log, "totabs")
        check(tt and re.search(r"1:.*Editor", tt[0]) and "window:" not in tt[0],
              "Move to Tabs puts the editor back as a tab", str(tt))
        inproc = pb.answers(log, "inprocess")
        check(len(inproc) == 1 and "CalculationEditor=1" in inproc[0],
              "the same editor throughout", str(inproc))
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log,
              "the Organizer with editor and Builder tabs ends cleanly",
              "returncode %s\n%s" % (result.returncode, log[-2000:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def fnv(data):
    h = 1469598103934665603
    for c in data:
        h = ((h ^ c) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return "%d:%016x" % (len(data), h)


def runSetupTab(display):
    """Normal layout (#983 stage 2): the editor's tab is the Set-up tab.
    Theory and Runtype Details fill its detail area, with no detail the
    area shows the input file Save writes; the editor column folds.  An
    ORCA calculation: both its details have a .dlg."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    shots = os.environ.get("ECCE_TABS_SHOTS")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = pb.makeCalc(project, "water-orca", "created", None)
        meta = os.path.join(calc, ".ecce-meta")
        text = open(meta).read().replace("ecce:application\t\tNWChem",
                                         "ecce:application\t\tORCA")
        open(meta, "w").write(text)
        url = "file://" + calc
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def shot(name):
            if shots:
                os.makedirs(shots, exist_ok=True)
                subprocess.run(["import", "-display", ":%s" % display.number,
                                "-window", "root", os.path.join(shots, name)],
                               check=False)

        def inspect(_display):
            append("start CalculationEditor " + url, 25)
            append("tabs", 2)
            append("setup info", 2)            # 1: the input preview
            append("setup theory", 4)          # 2
            shot("setup-theory.png")
            append("setup set ES.Theory.SCF.ConvergenceIterations 37", 3)
            append("setup info", 2)            # 11 (appended last)
            append("setup runtype", 4)         # 3
            shot("setup-runtype.png")
            append("setup input", 8)           # 4
            append("setup find 37", 2)         # unsaved 37 in the preview
            shot("setup-input-unsaved.png")
            append("setup save", 15)           # 5
            append("setup preview", 2)         # 6
            shot("setup-input.png")
            append("setup fold", 2)            # 7
            append("setup unfold", 2)          # 8
            append("towindow 1", 8)
            append("setup info", 2)            # 9: a window, no area
            append("totabs Editor", 8)
            append("setup info", 2)            # 10: the area again
            append("setup basis filter 6-31G", 3)       # 13
            append("setup basis pick 6-31G*", 3)          # 14
            shot("setup-basis.png")
            append("setup basis filter lanl", 3)          # 15
            append("setup basis pick LANL08", 3)          # 16: not covering O
            append("setup basis filter 3-21G", 3)         # 17
            append("setup basis pick 3-21G", 3)           # 18
            append("setup basis all", 15)                 # 19: 3-21G, saved
            append("setup basis filter 6-31G", 3)         # 20
            append("setup basis pick 6-31G*", 3)          # 21
            append("setup basis all", 15)                 # 22: 6-31G*, saved
            append("setup input", 8)                      # 23
            append("setup width 1024", 4)
            append("setup info", 2)
            shot("setup-orca-1024.png")
            append("close", 3)
            append("answer", 3)
            append("answer no", 25)

        setLayout(display, "normal")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        su = pb.answers(log, "setup")
        print("      setup: " + "\n             ".join(su))
        areas = [a for a in su if a.startswith("area=")]
        check(areas and all("overlaps=0" in a for a in areas),
              "ORCA Set-up tab: no two controls of the editor's column overlap "
              "(default width and 1024 px)",
              "\n".join(a for a in areas if "overlaps=0" not in a))
        m = re.search(r"window=(\d+)x", areas[-1]) if areas else None
        check(m and int(m.group(1)) <= 1024,
              "the Organizer window narrows to 1024 px",
              areas[-1] if areas else "")
        tabs = pb.answers(log, "tabs")
        check(tabs and re.search(r"1:.*Editor.*\*", tabs[0]) is not None,
              "Normal: the editor opens as a tab", str(tabs) + log[-1500:])
        su += [""] * (24 - len(su))
        basis = su[13:24]
        su = su[:13]
        # Answers in command order: index 2 is the "set", 3 its info.
        setAns, setInfo = su[2], su[3]
        found = su[6]
        su = su[:2] + su[4:6] + su[7:]
        check(found == "found=1" and "dry=1" in su[3],
              "the preview follows an unsaved change (dry run)",
              found + " / " + su[3])
        m = re.search(r"window=(\d+)x", su[0])
        check(m and int(m.group(1)) >= 1100,
              "the window is widened to editor + a usable detail area", su[0])
        check(setAns.startswith("area=theory") and
              "theorysum=" in setInfo and "|37" in setInfo.split(
                  "theorysum=")[1].split(" ")[0],
              "a value changed in the embedded dialog reaches the editor's "
              "summary", setAns + " / " + setInfo)
        check(su[0].startswith("area=input"),
              "the Set-up tab shows the input preview with no detail chosen",
              su[0])
        m = re.search(r"controls=(\d+)", su[1])
        v = re.search(r"size=(\d+)x(\d+) window=\S+ virtual=(\d+)x(\d+)", su[1])
        check(v and int(v.group(4)) > 100,
              "the detail area has the dialog's virtual size (scrolls)", su[1])
        check(su[1].startswith("area=theory") and m and int(m.group(1)) > 3
              and "marked=Theory" in su[1],
              "Theory Details fill the detail area, the button stays marked",
              su[1])
        m = re.search(r"controls=(\d+)", su[2])
        check(su[2].startswith("area=runtype") and m and int(m.group(1)) > 3
              and "marked=Runtype" in su[2] and "marked=Theory" not in su[2],
              "Runtype Details replace them in the same area", su[2])
        check(su[3].startswith("area=input"), "Input brings the preview back",
              su[3])
        m = re.search(r"shown=(\S+) file=(\S+)", su[5])
        files = set()
        for root, _dirs, names in os.walk(calc):
            for n in names:
                files.add(fnv(open(os.path.join(root, n), "rb").read()))
        check(m and m.group(1) == m.group(2) and not m.group(1).startswith("0:")
              and m.group(1) in files,
              "after Save the preview is the input file Save wrote",
              su[5] + " files=" + str(sorted(files)))
        check("folded=1" in su[6] and "folded=0" in su[7],
              "the editor column folds and unfolds", su[6] + " / " + su[7])
        check(su[8] == "not a set-up tab" or su[8] == "no set-up area",
              "Open in Window: no detail area (details as dialogs)", su[8])
        check(su[9].startswith("area=input"),
              "Move to Tabs makes the Set-up tab again", su[9])
        check(basis[0].startswith("area=basis") and "list=" in basis[0],
              "the basis button/hook shows the basis panel in the area",
              basis[0])
        check("picked=6-31G*" in basis[1] and "covered=O,H" in basis[1]
              and "uncovered= " in basis[1] and "use=on" in basis[1],
              "6-31G* covers the water system, Use for all atoms is on",
              basis[1])
        check("picked=LANL08" in basis[3] and "uncovered=" in basis[3]
              and "uncovered= " not in basis[3] and "use=off" in basis[3],
              "a set that lacks an element of the system marks it and "
              "Use for all atoms is off", basis[3])
        check("assigned=All_atoms:_3-21G " in basis[6] + " "
              and "assigned=All_atoms:_6-31G* " in basis[9] + " ",
              "Use for all atoms assigns the set to the whole system "
              "(3-21G, then 6-31G*)", basis[6] + " / " + basis[9])
        stored = set()
        for root, _dirs, names in os.walk(calc):
            for n in names:
                try:
                    txt = open(os.path.join(root, n), errors="replace").read()
                except OSError:
                    continue
                if "6-31G*" in txt and ("polarization" in txt.lower()
                                        or "gbs" in n.lower()
                                        or "basis" in n.lower()):
                    stored.add(n)
        check(stored, "the saved calculation stores the 6-31G* basis "
              "(same config the Basis Set Tool's save writes)", str(stored))
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log,
              "the Organizer with a Set-up tab ends cleanly",
              "returncode %s\n%s" % (result.returncode, log[-2000:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runTabsTools(display):
    """#983 stage 2: MD Energy, MD Prepare and the Basis Set Tool as tabs.
    Each shows its menus and windows, its frame stays unmapped, the page is
    painted and the key buttons reachable; Open in Window and Move to Tabs
    work; Quit ends cleanly.  ECCE_TABS_SHOTS=<dir> saves each tab."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    shots = os.environ.get("ECCE_TABS_SHOTS")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-m", "created", None)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def shot(name):
            if shots:
                os.makedirs(shots, exist_ok=True)
                subprocess.run(["import", "-display", ":%s" % display.number,
                                "-window", "root", os.path.join(shots, name)],
                               check=False)

        def inspect(_display):
            append("start MDEnergy", 15)
            append("tabs", 2)                  # 1: MD Energy
            append("tabinfo Launch...,Save", 2)
            shot("tabs-mdenergy.png")
            append("start MDPrepare", 15)
            append("tabs", 2)                  # 2: MD Prepare
            append("tabinfo Run Local,Save", 2)
            shot("tabs-mdprepare.png")
            append("start Basistool " + url, 20)
            append("tabs", 2)                  # 3: Basis Set Tool
            append("tabinfo Details...,Add to Table", 2)
            shot("tabs-basistool.png")
            append("towindow 3", 8)
            shot("tabs-basistool-window.png")
            append("totabs Basis", 8)
            append("towindow 2", 8)
            append("totabs Prepare", 8)
            append("inprocess", 2)
            append("close", 3)                 # File > Quit
            append("answer", 3)
            append("answer no", 3)
            append("answer no", 3)
            append("answer no", 20)

        setLayout(display, "small")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        if GDB:
            print(log[-6000:])
        tabs = pb.answers(log, "tabs")
        print("      tabs: " + "\n            ".join(tabs))
        info = pb.answers(log, "tabinfo")
        print("      tabinfo: " + "\n               ".join(info))
        for i, (name, first, buttons) in enumerate(
                (("MD Energy", "File", ("Launch...", "Save")),
                 ("MD Prepare", "File", ("Run Local", "Save")),
                 ("Basis Set", "Molecular Basis", ("Details...", "Add to Table")))):
            check(len(tabs) > i and re.search(r"%d:.*%s.*\* " % (i + 1, name),
                                              tabs[i]) is not None
                  and first in tabs[i].split("menus=")[-1]
                  and "Render" not in tabs[i].split("menus=")[-1],
                  "%s opened as the active tab with its menus" % name,
                  str(tabs) + log[-1500:])
            line = info[i] if len(info) > i else ""
            m = re.search(r"page=(\d+)x(\d+) onscreen=1 children=\d+ "
                          r"shown=(\d+) .*frame=hidden", line)
            check(m is not None and int(m.group(1)) > 300 and
                  int(m.group(2)) > 200 and int(m.group(3)) >= 1,
                  "%s tab shows its windows, its frame stays unmapped" % name,
                  line)
            check(all(("%s=reachable" % b) in line for b in buttons)
                  and "STALE" not in line,
                  "%s: %s reachable in the tab" % (name, ", ".join(buttons)),
                  line)
            m = re.search(r"colours=(\d+)", line)
            check(m is not None and int(m.group(1)) > 20,
                  "%s tab is painted, not one flat colour" % name, line)
        tw = pb.answers(log, "towindow")
        check(len(tw) >= 2 and all("window:" in t and "(hidden)" not in t
                                   and "(no menus)" not in t for t in tw),
              "Open in Window gives each tool a shown window with its menus",
              str(tw))
        tt = pb.answers(log, "totabs")
        check(len(tt) >= 2 and all("window:" not in t for t in tt),
              "Move to Tabs puts each tool back as a tab", str(tt))
        inproc = pb.answers(log, "inprocess")
        check(len(inproc) == 1 and "MDEnergy=1" in inproc[0]
              and "BasisTool=1" in inproc[0] and "MDPrepare=1" in inproc[0],
              "the same three tools throughout", str(inproc))
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log,
              "the Organizer with MD and Basis Set Tool tabs ends cleanly",
              "returncode %s\n%s" % (result.returncode, log[-2000:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runTabs1088(display):
    """#1088: (A) Builder and Basis Set Tool tabs opened and closed twice,
    then a Builder beside an editor tab: no crash (closed tools left
    listeners on the process-wide event dispatcher).  (B) Save in the
    Builder tab and in the editor tab, and code focusing a window of the
    hidden Organizer page, keep the tool's tab in front; the editor tab's
    close button closes it at once.  ECCE_1088_ONLY=1 runs only this."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-1088", "created", None)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            for _ in range(2):
                append("start Builder " + url, 25)
                append("closetools", 8)
                append("start Basistool " + url, 20)
                append("closetools", 8)
            append("start CalculationEditor " + url, 25)
            append("start Builder " + url, 25)
            append("tabs", 2)                  # 1: editor, 2: Builder
            append("tabsave", 8)               # the Builder's Save
            append("tabsel", 2)
            append("tab 1", 3)
            append("tabsave", 8)               # the editor's Save
            append("tabsel", 2)
            append("tabfocus 0", 3)            # a hidden Organizer window
            append("tabsel", 2)
            append("tabclose 1", 8)            # the editor's close button
            append("tabs", 2)
            append("inprocess", 2)             # inprocess[0]
            append("start Builder " + url, 10) # again: the tab is reused
            append("start CalculationViewer " + url, 25)   # item 1: a tab
            append("start CalculationViewer " + url, 10)   # item 2: reused
            append("inprocess", 2)             # inprocess[1]
            append("start Basistool " + url, 20)
            append("start Basistool " + url, 10)           # item 2: reused
            append("tabs", 2)                  # tabs[2]: Organizer, Builder, Viewer, Basis
            append("tabclose 3", 8)            # item 4: closes only that tool
            append("answer no", 4)             # the Basis Set Tool always asks
            append("tabs", 2)                  # tabs[3]
            append("inprocess", 2)             # inprocess[2]
            append("close", 3)                 # File > Quit
            append("answer", 3)
            append("answer no", 3)
            append("answer no", 20)

        setLayout(display, "small")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        if GDB:
            print(log[-6000:])
        # "tabs" alone: answers() matches a prefix, which takes tabsel's too.
        tabs = re.findall(r"ECCE_TEST_ORGANIZER: tabs: (.*)$", log, re.M)
        print("      tabs: " + "\n            ".join(tabs))
        sel = pb.answers(log, "tabsel")
        print("      tabsel: " + str(sel))
        check(len(tabs) >= 1 and re.search(r"1:.*Editor", tabs[0])
              and re.search(r"2:.*Builder.*\* ", tabs[0]),
              "after Builder and Basis Set Tool tabs came and went, a Builder "
              "opens beside the editor", str(tabs) + log[-3000:])
        check(sel[:1] == ["selected=2"],
              "Save in the Builder tab keeps the Builder tab in front", str(sel))
        check(sel[1:2] == ["selected=1"],
              "Save in the editor tab keeps the editor tab in front", str(sel))
        check(sel[2:3] == ["selected=1"],
              "focusing a window of the hidden Organizer page does not "
              "switch to it", str(sel))
        check(len(tabs) >= 2 and "Editor" not in tabs[1],
              "the editor tab's close button closes it with one click",
              str(tabs))
        inp = pb.answers(log, "inprocess")
        check(len(inp) >= 3 and " Builder=1 " in inp[1] and " Viewer=1 " in inp[1],
              "the Viewer opens in this process, and a second click on Builder "
              "or Viewer for the same calculation opens no second one", str(inp))
        check(len(tabs) >= 3 and re.search(r"3:.*Basis", tabs[2])
              and "4:" not in tabs[2],
              "a second click on Basis Set Tool for the same calculation opens "
              "no second tab", str(tabs))
        check(len(tabs) >= 4 and "Basis" not in tabs[3] and "2:" in tabs[3]
              and len(inp) >= 3 and " BasisTool=0 " in inp[2]
              and " Viewer=1 " in inp[2],
              "closing the Basis Set Tool's tab closes only that tool",
              str(tabs) + str(inp))
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log and "SIGSEGV" not in log,
              "the Organizer ends cleanly", "returncode %s\n%s"
              % (result.returncode, log[-3000:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runTabsLauncher(display):
    """#983: the Launcher joins the Organizer's process.  Normal layout: the
    editor's Launch... opens a Launcher tab (no new process), the local
    machine is selectable, Launch from the tab submits a water calculation
    (state leaves 'created') as the window route does; Open in Window and
    Move to Tabs keep it; Quit ends cleanly."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        # An ECCE-QM water calculation, which runs in the isolated session.
        calc = os.path.join(project, "water-l")
        shutil.copytree(os.path.join(HERE, "..", "ecceqm", "fixtures",
                                     "h2o-geometry"), calc, symlinks=True)
        meta = open(os.path.join(calc, ".ecce-meta")).read()
        open(os.path.join(calc, ".ecce-meta"), "w").write(
            re.sub(r"(ecce:state\t\t)\w+", r"\1ready", meta))
        url = "file://" + calc
        cmds = os.path.join(work, "commands")
        calced = os.path.join(work, "calced-script")
        open(cmds, "w").close()
        with open(calced, "w") as handle:
            handle.write("wait 5000\ntool Launcher\n")   # the editor's Launch...

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        shots = os.environ.get("ECCE_TABS_SHOTS")

        def shot(name):
            if shots:
                os.makedirs(shots, exist_ok=True)
                subprocess.run(["import", "-display", ":%s" % display.number,
                                "-window", "root", os.path.join(shots, name)],
                               check=False)

        def inspect(_display):
            append("start CalculationEditor " + url, 30)   # then its script
            append("tabs", 2)
            append("launcher get machines", 2)
            append("launcher get machine", 2)
            append("inprocess", 2)
            append("tabinfo Launch", 2)
            append("launcher launch", 90)
            append("launcher get machine", 2)
            append("towindow 2", 8)
            append("totabs Launcher", 8)
            append("tabs", 2)
            append("close", 3)
            shot("launcher-quit.png")
            append("answer", 3)
            append("answer no", 3)
            append("answer no", 20)

        setLayout(display, "normal")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_TEST_MESSAGES": "1",
                               "ECCE_CALCED_SCRIPT": calced,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        tabs = pb.answers(log, "tabs")
        print("      tabs: " + "\n            ".join(tabs))
        check(len(tabs) >= 1 and "Launcher" in tabs[0],
              "the editor's Launch... opened a Launcher tab", str(tabs))
        inproc = pb.answers(log, "inprocess")
        check(len(inproc) == 1 and "Launcher=1" in inproc[0]
              and "CalculationEditor=1" in inproc[0],
              "the Launcher is one of the Organizer's own windows", str(inproc))
        trace = re.findall(r"ECCE_TEST_MESSAGES: (\S+) (\S+) pid=(\d+) "
                           r"(\S+) from", log)
        pids = set(t[2] for t in trace)
        check(any(t[0] == "Launcher" and t[3] == "ecce_invoke" for t in trace)
              and len(pids) == 1,
              "the Launcher got its invoke, in the Organizer's process (pids %s)"
              % sorted(pids))
        machines = pb.answers(log, "launcher get machines")
        check(machines and "localhost" in machines[0],
              "localhost is in the Launcher's machine list", str(machines))
        m = re.search(r"ecce:state\t\t(\w+)",
                      open(os.path.join(calc, ".ecce-meta")).read())
        state = m.group(1) if m else "?"
        check(state.lower() in ("submitted", "running", "complete", "completed"),
              "Launch from the tab ran the ECCE-QM water calculation "
              "(state %s)" % state)
        tt = pb.answers(log, "tabs")
        check(len(tt) >= 1 and "Launcher" in tt[-1],
              "Open in Window and Move to Tabs keep the Launcher tab",
              str(tt))
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log,
              "the Organizer with a Launcher tab ends cleanly",
              "returncode %s\n%s" % (result.returncode, log[-2000:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runSetupLaunch(display):
    """#983: in the Normal layout Launch... shows the Launcher's panel in the
    Set-up area (no new tab): the local machine is offered, and launching an
    ECCE-QM water calculation from it runs to Complete."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "water-s")
        shutil.copytree(os.path.join(HERE, "..", "ecceqm", "fixtures",
                                     "h2o-geometry"), calc, symlinks=True)
        meta = open(os.path.join(calc, ".ecce-meta")).read()
        open(os.path.join(calc, ".ecce-meta"), "w").write(
            re.sub(r"(ecce:state\t\t)\w+", r"\1ready", meta))
        url = "file://" + calc
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        shots = os.environ.get("ECCE_TABS_SHOTS")

        def shot(name):
            if shots:
                os.makedirs(shots, exist_ok=True)
                subprocess.run(["import", "-display", ":%s" % display.number,
                                "-window", "root", os.path.join(shots, name)],
                               check=False)

        def setState(name):
            text = open(os.path.join(calc, ".ecce-meta")).read()
            open(os.path.join(calc, ".ecce-meta"), "w").write(
                re.sub(r"(ecce:state\t\t)\w+", r"\g<1>" + name, text))

        def inspect(_display):
            append("start CalculationEditor " + url, 30)
            append("setup launch", 6)
            append("setup launch info", 2)
            append("tabs", 2)
            shot("setup-launch.png")
            append("setup width 1024", 4)
            append("setup launch info", 2)
            append("setup launch info", 2)
            shot("setup-launch-1024.png")
            # Follows the broker (#1064): a machine registered and a state
            # change while the panel is open, without reopening it.
            append("setup launch get machines", 2)
            prefs = os.path.join(_display.env().get(
                "ECCE_REALUSERHOME", os.environ.get("ECCE_REALUSERHOME", "")),
                ".ECCE")
            os.makedirs(prefs, exist_ok=True)
            with open(os.path.join(prefs, "MyMachines"), "w") as h:
                h.write("followbox\tlocalhost\tGeneric\tregistered while open\t"
                        "Unspecified\t64:1\tssh\t:ECCE-QM\tWS\n")
            append("setup launch notify machreg", 5)
            append("setup launch get machines", 2)
            setState("running")
            append("setup launch notify state running", 5)
            append("setup launch get state", 2)
            setState("ready")
            append("setup launch notify state ready", 5)
            append("setup launch get state", 2)
            append("setup launch go", 90)
            append("setup launch info", 2)
            append("close", 3)
            append("answer", 3)
            append("answer no", 3)
            append("answer no", 20)

        setLayout(display, "normal")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        first = pb.answers(log, "setup launch")
        check(first and "area=launch" in first[0] and "machine=localhost" in first[0],
              "Launch... shows the Launcher panel in the Set-up area, on localhost",
              str(first))
        info = pb.answers(log, "setup launch info")
        check(info and "launch=enabled" in info[0],
              "the panel's Launch button is enabled", str(info))
        check(info and all("overlaps=0" in i for i in info),
              "no two controls of the editor's column overlap (ECCE-QM, "
              "default width and 1024 px)", str(info))
        # No hidden code: the button is correctly absent (code_choice_calced
        # _test checks the count); when there is one it must be the "+N" form.
        check(info and all(re.search(r'more="(none|\+\d+ hidden)"', i)
                           for i in info),
              "the code bar has no '+N hidden' button, or one of the right form",
              str(info))
        check(info and all("clipped=0" in i for i in info[:-1]),
              "no control of the panel is clipped at the default width or at "
              "1024 px", str(info))
        machines = pb.answers(log, "setup launch get machines")
        check(len(machines) == 2 and "followbox" not in machines[0]
              and "followbox" in machines[1],
              "a machine registered while the Set-up panel is open appears in "
              "its list", str(machines))
        states = pb.answers(log, "setup launch get state")
        check(len(states) == 2 and states[0].lower() == "running"
              and states[1].lower() == "ready",
              "a state notice reaches the open Set-up panel (running, then "
              "ready again)", str(states))
        tt = pb.answers(log, "tabs")
        check(tt and "Launcher" not in tt[0],
              "no Launcher tab was opened", str(tt))
        m = re.search(r"ecce:state\t\t(\w+)",
                      open(os.path.join(calc, ".ecce-meta")).read())
        state = m.group(1) if m else "?"
        check(state.lower() in ("complete", "completed"),
              "Launch from the Set-up panel ran the water calculation to "
              "Complete (state %s)" % state)
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log,
              "the Organizer with the Set-up launch panel ends cleanly",
              "returncode %s\n%s" % (result.returncode, log[-2000:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runTabsPopups(display):
    """Popup menus and secondary windows of a tab (#1068): a menu popped up
    over the Builder tab and the editor tab attaches to the visible page and
    is on screen at the requested point; a floated Builder pane is shown and
    transient for the Organizer's window."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-p", "created", None)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start Builder " + url, 25)
            append("popup", 3)
            append("popupinfo", 2)
            append("float", 3)
            append("floatinfo", 2)
            append("start CalculationEditor " + url, 25)
            append("popup", 3)
            append("popupinfo", 2)
            append("close", 3)
            append("answer", 3)
            append("answer no", 3)
            append("close", 3)
            append("answer", 3)
            append("answer no", 25)

        setLayout(display, "small")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        pops = pb.answers(log, "popupinfo")
        good = "parent=page shown=1 mapped=1 inside=1 near=1"
        for i, what in enumerate(("Builder", "editor")):
            check(len(pops) > i and good in pops[i],
                  "a popup menu over the %s tab pops up from its page, is "
                  "mapped, inside the Organizer's window and at the point"
                  % what, str(pops) + log[-1500:])
        fl = pb.answers(log, "floatinfo")
        check(fl and "frame=1 shown=1 transient=1 mapped=1" in fl[0],
              "a floated Builder pane is shown and transient for the "
              "Organizer's window", str(fl))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runUtilityWindows(display):
    """#1066: the periodic table and the machine browser open as windows of
    the Organizer's process, a second request reuses the open one, closing
    them leaves the Organizer running, and a new request opens a fresh one."""
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    try:
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start PeriodicTable", 8)
            append("start MachineBrowser", 8)
            append("start MachineRegister", 10)
            append("inprocess", 2)
            append("start PeriodicTable", 4)   # the open one again
            append("start MachineRegister", 4)
            append("inprocess", 2)
            append("closetools", 5)
            append("inprocess", 2)
            append("start PeriodicTable", 8)   # a fresh one after closing
            append("inprocess", 2)
            append("closetools", 5)

        result = apps.run(display, "organizer", windowTimeout=60, settle=15,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_TEST_MESSAGES": "1",
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        check(not result.crashed and "Aborted" not in log,
              "the Organizer ran to the end without a crash",
              result.signalName or log[-1500:])
        inproc = pb.answers(log, "inprocess")
        if check(len(inproc) == 4, "four window counts answered", str(inproc)):
            check("PeriodicTable=1 MachineBrowser=1 MachineRegister=1" in inproc[0]
                  and "PeriodicTable(hidden)" not in inproc[0],
                  "both open in the Organizer, shown, with their menus",
                  inproc[0])
            check("PeriodicTable=1" in inproc[1] and "MachineRegister=1" in inproc[1],
                  "a second request reuses the open windows", inproc[1])
            check("PeriodicTable=0 MachineBrowser=0 MachineRegister=0" in inproc[2],
                  "closing leaves the Organizer running", inproc[2])
            check("PeriodicTable=1 MachineBrowser=0" in inproc[3],
                  "a request after closing opens a fresh one", inproc[3])
        check(pb.answers(log, "closetools").count("closed") == 2,
              "the windows closed", str(pb.answers(log, "closetools")))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        if os.environ.get("ECCE_SINGLE_ONLY") == "1":
            runSingleInstance(display)
            runUntrustedDir(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_UTILS_ONLY") == "1":
            runUtilityWindows(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_LAUNCHER_ONLY") == "1":
            runTabsLauncher(display)
            runSetupLaunch(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_SETUP_ONLY") == "1":
            runSetupTab(display)
            runSetupLaunch(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_1088_ONLY") == "1":
            runTabs1088(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_POPUPS_ONLY") == "1":
            runTabsPopups(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        runLayouts(display)
        runTabs(display)
        runQuitHost(display)
        runSingleInstance(display)
        runUntrustedDir(display)
        runGeometry(display)
        runTabsEditor(display)
        runSetupTab(display)
        runTabsTools(display)
        runTabsLauncher(display)
        runSetupLaunch(display)
        runTabsPopups(display)
        runUtilityWindows(display)
        runTabs1088(display)
        if os.environ.get("ECCE_TABS_ONLY") == "1":
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        run(display)
        runQuit(display)
        runQuit(display, gateway=True)
        if failures:
            results.fail("one-process", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
