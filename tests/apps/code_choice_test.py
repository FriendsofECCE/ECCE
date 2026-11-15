#!/usr/bin/env python3
"""Preferences > Codes (#1049), headless, through the Organizer's hooks.

Codes unticked in the Codes page leave File > New, the right-click New menu
and Continue in Another Code; each of those lists ends with "N more codes
(hidden)..." where N is the number of unticked codes, and ticking a code
adds it back without a restart.  A calculation of a hidden code is still in
the tree and still offers the usual tools.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
FIX = os.path.join(HERE, "fixtures")
PROJECT_META = os.path.join(HERE, "..", "..", "tools", "screenshots", "data",
                            "project.ecce-meta")
SHOT = os.environ.get("CODE_CHOICE_SHOT", "")
failures = []


def check(ok, what, detail=""):
    print("%s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
        print("      | " + detail[-400:])


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings), flush=True)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    os.environ.pop("DISPLAY", None)
    work = tempfile.mkdtemp(prefix="codechoice-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    project = os.path.join(data, "users", "local", "p")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    shutil.copytree(os.path.join(FIX, "calc-water-opt"), os.path.join(project, "nw"),
                    symlinks=True)
    os.environ["ECCE_LOCAL_DATA"] = data
    os.environ["ECCE_TEST_ORGANIZER_NOLAUNCH"] = "1"
    nw = "file://" + os.path.join(project, "nw")
    proj = "file://" + project
    steps = [
        "prefs codes all",
        "filemenu",                                  # 1
        "prefs codes list",
        "prefs codes tick ORCA off",
        "prefs codes tick MOPAC off",
        "prefs codes tick NWChem off",
        "prefs codes list",                          # 5
        "filemenu",
        "contextmenu " + proj,
        "prefs codes tick ORCA on",
        "prefs codes list",
        "filemenu",                                  # 10
        "prefs codes page hiddenfirst",
        "contextmenu " + proj,
        "anothercodelabels",                         # 14 (after the page)
        "prefs codes page-state",
        "prefs codes list",
        "select " + nw,
        "toolsmenu " + nw,
        "toolsclick Electronic Structure Editor...",
    ]
    if SHOT:
        steps.insert(12, "prefs codes snap " + SHOT)
    off = 1 if SHOT else 0
    cmds = os.path.join(work, "commands")
    open(cmds, "w").write("".join(s + "\n" for s in steps))
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            result = apps.run(display, "organizer", windowTimeout=60, settle=40,
                              env={"ECCE_TEST_ORGANIZER": cmds})
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    log = result.log or ""
    answers = {}
    seq = [l.split("ECCE_TEST_ORGANIZER: ", 1)[1] for l in log.splitlines()
           if "ECCE_TEST_ORGANIZER: " in l]
    # The hook answers in order, one line per command.
    for number, line in enumerate(steps):
        answers[number] = seq[number][len(line) + 2:] if number < len(seq) and \
            seq[number].startswith(line + ": ") else None
    check(not result.crashed, "the Organizer did not crash", log)
    a = answers
    check(a[1] and "new=ORCA" in a[1] and "hidden" not in a[1],
          "all codes ticked: ORCA in File > New, no hidden entry", str(a[1]))
    check(a[2] and a[2].startswith("hidden=0"), "list: nothing hidden", str(a[2]))
    check(a[6] and a[6].startswith("hidden=3") and "ORCA=off" in a[6],
          "three codes unticked: hidden=3", str(a[6]))
    check(a[7] and "new=ORCA" not in a[7] and "new=3 more codes (hidden)..." in a[7],
          "File > New: no ORCA, ends with '3 more codes (hidden)...'", str(a[7]))
    check(a[8] and "3 more codes (hidden)...|" in a[8].split("new=")[-1] and
          "ORCA|" not in a[8].split("new=")[-1],
          "right-click New: ends with '3 more codes (hidden)...'", str(a[8]))
    check(a[8] and "selection=" + proj in a[8],
          "the project holding a calculation of a hidden code opens", str(a[8]))
    check(a[10] and a[10].startswith("hidden=2") and "ORCA=on" in a[10],
          "ticking ORCA: hidden=2", str(a[10]))
    check(a[11] and "new=ORCA" in a[11] and "new=2 more codes (hidden)..." in a[11],
          "File > New: ORCA back without a restart, label says 2", str(a[11]))
    check(a[14 + off] and "code=2 more codes (hidden)...;" in a[14 + off] and
          "code=ORCA;" in a[14 + off] and "code=NWChem;" not in a[14 + off],
          "Continue in Another Code: ORCA offered, last entry says 2 hidden",
          str(a[14 + off]))
    # The Codes page shows what is stored: same ticks, same count, hidden
    # codes first (here MOPAC and NWChem), then the core order.
    page, lst = a[15 + off] or "", a[16 + off] or ""
    ticks = dict(t.split("=") for t in page.split("rows=")[-1].split(",") if t)
    stored = dict(t.split("=") for t in lst.split()[1:])
    check(ticks == stored, "page ticks equal the stored list", page + " | " + lst)
    check(page.startswith("note=2 codes are hidden"), "page count line says 2", page)
    order = [t.split("=")[0] for t in page.split("rows=")[-1].split(",") if t]
    check(order[:2] == ["NWChem", "MOPAC"] and order[2:9] == [
        "ECCE-QM", "ORCA", "Gaussian-16", "xTB", "CREST", "GROMACS", "CP2K"],
        "page order: hidden first, then core, then experimental", str(order))
    tools = a[18 + off] or ""
    check(tools.startswith("ok") and "Electronic Structure &Editor" in tools,
          "the NWChem calculation itself still offers the Calculation Editor",
          tools)
    check("startApp CalculationEditor force=0 url=" + nw in log,
          "the NWChem calculation opens in the Calculation Editor", log[-500:])
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
