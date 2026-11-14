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
    cancels the Quit, No lets the process end."""
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
        if GDB:
            print(log[-20000:])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runTabs(display):
    """ECCE_LAYOUT=small (#983 stage 2): the Builder opens as a tab, the menu
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
            append("tabs", 2)
            append("start Builder " + url, 25)
            append("tabs", 2)                  # Builder tab active
            append("tabinfo", 2)
            shot("tabs-builder.png")
            append("charhook 27", 2)           # Esc, as GTK delivers it
            append("tab 0", 3)
            append("tabs", 2)                  # Organizer's menus back
            shot("tabs-organizer.png")
            append("towindow 1", 8)            # Open in Window
            append("inprocess", 2)
            append("sbinfo Builder", 2)
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

        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1", "ECCE_LAYOUT": "small",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_BUILDER_SCRIPT": script,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        if GDB:
            print(log[-6000:])
        tabs = pb.answers(log, "tabs")
        print("      tabs: " + "\n            ".join(tabs))
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
        info = pb.answers(log, "tabinfo")
        print("      tabinfo: " + str(info))
        m = re.search(r"page=(\d+)x(\d+) onscreen=1 children=\d+ shown=(\d+) "
                      r"statusbar=page frame=hidden", info[0] if info else "")
        check(m is not None and int(m.group(1)) > 400 and
              int(m.group(2)) > 300 and int(m.group(3)) >= 3,
              "the Builder tab shows its windows and status bar, its own "
              "frame stays unmapped", str(info))
        check("BUILDER: charhook key=27" in log,
              "a key hook on the main window reaches the Builder tab",
              str(pb.answers(log, "charhook")))
        tw = pb.answers(log, "towindow")
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
              ["answered Save Builder Changes?"] and
              pb.answers(log, "answer no") ==
              ["answered Save Builder Changes?"],
              "Quit asks about the Builder tab: Cancel keeps it, No quits",
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

        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1", "ECCE_LAYOUT": "small",
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


def main():
    def checkApp(display, name, results, verbose=False):
        runTabs(display)
        runTabsEditor(display)
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
