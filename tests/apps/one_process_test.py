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

import faulthandler
import os
import re
import shutil
import signal
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
# --panels-only: only runPanelTeardown (as ECCE_PANELS_ONLY=1).
if "--panels-only" in sys.argv:
    sys.argv.remove("--panels-only")
    os.environ["ECCE_PANELS_ONLY"] = "1"
# --1210-only: only runTabPaint (as ECCE_1210_ONLY=1).
if "--1210-only" in sys.argv:
    sys.argv.remove("--1210-only")
    os.environ["ECCE_1210_ONLY"] = "1"
# --1088-only: only runTabs1088 (as ECCE_1088_ONLY=1).
# --1256-only: only runTabIdentity (as ECCE_1256_ONLY=1).
if "--1256-only" in sys.argv:
    sys.argv.remove("--1256-only")
    os.environ["ECCE_1256_ONLY"] = "1"
if "--1088-only" in sys.argv:
    sys.argv.remove("--1088-only")
    os.environ["ECCE_1088_ONLY"] = "1"
# --launcher-only: only the Launcher tab and Set-up launch runs.
if "--launcher-only" in sys.argv:
    sys.argv.remove("--launcher-only")
    os.environ["ECCE_LAUNCHER_ONLY"] = "1"


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
                               # the retired window (#1063)
                               "ECCE_FULL_BASIS_TOOL": "1",
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
        basis = os.path.join(work, "basistool-commands")
        open(cmds, "w").close()
        open(basis, "w").close()
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
                with open(basis, "a") as handle:
                    handle.write("changed\n")
                time.sleep(2)
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
            # No question from the unchanged Basis Set Tool; this waits out
            # the broker's 10 s linger (OnExit).
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
                               # the retired window (#1063)
                               "ECCE_FULL_BASIS_TOOL": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_BUILDER_SCRIPT": script,
                               "ECCE_TEST_BASISTOOL": basis,
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
            # The Basis Set Tool only opened the calculation, so it has
            # nothing to save and must not ask (#1088).
            check("BASISTOOL: changed=0" in log,
                  "the Basis Set Tool has no changes",
                  "\n".join(l for l in log.splitlines() if "BASISTOOL" in l))
            asked = pb.answers(log, "answer no")
            check(pb.answers(log, "answer cancel") ==
                  ["answered Save Builder Changes?"] and
                  asked[:1] == ["answered Save Builder Changes?"] and
                  "answered Save Basis Set Tool Changes?" not in asked,
                  "the Builder asked about its changes at each Quit, the "
                  "unchanged Basis Set Tool did not ask",
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
            # Own process group, output to a file: the wrapper's children
            # inherit a pipe and would keep communicate() waiting forever.
            outPath = os.path.join(work, "after.out")
            with open(outPath, "w") as out:
                proc = subprocess.Popen(
                    [os.path.join(apps.WRAPPERS, "ecce-organizer")],
                    env=environment, stdout=out, stderr=subprocess.STDOUT,
                    start_new_session=True)
            time.sleep(25)
            seen["after"] = proc.poll() is None
            for sig, wait in ((signal.SIGTERM, 20), (signal.SIGKILL, 10)):
                try:
                    os.killpg(proc.pid, sig)
                    proc.wait(timeout=wait)
                    break
                except (ProcessLookupError, subprocess.TimeoutExpired):
                    pass
            with open(outPath, errors="replace") as out:
                seen["after_out"] = out.read()

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
            # An .xyz asks for its units (modal) before it loads.
            handle.write("wait 5000\nmodal ok\nwait 2000\nexpect atoms 3\n")
        env = {"ECCE_ONE_PROCESS": "1", "ECCE_LOCAL_DATA": data}
        seen = {}

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def organizers():
            # Only this test's organizers: ctest -j runs other suites that
            # start and quit their own, which a system-wide count picks up.
            mark = ("ECCE_LOCAL_DATA=%s" % data).encode()
            count = 0
            for pid in os.listdir("/proc"):
                try:
                    if (pid.isdigit() and open("/proc/%s/comm" % pid).read()
                            .strip() == "organizer" and mark in open(
                                "/proc/%s/environ" % pid, "rb").read()
                            .split(b"\0")):
                        count += 1
                except OSError:
                    pass
            return count

        def second(args):
            environment = display.env()
            environment.update(env)
            started = time.time()
            done = subprocess.run(
                [os.path.join(apps.WRAPPERS, "ecce-organizer")] + args,
                env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True, timeout=90)
            return done.returncode, time.time() - started, done.stdout

        def client():
            # What `ecce -remote` runs before it starts a gateway (#1069),
            # with the environment the Organizer's wrapper gave it.
            environment = display.env()
            environment.update(env)
            environment.setdefault("ECCE_REALUSERHOME",
                                   os.environ.get("HOME", ""))
            done = subprocess.run(
                [os.path.join(apps.INSTALL, "libexec", "ecce-one-instance")],
                env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True, timeout=30)
            return done.returncode, 0, done.stdout

        def inspect(_display):
            append("raised", 2)
            seen["before"] = organizers()
            seen["plain"] = second([])
            seen["file"] = second([xyz])
            seen["client"] = client()
            time.sleep(20)                  # the Builder tab opens
            append("raised", 2)
            append("inprocess", 3)
            seen["after"] = organizers()

        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**env, "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_BUILDER_SCRIPT": script},
                          inspect=inspect)
        log = result.log or ""
        for what in ("plain", "file", "client"):
            rc, secs, text = seen.get(what, (None, 0, ""))
            check(rc == 0 and secs < 30 and "already running" in text,
                  "a second start (%s) exits at once with a message" % what,
                  "rc=%s %.0f s\n%s" % (rc, secs, text[-800:]))
        check(pb.answers(log, "raised") == ["0", "3"],
              "the running window was asked to come to the front three times",
              str(pb.answers(log, "raised")) + log[-1500:])
        check(seen.get("before") == 1 and seen.get("after") == 1,
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
        environment = display.env()
        environment.update(env)
        done = subprocess.run(
            [os.path.join(apps.INSTALL, "libexec", "ecce-one-instance")],
            env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, timeout=30)
        check(done.returncode == 1 and open(victim).read() == "keep"
              and sorted(os.listdir(folder)) == ["x.tok"],
              "ecce-one-instance does not use the untrusted folder either "
              "(the start goes on)", "rc=%s %s" % (done.returncode, done.stdout))
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


def runTabIdentity(display):
    """#1256: Viewers of two calculations open side by side, the editor
    opened twice for one calculation keeps one tab, each tab's tooltip names
    the calculation's path and the tool, Next/Previous Tab (menu and keys)
    wrap around, and the Window menu lists every tab and selects it."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        urlA = "file://" + pb.makeCalc(project, "ident-a", "created", None)
        urlB = "file://" + pb.makeCalc(project, "ident-b", "created", None)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start CalculationViewer " + urlA, 25)
            append("start CalculationViewer " + urlB, 25)
            append("start CalculationEditor " + urlA, 25)
            append("start CalculationEditor " + urlA, 15)  # reused
            append("tabs", 2)
            append("tabs dots", 2)
            append("tabtips", 2)
            append("windowitems", 2)
            append("tab 0", 3)
            for c in ("prevtab", "nexttab"):
                append(c, 2)
            for k in ("next", "prev", "pgup", "pgdn"):
                append("tabkey " + k, 2)
            append("tabsteps 3", 3)
            append("windowmenu 2", 3)
            append("tabs", 2)
            append("close", 3)
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
        tabs = re.findall(r"ECCE_TEST_ORGANIZER: tabs: (.*)$", log, re.M)
        tips = pb.answers(log, "tabtips")
        items = pb.answers(log, "windowitems")
        print("      tabs: " + "\n            ".join(tabs))
        print("      tabtips: " + str(tips))
        print("      windowitems: " + str(items))
        dots = pb.answers(log, "tabs dots")
        check(len(dots) == 1 and re.fullmatch(
                  r"0:role:#3060b0 1:state:\w+ 2:state:\w+ 3:state:\w+",
                  dots[0].split(" dots=")[-1].strip().lower()) is not None,
              "the Organizer keeps its blue dot; the Viewer and Editor tabs "
              "of a calculation show its run state", str(dots))
        dash = " \u2013 "
        tipList = re.findall(r"\d+:([^|]*)\|", tips[0]) if tips else []
        check(len(tipList) == 4,
              "two Viewers of different calculations and one editor tab "
              "(the second editor start reused it)", str(tipList) + str(tabs))
        if len(tipList) == 4:
            check(tipList[0] == "", "the Organizer's tab has no tooltip",
                  tipList[0])
            a = [t for t in tipList if "oneproc/ident-a" + dash in t]
            b = [t for t in tipList if "oneproc/ident-b" + dash in t]
            tools = [t.split(dash)[-1] for t in tipList[1:]]
            check(len(a) == 2 and len(b) == 1 and tools[0] == tools[1]
                  and tools[2] != tools[0],
                  "each tool tab's tooltip is '<path> \u2013 <tool>'",
                  str(tipList))
        steps = [pb.answers(log, c) for c in ("prevtab", "nexttab")] + \
                [pb.answers(log, "tabkey " + k) for k in
                 ("next", "prev", "pgup", "pgdn")]
        check([s[0] if s else "" for s in steps] ==
              ["3", "0", "1", "0", "3", "0"],
              "Next/Previous Tab and Ctrl+Tab, Ctrl+Shift+Tab, Ctrl+PgUp, "
              "Ctrl+PgDn step through the tabs and wrap around", str(steps))
        rows = pb.answers(log, "tabsteps 3")
        check(rows == ["selected=3 selects=2"],
              "three steps in a row draw only the first tab and the last "
              "(#1267)", str(rows))
        it = items[0].split("|") if items else []
        check(len(it) >= 7 and it[0].startswith("Next Tab[") and
              it[1].startswith("Previous Tab[") and it[2] == "-" and
              it[3] == "Organizer" and it[6].endswith("*") and
              sum(1 for x in it[4:7] if "oneproc/ident-" in x and dash in x)
              == 3,
              "the Window menu has Next/Previous Tab with shortcuts, a "
              "separator, then every tab as '<tool> \u2013 <path>', the "
              "active one checked", str(it))
        check(len(tabs) >= 2 and re.search(r"(^| )2:[^ ]*\*", tabs[-1])
              is not None,
              "choosing a Window menu entry selects that tab", str(tabs))
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log,
              "the tabbed Organizer ends cleanly",
              "returncode %s\n%s" % (result.returncode, log[-2000:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runBuilderRoleDot(display):
    """#1254: on a finished calculation the Builder opens its Viewer, one
    purple tab for both buttons; on an editable one it stays a red Builder."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        done = "file://" + pb.makeCalc(project, "role-done", "complete", None)
        edit = "file://" + pb.makeCalc(project, "role-edit", "created", None)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start Builder " + done, 25)
            append("start CalculationViewer " + done, 15)  # the same tab
            append("start Builder " + edit, 25)
            append("tabs", 2)
            append("tabs dots", 2)

        setLayout(display, "small")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        tabs = re.findall(r"ECCE_TEST_ORGANIZER: tabs: (.*)$", log, re.M)
        dots = pb.answers(log, "tabs dots")
        got = dots[0].split(" dots=")[-1].strip().lower() if dots else ""
        check(re.fullmatch(r"0:role:#3060b0 1:state:\w+ 2:role:#c02020", got)
              is not None,
              "a finished calculation's Builder and Viewer share one Viewer "
              "tab with its state icon; an editable one's Builder keeps its "
              "red dot",
              str(dots) + str(tabs))
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

        # #1063: calculations for the per-element, ECP and fitting cases;
        # no stored basis, so each starts from the code's default.
        def extra(name, code, molecule, theory=None):
            path = pb.makeCalc(project, name, "created", None)
            meta = os.path.join(path, ".ecce-meta")
            text = open(meta).read().replace(
                "ecce:application\t\tNWChem", "ecce:application\t\t" + code)
            if theory:
                text = text.replace("ecce:theory\t\tSCF/RHF",
                                    "ecce:theory\t\t" + theory)
            open(meta, "w").write(text)
            os.remove(os.path.join(path, "Parameters", "BasisSet.ecce_basisset"))
            if molecule:
                with open(os.path.join(path, "Parameters", "chemsys.mvm"),
                          "w") as h:
                    h.write(molecule)
            return path

        calc2 = extra("water-elements", "ORCA", None)
        calc3 = extra("water-fit", "NWChem", None, "DFT/RDFT")
        hi = extra("hi-orca", "ORCA",
                   "title: hi\ntype: molecule\nnum_atoms: 2\n"
                   "atom_info: symbol cart\natom_list: \n"
                   "H 0.00000 0.00000 0.00000\nI 0.00000 0.00000 1.60900\n"
                   "attr_list:\n 0.000000   1   0   0   0\n"
                   " 0.000000   1   0   0   0\natom_type_list:\n"
                   "        1   2\n       53   2\nnum_bonds: 1\n"
                   "bond_list: \n1 2 1.00000 2\n")
        # #1063 stage 3: one step for a mixed set, Def2-ECP only where it
        # has a core, fitting hidden without RI, per-atom and augmenting sets
        # atom names reach the editor only through pdb_list (O1, H1, H2)
        water = open(os.path.join(calc2, "Parameters", "chemsys.mvm")).read()
        names = "".join("M %-4s HOH      1        A\n" % n
                        for n in ("O1", "H1", "H2"))
        calc4 = extra("water-mixed", "ORCA", water.replace(
            "num_bonds:", "pdb_list: \n" + names + "num_bonds:", 1))
        hi2 = extra("hi-def2ecp", "ORCA",
                    open(os.path.join(hi, "Parameters", "chemsys.mvm")).read())
        url2, url3, hiUrl = ("file://" + calc2, "file://" + calc3,
                             "file://" + hi)
        url4, hi2Url = "file://" + calc4, "file://" + hi2
        # #1063 stage 3: the per-element mix must reach every code's deck
        mixes = {code: extra("mix-" + code.lower(), code, None, theory)
                 for code, theory in (("ORCA", None), ("Gaussian-16", None),
                                      ("NWChem", None),
                                      ("ECCE-QM", "SCF/HF"))}
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
            # #1063 stage 2: more editors as tabs (File > Quit ends the process,
            # so these come before it; answers found by "area=basis" after the above).
            append("start CalculationEditor " + url2, 25)
            append("setup basis assign def2-tzvp @ O", 15)       # m0
            append("setup basis assign def2-svp @ H", 15)        # m1
            append("setup basis assign Nonesuch @ O", 3)
            append("setup basis assign 6-31G* @ C", 5)           # m2
            shot("setup-basis-elements.png")
            append("start CalculationEditor " + hiUrl, 25)
            append("setup basis assign def2-svp @ H", 15)        # m3
            append("setup basis assign def2-tzvp @ I", 15)       # m4
            append("setup basis ecp", 15)                        # m5
            shot("setup-basis-ecp.png")
            append("setup basis ecp", 3)                         # none left
            append("start CalculationEditor " + url3, 25)
            append("setup basis assign DZVP (DFT Orbital)", 15)  # m6
            append("setup basis fit Ahlrichs Coulomb Fitting", 15)   # m7
            shot("setup-basis-fit.png")
            append("start CalculationEditor " + url4, 25)
            append("setup basis assign def2-tzvp @ O ; def2-svp @ H", 15)  # n0
            append("setup basis fit Ahlrichs Coulomb Fitting", 3)  # hidden
            append("setup basis aug Pople-style Diffuse @ O", 15)  # n1
            append("setup basis tight O 0 1", 15)                  # n2
            append("setup basis assign def2-tzvp @ H1", 15)        # n3
            append("start CalculationEditor " + hi2Url, 25)
            append("setup basis assign def2-tzvp", 15)             # n4
            append("setup basis ecp def2-tzvp-ecp", 15)            # n5
            shot("setup-basis-atoms.png")
            for mixCalc in mixes.values():
                append("start CalculationEditor file://" + mixCalc, 25)
                append("setup basis assign def2-tzvp @ O ; def2-svp @ H", 15)
                append("setup basis pick def2-tzvp-ecp", 3)
            # The retired Basis Set Tool: its start opens the editor's panel.
            append("start Basistool " + url, 15)
            append("setup info", 2)
            append("inprocess", 2)
            append("close", 3)
            append("answer", 3)
            append("answer no", 6)
            append("answer no", 6)
            append("answer no", 6)
            append("answer no", 6)
            append("answer no", 6)
            append("answer no", 6)
            append("answer no", 6)
            append("answer no", 6)
            append("answer no", 6)
            append("answer no", 15)

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
        allSu = list(su)
        more = [a for a in su[24:] if a.startswith("area=basis")]
        areas = [a for a in su if a.startswith("area=")]
        check(areas and all("overlaps=0" in a for a in areas),
              "ORCA Set-up tab: no two controls of the editor's column overlap "
              "(default width and 1024 px)",
              "\n".join(a for a in areas if "overlaps=0" not in a))
        # The last area of the width test, before the #1063 editors.
        own = areas[:len(areas) - len(more)] or areas
        m = re.search(r"window=(\d+)x", own[-1]) if own else None
        check(m and int(m.group(1)) <= 1024,
              "the Organizer window narrows to 1024 px",
              own[-1] if own else "")
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
        more += [""] * (14 - len(more))
        # ORCA attaches its own Def2-ECP to def2 sets, so ECP text is only
        # compared for the matching ECP; equal elements print as All atoms.
        check("def2-tzvp" in more[0] and "H_took_def2-tzvp" in more[0]
              and "ecpoffer=- " in more[0] + " ",
              "water: def2-TZVP on O (H takes it for now); no ECP offered "
              "for light elements", more[0])
        check("H:_def2-svp" in more[1] and "O:_def2-tzvp" in more[1],
              "water: def2-SVP on H and def2-TZVP on O (per element)",
              more[1])
        check("H:_def2-svp" in more[2] and "O:_def2-tzvp" in more[2]
              and "msg=C_is_not_in_this_chemical_system" in more[2],
              "an element not in the system: nothing assigned, the panel "
              "says so",
              more[2])
        check("no such basis set" in allSu, "an unknown set name is refused",
              str(allSu[-12:]))
        check("H:_def2-svp" in more[4] and "I:_def2-tzvp" in more[4]
              and "ecpoffer=def2-tzvp-ecp:I " in more[4] + " ",
              "HI: def2-TZVP on I offers the matching ECP, for I only",
              more[4])
        check(more[5].count("ECP_def2-tzvp") == 1
              and "I:_def2-tzvp_+_ECP_def2-tzvp" in more[5]
              and "ecpoffer=- " in more[5] + " ",
              "accepting the offer stores the ECP on I, not on H", more[5])
        check("no ECP offered" in allSu,
              "a second accept finds no offer left", str(allSu[-12:]))
        check("charge_fit_Ahlrichs" in more[7] and "charge_fit" not in more[6],
              "NWChem: an orbital set for all atoms, then a charge fitting "
              "set added to the elements", more[6] + " / " + more[7])
        check("fit=shown" in more[7],
              "NWChem DFT: fitting sets are listed", more[7])
        check("O:_def2-tzvp" in more[8] and "H:_def2-svp" in more[8]
              and "took" not in more[8] and "fit=hidden" in more[8],
              "ORCA without RI: 'O def2-TZVP ; H def2-SVP' is one step, "
              "nothing is cleared, fitting sets are hidden", more[8])
        check(allSu.count("no such basis set") >= 2,
              "a hidden fitting set cannot be assigned", str(allSu[-20:]))
        check("O:_def2-tzvp_+_augmenting_Pople-style_Diffuse" in more[9],
              "an augmenting set is added to the element, which keeps its "
              "orbital set", more[9])
        check("not possible" not in allSu and "O:_def2-tzvp" in more[10],
              "tight functions are added to a shell of O's set", more[10])
        check("H1:_def2-tzvp" in more[11] and "H:_def2-svp" in more[11]
              and "O:_def2-tzvp" in more[11],
              "an atom (H1) takes its own set; the element's group stays "
              "for the other hydrogens", more[11])
        check("All_atoms:_def2-tzvp_+_ECP_Def2-ECP" in more[12],
              "def2-TZVP for all atoms keeps the set's own Def2-ECP (for I)",
              more[12])
        check("I:_def2-tzvp_+_ECP_def2-tzvp" in more[13]
              and "H:_def2-tzvp_+_ECP" not in more[13]
              and "H:_def2-tzvp" in more[13],
              "an ECP for all atoms goes on I (it has a core there) and not "
              "on H", more[13])
        # O def2-TZVP and H def2-SVP, by name or written out; never the
        # other set's exponents (H 34.06134 is TZVP, O 2266.176 is SVP).
        for code, mixCalc in mixes.items():
            deck, used = "", []
            for root, _dirs, names in os.walk(mixCalc):
                for n in names:
                    if n in ("BasisSet.ecce_basisset", ".ecce-meta",
                             "chemsys.mvm") or n.endswith(".meta"):
                        continue
                    try:
                        txt = open(os.path.join(root, n),
                                   errors="replace").read()
                    except OSError:
                        continue
                    if re.search(r"(?i)def2-?svp|13\.01070", txt):
                        deck += txt
                        used.append(n)
            check(re.search(r"(?i)def2-?tzvp|27032\.38", deck)
                  and re.search(r"(?i)def2-?svp|13\.01070", deck)
                  and "34.06134" not in deck and "2266.176" not in deck
                  and not re.search(r'(?i)NewGTO\s+H\s+"def2-?tzvp', deck)
                  and not re.search(r'(?i)NewGTO\s+O\s+"def2-?svp', deck)
                  and not re.search(r"(?im)^\s*H\s+0\s*\n\s*def2-?tzvp",
                                    deck),
                  code + ": the input carries O def2-TZVP and H def2-SVP",
                  str(used) + "\n" + deck[:1500])
        hiPick = [a for a in allSu if "picked=def2-tzvp-ecp" in a
                  and "checked=H,I" in a]
        check(hiPick and " covered=I uncovered=H " in hiPick[0],
              "HI: def2-tzvp-ecp has a core for I only", str(hiPick))
        ecpPick = [a for a in allSu if "picked=def2-tzvp-ecp" in a
                   and "checked=O,H" in a]
        check(len(ecpPick) == len(mixes)
              and all(" covered= " in a and "assign=off" in a
                      for a in ecpPick),
              "def2-tzvp-ecp covers no element of water (no core): nothing to add",
              str(ecpPick))
        retired = [a for a in allSu if a.startswith("area=")][-1:]
        inproc = pb.answers(log, "inprocess")
        check(retired and retired[0].startswith("area=basis") and inproc
              and " BasisTool=0 " in inproc[-1],
              "Basis Set Tool start opens the editor's basis panel, no "
              "Basis Set Tool window", str(retired) + " / " + str(inproc))
        perElement = ""
        for root, _dirs, names in os.walk(calc2):
            for n in names:
                if n == "BasisSet.ecce_basisset":
                    perElement = open(os.path.join(root, n),
                                      errors="replace").read()
        check("def2-tzvp" in perElement and "def2-svp" in perElement
              and 'Tag="O"' in perElement and 'Tag="H"' in perElement,
              "the saved basis file has one group per element "
              "(what the input generator reads)", perElement[:600])
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


def runBasisContractions(display):
    """#1063: the basis panel's contraction editor and exponents view; the
    hooks do what the dialog's boxes do."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = pb.makeCalc(project, "water-orca", "created", None)
        meta = os.path.join(calc, ".ecce-meta")
        text = open(meta).read().replace("ecce:application\t\tNWChem",
                                         "ecce:application\t\tORCA")
        open(meta, "w").write(text)
        os.remove(os.path.join(calc, "Parameters", "BasisSet.ecce_basisset"))
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()
        decks = []

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def deck():
            """The saved input's text and its number of one-primitive S
            shells."""
            for root, _dirs, names in os.walk(calc):
                for n in names:
                    try:
                        body = open(os.path.join(root, n)).read()
                    except (UnicodeDecodeError, OSError):
                        continue
                    if "%basis" in body:
                        return body, len(re.findall(r"^\s*S 1\s*$", body, re.M))
            return "", 0

        def inspect(_display):
            append("start CalculationEditor file://" + calc, 25)
            append("setup basis filter 6-31G", 3)
            append("setup basis pick 6-31G*", 3)
            append("setup basis all", 15)
            append("setup basis contractions O", 3)               # c0
            append("setup save", 15)
            decks.append(deck())
            append("setup basis contractions O 0 uncontract", 15)  # c1
            append("setup basis contractions O 2 uncontract", 3)   # refused
            append("setup save", 15)
            decks.append(deck())
            append("setup basis coefficients", 3)
            append("setup basis contractions O 0 keep", 15)        # c2
            append("setup basis contractions O 1 delete", 15)      # c3
            append("close", 3)
            append("answer no", 6)
            append("answer no", 15)

        setLayout(display, "normal")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        su = pb.answers(log, "setup")
        summ = [a for a in su if a.startswith("contractions O:")]
        coef = [a for a in su if a.startswith("coefficients=")]
        shown = lambda a, l: re.search(r" %s=(\d+)/(\d+)" % l, a)
        ok = len(summ) == 4 and len(decks) == 2
        check(ok, "the contraction hooks answered", "\n".join(su))
        if ok:
            s0, s1, s2 = (shown(a, "s") for a in summ[:3])
            check(s0 and s1 and s2 and s0.group(1) != s0.group(2) and
                  s1.group(1) == s0.group(1) and s1.group(2) == s1.group(1) and
                  s2.groups() == s0.groups(),
                  "uncontracting the oxygen s function makes every primitive "
                  "a function, keep restores it", " / ".join(summ[:3]))
            check(decks[1][1] >= decks[0][1] + 5,
                  "the saved input lists the uncontracted primitives as "
                  "one-primitive shells",
                  "S 1 shells %d -> %d" % (decks[0][1], decks[1][1]))
            p0, p3 = shown(summ[0], "p"), shown(summ[3], "p")
            check(p0 and (not p3 or p3.groups() != p0.groups()),
                  "deleting the p function changes the summary",
                  summ[0] + " / " + summ[3])
        check(any(a.startswith("row 2 ") for a in su),
              "a function that is not contracted cannot be uncontracted",
              "\n".join(su))
        check(coef and "5484.6" in coef[0],
              "the exponents view lists the 6-31G* oxygen exponent 5484.67",
              coef[0][:300] if coef else "\n".join(su))
        check(not result.crashed and result.returncode == 0,
              "the Organizer ends cleanly after the contraction edits",
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
                               # the retired window (#1063)
                               "ECCE_FULL_BASIS_TOOL": "1",
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
        # A run calculation: its Builder opens as its Viewer (#1254).
        runUrl = "file://" + pb.makeCalc(project, "water-run", "complete", None)
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
            append("answer no", 4)             # nothing was changed: no question
            append("tabs", 2)                  # tabs[3]
            append("inprocess", 2)             # inprocess[2]
            append("start Builder " + runUrl, 25)
            append("tabs", 2)                  # tabs[4]: + the run one's Viewer
            append("close", 3)                 # File > Quit
            append("answer", 3)
            append("answer no", 3)
            append("answer no", 20)

        setLayout(display, "small")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1",
                               # the retired window (#1063)
                               "ECCE_FULL_BASIS_TOOL": "1",
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
        asked = pb.answers(log, "answer no")
        check(asked[:1] == ["no dialog"],
              "closing a Basis Set Tool tab nobody changed asks no save "
              "question", str(asked) + log[-3000:])
        check(len(tabs) >= 3 and re.search(r"1:Builder ", tabs[2])
              and re.search(r"2:Viewer ", tabs[2]),
              "the Viewer's tab reads Viewer, the Builder's Builder, on an "
              "editable calculation", str(tabs))
        # #1254 made a run calculation's Builder open its Viewer, which
        # replaced #1106's read-only Builder tab.
        check(len(tabs) >= 5 and len(re.findall(r"\d+:Builder\*? ", tabs[4])) == 1
              and len(re.findall(r"\d+:Viewer\*? ", tabs[4])) == 2,
              "a Builder on a run (read-only) calculation opens as its "
              "Viewer", str(tabs))
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
            append("greywatch on", 2)
            append("start CalculationEditor " + url, 30)
            append("setup launch", 6)
            append("greywatch", 2)
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
            append("setup results", 2)
            append("setup launch go", 90)
            append("setup launch get greyed", 2)
            append("setup launch info", 2)
            shot("setup-after-launch.png")
            append("setup info", 2)
            shot("setup-output.png")
            append("setup results", 2)
            append("setup input", 2)
            append("setup info", 2)
            append("setup output", 2)
            append("setup info", 2)
            append("setup results viewer", 25)
            append("tabs", 2)
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
        # The panel is left for the output after the launch: its last
        # answer is not a panel description.
        info = [i for i in pb.answers(log, "setup launch info")
                if "area=launch" in i]
        # Printed pass or fail: whether the panel clips depends on the
        # machine's text metrics, so a passing run has to say what it
        # measured for the failing one to be compared with (#1282).
        for i in info:
            print("      set-up Launch panel: %s" % i)
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
        grey = pb.answers(log, "greywatch")
        check(grey and grey[-1] == "0",
              "opening the editor from the Organizer does not grey the "
              "main window: its flushed status line no longer yields "
              "through wxSafeYield (#1268)", str(grey))
        greyed = pb.answers(log, "setup launch get greyed")
        check(greyed == ["0"],
              "the editor is not greyed out during the launch: no window "
              "is disabled for each log line (#1266)", str(greyed))
        tt = pb.answers(log, "tabs")
        check(tt and "Launcher" not in tt[0],
              "no Launcher tab was opened", str(tt))
        res = pb.answers(log, "setup results")
        check(len(res) == 3 and "results=hidden" in res[0]
              and "editlaunch=enabled" in res[0]
              and all("results=shown" in r and "editlaunch=disabled" in r
                      and re.search(r"actions=(terminate|duplicate,rerun)", r)
                      and "state=" in r for r in res[1:]),
              "after the launch the Results box is shown with Terminate or "
              "Duplicate and Reset, and the editor's Launch... is off",
              str(res))
        infos = pb.answers(log, "setup info")
        check(len(infos) >= 3 and "area=output" in infos[0]
              and "area=input" in infos[-2] and "area=output" in infos[-1],
              "the panel opens on the output file after the launch without "
              "a click, and the Input/Output switch changes it", str(infos))
        out = [i for i in infos if "area=output" in i]
        m = re.search(r"lines=(\d+)", out[-1]) if out else None
        check(m and int(m.group(1)) > 20 and "editor=1" in out[-1]
              and "following=0" in out[-1],
              "Output shows the finished job's output in the Set-up area, "
              "with Open in editor", str(out))
        check(tt and "Viewer" in tt[-1],
              "Viewer opens the calculation's Viewer tab", str(tt))
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


def runQuitDuringClose(display):
    """#1131: Quit answered while a Builder tab's close is still running (held
    5 s, yielding, by ECCE_TEST_CLOSE_HOLD): the tab turns the Quit's close
    request down, and the Quit finishes once that close has returned."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "water-c")
        shutil.copytree(os.path.join(HERE, "..", "ecceqm", "fixtures",
                                     "h2o-geometry"), calc, symlinks=True)
        # The fixture is a finished run, whose Builder opens as its Viewer
        # (#1254); this test holds a Builder tab's close, so make it editable.
        meta = os.path.join(calc, ".ecce-meta")
        text = open(meta).read()
        open(meta, "w").write(re.sub(r"(ecce:state\t\t)[^\n]*", r"\g<1>Created", text))
        url = "file://" + calc
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start Builder " + url, 15)
            append("tabs", 2)
            append("closetools", 1)
            append("menu exit", 1.5)
            append("answer ok", 25)

        setLayout(display, "small")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_TEST_CLOSE_HOLD": "5000",
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        tabs = pb.answers(log, "tabs")
        check(tabs and "Builder" in tabs[0], "the Builder was a tab", str(tabs))
        check(pb.answers(log, "answer ok") == ["answered Quit ECCE"]
              and "close already in progress" in log
              and "quit waits for a tool's close" in log,
              "the Quit came while the Builder's close was running and "
              "waited for it", log[-2000:])
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log and "host destroyed" in log,
              "the Quit finished after the close: the Organizer exits 0",
              "returncode %s\n%s" % (result.returncode, log[-2500:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runQuitJob(display):
    """#1131: Builder, Editor and Viewer tabs closed by closetools while a
    localhost job runs, then Quit: no tool window is left behind, and the
    Organizer exits 0 with the job's watchers still connected."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "water-q")
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

        def inspect(_display):
            append("start Builder " + url, 15)
            append("start CalculationEditor " + url, 30)
            append("setup launch", 6)
            append("setup launch go", 8)
            append("setup launch get state", 2)
            append("start CalculationViewer " + url, 20)
            append("tabs", 2)
            append("closetools", 8)
            append("tabs", 2)
            append("inprocess", 2)
            append("setup launch get state", 2)
            append("menu exit", 3)
            append("answer ok", 25)

        setLayout(display, "normal")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        print("job state before/after closetools:",
              pb.answers(log, "setup launch get state"))
        check(pb.answers(log, "closetools") == ["queued", "closed"],
              "closetools closed the tool tabs",
              str(pb.answers(log, "closetools")))
        tabs = pb.answers(log, "tabs")
        check(len(tabs) >= 2 and "Viewer" in tabs[0] and "Builder" in tabs[0]
              and re.match(r"0:Organizer\*? menus", tabs[-1]),
              "Builder, Editor and Viewer were tabs; only the Organizer is "
              "left after closetools", str(tabs))
        inproc = pb.answers(log, "inprocess")
        left = re.findall(r"tlw=(Builder|CalcEd|PartialCharge|MoCoeffs|"
                          r"GridDlg|wxAuiFloatingFrame)\b", inproc[-1]) \
            if inproc else ["no answer"]
        check(not left, "no tool frame is left after closetools",
              inproc[-1] if inproc else "")
        check(not result.crashed and result.returncode == 0
              and "Aborted" not in log,
              "Quit after closetools with a job's watchers connected exits 0",
              "returncode %s\n%s" % (result.returncode, log[-2500:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runTabPaint(display):
    """#1210: what is painted on the main window while a Viewer tab opens
    and closes (paintlog: each painted window, in order)."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "f_etcl_sn2_irc")
        shutil.copytree(os.path.join(
            HERE, "..", "..", "data", "client", "examples", "organic",
            "substitution-vs-elimination", "ecceqm-f_etcl_sn2_irc", "stored",
            "f_etcl_sn2_irc"), calc, symlinks=True)
        url = "file://" + calc
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("paintlog on", 3)
            append("paintlog", 2)
            append("paintlog on", 1)
            append("start CalculationViewer " + url, 25)
            append("paintlog", 2)
            append("builderinfo", 2)
            append("paintlog on", 1)
            append("tabclose 1", 10)
            append("paintlog", 2)
            append("tabs", 2)
            append("menu exit", 3)
            append("answer ok", 20)

        setLayout(display, "normal")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        # "paintlog on" answers "on"; only the reports carry paints=.
        logs = [a for a in pb.answers(log, "paintlog")
                if a.startswith("paints=")]
        for name, entry in zip(("idle", "open", "close"), logs):
            print("paintlog %s: %s" % (name, entry))
        check(len(logs) == 3, "paintlog answered for idle, open and close",
              str(logs))
        if len(logs) == 3:
            # Frames of the main window drawn between the Organizer page and
            # the Viewer's first paint: the tab's page shown empty (#1210).
            seq = logs[1].split("seq=", 1)[1].split(",")
            first = next((i for i, e in enumerate(seq)
                          if e.startswith("SoWxExaminerViewer")), len(seq))
            last = max([i for i, e in enumerate(seq[:first])
                        if e.startswith("ProjectContextPanel")] or [0])
            empty = sum(1 for e in seq[last:first]
                        if e.startswith("MainFrame"))
            empty = max(0, empty - (first < len(seq)))  # the Viewer's own frame
            print("paintlog open: %d frame(s) of an empty tab page before "
                  "the Viewer draws" % empty)
        info = pb.answers(log, "builderinfo")
        check(info and re.search(r"atoms=[1-9]\d* inscene=1", info[0]),
              "the Viewer draws its structure", str(info))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runReopenViewer(display):
    """#1209: a Viewer opened, closed and opened again on the same
    finished calculation draws its structure the second time too."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "f_etcl_sn2_irc")
        shutil.copytree(os.path.join(
            HERE, "..", "..", "data", "client", "examples", "organic",
            "substitution-vs-elimination", "ecceqm-f_etcl_sn2_irc", "stored",
            "f_etcl_sn2_irc"), calc, symlinks=True)
        url = "file://" + calc
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start CalculationViewer " + url, 25)
            append("tabs", 2)
            append("builderinfo", 2)
            append("tabclose 1", 10)
            append("tabs", 2)
            append("start CalculationViewer " + url, 25)
            append("tabs", 2)
            append("builderinfo", 2)
            append("menu exit", 3)
            append("answer ok", 20)

        setLayout(display, "normal")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        info = pb.answers(log, "builderinfo")
        print("tabs:", pb.answers(log, "tabs"))
        print("builderinfo:", info)
        drawn = [re.search(r"atoms=(\d+) inscene=1", i) for i in info]
        check(len(info) == 2 and drawn[0] and int(drawn[0].group(1)) > 0,
              "the first Viewer draws the structure", str(info))
        check(len(info) == 2 and drawn[1] and drawn[0]
              and drawn[1].group(1) == drawn[0].group(1),
              "a Viewer reopened after its tab was closed draws it too",
              str(info))
        check(not result.crashed, "no crash", log[-2500:])
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
        closing = [l for l in log.splitlines()
                   if "closeAll" in l or "closetools" in l or "ECCE_TEST_ORGANIZER" in l
                   or re.match(r"#\d+ ", l) or "SIGSEGV" in l]
        if check(len(inproc) == 4, "four window counts answered",
                 str(inproc) + "\n" + "\n".join(closing[-80:])):
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


# Panels the water example must offer; with them the test cannot pass on a
# renamed or missing panel (#1135).
TEARDOWN_REQUIRED = ("Density Map", "MOs", "MO Diagram", "Dipole Moment")


def panelNames():
    """Every property panel name the descriptor defines."""
    path = os.path.join(os.path.dirname(os.path.dirname(HERE)), "data",
                        "client", "config", "PropertyPanelDescriptor.xml")
    return set(re.findall(r"<name>([^<]+)</name>", open(path).read()))


def runPanelTeardown(display):
    """Closing a Viewer tab with a property panel up leaves the Organizer
    running (#1135): the panels' destructors take their overlays out of the
    viewer, so they have to go before it.  A first run lists the water
    example's Properties; the second opens each of them in its own Viewer
    tab and closes the tab.  Last, the first Add from SMILES on a new
    calculation marks it changed (#1137)."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        fresh = "file://" + pb.makeCalc(project, "water-s", "created", None)
        water = "file://" + os.path.join(data, "users", "local", "Examples",
                                         "Spectroscopy", "Small_molecules",
                                         "water")
        cmds = os.path.join(work, "commands")
        script = os.path.join(work, "builder-script")

        def builderScript(lines):
            with open(script, "w") as handle:
                handle.write("\n".join(lines) + "\n")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def organizer(inspect):
            open(cmds, "w").close()
            setLayout(display, "small")
            return apps.run(display, "organizer", windowTimeout=60, settle=20,
                            env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                                 "ECCE_ONE_PROCESS": "1",
                                 "ECCE_TEST_ORGANIZER": cmds,
                                 "ECCE_BUILDER_SCRIPT": script,
                                 "ECCE_BUILDER_SCRIPT_EACH": "1",
                                 "ECCE_LOCAL_DATA": data},
                            inspect=inspect)

        # 1. The Properties list of the water example.
        waterDir = water[len("file://"):]

        def listing(_display):
            # The set copies one calculation at a time (~30 s each).
            append("examples spectroscopy", 5)
            deadline = time.time() + 1200
            while (time.time() < deadline and not os.path.exists(
                    os.path.join(waterDir, ".ecce-meta"))):
                time.sleep(5)
            time.sleep(10)
            builderScript(["wait 10000", "plist"])
            append("start CalculationViewer " + water, 30)

        builderScript(["wait 1000"])
        log = organizer(listing).log or ""
        entries = re.findall(r"BUILDER: entry (.*)", log)
        known = panelNames()
        names = sorted(n for n in known
                       if any(e == n or e.endswith("/" + n) for e in entries))
        print("      panels: " + ", ".join(names))
        if not check(all(n in names for n in TEARDOWN_REQUIRED),
                     "the water example lists %s" % ", ".join(TEARDOWN_REQUIRED),
                     "\n".join(entries) or log[-3000:]):
            return

        # 2. Each panel up, then its Viewer tab closed.
        def teardown(_display):
            for name in names:
                builderScript(["wait 10000", "list " + name, "wait 3000"])
                append("start CalculationViewer " + water, 25)
                append("tabs", 2)
                append("tabclose 1", 10)
                append("tabs", 2)
            builderScript(["wait 8000", "smiles CCO", "savestate"])
            append("start Builder " + fresh, 20)
            append("tabs", 2)

        result = organizer(teardown)
        log = result.log or ""
        if GDB:
            print(log[-8000:])
        check(not result.crashed and "Segmentation" not in log,
              "closing Viewer tabs with property panels up: no crash",
              result.signalName or log[-3000:])
        listed = dict(re.findall(r"ECCE_BUILDER_SCRIPT: list (.+?): (.*)", log))
        tabs = pb.answers(log, "tabs")
        n = len(names)
        check(len(tabs) == 2 * n + 1,
              "the Organizer answers after every tab close (%d of %d)"
              % (len(tabs), 2 * n + 1), "\n".join(tabs))
        for i, name in enumerate(names):
            check(listed.get(name) == "ok", "%s opened from the Properties "
                  "list" % name, str(listed.get(name)))
            if 2 * i + 1 < len(tabs):
                check("1:Viewer" in tabs[2 * i] and "1:" not in tabs[2 * i + 1],
                      "the Viewer tab with %s closed" % name,
                      tabs[2 * i] + " -> " + tabs[2 * i + 1])
        state = re.findall(r"BUILDER: dirty=(\d) save_enabled=(\d)", log)
        check(state and state[-1] == ("1", "1"),
              "the first SMILES on a new calculation marks it changed "
              "and enables Save (#1137)", str(state))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runNewCalcEditor(display):
    """A new calculation, selected in the Organizer (which looks for a
    structure to make its thumbnail), opens in an editable Editor for its
    code: the tab names the code, not the generic Electronic Structure
    Editor of a read-only open."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()
        calc = "file://" + os.path.join(data, "users", "local", "nc", "CO")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("newproject nc", 3)
            append("newcalc nc CO ECCE-QM", 5)
            append("start CalculationEditor " + calc, 25)
            append("tabs", 2)

        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        tabs = re.findall(r"ECCE_TEST_ORGANIZER: tabs: (.*)$", log, re.M)
        check(bool(tabs) and "ECCE-QM Editor" in tabs[-1] and
              "Electronic Structure Editor" not in tabs[-1],
              "a new calculation selected in the Organizer opens in its "
              "code's editor, not read-only", str(tabs))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


TABS_PARTS = [
    [runLayouts, runTabs, runQuitHost, runUntrustedDir, runGeometry],
    [runTabsEditor, runSetupTab],
    [runTabsTools, runTabsLauncher, runSetupLaunch],
    [runQuitDuringClose, runQuitJob, runReopenViewer, runTabPaint],
    [runTabsPopups, runUtilityWindows, runBuilderRoleDot, runNewCalcEditor],
    [runTabs1088, runTabIdentity, runPanelTeardown],
]


def main():
    # A stalled run is killed by its wall-clock limit without saying where;
    # every 10 min the Python stacks go to stderr so the log shows it.
    faulthandler.dump_traceback_later(600, repeat=True)
    if "--1131" in sys.argv:    # same as ECCE_1131_ONLY=1
        sys.argv.remove("--1131")
        os.environ["ECCE_1131_ONLY"] = "1"
    if "--1209" in sys.argv:
        sys.argv.remove("--1209")
        os.environ["ECCE_1209_ONLY"] = "1"
    if "--1088" in sys.argv:    # same as ECCE_1088_ONLY=1, for the offload
        sys.argv.remove("--1088")
        os.environ["ECCE_1088_ONLY"] = "1"

    def checkApp(display, name, results, verbose=False):
        if os.environ.get("ECCE_SINGLE_ONLY") == "1":
            runSingleInstance(display)
            runUntrustedDir(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_NEWCALC_ONLY") == "1":
            runNewCalcEditor(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_UTILS_ONLY") == "1":
            runUtilityWindows(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_1210_ONLY") == "1":
            runTabPaint(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_1209_ONLY") == "1":
            runReopenViewer(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_1131_ONLY") == "1":
            runQuitDuringClose(display)
            runQuitJob(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_LAUNCHER_ONLY") == "1":
            runTabsLauncher(display)
            runSetupLaunch(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_CONTRACTIONS_ONLY") == "1":
            runBasisContractions(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_SETUP_ONLY") == "1":
            runSetupTab(display)
            runBasisContractions(display)
            runSetupLaunch(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_1256_ONLY") == "1":
            runTabIdentity(display)
            runBuilderRoleDot(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_1088_ONLY") == "1":
            runTabs1088(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_LAYOUTS_ONLY") == "1":
            runLayouts(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_PANELS_ONLY") == "1":
            runPanelTeardown(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        if os.environ.get("ECCE_POPUPS_ONLY") == "1":
            runTabsPopups(display)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        # The core part (organizer start, Quit with and without the gateway)
        # runs as its own ctest: together with the tabs part it outgrew the
        # per-test budget.
        if os.environ.get("ECCE_CORE_ONLY") == "1":
            run(display)
            runQuit(display)
            runQuit(display, gateway=True)
            if failures:
                results.fail("one-process", "; ".join(failures))
            return
        # The tabbed part in groups, one ctest each (ECCE_TABS_PART=N), so
        # each stays under 30 min; ECCE_TABS_ONLY=1 alone runs them all.
        # runSingleInstance and runBasisContractions have ctests of their own.
        part = os.environ.get("ECCE_TABS_PART", "")
        for n, group in enumerate(TABS_PARTS, 1):
            if part and part != str(n):
                continue
            for step in group:
                began = time.time()
                step(display)
                print("time %s %.0f s" % (step.__name__, time.time() - began))
                sys.stdout.flush()
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
    # ctest runs the parts separately, each inside its TIMEOUT (#1253).
    budget = "1700" if os.environ.get("ECCE_TABS_PART") else "5700"
    sys.argv = ([sys.argv[0], "--app", "organizer", "--budget", budget] +
                sys.argv[1:])
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
